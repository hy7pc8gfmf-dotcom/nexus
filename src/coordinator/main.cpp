/**
 * @file main.cpp — coordinator.exe
 * @brief 生命周期管理器 — 启动/监控/恢复/关闭
 *
 * 启动: coordinator.exe --start      完整启动
 *       coordinator.exe --stop       优雅关闭
 *       coordinator.exe --status     查看状态
 *       coordinator.exe --restart daemon  重启单个组件
 *
 * 编排顺序:
 *   Phase 0: env_checker (串行)
 *   Phase 1: core + algo + daemon (并行)
 *   Phase 2: psyche + bridge (等待 core 就绪后)
 *   Running: 健康检查循环 (每 10s)
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "nexus/ipc/state_file.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

// ═══════════════════════════════════════════════════════════════════
// 全局状态
// ═══════════════════════════════════════════════════════════════════

static std::atomic<bool> g_running = true;
static std::atomic<bool> g_stopping = false;

static void signal_handler(int) {
  g_running.store(false, std::memory_order_release);
  g_stopping.store(true, std::memory_order_release);
}

// ═══════════════════════════════════════════════════════════════════
// 组件定义
// ═══════════════════════════════════════════════════════════════════

struct ComponentDef {
  std::string name;
  std::string exe_name;
  std::vector<std::string> args;
  std::string state_file;
  int startup_timeout_ms = 30000;
  int max_restarts = 3;
  std::vector<std::string> depends_on;
  bool is_daemon = false;  // true = 常驻进程, false = 一次性
};

struct ComponentInstance {
  ComponentDef def;
  void* process_handle = nullptr;  // HANDLE
  int pid = 0;
  bool running = false;
  int restart_count = 0;
  std::chrono::steady_clock::time_point last_restart;
  std::string last_status;
};

// ═══════════════════════════════════════════════════════════════════
// 进程管理
// ═══════════════════════════════════════════════════════════════════

static auto start_process(const ComponentDef& def, const std::string& data_dir)
    -> std::pair<void*, int> {
  // 构建命令行
  std::string cmdline = def.exe_name;
  for (const auto& a : def.args) {
    cmdline += " \"" + a + "\"";
  }

  STARTUPINFOA si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};

  // 创建子进程 (继承句柄, 无窗口)
  std::vector<char> cmd_buf(cmdline.begin(), cmdline.end());
  cmd_buf.push_back('\0');

  BOOL ok = CreateProcessA(
    nullptr,                  // 可执行文件 (NULL = 使用 cmdline 第一个 token)
    cmd_buf.data(),           // 命令行
    nullptr,                  // 进程安全属性
    nullptr,                  // 线程安全属性
    FALSE,                    // 不继承句柄
    CREATE_NO_WINDOW,         // 无控制台窗口
    nullptr,                  // 环境 (继承父进程)
    nullptr,                  // 工作目录 (继承父进程)
    &si, &pi);

  if (!ok) return {nullptr, 0};

  CloseHandle(pi.hThread);
  return {pi.hProcess, static_cast<int>(pi.dwProcessId)};
}

static auto stop_process(void* handle) -> bool {
  if (!handle) return true;
  // 先优雅通知 (TerminateProcess 作为后备)
  TerminateProcess(handle, 0);
  // 等待 100ms 让进程退出
  if (WaitForSingleObject(handle, 100) == WAIT_OBJECT_0) {
    CloseHandle(handle);
    return true;
  }
  // 强制终止
  TerminateProcess(handle, 1);
  CloseHandle(handle);
  return true;
}

static auto is_process_alive(void* handle) -> bool {
  if (!handle) return false;
  DWORD exit_code = 0;
  if (!GetExitCodeProcess(handle, &exit_code)) return false;
  return exit_code == STILL_ACTIVE;
}

// ═══════════════════════════════════════════════════════════════════
// 状态读取
// ═══════════════════════════════════════════════════════════════════

static auto read_component_status(const std::string& path) -> std::string {
  nexus::ipc::StateFileReader reader(path);
  auto result = reader.read(/*retries=*/0);
  if (!result.ok()) return "unknown";
  return result.value().value("status", std::string("unknown"));
}

