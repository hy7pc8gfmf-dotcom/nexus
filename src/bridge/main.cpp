/**
 * bridge.exe — 外部桥接 (CPU)
 *
 * 启动方式: bridge.exe --env .nexus/env.json --core .nexus/core_state.json
 * 职责: MCP 桥接, 种子通道, Shell C 虚化桥接
 */

#include <cstdlib>
#include <iostream>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "nexus/ipc/state_file.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"

struct CliArgs {
  std::string env_path    = ".nexus/env.json";
  std::string core_path   = ".nexus/core_state.json";
  std::string data_dir    = ".nexus";
  bool dry_run = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env"  && i + 1 < argc)  args.env_path = argv[++i];
    if (arg == "--core" && i + 1 < argc)  args.core_path = argv[++i];
    else if (arg == "--dry-run")          args.dry_run = true;
    else if (arg == "--help" || arg == "-h") {
      fmt::println("用法: bridge.exe --env <path> --core <path> [--dry-run]");
      std::exit(0);
    }
  }
  return args;
}

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  auto logger = nexus::utils::init_logger("bridge", ".nexus/logs");

  NEXUS_LOG(logger, info, "bridge v{} 启动", "1.0.0");

  // 1. 读取 env.json
  nexus::ipc::StateFileReader env_reader(args.env_path);
  auto env = env_reader.read();
  if (!env.ok()) {
    NEXUS_LOG(logger, warn, "env.json 不可用");
  }

  // 2. MCP 桥接
  // TODO: mcp_bridge.connect()
  NEXUS_LOG(logger, info, "MCP 桥接初始化");

  // 3. 种子通道
  // TODO: seed_channel.plant()
  NEXUS_LOG(logger, info, "种子通道初始化");

  // 4. Shell C 虚化
  // TODO: virtual_shell_c.init()
  NEXUS_LOG(logger, info, "Shell C 桥接初始化");

  // 5. 写状态文件
  nexus::ipc::StateFileWriter writer(args.data_dir + "/bridge_state.json");
  writer.write(nlohmann::json{
    {"$schema", "nexus-state-v1"},
    {"version", "1.0"},
    {"component", "bridge"},
    {"status", "ready"},
    {"pid", nexus::ipc::current_pid()},
    {"started_at", nexus::ipc::current_iso_timestamp()},
    {"updated_at", nexus::ipc::current_iso_timestamp()},
    {"details", {
      {"mcp_connected", false},
      {"seeds_planted", 0},
    }},
  });

  NEXUS_LOG(logger, info, "就绪");
  return 0;
}
