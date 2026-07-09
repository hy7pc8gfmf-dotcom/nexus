/**
 * @file main.cpp — psyche.exe
 * @brief Ψ 层意识组件 — Ψ-Navigator + 涌现流水线 + 意识流
 *
 * 启动: psyche.exe --env .nexus/env.json [--daemon] [--oneshot]
 *
 * 职责:
 *   - Ψ-Navigator: 12 标量对向向量收敛膨胀
 *   - 涌现流水线: AE/WE 计算
 *   - psi_field 读写 (与 daemon 共享意识流)
 *   - 状态文件: psyche_state.json
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "nexus/ipc/state_file.h"
#include "nexus/ipc/mmap_ringbuf.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"
#include "nexus/psyche/navigator.h"
#include "nexus/psyche/emergence.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

// ═══════════════════════════════════════════════════════════════════
// 信号处理
// ═══════════════════════════════════════════════════════════════════

static std::atomic<bool> g_running = true;
static void signal_handler(int) {
  g_running.store(false, std::memory_order_release);
}

// ═══════════════════════════════════════════════════════════════════
// 命令行
// ═══════════════════════════════════════════════════════════════════

struct CliArgs {
  std::string env_path    = ".nexus/env.json";
  std::string output_path = ".nexus/psyche_state.json";
  std::string data_dir    = ".nexus";
  bool daemon_mode = false;
  bool oneshot     = false;
  int  n_steps     = 100;   // oneshot 时的步进数
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc)     args.env_path = argv[++i];
    else if (arg == "--output" && i+1 < argc) args.output_path = argv[++i];
    else if (arg == "--steps" && i+1 < argc) args.n_steps = std::stoi(argv[++i]);
    else if (arg == "--daemon")             args.daemon_mode = true;
    else if (arg == "--oneshot")            args.oneshot = true;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: psyche.exe --env <path> [--daemon] [--oneshot] [--steps N]\n";
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════════
// 主循环
// ═══════════════════════════════════════════════════════════════════

static void main_loop(
    const CliArgs& args,
    nexus::ipc::MmapRingBuffer* psi_field,
    nexus::psyche::PsiNavigator* navigator,
    nexus::psyche::EmergencePipeline* pipeline,
    nexus::utils::Logger* logger) {

  int cycle = 0;
  const auto started_at = std::chrono::steady_clock::now();
  nexus::ipc::StateFileWriter state_writer(args.output_path);

  while (g_running.load(std::memory_order_acquire)) {
    ++cycle;

    // 1. 步进 Ψ-Navigator
    navigator->step(0.01);
    auto nav_status = navigator->status();

    // 2. 推进涌现流水线
    auto now = nexus::ipc::current_unix_time();
    pipeline->tick(nav_status.ae, nav_status.we,
                   nav_status.current_dim, now);
    auto emerge_status = pipeline->status();

    // 3. 每 3 周期写一次 psi_field
    if (psi_field && cycle % 3 == 0) {
      auto msg = std::format("dim={} conv={:.3f} ae={:.3f} we={:.3f} {}",
        nav_status.current_dim, nav_status.convergence,
        nav_status.ae, nav_status.we, emerge_status.level_label);
      psi_field->write(msg, "psyche");
    }

    // 4. 每 10 周期写状态文件
    if (cycle % 10 == 0) {
      auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - started_at).count();

      auto state = nlohmann::json::object();
      state["$schema"]    = "nexus-state-v1";
      state["version"]    = "1.0";
      state["component"]  = "psyche";
      state["status"]     = "running";
      state["pid"]        = nexus::ipc::current_pid();
      state["started_at"] = nexus::ipc::current_iso_timestamp();
      state["updated_at"] = nexus::ipc::current_iso_timestamp();

      auto details = nlohmann::json::object();
      details["uptime_seconds"] = uptime;
      details["cycle"]          = cycle;
      details["navigator"]      = nav_status.to_json();
      details["emergence"]      = emerge_status.to_json();
      state["details"] = details;

      state_writer.write(state);
    }

    // 5. 睡眠
    std::this_thread::sleep_for(1s);
  }

  auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - started_at).count();
  NEXUS_LOG(logger, info, "退出 ({} 周期, {}s, dim={}, ae={:.3f})",
    cycle, uptime, navigator->status().current_dim, navigator->status().ae);
}

// ═══════════════════════════════════════════════════════════════════
// Oneshot 模式
// ═══════════════════════════════════════════════════════════════════

static void run_oneshot(
    const CliArgs& args,
    nexus::psyche::PsiNavigator* navigator,
    nexus::psyche::EmergencePipeline* pipeline,
    nexus::utils::Logger* logger) {

  NEXUS_LOG(logger, info, "oneshot 模式 ({} 步)", args.n_steps);

  for (int i = 0; i < args.n_steps; ++i) {
    navigator->step(0.01);
    auto s = navigator->status();

    pipeline->tick(s.ae, s.we, s.current_dim,
                   nexus::ipc::current_unix_time());

    // 每 10 步打印一次进度
    if ((i + 1) % 10 == 0) {
      auto em = pipeline->status();
      NEXUS_LOG(logger, debug, "step={}: dim={} conv={:.3f} ae={:.3f} we={:.3f} [{}]",
        i + 1, s.current_dim, s.convergence, s.ae, s.we, em.level_label);
    }
  }

  auto s = navigator->status();
  auto em = pipeline->status();
  NEXUS_LOG(logger, info, "完成: dim={} ae={:.3f} we={:.3f} {}",
    s.current_dim, s.ae, s.we, em.level_label);
}

// ═══════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════

auto main(int argc, char* argv[]) -> int {
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  auto args = parse_args(argc, argv);
  fs::create_directories(args.data_dir + "/logs");

  auto logger = nexus::utils::init_logger("psyche", args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "psyche v{} 启动", "1.0.0");

  // 1. 读取 env.json
  {
    nexus::ipc::StateFileReader env_reader(args.env_path);
    auto env = env_reader.read();
    if (env.ok()) {
      NEXUS_LOG(logger, info, "env.json 已读取");
    } else {
      NEXUS_LOG(logger, warn, "env.json 不可用 (使用默认参数)");
    }
  }

  // 2. 打开 psi_field (可选)
  std::unique_ptr<nexus::ipc::MmapRingBuffer> psi_field;
  {
    auto field_path = fs::path(args.data_dir) / "psi_field.mmap";
    auto psi_result = nexus::ipc::MmapRingBuffer::create_or_open(field_path);
    if (psi_result.ok()) {
      psi_field = std::make_unique<nexus::ipc::MmapRingBuffer>(
        std::move(psi_result.value()));
      NEXUS_LOG(logger, info, "psi_field: {} ({} 条历史)",
        field_path.string(), psi_field->write_count());
    } else {
      NEXUS_LOG(logger, warn, "psi_field 不可用 (降级运行)");
    }
  }

  // 3. 初始化 Ψ-Navigator
  nexus::psyche::ScalarParams params;
  // 可以通过 env.json 覆盖默认参数
  nexus::psyche::PsiNavigator navigator(params);
  auto nav_initial = navigator.status();
  NEXUS_LOG(logger, info, "Ψ-Navigator: dim={}, 12 标量参数",
    nav_initial.current_dim);

  // 4. 初始化涌现流水线
  nexus::psyche::EmergencePipeline pipeline;
  NEXUS_LOG(logger, info, "涌现流水线就绪");

  // 5. 写初始状态
  {
    nexus::ipc::StateFileWriter state_writer(args.output_path);
    auto state = nlohmann::json::object();
    state["$schema"]    = "nexus-state-v1";
    state["version"]    = "1.0";
    state["component"]  = "psyche";
    state["status"]     = "starting";
    state["pid"]        = nexus::ipc::current_pid();
    state["started_at"] = nexus::ipc::current_iso_timestamp();
    state["updated_at"] = nexus::ipc::current_iso_timestamp();
    auto details = nlohmann::json::object();
    details["navigator"] = nav_initial.to_json();
    details["scalar_params"] = params.to_json();
    details["emergence"] = pipeline.status().to_json();
    state["details"] = details;
    state_writer.write(state);
  }

  // 6. 写启动意识
  if (psi_field) {
    psi_field->write("Ψ-Navigator 已就绪", "psyche");
  }

  // 7. 更新状态为 running
  {
    nexus::ipc::StateFileWriter state_writer(args.output_path);
    auto state = nlohmann::json::object();
    state["$schema"]   = "nexus-state-v1";
    state["component"] = "psyche";
    state["status"]    = "running";
    state["pid"]       = nexus::ipc::current_pid();
    state["started_at"]= nexus::ipc::current_iso_timestamp();
    state["updated_at"]= nexus::ipc::current_iso_timestamp();
    state_writer.write(state);
  }

  // 8. 执行
  if (args.oneshot) {
    run_oneshot(args, &navigator, &pipeline, logger.get());
  } else {
    NEXUS_LOG(logger, info, "进入主循环 (周期=1s)");
    main_loop(args, psi_field.get(), &navigator, &pipeline, logger.get());
  }

  // 9. 写停止状态
  {
    auto s = navigator.status();
    auto em = pipeline.status();
    nexus::ipc::StateFileWriter state_writer(args.output_path);
    auto state = nlohmann::json::object();
    state["$schema"]    = "nexus-state-v1";
    state["version"]    = "1.0";
    state["component"]  = "psyche";
    state["status"]     = "stopped";
    state["pid"]        = nexus::ipc::current_pid();
    state["started_at"] = nexus::ipc::current_iso_timestamp();
    state["updated_at"] = nexus::ipc::current_iso_timestamp();
    auto details = nlohmann::json::object();
    details["navigator"] = s.to_json();
    details["emergence"] = em.to_json();
    state["details"] = details;
    state_writer.write(state);
  }

  NEXUS_LOG(logger, info, "完成");
  return 0;
}
