// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NEXUS_BRIDGE_MCP_CLIENT_H
#define NEXUS_BRIDGE_MCP_CLIENT_H

/**
 * @file mcp_client.h
 * @brief MCP 客户端 — STDIO JSON-RPC 协议
 *
 * 连接外部 MCP 服务器, 通过 STDIO 管道通信。
 * 生命周期: connect → initialize → list_tools → call_tool → disconnect
 */

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "nexus/types/status.h"
#include <nlohmann/json.hpp>

namespace nexus::bridge {

/// MCP 服务器配置
struct McpServerConfig {
  std::string name;               // 服务器名称
  std::string command;            // 启动命令 (如 "node")
  std::vector<std::string> args;  // 命令行参数
  std::string transport = "stdio";// 传输方式
};

/// MCP 工具定义
struct McpTool {
  std::string name;
  std::string description;
  nlohmann::json input_schema;

  auto to_json() const -> nlohmann::json;
};

/// MCP 服务器状态
struct McpServerStatus {
  std::string name;
  bool connected = false;
  int  tool_count = 0;
  std::string error;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// McpClient — 单个 MCP 服务器连接
// ═══════════════════════════════════════════════════════════════════

class McpClient {
public:
  explicit McpClient(McpServerConfig config) noexcept;
  ~McpClient() noexcept;

  McpClient(const McpClient&) = delete;
  McpClient& operator=(const McpClient&) = delete;

  /// 连接并初始化
  auto connect() noexcept -> Status;

  /// 列出可用工具
  auto list_tools() noexcept -> Result<std::vector<McpTool>>;

  /// 调用工具
  auto call_tool(const std::string& name,
                 const nlohmann::json& args) noexcept
      -> Result<nlohmann::json>;

  /// 断开连接
  auto disconnect() noexcept -> void;

  /// 获取状态
  [[nodiscard]] auto status() const noexcept -> McpServerStatus;

  /// 是否已连接
  [[nodiscard]] auto is_connected() const noexcept -> bool;

private:
  McpServerConfig config_;
  void* process_handle_ = nullptr;  // HANDLE
  int  write_fd_ = -1;
  int  read_fd_  = -1;
  int  next_id_  = 1;
  bool connected_ = false;

  /// 发送 JSON-RPC 请求并接收响应
  auto send_request_(const std::string& method,
                     const nlohmann::json& params) noexcept
      -> Result<nlohmann::json>;
};

// ═══════════════════════════════════════════════════════════════════
// McpBridge — MCP 桥接管理器
// ═══════════════════════════════════════════════════════════════════

class McpBridge {
public:
  /// 注册并连接 MCP 服务器
  auto add_server(const McpServerConfig& config) noexcept -> Status;

  /// 连接所有已注册的服务器
  auto connect_all() noexcept -> Status;

  /// 获取所有服务器状态
  [[nodiscard]] auto list_status() const noexcept -> std::vector<McpServerStatus>;

  /// 获取所有可用工具
  [[nodiscard]] auto all_tools() const noexcept -> std::vector<McpTool>;

  /// 调用工具 (在第一个提供该工具的服务器上执行)
  auto call_tool(const std::string& tool_name,
                 const nlohmann::json& args) noexcept
      -> Result<nlohmann::json>;

  /// 获取连接数
  [[nodiscard]] auto connected_count() const noexcept -> int;

  /// 单例
  static auto instance() noexcept -> McpBridge&;

private:
  struct ServerInstance {
    McpServerConfig config;
    std::unique_ptr<McpClient> client;
    std::vector<McpTool> tools;
  };
  std::vector<ServerInstance> servers_;
};

}  // namespace nexus::bridge

#endif  // NEXUS_BRIDGE_MCP_CLIENT_H
