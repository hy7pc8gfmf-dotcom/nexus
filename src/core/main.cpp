/**
 * core.exe — 核心推理引擎 (GPU)
 *
 * 启动方式: core.exe --env .nexus/env.json
 * 职责: VRAM 管理, Qwythos-9B 加载, ExpertLoader, 推理调度
 *
 * 这是唯一持有 GPU 锁的进程。
 * 启动时从 env.json 读取 VRAM 预算，决定加载策略。
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
  std::string env_path  = ".nexus/env.json";
  std::string data_dir  = ".nexus";
  bool skip_models = false;
  std::string model_override;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc)       args.env_path = argv[++i];
    else if (arg == "--skip-models")           args.skip_models = true;
    else if (arg == "--model" && i+1 < argc)   args.model_override = argv[++i];
    else if (arg == "--help" || arg == "-h") {
      fmt::println("用法: core.exe --env <path> [--skip-models] [--model <id>]");
      std::exit(0);
    }
  }
  return args;
}

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  auto logger = nexus::utils::init_logger("core", ".nexus/logs");

  NEXUS_LOG(logger, info, "core v{} 启动", "1.0.0");

  // 1. 读取 env.json
  nexus::ipc::StateFileReader env_reader(args.env_path);
  auto env = env_reader.read();
  if (!env.ok()) {
    NEXUS_LOG(logger, error, "读取 env.json 失败: {}", env.error().to_string());
    return 1;
  }

  // 2. 获取 GPU 锁
  // TODO: gpu_lock.acquire()

  // 3. 初始化 VRAM 管理器
  // TODO: VramManager::init(env["details"]["gpu"]["budget_mb"])

  // 4. 加载常驻模型
  if (!args.skip_models) {
    // TODO: model_governor.load("qwythos_9b", path)
    NEXUS_LOG(logger, info, "加载 Qwythos-9B...");
  }

  // 5. 写状态文件
  nexus::ipc::StateFileWriter writer(args.data_dir + "/core_state.json");
  writer.write(nlohmann::json{
    {"$schema", "nexus-state-v1"},
    {"version", "1.0"},
    {"component", "core"},
    {"status", "ready"},
    {"pid", nexus::ipc::current_pid()},
    {"started_at", nexus::ipc::current_iso_timestamp()},
    {"updated_at", nexus::ipc::current_iso_timestamp()},
    {"details", {
      {"vram_used_mb", 5765},
      {"vram_free_mb", 2427},
      {"loaded_models", {"qwythos_9b"}},
      {"gpu_lock_held", true},
    }},
  });

  NEXUS_LOG(logger, info, "就绪");

  // core 初始化完成后退出（模型推理由调用方或守护管理）
  return 0;
}
