#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "nexus/ipc/state_file.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"

// ═══════════════════════════════════════════════════════════════════
// Coordinator — 神经系统生命周期管理器
// ═══════════════════════════════════════════════════════════════════

// ─── 信号处理 ────────────────────────────────────────────────
static std::atomic<bool> g_running = true;
static void signal_handler(int) {
  g_running.store(false, std::memory_order_release);
}

// ─── 组件定义 ────────────────────────────────────────────────
struct ComponentDef {
  std::string name;
  std::string exe_name;
  std::vector<std::string> args;
  std::string state_file;        // 状态文件路径
  int startup_timeout_ms = 30000; // 启动超时
  int max_restarts_per_minute = 3;
  std::vector<std::string> depends_on;  // 依赖组件
};

struct ComponentInstance {
  ComponentDef def;
  void* process_handle = nullptr;  // HANDLE
  int  pid = 0;
  bool running = false;
  int  restart_count = 0;
  std::chrono::steady_clock::time_point last_restart;
};

// ─── 启动阶段 ├──────────────────────────────────────────────
enum class Phase {
  kInit,
  kLaunchEnv,
  kWaitEnv,
  kLaunchCore,
  kLaunchAlgoDaemon,
  kWaitCore,
  kLaunchPsycheBridge,
  kRunning,
  kStopping,
  kDone,
};

// ═══════════════════════════════════════════════════════════════
// 组件进程管理
// ═══════════════════════════════════════════════════════════════

// ── Windows 进程启动 ─────────────────────────────────────────
static auto start_process(const ComponentDef& def) -> void* {
  // 构建命令行
  std::string cmdline = def.exe_name;
  for (const auto& a : def.args) {
    cmdline += " \"" + a + "\"";
  }

  // ── 使用 CreateProcess 启动 ──
  // 实际实现将使用 CreateProcessW 并设置 DETACHED_PROCESS
  // 此处为架构占位

  fmt::println("  [coordinator] 启动 {}: {}", def.name, cmdline);
  return nullptr;  // 返回进程 HANDLE
}

static auto stop_process(void* handle) -> bool {
  if (!handle) return true;
  // ── PostThreadMessage(SIGTERM) → 等待 5s → TerminateProcess ──
  fmt::println("  [coordinator] 停止进程");
  return true;
}

static auto is_process_alive(void* handle) -> bool {
  if (!handle) return false;
  // ── WaitForSingleObject(handle, 0) == WAIT_TIMEOUT ──
  return false;
}

// ─── 组件状态读取 ────────────────────────────────────────────
static auto read_component_status(const std::string& state_file)
    -> ComponentStatus {
  nexus::ipc::StateFileReader reader(state_file);
  auto result = reader.read();
  if (!result.ok()) return ComponentStatus::kError;

  auto state = ComponentStateBase::from_json(result.value());
  if (!state.ok()) return ComponentStatus::kError;

  return state.value().status;
}

