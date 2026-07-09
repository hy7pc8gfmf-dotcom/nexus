/**
 * algo.exe — 算法引擎池 (CPU)
 *
 * 启动方式: algo.exe --env .nexus/env.json
 * 职责: 10 个算法引擎注册, 审计, 自演进
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
  std::string env_path   = ".nexus/env.json";
  std::string data_dir   = ".nexus";
  bool validate = false;
  std::vector<std::string> engine_filter;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc) args.env_path = argv[++i];
    else if (arg == "--validate")        args.validate = true;
    else if (arg == "--engines" && i + 1 < argc) {
      auto list = std::string(argv[++i]);
      size_t pos = 0;
      while ((pos = list.find(',')) != std::string::npos) {
        args.engine_filter.push_back(list.substr(0, pos));
        list.erase(0, pos + 1);
      }
      args.engine_filter.push_back(list);
    }
    else if (arg == "--help" || arg == "-h") {
      fmt::println("用法: algo.exe --env <path> [--validate] [--engines mcmc,dual_pruning]");
      std::exit(0);
    }
  }
  return args;
}

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  auto logger = nexus::utils::init_logger("algo", ".nexus/logs");

  NEXUS_LOG(logger, info, "algo v{} 启动", "1.0.0");

  // 1. 读取 env.json
  nexus::ipc::StateFileReader env_reader(args.env_path);
  auto env = env_reader.read();
  if (!env.ok()) {
    NEXUS_LOG(logger, error, "读取 env.json 失败: {}", env.error().to_string());
    return 1;
  }

  // 2. 注册算法引擎
  const std::vector<std::string> all_engines = {
    "mcmc", "dual_pruning", "mtcs", "multimodal_verifier",
    "infinitas_truth", "dre", "temporal_kg",
    "dialectical_consensus", "ethical_evaluation", "paper_generation"
  };

  for (const auto& e : all_engines) {
    if (!args.engine_filter.empty() &&
        std::find(args.engine_filter.begin(), args.engine_filter.end(), e)
        == args.engine_filter.end()) {
      continue;
    }
    NEXUS_LOG(logger, info, "注册算法引擎: {}", e);
    // TODO: engine_registry.register(e)
  }

  // 3. 启动隔离子进程
  // TODO: 启动 algo_engine.exe, recursor.exe

  // 4. 审计引擎初始化
  // TODO: audit_engine.init(40 rules)

  // 5. 写状态文件
  nexus::ipc::StateFileWriter writer(args.data_dir + "/algo_state.json");
  writer.write(nlohmann::json{
    {"$schema", "nexus-state-v1"},
    {"version", "1.0"},
    {"component", "algo"},
    {"status", "ready"},
    {"pid", nexus::ipc::current_pid()},
    {"started_at", nexus::ipc::current_iso_timestamp()},
    {"updated_at", nexus::ipc::current_iso_timestamp()},
    {"details", {
      {"algorithms", all_engines.size()},
      {"registered", args.engine_filter.empty() ? all_engines.size() : args.engine_filter.size()},
    }},
  });

  NEXUS_LOG(logger, info, "完成 ({} 引擎注册)", all_engines.size());
  return 0;
}
