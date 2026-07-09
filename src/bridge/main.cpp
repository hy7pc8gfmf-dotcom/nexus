/**
 * @file main.cpp — bridge.exe
 * @brief 外部桥接 — MCP连接 + 种子通道 + Shell C
 *
 * 启动: bridge.exe --env .nexus/env.json [--dry-run]
 *
 * 职责:
 *   - 连接 MCP 服务器, 注册可用工具
 *   - 注入默认种子到三目标 (模型/核心/自我)
 *   - 写状态文件
 */

#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "nexus/ipc/state_file.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"
#include "nexus/bridge/mcp_client.h"
#include "nexus/bridge/seed_channel.h"

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════
// 命令行
// ═══════════════════════════════════════════════════════════════════

struct CliArgs {
  std::string env_path    = ".nexus/env.json";
  std::string output_path = ".nexus/bridge_state.json";
  std::string data_dir    = ".nexus";
  bool dry_run = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc)   args.env_path = argv[++i];
    else if (arg == "--output" && i+1<argc) args.output_path = argv[++i];
    else if (arg == "--dry-run")           args.dry_run = true;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: bridge.exe --env <path> [--dry-run]\n";
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════════
// MCP 服务器配置
// ═══════════════════════════════════════════════════════════════════

static auto default_mcp_servers() -> std::vector<nexus::bridge::McpServerConfig> {
  // 注册已知的本地 MCP 工具
  // agi_capability 工具集
  return {
    {"agi_capability", "node",
     {"D:/synapse/mcp-servers/agi-capability-server/build/index.js"}, "stdio"},
  };
}

// ═══════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  fs::create_directories(args.data_dir + "/logs");

  auto logger = nexus::utils::init_logger("bridge", args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "bridge v{} 启动", "1.0.0");

  // 1. 读取 env.json
  {
    nexus::ipc::StateFileReader env_reader(args.env_path);
    auto env = env_reader.read();
    if (env.ok()) {
      NEXUS_LOG(logger, info, "env.json 已读取");
    } else {
      NEXUS_LOG(logger, warn, "env.json 不可用 (降级运行)");
    }
  }

  int mcp_connected = 0;
  int mcp_total = 0;
  int mcp_tools = 0;

  // 2. MCP 桥接 (非 dry-run)
  if (!args.dry_run) {
    auto& mcp = nexus::bridge::McpBridge::instance();

    // 注册服务器
    for (const auto& cfg : default_mcp_servers()) {
      mcp.add_server(cfg);
      NEXUS_LOG(logger, info, "MCP 服务器注册: {}", cfg.name);
    }

    // 连接
    mcp.connect_all();
    mcp_connected = mcp.connected_count();
    mcp_total = static_cast<int>(default_mcp_servers().size());
    mcp_tools = static_cast<int>(mcp.all_tools().size());

    NEXUS_LOG(logger, info, "MCP 桥接: {}/{} 已连接, {} 工具",
      mcp_connected, mcp_total, mcp_tools);
  } else {
    NEXUS_LOG(logger, info, "dry-run: 跳过 MCP 连接");
  }

  // 3. 种子通道
  int seed_total = 0;
  int seed_model = 0, seed_core = 0, seed_self = 0;

  if (!args.dry_run) {
    nexus::bridge::SeedChannel channel(args.data_dir);
    channel.plant_defaults();
    seed_total = channel.total_seeds();

    auto counts = channel.count_by_target();
    for (const auto& [target, count] : counts) {
      if (target == "model") seed_model = count;
      else if (target == "core") seed_core = count;
      else if (target == "self") seed_self = count;
    }

    NEXUS_LOG(logger, info, "种子通道: {} 颗 (M:{} C:{} S:{})",
      seed_total, seed_model, seed_core, seed_self);
  }

  // 4. Shell C 虚化桥接 (配置声明)
  NEXUS_LOG(logger, info, "Shell C 桥接就绪");
  NEXUS_LOG(logger, info, "能力自认知就绪");

  // 5. 写状态文件
  if (!args.dry_run) {
    auto state = nlohmann::json::object();
    state["$schema"]    = "nexus-state-v1";
    state["version"]    = "1.0";
    state["component"]  = "bridge";
    state["status"]     = "ready";
    state["pid"]        = nexus::ipc::current_pid();
    state["started_at"] = nexus::ipc::current_iso_timestamp();
    state["updated_at"] = nexus::ipc::current_iso_timestamp();

    auto details = nlohmann::json::object();
    details["mcp_connected"]  = mcp_connected;
    details["mcp_total"]      = mcp_total;
    details["mcp_tools"]      = mcp_tools;
    details["seeds_total"]    = seed_total;
    details["seeds_model"]    = seed_model;
    details["seeds_core"]     = seed_core;
    details["seeds_self"]     = seed_self;
    state["details"] = details;

    nexus::ipc::StateFileWriter writer(args.output_path);
    writer.write(state);
  }

  NEXUS_LOG(logger, info, "完成 (MCP: {}/{}, 种子: {})",
    mcp_connected, mcp_total, seed_total);
  return 0;
}
