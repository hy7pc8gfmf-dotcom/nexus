// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/self_evolve_trigger.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

EmergenceMonitor::EmergenceMonitor() noexcept
  : window_size_(50), var_threshold_(0.01), consecutive_rounds_(10) {}

// ═══════════════════════════════════════════════════════════════════
// 记录
// ═══════════════════════════════════════════════════════════════════

void EmergenceMonitor::record(double ae, const nlohmann::json& profile) noexcept {
  ae_history_.push_back(ae);
  profile_history_.push_back(profile);
  total_rounds_++;

  // 限制历史窗口
  while (static_cast<int>(ae_history_.size()) > window_size_) {
    ae_history_.pop_front();
    profile_history_.pop_front();
  }
}

// ═══════════════════════════════════════════════════════════════════
// 稳定性检查
// ═══════════════════════════════════════════════════════════════════

auto EmergenceMonitor::stability_check() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_rounds"] = total_rounds_;
  j["history_size"] = static_cast<int>(ae_history_.size());

  if (ae_history_.size() < 2) {
    j["stable"] = false;
    j["variance"] = 0.0;
    j["mean"] = ae_history_.empty() ? 0.0 : ae_history_.back();
    return j;
  }

  // 计算方差
  double mean = 0;
  for (double v : ae_history_) mean += v;
  mean /= ae_history_.size();

  double variance = 0;
  for (double v : ae_history_) variance += (v - mean) * (v - mean);
  variance /= ae_history_.size();

  j["mean"]     = std::round(mean * 10000) / 10000;
  j["variance"] = std::round(variance * 10000) / 10000;
  j["stable"]   = variance < var_threshold_;
  j["stable_rounds"] = stable_rounds_;

  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 触发判断
// ═══════════════════════════════════════════════════════════════════

auto EmergenceMonitor::should_trigger() const noexcept -> bool {
  if (ae_history_.size() < 2) return false;

  double mean = 0;
  for (double v : ae_history_) mean += v;
  mean /= ae_history_.size();

  double variance = 0;
  for (double v : ae_history_) variance += (v - mean) * (v - mean);
  variance /= ae_history_.size();

  if (variance < var_threshold_) {
    return stable_rounds_ >= consecutive_rounds_;
  }
  return false;
}

// ═══════════════════════════════════════════════════════════════════
// 快照
// ═══════════════════════════════════════════════════════════════════

auto EmergenceMonitor::snapshot() const noexcept -> nlohmann::json {
  auto j = stability_check();
  auto recent = nlohmann::json::array();
  int start = std::max(0, static_cast<int>(ae_history_.size()) - 10);
  for (int i = start; i < static_cast<int>(ae_history_.size()); ++i) {
    recent.push_back(ae_history_[i]);
  }
  j["recent_ae"] = recent;
  j["consecutive_rounds"] = consecutive_rounds_;
  j["var_threshold"] = var_threshold_;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 重置
// ═══════════════════════════════════════════════════════════════════

void EmergenceMonitor::reset() noexcept {
  ae_history_.clear();
  profile_history_.clear();
  stable_rounds_ = 0;
  total_rounds_ = 0;
}

}  // namespace nexus::psyche
