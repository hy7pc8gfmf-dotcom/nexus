// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/core/thermal_governor.h"

// <thread> 和 <atomic> 仅在 .cpp 中包含, 避免 MSVC SAL 冲突
#include <atomic>
#include <thread>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

// _popen (MSVC) / popen (POSIX) 均在 <cstdio> 中声明
// 不需要 #include <windows.h>

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto ThermalStatus::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["gpu_temp"]       = std::round(gpu_temp * 10) / 10;
  j["cpu_temp"]       = std::round(cpu_temp * 10) / 10;
  j["throttled"]      = throttled;
  j["cooling"]        = cooling;
  j["cooldown_until"] = cooldown_until;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造 / 析构
// ═══════════════════════════════════════════════════════════════════

ThermalGovernor::ThermalGovernor() noexcept {
  status_.gpu_temp = 0.0;
  status_.cpu_temp = 0.0;
  status_.throttled = false;
  status_.cooling = false;
  status_.cooldown_until = 0.0;
  last_check_ = 0.0;
}

ThermalGovernor::~ThermalGovernor() noexcept {
  stop_background_monitor();
}

// ═══════════════════════════════════════════════════════════════════
// 温度读取辅助
// ═══════════════════════════════════════════════════════════════════

static std::string run_cmd_(const char* cmd) {
  std::array<char, 256> buffer{};
  std::string result;
  FILE* pipe = nullptr;
#ifdef _WIN32
  pipe = _popen(cmd, "r");
#else
  pipe = popen(cmd, "r");
#endif
  if (!pipe) return result;
  while (fgets(buffer.data(), (int)buffer.size(), pipe)) {
    result += buffer.data();
  }
#ifdef _WIN32
  _pclose(pipe);
#else
  pclose(pipe);
#endif
  auto e = result.find_last_not_of(" \r\n\t");
  if (e != std::string::npos) result.erase(e + 1);
  auto b = result.find_first_not_of(" \r\n\t");
  if (b != std::string::npos) result.erase(0, b);
  return result;
}

double ThermalGovernor::read_gpu_temp_() noexcept {
  auto out = run_cmd_(
    "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits 2>/dev/null");
  if (out.empty()) return 0.0;
  try { return std::stod(out); } catch (...) { return 0.0; }
}

double ThermalGovernor::read_cpu_temp_() noexcept {
#ifdef _WIN32
  auto out = run_cmd_(
    "powershell -Command \"Get-CimInstance -Namespace root/wmi "
    "-ClassName MSAcpi_ThermalZoneTemperature "
    "| Select-Object -First 1 @{N='T';E={[math]::Round(($_.CurrentTemperature - 2732) / 10.0, 1)}} "
    "| Select-Object -ExpandProperty T\" 2>nul");
#else
  auto out = run_cmd_("cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null");
  if (!out.empty()) {
    try { return std::stod(out) / 1000.0; } catch (...) { return 0.0; }
  }
  return 0.0;
#endif
  if (out.empty()) return 0.0;
  try { return std::stod(out); } catch (...) { return 0.0; }
}

// ═══════════════════════════════════════════════════════════════════
// 温度检查
// ═══════════════════════════════════════════════════════════════════

auto ThermalGovernor::check() noexcept -> ThermalStatus {
  auto now = std::chrono::duration<double>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  if (now - last_check_ < check_interval_) return status_;

  double gpu = read_gpu_temp_();
  double cpu = read_cpu_temp_();
  status_.gpu_temp = gpu;
  status_.cpu_temp = cpu;
  last_check_ = now;

  if (gpu >= gpu_cooldown_) {
    status_.cooling = true;
    status_.cooldown_until = now + cooldown_sec_;
    status_.throttled = true;
  } else if (gpu >= gpu_throttle_) {
    status_.throttled = true;
    status_.cooling = false;
  } else {
    status_.throttled = false;
    status_.cooling = false;
  }

  if (cpu >= cpu_stop_) {
    status_.cooling = true;
    status_.cooldown_until = now + cooldown_sec_;
    status_.throttled = true;
  } else if (cpu >= cpu_throttle_) {
    status_.throttled = true;
  }

  if (status_.cooling && now >= status_.cooldown_until) {
    status_.cooling = false;
    status_.throttled = false;
  }

  if (gpu >= gpu_throttle_) {
    std::cout << "[thermal] GPU " << (int)gpu << "C "
              << (status_.cooling ? "COOLDOWN" : "THROTTLE") << std::endl;
  }
  return status_;
}

auto ThermalGovernor::status() const noexcept -> ThermalStatus {
  return status_;
}

auto ThermalGovernor::wait_if_hot() noexcept -> bool {
  auto s = check();
  if (s.cooling) {
    double wait = s.cooldown_until -
      std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (wait > 0) {
      std::this_thread::sleep_for(
        std::chrono::milliseconds((int)(wait * 1000)));
    }
    return true;
  }
  return false;
}

void ThermalGovernor::start_background_monitor() noexcept {
  if (running_) return;
  running_ = true;

  // 后台监测线程 — 纯粹在 .cpp 中管理
  std::thread t([this]() {
    while (running_) {
      check();
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
  });
  t.detach();
}

void ThermalGovernor::stop_background_monitor() noexcept {
  running_ = false;
}

}  // namespace nexus::core