static auto wait_for_ready(const std::string& state_file, int timeout_ms) -> bool {
  auto deadline = std::chrono::steady_clock::now()
               + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    auto status = read_component_status(state_file);
    if (status == "ready" || status == "running" || status == "degraded") return true;
    if (status == "error") return false;
    std::this_thread::sleep_for(200ms);
  }
  return false;
}

// ═══════════════════════════════════════════════════════════════════
// 组件定义表
// ═══════════════════════════════════════════════════════════════════

static auto create_components(const std::string& data_dir)
    -> std::map<std::string, ComponentInstance> {

  auto state = [&](const std::string& name) {
    return data_dir + "/" + name + (name == "daemon" ? ".json" : "_state.json");
  };

  return {
    {"env",    {{"env",    "env_checker.exe", {"--output", state("env")},       state("env"),    10000, 3, {},    false}}},
    {"core",   {{"core",   "core.exe",        {"--env", state("env"), "--skip-models"},  state("core"),   60000, 3, {"env"}, false}}},
    {"algo",   {{"algo",   "algo.exe",        {"--env", state("env")},          state("algo"),   30000, 3, {"env"}, false}}},
    {"daemon", {{"daemon", "daemon.exe",      {"--env", state("env"), "--daemon"}, state("daemon"), 15000, 3, {"env"}, true}}},
    {"psyche", {{"psyche", "psyche.exe",      {"--env", state("env"), "--oneshot", "--steps", "50"}, state("psyche"), 30000, 3, {"core"}, false}}},
    {"bridge", {{"bridge", "bridge.exe",      {"--env", state("env")},          state("bridge"), 30000, 3, {"core"}, false}}},
  };
}

// ═══════════════════════════════════════════════════════════════════
// 启动编排
// ═══════════════════════════════════════════════════════════════════

static auto launch_phase0(std::map<std::string, ComponentInstance>& components,
                          nexus::utils::Logger* logger) -> bool {
  // Phase 0: env_checker (串行)
  NEXUS_LOG(logger, info, "阶段 0: 启动 env_checker");
  auto& env = components["env"];
  auto [handle, pid] = start_process(env.def, "");
  if (!handle) {
    NEXUS_LOG(logger, error, "env_checker 启动失败");
    return false;
  }
  env.process_handle = handle;
  env.pid = pid;

  bool ready = wait_for_ready(env.def.state_file, env.def.startup_timeout_ms);
  if (ready) {
    env.running = true;
    env.last_status = "ready";
    NEXUS_LOG(logger, info, "env_checker 就绪");
    return true;
  }
  NEXUS_LOG(logger, error, "env_checker 启动超时");
  stop_process(handle);
  return false;
}

static auto launch_phase1(std::map<std::string, ComponentInstance>& components,
                          nexus::utils::Logger* logger) -> bool {
  // Phase 1: core + algo + daemon (并行)
  NEXUS_LOG(logger, info, "阶段 1: 启动 core, algo, daemon (并行)");

  for (const auto& name : {"core", "algo", "daemon"}) {
    auto& comp = components[name];
    auto [handle, pid] = start_process(comp.def, "");
    if (handle) {
      comp.process_handle = handle;
      comp.pid = pid;
      NEXUS_LOG(logger, info, "  {} 已启动 (PID {})", name, pid);
    } else {
      NEXUS_LOG(logger, warn, "  {} 启动失败", name);
    }
  }

  // 等待 core 就绪 (最慢)
  {
    auto& core = components["core"];
    bool ready = wait_for_ready(core.def.state_file, core.def.startup_timeout_ms);
    if (ready) {
      core.running = true;
      core.last_status = "ready";
      NEXUS_LOG(logger, info, "core 就绪");
    } else {
      NEXUS_LOG(logger, warn, "core 启动超时, 降级运行");
      core.last_status = "timeout";
    }
  }

  return true;
}

