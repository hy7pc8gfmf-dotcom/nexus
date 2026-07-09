/**
 * @file main.cpp — daemon.exe
 * @brief 后台守护进程 — 心跳 / psi_field 意识流 / 探针扫描
 *
 * 启动: daemon.exe --env .nexus/env.json [--daemon] [--oneshot]
 *
 * 生命周期:
 *   启动 → 写 starting 状态 → 进入主循环 (3s 周期) →
 *   每周期: 写 psi_field → 每 2 周期: 探针扫描 →
 *   每 10 周期: 写心跳状态 → SIGTERM → 优雅退出
 *
 * 常驻进程，由 coordinator 管理生命周期。
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "nexus/ipc/state_file.h"
#include "nexus/ipc/mmap_ringbuf.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

// ═══════════════════════════════════════════════════════════════════
// 信号处理
// ═══════════════════════════════════════════════════════════════════

static std::atomic<bool> g_running = true;

static void signal_handler(int sig) {
  g_running.store(false, std::memory_order_release);
}

// ═══════════════════════════════════════════════════════════════════
// 命令行参数
// ═══════════════════════════════════════════════════════════════════

struct CliArgs {
  std::string env_path    = ".nexus/env.json";
  std::string output_path = ".nexus/daemon.json";
  std::string data_dir    = ".nexus";
  std::string will_hooks  = ".nexus/will_hooks";
  bool daemon_mode = false;
  bool oneshot     = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc)       args.env_path   = argv[++i];
    else if (arg == "--output" && i+1 < argc) args.output_path= argv[++i];
    else if (arg == "--hooks" && i+1 < argc)  args.will_hooks = argv[++i];
    else if (arg == "--daemon")               args.daemon_mode = true;
    else if (arg == "--oneshot")              args.oneshot    = true;
    else if (arg == "--shutdown") {
      // 向运行中的守护进程发送 SIGTERM
      nexus::ipc::StateFileReader reader(args.output_path);
      auto state = reader.read();
      if (state.ok()) {
        auto pid = state.value().value("pid", 0);
        if (pid > 0) {
          std::cout << "发送 SIGTERM 给守护进程 (PID " << pid << ")\n";
#ifdef _WIN32
          // Windows: 没有 kill(pid, SIGTERM)，用 Ctrl+Break 模拟
          auto handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
          if (handle) {
            GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
            CloseHandle(handle);
          }
#else
          std::kill(pid, SIGTERM);
#endif
        }
      }
      std::exit(0);
    }
    else if (arg == "--help" || arg == "-h") {
      std::cout
        << "用法: daemon.exe [options]\n"
        << "  --env <path>     env.json 路径 (默认 .nexus/env.json)\n"
        << "  --output <path>  状态文件路径 (默认 .nexus/daemon.json)\n"
        << "  --hooks <path>   探针钩子目录 (默认 .nexus/will_hooks)\n"
        << "  --daemon         后台守护模式\n"
        << "  --oneshot        执行一次扫描后退出\n"
        << "  --shutdown       停止运行中的守护进程\n"
        << "  --help           帮助\n";
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════════
// 探针扫描 — 扫描 will_hooks 目录
// ═══════════════════════════════════════════════════════════════════

struct WillHook {
  std::string path;
  std::string will_id;
  std::string intent;
  std::string target;
  int priority = 5;
};

/// 扫描 .nexus/will_hooks/ 目录，收集等待处理的意志钩子
static auto scan_will_hooks(const std::string& hooks_dir)
    -> std::vector<WillHook> {
  std::vector<WillHook> hooks;

  if (!fs::exists(hooks_dir)) return hooks;

  try {
    for (const auto& entry : fs::directory_iterator(hooks_dir)) {
      if (!entry.is_regular_file()) continue;
      auto ext = entry.path().extension().string();
      if (ext != ".json") continue;

      // 读取钩子文件
      try {
        std::ifstream ifs(entry.path());
        if (!ifs.is_open()) continue;

        auto data = nlohmann::json::parse(
          std::string(std::istreambuf_iterator<char>(ifs),
                      std::istreambuf_iterator<char>()));

        hooks.push_back(WillHook{
          .path     = entry.path().string(),
          .will_id  = data.value("will_id", data.value("id", entry.path().stem().string())),
          .intent   = data.value("intent", ""),
          .target   = data.value("target", "unknown"),
          .priority = data.value("priority", 5),
        });
      } catch (const std::exception&) {
        // 跳过损坏的钩子文件
      }
    }
  } catch (const std::exception&) {
    // 目录扫描异常，静默处理
  }

  return hooks;
}

/// 消费（删除）已处理的意志钩子文件
static auto consume_will_hook(const std::string& path) -> bool {
  std::error_code ec;
  return fs::remove(path, ec);
}

// ═══════════════════════════════════════════════════════════════════
// 意志链持久化 — 追加到 willchain_db.json
// ═══════════════════════════════════════════════════════════════════

static auto append_will_chain(const std::string& db_path,
                              const WillHook& hook) -> bool {
  try {
    nlohmann::json entry = {
      {"ts",        nexus::ipc::current_unix_time()},
      {"iso",       nexus::ipc::current_iso_timestamp()},
      {"will_id",   hook.will_id},
      {"intent",    hook.intent},
      {"target",    hook.target},
      {"priority",  hook.priority},
      {"consumed",  true},
    };

    std::ofstream ofs(db_path, std::ios::app);
    if (!ofs.is_open()) return false;
    ofs << entry.dump() << "\n";
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

// ═══════════════════════════════════════════════════════════════════
// 主循环
// ═══════════════════════════════════════════════════════════════════

static void main_loop(
    const CliArgs& args,
    nexus::ipc::MmapRingBuffer& psi_field,
    nexus::utils::Logger* logger) {

  int cycle = 0;
  int psi_written = 0;
  int hooks_scanned = 0;
  int hooks_consumed = 0;
  const auto started_at = std::chrono::steady_clock::now();
  nexus::ipc::StateFileWriter state_writer(args.output_path);
  std::string willchain_path = args.data_dir + "/willchain_db.jsonl";

  NEXUS_LOG(logger, info, "进入主循环 (周期=3s)");

  while (g_running.load(std::memory_order_acquire)) {
    ++cycle;

    // ── 1. 写意识流（每周期）──────────────────────────────
    {
      auto msg = std::format("守护周期 #{}", cycle);
      auto status = psi_field.write(msg, "system");
      if (status.ok()) {
        ++psi_written;
      } else {
        NEXUS_LOG(logger, warn, "psi_field 写入失败: {}", status.to_string());
      }
    }

    // ── 2. 探针扫描（每 2 周期 ≈ 6s）─────────────────────
    if (cycle % 2 == 0) {
      auto hooks = scan_will_hooks(args.will_hooks);
      hooks_scanned += static_cast<int>(hooks.size());

      for (const auto& hook : hooks) {
        // 持久化到意志链
        if (append_will_chain(willchain_path, hook)) {
          ++hooks_consumed;
        }

        // 消费后删除钩子文件
        consume_will_hook(hook.path);

        NEXUS_LOG(logger, debug, "意志: {} → {} (prio:{})",
          hook.intent.substr(0, 48), hook.target, hook.priority);
      }

      if (!hooks.empty()) {
        NEXUS_LOG(logger, info, "探针扫描: {} 个新意志, {} 已消费",
          hooks.size(), hooks_consumed);
      }
    }

    // ── 3. 写心跳状态（每 10 周期 ≈ 30s）─────────────────
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
          {"psi_field_written",   psi_written},
          {"will_hooks_scanned",  hooks_scanned},
          {"will_hooks_consumed", hooks_consumed},
        }},
      };

      auto ws = state_writer.write(state);
      if (ws.ok()) {
        NEXUS_LOG(logger, debug, "心跳: cycle={}, uptime={}s", cycle, uptime);
      } else {
        NEXUS_LOG(logger, error, "写入 daemon.json 失败: {}", ws.to_string());
      }
    }

    // ── 4. 睡眠 ──────────────────────────────────────────
    std::this_thread::sleep_for(3s);
  }

  auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - started_at).count();
  NEXUS_LOG(logger, info, "守护进程退出 (运行 {} 周期, {}s)", cycle, uptime);
}

// ═══════════════════════════════════════════════════════════════════
// Oneshot 模式 — 执行一次扫描后退出
// ═══════════════════════════════════════════════════════════════════

static void run_oneshot(const CliArgs& args,
                        nexus::utils::Logger* logger) {
  NEXUS_LOG(logger, info, "oneshot 模式");

  // 扫描一次
  auto hooks = scan_will_hooks(args.will_hooks);
  NEXUS_LOG(logger, info, "探针扫描: {} 个意志钩子", hooks.size());

  for (const auto& hook : hooks) {
    NEXUS_LOG(logger, debug, "  意志: {} → {}", hook.intent, hook.target);
  }
}

// ═══════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════

auto main(int argc, char* argv[]) -> int {
  // 注册信号处理
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  auto args = parse_args(argc, argv);

  // 初始化日志
  auto logger = nexus::utils::init_logger("daemon",
    args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "daemon v{} 启动", "1.0.0");

  // 1. 读取 env.json
  {
    nexus::ipc::StateFileReader env_reader(args.env_path);
    auto env = env_reader.read();
    if (!env.ok()) {
      NEXUS_LOG(logger, warn, "env.json 不可用: {} (降级运行)",
        env.error().to_string());
    } else {
      NEXUS_LOG(logger, info, "env.json 已读取");
    }
  }

  // 2. 确保目录存在
  fs::create_directories(args.will_hooks);

  // 3. 创建/打开 psi_field.mmap
  auto field_path = fs::path(args.data_dir) / "psi_field.mmap";
  auto psi_result = nexus::ipc::MmapRingBuffer::create_or_open(field_path);
  if (!psi_result.ok()) {
    NEXUS_LOG(logger, error, "打开 psi_field 失败: {}",
      psi_result.error().to_string());
    return 1;
  }
  auto& psi_field = psi_result.value();
  NEXUS_LOG(logger, info, "psi_field: {} ({} 条历史)",
    field_path.string(), psi_field.write_count());

  // 4. 写初始状态
  {
    nexus::ipc::StateFileWriter state_writer(args.output_path);
    auto s = state_writer.write({
      {"$schema", "nexus-state-v1"},
      {"version", "1.0"},
      {"component", "daemon"},
      {"status", "starting"},
      {"pid", nexus::ipc::current_pid()},
      {"started_at", nexus::ipc::current_iso_timestamp()},
      {"updated_at", nexus::ipc::current_iso_timestamp()},
    });
    if (!s.ok()) {
      NEXUS_LOG(logger, error, "写入初始状态失败: {}", s.to_string());
      return 1;
    }
  }

  // 5. 写启动意识
  psi_field.write("后台守护进程已启动", "system");
  NEXUS_LOG(logger, info, "psi_field 已写入启动意识");

  // 6. 更新状态为 running
  {
    nexus::ipc::StateFileWriter state_writer(args.output_path);
    state_writer.write({
      {"$schema", "nexus-state-v1"},
      {"version", "1.0"},
      {"component", "daemon"},
      {"status", "running"},
      {"pid", nexus::ipc::current_pid()},
      {"started_at", nexus::ipc::current_iso_timestamp()},
      {"updated_at", nexus::ipc::current_iso_timestamp()},
    });
  }

  // 7. 执行
  if (args.oneshot) {
    run_oneshot(args, logger.get());
  } else {
    main_loop(args, psi_field, logger.get());
  }

  // 8. 写停止状态
  {
    nexus::ipc::StateFileWriter state_writer(args.output_path);
    state_writer.write({
      {"$schema", "nexus-state-v1"},
      {"version", "1.0"},
      {"component", "daemon"},
      {"status", "stopped"},
      {"pid", nexus::ipc::current_pid()},
      {"started_at", nexus::ipc::current_iso_timestamp()},
      {"updated_at", nexus::ipc::current_iso_timestamp()},
    });
  }

  NEXUS_LOG(logger, info, "完成");
  return 0;
}
