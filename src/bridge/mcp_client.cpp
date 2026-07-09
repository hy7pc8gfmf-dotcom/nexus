/**
 * @file mcp_client.cpp
 * @brief MCP 客户端实现 — STDIO JSON-RPC
 */

#include "nexus/bridge/mcp_client.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <io.h>
#include <fcntl.h>
#include <sstream>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace nexus::bridge {

// ═══════════════════════════════════════════════════════════════════
// McpTool
// ═══════════════════════════════════════════════════════════════════

auto McpTool::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]         = name;
  j["description"]  = description;
  j["input_schema"] = input_schema;
  return j;
}

auto McpServerStatus::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]       = name;
  j["connected"]  = connected;
  j["tool_count"] = tool_count;
  j["error"]      = error;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// McpClient
// ═══════════════════════════════════════════════════════════════════

McpClient::McpClient(McpServerConfig config) noexcept
  : config_(std::move(config)) {}

McpClient::~McpClient() noexcept {
  disconnect();
}

auto McpClient::connect() noexcept -> Status {
  if (connected_) return Status::Ok();

  // Windows: 创建子进程
  std::string cmdline = config_.command;
  for (const auto& a : config_.args) {
    cmdline += " \"" + a + "\"";
  }

  // 创建管道
  HANDLE h_stdin_rd = nullptr, h_stdin_wr = nullptr;
  HANDLE h_stdout_rd = nullptr, h_stdout_wr = nullptr;
  SECURITY_ATTRIBUTES sa = {sizeof(sa), nullptr, TRUE};

  if (!CreatePipe(&h_stdin_rd, &h_stdin_wr, &sa, 0) ||
      !CreatePipe(&h_stdout_rd, &h_stdout_wr, &sa, 0)) {
    return Status::Error(ErrorCode::kIoError, "CreatePipe failed");
  }

  // 确保子进程使用管道句柄
  SetHandleInformation(h_stdin_wr, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(h_stdout_rd, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si = {};
  si.cb = sizeof(si);
  si.hStdInput  = h_stdin_rd;
  si.hStdOutput = h_stdout_wr;
  si.hStdError  = h_stdout_wr;
  si.dwFlags    = STARTF_USESTDHANDLES;

  PROCESS_INFORMATION pi = {};

  // 创建进程
  std::vector<char> cmd_buf(cmdline.begin(), cmdline.end());
  cmd_buf.push_back('\0');

  if (!CreateProcessA(nullptr, cmd_buf.data(), nullptr, nullptr, TRUE,
                      CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
    CloseHandle(h_stdin_rd);  CloseHandle(h_stdin_wr);
    CloseHandle(h_stdout_rd); CloseHandle(h_stdout_wr);
    return Status::Error(ErrorCode::kEngineFailed,
      "CreateProcess failed for: " + config_.command);
  }

  // 关闭子进程端句柄
  CloseHandle(h_stdin_rd);
  CloseHandle(h_stdout_wr);
  CloseHandle(pi.hThread);

  process_handle_ = pi.hProcess;
  write_fd_ = _open_osfhandle(reinterpret_cast<intptr_t>(h_stdin_wr), _O_TEXT);
  read_fd_  = _open_osfhandle(reinterpret_cast<intptr_t>(h_stdout_rd), _O_TEXT);

  if (write_fd_ < 0 || read_fd_ < 0) {
    disconnect();
    return Status::Error(ErrorCode::kIoError, "fd conversion failed");
  }

  // JSON-RPC 初始化握手
  auto init_result = send_request_("initialize", nlohmann::json::object());
  if (!init_result.ok()) {
    disconnect();
    return Status::Error(ErrorCode::kEngineFailed,
      "MCP initialize failed: " + init_result.error().to_string());
  }

  connected_ = true;
  return Status::Ok();
}

auto McpClient::list_tools() noexcept -> Result<std::vector<McpTool>> {
  if (!connected_) {
    return Status::Error(ErrorCode::kEngineFailed, "not connected");
  }

  auto result = send_request_("tools/list", nlohmann::json::object());
  if (!result.ok()) {
    return result.error();
  }

  std::vector<McpTool> tools;
  auto tools_array = result.value()["tools"];
  if (tools_array.is_array()) {
    for (const auto& t : tools_array) {
      McpTool tool;
      tool.name         = t.value("name", std::string(""));
      tool.description  = t.value("description", std::string(""));
      tool.input_schema = t.value("inputSchema", nlohmann::json::object());
      // 也尝试 input_schema
      if (tool.input_schema.empty()) {
        tool.input_schema = t.value("input_schema", nlohmann::json::object());
      }
      tools.push_back(tool);
    }
  }

  return tools;
}

auto McpClient::call_tool(const std::string& name,
                          const nlohmann::json& args) noexcept
    -> Result<nlohmann::json> {
  if (!connected_) {
    return Status::Error(ErrorCode::kEngineFailed, "not connected");
  }

  auto params = nlohmann::json::object();
  params["name"]      = name;
  params["arguments"] = args;

  return send_request_("tools/call", params);
}

auto McpClient::disconnect() noexcept -> void {
  connected_ = false;
  if (write_fd_ >= 0) { close(write_fd_); write_fd_ = -1; }
  if (read_fd_  >= 0) { close(read_fd_);  read_fd_  = -1; }
  if (process_handle_) {
    CloseHandle(process_handle_);
    process_handle_ = nullptr;
  }
}

auto McpClient::status() const noexcept -> McpServerStatus {
  McpServerStatus s;
  s.name      = config_.name;
  s.connected = connected_;
  return s;
}

auto McpClient::is_connected() const noexcept -> bool {
  return connected_;
}

auto McpClient::send_request_(const std::string& method,
                              const nlohmann::json& params) noexcept
    -> Result<nlohmann::json> {
  if (write_fd_ < 0 || read_fd_ < 0) {
    return Status::Error(ErrorCode::kIoError, "pipe not open");
  }

  int id = next_id_++;

  // 构建 JSON-RPC 请求
  auto request = nlohmann::json::object();
  request["jsonrpc"] = "2.0";
  request["id"]      = id;
  request["method"]  = method;
  request["params"]  = params;

  std::string request_str = request.dump() + "\n";

  // 写入
  FILE* wf = fdopen(write_fd_, "w");
  if (!wf) return Status::Error(ErrorCode::kIoError, "fdopen write failed");
  if (fputs(request_str.c_str(), wf) == EOF || fflush(wf) == EOF) {
    fclose(wf);
    return Status::Error(ErrorCode::kIoError, "write to MCP server failed");
  }
  // 注意: 我们保持 wf 打开供后续使用。实际的 fdopen/fclose 管理需要改进。
  // 当前简化实现: 每次发送都 fdopen

  // 读取响应 (简化: 读取一行)
  FILE* rf = fdopen(read_fd_, "r");
  if (!rf) return Status::Error(ErrorCode::kIoError, "fdopen read failed");

  std::array<char, 65536> buf = {0};
  if (!fgets(buf.data(), static_cast<int>(buf.size()) - 1, rf)) {
    return Status::Error(ErrorCode::kIoError, "read from MCP server failed");
  }

  try {
    auto response = nlohmann::json::parse(std::string(buf.data()));
    if (response.is_object()) {
      // 检查是否有 error 字段
      bool has_error = false;
      std::string error_msg;
      try {
        auto err_val = response.at("error");
        has_error = !err_val.is_null();
        error_msg = err_val.value("message", std::string("MCP error"));
      } catch (...) {}
      if (has_error) {
        return Status::Error(ErrorCode::kEngineFailed, error_msg);
      }
      return response["result"];
    }
    return Status::Error(ErrorCode::kJsonParseError, "MCP response not an object");
  } catch (const std::exception& e) {
    return Status::Error(ErrorCode::kJsonParseError,
      "MCP response parse failed: " + std::string(e.what()));
  }
}

// ═══════════════════════════════════════════════════════════════════
// McpBridge
// ═══════════════════════════════════════════════════════════════════

auto McpBridge::add_server(const McpServerConfig& config) noexcept -> Status {
  ServerInstance inst;
  inst.config = config;
  inst.client = std::make_unique<McpClient>(config);
  servers_.push_back(std::move(inst));
  return Status::Ok();
}

auto McpBridge::connect_all() noexcept -> Status {
  for (auto& inst : servers_) {
    auto s = inst.client->connect();
    if (s.ok()) {
      auto tools_result = inst.client->list_tools();
      if (tools_result.ok()) {
        inst.tools = std::move(tools_result.value());
      }
    }
  }
  return Status::Ok();
}

auto McpBridge::list_status() const noexcept -> std::vector<McpServerStatus> {
  std::vector<McpServerStatus> statuses;
  for (const auto& inst : servers_) {
    statuses.push_back(inst.client->status());
  }
  return statuses;
}

auto McpBridge::all_tools() const noexcept -> std::vector<McpTool> {
  std::vector<McpTool> all;
  for (const auto& inst : servers_) {
    all.insert(all.end(), inst.tools.begin(), inst.tools.end());
  }
  return all;
}

auto McpBridge::call_tool(const std::string& tool_name,
                          const nlohmann::json& args) noexcept
    -> Result<nlohmann::json> {
  for (auto& inst : servers_) {
    for (const auto& t : inst.tools) {
      if (t.name == tool_name) {
        return inst.client->call_tool(tool_name, args);
      }
    }
  }
  return Status::Error(ErrorCode::kFileNotFound,
    "tool not found: " + tool_name);
}

auto McpBridge::connected_count() const noexcept -> int {
  int count = 0;
  for (const auto& inst : servers_) {
    if (inst.client->is_connected()) ++count;
  }
  return count;
}

auto McpBridge::instance() noexcept -> McpBridge& {
  static McpBridge bridge;
  return bridge;
}

}  // namespace nexus::bridge