static auto launch_phase2(std::map<std::string, ComponentInstance>& components,
                          nexus::utils::Logger* logger) -> void {
  // Phase 2: psyche + bridge (等 core 就绪后)
  NEXUS_LOG(logger, info, "阶段 2: 启动 psyche, bridge (并行)");

  for (const auto& name : {"psyche", "bridge"}) {
    auto& comp = components[name];
    auto [handle, pid] = start_process(comp.def, "");
    if (handle) {
      comp.process_handle = handle;
      comp.pid = pid;
      NEXUS_LOG(logger, info, "  {} 已启动 (PID {})", name, pid);
      bool ready = wait_for_ready(comp.def.state_file, comp.def.startup_timeout_ms);
      if (ready) {
        comp.running = true;
        comp.last_status = "ready";
        NEXUS_LOG(logger, info, "  {} 就绪", name);
      } else {
        NEXUS_LOG(logger, warn, "  {} 启动超时", name);
      }
    } else {
      NEXUS_LOG(logger, warn, "  {} 启动失败", name);
    }
  }
}

// ═══════════════════════════════════════════════════════════════════
// 健康检查循环
// ═══════════════════════════════════════════════════════════════════

static void health_check_loop(
    std::map<std::string, ComponentInstance>& components,
    nexus::utils::Logger* logger) {

  NEXUS_LOG(logger, info, "进入健康检查循环 (每 10s)");

  while (g_running.load(std::memory_order_acquire)) {
    for (auto& [name, inst] : components) {
      if (!inst.running) continue;
      if (name == "env") continue;  // env 是一次性的
      if (!inst.def.is_daemon) continue;  // 一次性组件不监控

      // 检查进程存活
      bool alive = is_process_alive(inst.process_handle);
      if (!alive) {
        auto status = read_component_status(inst.def.state_file);
        if (status == "stopped") {
          NEXUS_LOG(logger, info, "{} 正常停止", name);
          inst.running = false;
          continue;
        }

        // 检查重启频率
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          now - inst.last_restart).count();

        if (elapsed < 60 && inst.restart_count >= inst.def.max_restarts) {
          NEXUS_LOG(logger, error, "{} 崩溃过于频繁, 停止拉起", name);
          inst.running = false;
          continue;
        }

        // 自动拉起
        NEXUS_LOG(logger, warn, "{} 崩溃, 自动拉起 (第 {} 次)", name, inst.restart_count + 1);
        auto [handle, pid] = start_process(inst.def, "");
        if (handle) {
          inst.process_handle = handle;
          inst.pid = pid;
          inst.restart_count++;
          inst.last_restart = now;

          bool ready = wait_for_ready(inst.def.state_file, inst.def.startup_timeout_ms);
          if (ready) {
            inst.running = true;
            NEXUS_LOG(logger, info, "{} 恢复运行", name);
          } else {
            NEXUS_LOG(logger, warn, "{} 拉起超时", name);
          }
        }
      }

      // 读取状态更新
      auto status = read_component_status(inst.def.state_file);
      if (status != inst.last_status) {
        inst.last_status = status;
      }
    }

    // 写 coordinator 状态
    auto state = nlohmann::json::object();
    state["$schema"]    = "nexus-state-v1";
    state["version"]    = "1.0";
    state["component"]  = "coordinator";
    state["status"]     = g_stopping.load() ? "stopping" : "running";
    state["pid"]        = nexus::ipc::current_pid();
    state["started_at"] = nexus::ipc::current_iso_timestamp();
    state["updated_at"] = nexus::ipc::current_iso_timestamp();

    auto details = nlohmann::json::object();
    for (const auto& [name, inst] : components) {
      auto comp = nlohmann::json::object();
      comp["status"]  = inst.last_status;
      comp["pid"]     = inst.pid;
      comp["running"] = inst.running;
      comp["restarts"]= inst.restart_count;
      details[name]   = comp;
    }
    state["details"] = details;

    nexus::ipc::StateFileWriter writer(".nexus/coordinator_state.json");
    writer.write(state);

    if (g_stopping.load()) break;
    std::this_thread::sleep_for(10s);
  }
}

// ═══════════════════════════════════════════════════════════════════
// 优雅关闭
// ═══════════════════════════════════════════════════════════════════

