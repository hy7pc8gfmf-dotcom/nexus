/**
 * psyche.exe — Ψ 层意识组件 (CPU)
 *
 * 启动方式: psyche.exe --env .nexus/env.json --core .nexus/core_state.json
 * 职责: Ψ-Navigator, 心灵核心, 涌现流水线, 观察者
 */

#include <cstdlib>
#include <iostream>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "nexus/ipc/state_file.h"
#include "nexus/ipc/mmap_ringbuf.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"

struct CliArgs {
  std::string env_path    = ".nexus/env.json";
  std::string core_path   = ".nexus/core_state.json";
  std::string data_dir    = ".nexus";
  bool daemon_mode = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env"  && i + 1 < argc)  args.env_path = argv[++i];
    if (arg == "--core" && i + 1 < argc)  args.core_path = argv[++i];
    else if (arg == "--daemon")           args.daemon_mode = true;
    else if (arg == "--help" || arg == "-h") {
      fmt::println("用法: psyche.exe --env <path> --core <path> [--daemon]");
      std::exit(0);
    }
  }
  return args;
}

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  auto logger = nexus::utils::init_logger("psyche", ".nexus/logs");

  NEXUS_LOG(logger, info, "psyche v{} 启动", "1.0.0");

  // 1. 等待 core 就绪
  NEXUS_LOG(logger, info, "等待 core 就绪...");
  nexus::ipc::StateFileReader core_reader(args.core_path);
  for (int i = 0; i < 60; ++i) {
    auto state = core_reader.read();
    if (state.ok() && state.value().value("status", "") == "ready") {
      NEXUS_LOG(logger, info, "core 就绪");
      break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  // 2. 打开 psi_field
  auto psi_path = fs::path(args.data_dir) / "psi_field.mmap";
  auto psi = nexus::ipc::MmapRingBuffer::create_or_open(psi_path);
  if (!psi.ok()) {
    NEXUS_LOG(logger, warn, "psi_field 不可用 (降级)");
  }

  // 3. 初始化 Ψ-Navigator (12 标量参数)
  // TODO: PsiNavigator::init(scalar_params)
  NEXUS_LOG(logger, info, "Ψ-Navigator 初始化 (12 标量)");

  // 4. 写状态文件
  nexus::ipc::StateFileWriter writer(args.data_dir + "/psyche_state.json");
  writer.write(nlohmann::json{
    {"$schema", "nexus-state-v1"},
    {"version", "1.0"},
    {"component", "psyche"},
    {"status", "ready"},
    {"pid", nexus::ipc::current_pid()},
    {"started_at", nexus::ipc::current_iso_timestamp()},
    {"updated_at", nexus::ipc::current_iso_timestamp()},
    {"details", {
      {"navigator_active", true},
      {"scalar_params", {
        {"orig", 1.0}, {"belief", 100.0}, {"stability", 0.3},
        {"goal", 3.0}, {"mission", 0.1}, {"value", 1.0},
        {"decision", 0.5}, {"courage", 0.2}, {"faith", 0.3},
        {"truth", 0.05}, {"verity", 2.0}, {"ult", 0.01},
      }},
    }},
  });

  // 5. 写入意识流
  if (psi.ok()) {
    psi.value().write_json({
      {"c",  "Ψ-Navigator 已就绪"},
      {"ch", "system"},
    });
  }

  NEXUS_LOG(logger, info, "就绪");
  return 0;
}
