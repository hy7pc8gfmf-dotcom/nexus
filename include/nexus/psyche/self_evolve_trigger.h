// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SELF_EVOLVE_TRIGGER_H
#define NEXUS_PSYCHE_SELF_EVOLVE_TRIGGER_H

/**
 * @file self_evolve_trigger.h
 * @brief 自演进触发器 — 涌现稳定性监测
 *
 * 移植自 D:/synapse/semantic_tn_self_evolve.py (555行)
 *
 * 当全局涌现模式连续 N 轮稳定 (低方差) 时,
 * 触发 self_evolve 模块进行改进循环。
 */

#include <cstdint>
#include <deque>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 涌现监测器
// ═══════════════════════════════════════════════════════════════════

class EmergenceMonitor {
public:
  EmergenceMonitor() noexcept;

  /// 记录一轮涌现数据
  void record(double ae, const nlohmann::json& profile) noexcept;

  /// 稳定性检查
  auto stability_check() const noexcept -> nlohmann::json;

  /// 是否应触发自演进
  auto should_trigger() const noexcept -> bool;

  /// 获取当前快照
  auto snapshot() const noexcept -> nlohmann::json;

  /// 重置
  void reset() noexcept;

  /// 配置
  void set_window(int n) noexcept { window_size_ = n; }
  void set_variance_threshold(double t) noexcept { var_threshold_ = t; }
  void set_consecutive_rounds(int n) noexcept { consecutive_rounds_ = n; }

private:
  int window_size_ = 50;
  double var_threshold_ = 0.01;
  int consecutive_rounds_ = 10;

  std::deque<double> ae_history_;
  std::deque<nlohmann::json> profile_history_;
  int stable_rounds_ = 0;
  int total_rounds_ = 0;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SELF_EVOLVE_TRIGGER_H
