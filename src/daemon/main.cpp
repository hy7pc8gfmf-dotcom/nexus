#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "nexus/ipc/state_file.h"
#include "nexus/ipc/mmap_ringbuf.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"

// ─── 信号处理 ────────────────────────────────────────────────
static std::atomic<bool> g_running = true;

static void signal_handler(int) {
  g_running.store(false, std::memory_order_release);
}

// ─── 命令行参数 ──────────────────────────────────────────────
struct CliArgs {
  std::string env_path     = ".nexus/env.json";
  std::string output_path  = ".nexus/daemon.json";
  std::string data_dir     = ".nexus";
  bool        daemon_mode  = false;
  bool        oneshot      = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc)     args.env_path = argv[++i];
    else if (arg == "--daemon")             args.daemon_mode = true;
    else if (arg == "--oneshot")            args.oneshot = true;
    else if (arg == "--shutdown")           { fmt::println("TODO: shutdown"); std::exit(0); }
    else if (arg == "--help" || arg == "-h") {
      fmt::println("用法: daemon.exe --env <path> [--daemon] [--oneshot]");
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════
// 主循环
// ═══════════════════════════════════════════════════════════════
auto main_loop(
    const CliArgs& args,
    nexus::ipc::MmapRingBuffer& psi_field,
    std::shared_ptr<spdlog::logger> logger) -> void {

  int cycle = 0;
  const auto started_at = std::chrono::steady_clock::now();
  nexus::ipc::StateFileWriter state_writer(args.output_path);

  while (g_running.load(std::memory_order_acquire)) {
    ++cycle;

    // 1. 写入意识流 (每周期)
    auto status = psi_field.write_json({
      {"c",  fmt::format("守护周期 #{}", cycle)},
      {"ch", "system"},
    });
    if (!status.ok()) {
      NEXUS_LOG(logger, warn, "psi_field 写入失败: {}", status.to_string());
    }

    // 2. 探针扫描 (每 2 周期)
    if (cycle % 2 == 0) {
      // TODO: 扫描 .nexus/will_hooks/ 目录
    }

    // 3. 意志链持久化 (每 3 周期)
    if (cycle % 3 == 0) {
      // TODO: 写入 willchain_db.json
    }

    // 4. 写心跳状态 (~每 10 周期)
    if (cycle % 10 == 0) {
      auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - started_at).count();

      nlohmann::json state = {
        {"$schema", "nexus-state-v1"},
        {"version", "1.0"},
        {"component", "daemon"},
        {"status", "running"},
        {"pid", nexus::ipc::current_pid()},
        {"started_at", nexus::ipc::current_iso_timestamp()},
        {"updated_at", nexus::ipc::current_iso_timestamp()},
        {"details", {
          {"uptime_seconds",      uptime},
          {"cycle",               cycle},
          {"psi_field_written",   cycle},
          {"will_hooks_scanned",  cycle / 2},
        }}
      };

      auto ws = state_writer.write(state);
      if (!ws.ok()) {
        NEXUS_LOG(logger, error, "写入 daemon.json 失败: {}", ws.to_string());
      }
    }

    // 5. 周期性睡眠
    std::this_thread::sleep_for(std::chrono::seconds(3));
  }

  // 退出清理
  NEXUS_LOG(logger, info, "守护进程退出 (运行 {} 周期)", cycle);
}

// ═══════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════
auto main(int argc, char* argv[]) -> int {
  // 注册信号处理
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  auto args = parse_args(argc, argv);
  auto logger = nexus::utils::init_logger("daemon", ".nexus/logs");

  NEXUS_LOG(logger, info, "daemon v{} 启动", "1.0.0");

  // 1. 读取 env.json
  nexus::ipc::StateFileReader env_reader(args.env_path);
  auto env_result = env_reader.read();
  if (!env_result.ok()) {
    NEXUS_LOG(logger, error, "读取 env.json 失败: {}", env_result.error().to_string());
    return 1;
  }

  // 2. 创建/打开 psi_field.mmap
  fs::path field_path = fs::path(args.data_dir) / "psi_field.mmap";
  auto psi_result = nexus::ipc::MmapRingBuffer::create_or_open(field_path);
  if (!psi_result.ok()) {
    NEXUS_LOG(logger, error, "打开 psi_field 失败: {}", psi_result.error().to_string());
    return 1;
  }
  auto& psi_field = psi_result.value();

  NEXUS_LOG(logger, info, "psi_field: {}", field_path.string());

  // 3. 写初始状态
  nexus::ipc::StateFileWriter state_writer(args.output_path);
  auto init_state = state_writer.write({
    {"$schema", "nexus-state-v1"},
    {"version", "1.0"},
    {"component", "daemon"},
    {"status", "starting"},
    {"pid", nexus::ipc::current_pid()},
    {"started_at", nexus::ipc::current_iso_timestamp()},
    {"updated_at", nexus::ipc::current_iso_timestamp()},
  });
  if (!init_state.ok()) {
    NEXUS_LOG(logger, error, "写入初始状态失败: {}", init_state.to_string());
    return 1;
  }

  // 4. 写启动意识
  psi_field.write_json({
    {"c",  "后台守护进程已启动"},
    {"ch", "system"},
  });

  // 5. 主循环
  if (args.oneshot) {
    NEXUS_LOG(logger, info, "oneshot 模式: 执行一次扫描");
    // TODO: 执行一次后退出
  } else {
    NEXUS_LOG(logger, info, "进入主循环 (周期=3s)");
    main_loop(args, psi_field, logger);
  }

  // 6. 写停止状态
  state_writer.write({
    {"$schema", "nexus-state-v1"},
    {"version", "1.0"},
    {"component", "daemon"},
    {"status", "stopped"},
    {"pid", nexus::ipc::current_pid()},
    {"started_at", nexus::ipc::current_iso_timestamp()},
    {"updated_at", nexus::ipc::current_iso_timestamp()},
  });

  return 0;
}