static void orchestrate_shutdown(
    std::map<std::string, ComponentInstance>& components,
    nexus::utils::Logger* logger) {

  NEXUS_LOG(logger, info, "优雅关闭: bridge → psyche → algo → core → daemon");

  const std::vector<std::string> shutdown_order = {
    "bridge", "psyche", "algo", "core", "daemon"
  };

  for (const auto& name : shutdown_order) {
    auto& comp = components[name];
    if (!comp.running && !comp.process_handle) continue;
    if (!comp.def.is_daemon) continue;  // 一次性组件已退出, 无需关闭
    NEXUS_LOG(logger, info, "等待停止 {}", name);
    stop_process(comp.process_handle);
    comp.running = false;
    comp.process_handle = nullptr;
    std::this_thread::sleep_for(50ms);
  }

  NEXUS_LOG(logger, info, "所有组件已停止");
}

// ═══════════════════════════════════════════════════════════════════
// 命令行
// ═══════════════════════════════════════════════════════════════════

enum class Action { kStart, kStop, kRestart, kStatus, kList };

struct CliArgs {
  Action action = Action::kStart;
  std::string component;
  std::string data_dir = ".nexus";
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--start")                  args.action = Action::kStart;
    else if (arg == "--stop")              args.action = Action::kStop;
    else if (arg == "--restart" && i+1<argc){ args.action = Action::kRestart; args.component = argv[++i]; }
    else if (arg == "--status")            args.action = Action::kStatus;
    else if (arg == "--list")              args.action = Action::kList;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: coordinator.exe --start|--stop|--restart <comp>|--status|--list\n";
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════

auto main(int argc, char* argv[]) -> int {
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  auto args = parse_args(argc, argv);
  fs::create_directories(args.data_dir + "/logs");

  auto logger = nexus::utils::init_logger("coordinator", args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "coordinator v{} 启动", "1.0.0");

  auto components = create_components(args.data_dir);

  switch (args.action) {
    case Action::kStart: {
      // 完整启动序列
      if (!launch_phase0(components, logger.get())) {
        NEXUS_LOG(logger, error, "Phase 0 失败, 终止启动");
        return 1;
      }
      launch_phase1(components, logger.get());
      launch_phase2(components, logger.get());

      // 进入健康检查循环
      health_check_loop(components, logger.get());

      // 收到 SIGTERM 后优雅关闭
      orchestrate_shutdown(components, logger.get());
      break;
    }

    case Action::kStop: {
      for (auto& [name, inst] : components) {
        inst.running = true;  // 假装正在运行, 让 stop 执行
      }
      orchestrate_shutdown(components, logger.get());
      break;
    }

    case Action::kRestart: {
      auto it = components.find(args.component);
      if (it == components.end()) {
        std::cerr << "未知组件: " << args.component << "\n";
        return 1;
      }
      auto& comp = it->second;
      stop_process(comp.process_handle);
      comp.process_handle = nullptr;
      auto [handle, pid] = start_process(comp.def, "");
      if (handle) {
        comp.process_handle = handle;
        comp.pid = pid;
        bool ready = wait_for_ready(comp.def.state_file, comp.def.startup_timeout_ms);
        comp.running = ready;
        std::cout << args.component << " " << (ready ? "已重启" : "重启失败") << "\n";
      }
      break;
    }

    case Action::kStatus: {
      std::cout << "\n=== Nexus 神经系统状态 ===\n";
      std::cout << std::left;
      std::cout.width(14); std::cout << "组件";
      std::cout.width(14); std::cout << "状态";
      std::cout.width(8);  std::cout << "PID";
      std::cout << "重启次数\n";
      std::cout << std::string(50, '─') << "\n";

      for (const auto& [name, inst] : components) {
        auto status = read_component_status(inst.def.state_file);
        std::cout.width(14); std::cout << name;
        std::cout.width(14); std::cout << status;
        std::cout.width(8);  std::cout << (inst.pid ? std::to_string(inst.pid) : "-");
        std::cout << inst.restart_count << "\n";
      }
      std::cout << "\n";
      break;
    }

    case Action::kList: {
      std::cout << "可用组件:\n";
      for (const auto& [name, inst] : components) {
        auto deps = inst.def.depends_on.empty()
          ? std::string("无")
          : std::to_string(inst.def.depends_on.size()) + " 依赖";
        std::cout << "  " << name << ": " << inst.def.exe_name
                  << " (" << deps << ")\n";
      }
      break;
    }
  }

  NEXUS_LOG(logger, info, "完成");
  return 0;
}
