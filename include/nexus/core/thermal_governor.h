// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_CORE_THERMAL_GOVERNOR_H
#define NEXUS_CORE_THERMAL_GOVERNOR_H

/**
 * @file thermal_governor.h
 * @brief 热敏算力治理引擎 — 根据 GPU/CPU 温度调节推理负载
 *
 * 移植自 D:/synapse/engines/neural_thermal_governor.py
 *
 * 温度阈值:
 *   GPU: 88°C 降载 / 90°C 冷却
 *   CPU: 98°C 降载 / 99°C 停止
 *
 * 用法:
 *   ThermalGovernor governor;
 *   governor.start_background_monitor();
 *   // 每次推理前:
 *   if (governor.status().throttled) { /* 降低推理频率 */ }
 *   governor.wait_if_hot();  // 阻塞直到冷却
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "nexus/types/status.h"

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// 热状态
// ═══════════════════════════════════════════════════════════════════

struct ThermalStatus {
  double gpu_temp       = 0.0;  // °C
  double cpu_temp       = 0.0;  // °C
  bool   throttled      = false;
  bool   cooling        = false;
  double cooldown_until = 0.0;  // Unix 时间戳

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 热敏算力治理引擎
// ═══════════════════════════════════════════════════════════════════

class ThermalGovernor {
public:
  ThermalGovernor() noexcept;
  ~ThermalGovernor() noexcept;

  ThermalGovernor(const ThermalGovernor&) = delete;
  ThermalGovernor& operator=(const ThermalGovernor&) = delete;

  // ── 配置 ──

  void set_gpu_throttle(double temp) noexcept { gpu_throttle_ = temp; }
  void set_gpu_cooldown(double temp) noexcept { gpu_cooldown_ = temp; }
  void set_cpu_throttle(double temp) noexcept { cpu_throttle_ = temp; }
  void set_cpu_stop(double temp) noexcept { cpu_stop_ = temp; }
  void set_check_interval_sec(double sec) noexcept { check_interval_ = sec; }
  void set_cooldown_sec(double sec) noexcept { cooldown_sec_ = sec; }

  // ── 核心 API ──

  /// 执行一次温度检查 + 降载/冷却决策
  auto check() noexcept -> ThermalStatus;

  /// 返回当前热状态 (缓存, 不重新读取温度)
  [[nodiscard]] auto status() const noexcept -> ThermalStatus;

  /// 如果温度过高则阻塞等待冷却
  auto wait_if_hot() noexcept -> bool;

  /// 启动后台监测线程 (每 ~5 秒自动检查)
  void start_background_monitor() noexcept;

  /// 停止后台监测线程
  void stop_background_monitor() noexcept;

private:
  // 温度阈值
  double gpu_throttle_  = 88.0;
  double gpu_cooldown_  = 90.0;
  double cpu_throttle_  = 98.0;
  double cpu_stop_      = 99.0;
  double check_interval_ = 2.0;
  double cooldown_sec_  = 3.0;

  // 状态
  ThermalStatus status_;
  double last_check_ = 0.0;
  std::atomic<bool> running_{false};
  std::thread monitor_thread_;

  // 温度读取
  static double read_gpu_temp_() noexcept;
  static double read_cpu_temp_() noexcept;
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_THERMAL_GOVERNOR_H
