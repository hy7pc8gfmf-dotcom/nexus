// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <new>
#include <windows.h>
#endif

#include "nexus/core/thermal_governor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

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
// 温度读取
// ═══════════════════════════════════════════════════════════════════

double ThermalGovernor::read_gpu_temp_() noexcept {
#ifdef _WIN32
  // 通过 nvidia-smi 读取 GPU 温度
  std::array<char, 64> buffer{};
  std::string result;
  FILE* pipe = _popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits 2>nul", "r");
  if (!pipe) return 0.0;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
    result += buffer.data();
  }
  _pclose(pipe);

  if (result.empty()) return 0.0;
  // 去除空白字符
  auto trim_end = result.find_last_not_of(" \n\r\t");
  if (trim_end != std::string::npos) result.erase(trim_end + 1);
  auto trim_start = result.find_first_not_of(" \n\r\t");
  if (trim_start != std::string::npos) result.erase(0, trim_start);
  try {
    return std::stod(result);
  } catch (...) {
    return 0.0;
  }
#else
  // Linux: 通过 nvidia-smi 或 /sys/class/drm/card0/device/hwmon/hwmon*/temp1_input
  FILE* pipe = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
  if (!pipe) return 0.0;
  std::array<char, 64> buffer{};
  std::string result;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) result += buffer.data();
  pclose(pipe);
  if (result.empty()) return 0.0;
  try { return std::stod(result); } catch (...) { return 0.0; }
#endif
}

double ThermalGovernor::read_cpu_temp_() noexcept {
#ifdef _WIN32
  // 通过 WMI 读取 CPU 温度 (需要硬件支持)
  std::array<char, 256> buffer{};
  std::string result;
  FILE* pipe = _popen(
    "powershell -Command \"Get-CimInstance -Namespace root/wmi -ClassName MSAcpi_ThermalZoneTemperature "
    "| Select-Object -First 1 @{N='T';E={[math]::Round(($_.CurrentTemperature - 2732) / 10.0, 1)}} "
    "| Select-Object -ExpandProperty T\" 2>nul",
    "r");
  if (!pipe) return 0.0;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
    result += buffer.data();
  }
  _pclose(pipe);
  if (result.empty()) return 0.0;
  auto trim_end2 = result.find_last_not_of(" \n\r\t");
  if (trim_end2 != std::string::npos) result.erase(trim_end2 + 1);
  auto trim_start2 = result.find_first_not_of(" \n\r\t");
  if (trim_start2 != std::string::npos) result.erase(0, trim_start2);
  try { return std::stod(result); } catch (...) { return 0.0; }
#else
  // Linux: 通过 /sys/class/thermal/thermal_zone0/temp
  FILE* pipe = popen("cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null", "r");
  if (!pipe) return 0.0;
  std::array<char, 32> buffer{};
  std::string result;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) result += buffer.data();
  pclose(pipe);
  if (result.empty()) return 0.0;
  try {
    double millic = std::stod(result);
    return millic / 1000.0;
  } catch (...) { return 0.0; }
#endif
}

// ═══════════════════════════════════════════════════════════════════
// 温度检查 + 决策
// ═══════════════════════════════════════════════════════════════════

auto ThermalGovernor::check() noexcept -> ThermalStatus {
  auto now = std::chrono::duration<double>(
    std::chrono::system_clock::now().time_since_epoch()).count();

  // 缓存
  if (now - last_check_ < check_interval_) {
    return status_;
  }

  double gpu = read_gpu_temp_();
  double cpu = read_cpu_temp_();
  status_.gpu_temp = gpu;
  status_.cpu_temp = cpu;
  last_check_ = now;

  // GPU 决策
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

  // CPU 决策 (叠加在 GPU 之上)
  if (cpu >= cpu_stop_) {
    status_.cooling = true;
    status_.cooldown_until = now + cooldown_sec_;
    status_.throttled = true;
  } else if (cpu >= cpu_throttle_) {
    status_.throttled = true;
  }

  // 冷却结束
  if (status_.cooling && now >= status_.cooldown_until) {
    status_.cooling = false;
    status_.throttled = false;
  }

  // 日志输出 (与 Python 版行为一致)
  if (gpu >= gpu_throttle_) {
    std::cout << "[热敏] GPU " << static_cast<int>(gpu) << "°C "
              << (status_.cooling ? "🔥冷却" : "⚠️降载") << std::endl;
  }
  if (cpu >= cpu_throttle_ && cpu > 0) {
    std::cout << "[热敏] CPU " << static_cast<int>(cpu) << "°C "
              << (cpu >= cpu_stop_ ? "🔥停止" : "⚠️降载") << std::endl;
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
        std::chrono::milliseconds(static_cast<int>(wait * 1000)));
    }
    return true;
  }
  return false;
}

// ═══════════════════════════════════════════════════════════════════
// 后台监测线程
// ═══════════════════════════════════════════════════════════════════

void ThermalGovernor::start_background_monitor() noexcept {
  if (running_.exchange(true)) return;  // 已启动

  monitor_thread_ = std::thread([this]() {
    while (running_.load()) {
      check();
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
  });
  monitor_thread_.detach();
}

void ThermalGovernor::stop_background_monitor() noexcept {
  running_.store(false);
  // detach 的线程无法 join, 依赖 running_ 标志退出
}

}  // namespace nexus::core