// ─── 等待组件就绪 ────────────────────────────────────────────
static auto wait_for_component(const std::string& state_file,
                                int timeout_ms) -> bool {
  auto deadline = std::chrono::steady_clock::now()
                + std::chrono::milliseconds(timeout_ms);

  while (std::chrono::steady_clock::now() < deadline) {
    auto status = read_component_status(state_file);
    if (status == ComponentStatus::kReady) return true;
    if (status == ComponentStatus::kError) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  return false;  // timeout
}

// ═══════════════════════════════════════════════════════════════
// 主循环 — 健康检查 + 崩溃恢复
// ═══════════════════════════════════════════════════════════════
static void health_check_loop(
    std::map<std::string, ComponentInstance>& components,
    std::shared_ptr<spdlog::logger> logger) {

  while (g_running.load(std::memory_order_acquire)) {
    for (auto& [name, inst] : components) {
      if (!inst.running) continue;

      // 检查进程存活
      if (!is_process_alive(inst.process_handle)) {
        // 检查状态文件
        auto status = read_component_status(inst.def.state_file);
        if (status == ComponentStatus::kStopped) {
          inst.running = false;
          NEXUS_LOG(logger, info, "{} 正常停止", name);
          continue;
        }

        // 检查重启频率
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          now - inst.last_restart).count();

        if (elapsed < 60 && inst.restart_count >= inst.def.max_restarts_per_minute) {
          NEXUS_LOG(logger, error,
            "{} 崩溃过于频繁 ({} 次/分钟)，停止自动拉起", name, inst.restart_count);
          inst.running = false;
          continue;
        }

        // 自动拉起
        NEXUS_LOG(logger, warn, "{} 崩溃，自动拉起 (第 {} 次)", name, inst.restart_count + 1);
        inst.process_handle = start_process(inst.def);
        inst.restart_count++;
        inst.last_restart = now;

        // 等待就绪
        bool ready = wait_for_component(inst.def.state_file, inst.def.startup_timeout_ms);
        if (ready) {
          inst.running = true;
          NEXUS_LOG(logger, info, "{} 恢复运行", name);
        } else {
          NEXUS_LOG(logger, error, "{} 拉起失败", name);
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
}

// ═══════════════════════════════════════════════════════════════
// 启动编排
// ═══════════════════════════════════════════════════════════════
static auto orchestrate_startup(
    std::map<std::string, ComponentInstance>& components,
    std::shared_ptr<spdlog::logger> logger) -> Phase {

  // Phase 0: env_checker (串行)
  NEXUS_LOG(logger, info, "阶段 0: 启动 env_checker");
  {
    auto& env = components["env"];
    env.process_handle = start_process(env.def);
    bool ready = wait_for_component(env.def.state_file, env.def.startup_timeout_ms);
    if (!ready) {
      NEXUS_LOG(logger, error, "env_checker 启动失败");
      return Phase::kDone;
    }
    env.running = true;
  }

  // Phase 1: core + algo + daemon (并行)
  NEXUS_LOG(logger, info, "阶段 1: 启动 core, algo, daemon (并行)");
  for (const auto& name : {"core", "algo", "daemon"}) {
    auto& comp = components[name];
    comp.process_handle = start_process(comp.def);
  }

  // 等待 core 就绪 (最慢)
  {
    bool ready = wait_for_component(
      components["core"].def.state_file,
      components["core"].def.startup_timeout_ms);
    if (ready) {
      components["core"].running = true;
      NEXUS_LOG(logger, info, "core 就绪");
    } else {
      NEXUS_LOG(logger, error, "core 启动超时");
    }
  }

  // Phase 2: psyche + bridge (等 core 就绪后)
  NEXUS_LOG(logger, info, "阶段 2: 启动 psyche, bridge (并行)");
  for (const auto& name : {"psyche", "bridge"}) {
    auto& comp = components[name];
    comp.process_handle = start_process(comp.def);
    bool ready = wait_for_component(comp.def.state_file, comp.def.startup_timeout_ms);
    if (ready) {
      comp.running = true;
      NEXUS_LOG(logger, info, "{} 就绪", name);
    }
  }

  return Phase::kRunning;
}

// ═══════════════════════════════════════════════════════════════
// 优雅关闭
// ═══════════════════════════════════════════════════════════════
static void orchestrate_shutdown(
    std::map<std::string, ComponentInstance>& components,
    std::shared_ptr<spdlog::logger> logger) {

  NEXUS_LOG(logger, info, "优雅关闭: bridge → psyche → algo → daemon → core");

  const std::vector<std::string> shutdown_order = {
    "bridge", "psyche", "algo", "daemon", "core"
  };

  for (const auto& name : shutdown_order) {
    auto& comp = components[name];
    if (!comp.running) continue;

    NEXUS_LOG(logger, info, "停止 {}", name);
    if (!stop_process(comp.process_handle)) {
      NEXUS_LOG(logger, warn, "{} 未响应，强制终止", name);
    }
    comp.running = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  NEXUS_LOG(logger, info, "所有组件已停止");
}

// ═══════════════════════════════════════════════════════════════
// 定义组件
// ═══════════════════════════════════════════════════════════════
static auto create_component_defs(const std::string& data_dir) noexcept
    -> std::map<std::string, ComponentInstance> {

  auto env_path   = data_dir + "/env.json";
  auto core_path  = data_dir + "/core_state.json";
  auto algo_path  = data_dir + "/algo_state.json";
  auto daemon_path= data_dir + "/daemon.json";
  auto psyche_path= data_dir + "/psyche_state.json";
  auto bridge_path= data_dir + "/bridge_state.json";

  return {
    {"env",    {{"env",    "env_checker.exe", {"--output", env_path},                               env_path,    10000, 3, {}}}},
    {"core",   {{"core",   "core.exe",        {"--env", env_path},                                  core_path,   60000, 3, {"env"}}}},
    {"algo",   {{"algo",   "algo.exe",        {"--env", env_path},                                  algo_path,   30000, 3, {"env"}}}},
    {"daemon", {{"daemon", "daemon.exe",      {"--env", env_path, "--daemon"},                      daemon_path, 15000, 3, {"env"}}}},
    {"psyche", {{"psyche", "psyche.exe",      {"--env", env_path, "--core", core_path},             psyche_path, 30000, 3, {"core"}}}},
    {"bridge", {{"bridge", "bridge.exe",      {"--env", env_path, "--core", core_path},             bridge_path, 30000, 3, {"core"}}}},
  };
}

// ═══════════════════════════════════════════════════════════════
// 命令行参数
// ═══════════════════════════════════════════════════════════════
struct CliArgs {
  enum Action { kStart, kStop, kRestart, kStatus, kList };
  Action action = kStart;
  std::string component;  // for --restart
  std::string data_dir = ".nexus";
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--start")                     args.action = CliArgs::kStart;
    else if (arg == "--stop")                 args.action = CliArgs::kStop;
    else if (arg == "--restart" && i+1 < argc){ args.action = CliArgs::kRestart; args.component = argv[++i]; }
    else if (arg == "--status")               args.action = CliArgs::kStatus;
    else if (arg == "--list")                 args.action = CliArgs::kList;
    else if (arg == "--data-dir" && i+1 < argc) args.data_dir = argv[++i];
    else if (arg == "--help" || arg == "-h") {
      fmt::println("用法: coordinator.exe [--start|--stop|--restart <comp>|--status|--list] [--data-dir <path>]");
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════
auto main(int argc, char* argv[]) -> int {
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  auto args = parse_args(argc, argv);
  auto logger = nexus::utils::init_logger("coordinator", args.data_dir + "/logs");

  NEXUS_LOG(logger, info, "coordinator v{} 启动", "1.0.0");

  auto components = create_component_defs(args.data_dir);

  switch (args.action) {
    case CliArgs::kStart: {
      auto final_phase = orchestrate_startup(components, logger);
      if (final_phase == Phase::kRunning) {
        health_check_loop(components, logger);
      }
      // SIGTERM → shutdown
      orchestrate_shutdown(components, logger);
      break;
    }
    case CliArgs::kStop: {
      orchestrate_shutdown(components, logger);
      break;
    }
    case CliArgs::kRestart: {
      if (components.count(args.component) == 0) {
        fmt::println(stderr, "未知组件: {}", args.component);
        return 1;
      }
      auto& comp = components[args.component];
      stop_process(comp.process_handle);
      comp.process_handle = start_process(comp.def);
      bool ready = wait_for_component(
        comp.def.state_file, comp.def.startup_timeout_ms);
      comp.running = ready;
      fmt::println("{} {}", args.component, ready ? "已重启" : "重启失败");
      break;
    }
    case CliArgs::kStatus: {
      fmt::println("Nexus 神经系统状态");
      fmt::println("──────────────────────────────");
      for (const auto& [name, inst] : components) {
        auto status = read_component_status(inst.def.state_file);
        fmt::println("  {:12s}  {}", name,
          nexus::component_status_to_string(status));
      }
      break;
    }
    case CliArgs::kList: {
      fmt::println("可用组件:");
      for (const auto& [name, inst] : components) {
        fmt::println("  {:12s}  {}", name, inst.def.exe_name);
      }
      break;
    }
  }

  return 0;
}
