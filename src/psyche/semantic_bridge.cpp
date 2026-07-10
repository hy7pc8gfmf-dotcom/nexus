// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_bridge.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto SteerProfile::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]       = name;
  j["default"]    = default_val;
  j["min"]        = min_val;
  j["max"]        = max_val;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

SemanticBridge::SemanticBridge() noexcept {
  init_profiles_();
}

void SemanticBridge::init_profiles_() noexcept {
  profiles_ = {
    {"value", 0.0, -1.0, 1.0},     // 价值取向
    {"decision", 0.0, -1.0, 1.0},  // 决策力度
    {"mission", 0.5, 0.0, 1.0},    // 使命驱动
    {"curiosity", 0.5, 0.0, 1.0},  // 好奇心
    {"confidence", 0.5, 0.0, 1.0}, // 置信度
    {"abstraction", 0.5, 0.0, 1.0},// 抽象层级
    {"precision", 0.5, 0.0, 1.0},  // 精确度
    {"creativity", 0.3, 0.0, 1.0}, // 创造性
    {"risk", 0.3, 0.0, 1.0},       // 风险容忍
    {"speed", 0.5, 0.0, 1.0},      // 推理速度偏好
  };
}

// ═══════════════════════════════════════════════════════════════════
// 加载
// ═══════════════════════════════════════════════════════════════════

auto SemanticBridge::load(bridge::SeedBank* bank) noexcept -> Status {
  if (!bank) {
    return Status::Error(ErrorCode::kInvalidConfig, "seed bank is null");
  }
  bank_ = bank;
  loaded_ = bank->count() > 0;
  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 转向 → 种子投影
// ═══════════════════════════════════════════════════════════════════

auto SemanticBridge::steer_to_seed(const std::vector<double>& steer_vec) const noexcept
    -> std::vector<double> {
  std::vector<double> result(14, 0.0);
  if (steer_vec.empty() || !loaded_) return result;

  for (size_t i = 0; i < std::min(steer_vec.size(), size_t(10)); ++i) {
    double v = steer_vec[i];
    if (i < profiles_.size()) {
      v = std::clamp(v, profiles_[i].min_val, profiles_[i].max_val);
    }
    // 投影到 14D 种子空间 (简化线性映射)
    for (int j = 0; j < 14; ++j) {
      result[j] += v * (1.0 / (1.0 + static_cast<double>(j)));
    }
  }

  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 种子 → 转向投影
// ═══════════════════════════════════════════════════════════════════

auto SemanticBridge::seed_to_steer(const std::vector<double>& seed_vec) const noexcept
    -> std::vector<double> {
  std::vector<double> result(10, 0.0);
  if (seed_vec.size() < 14 || !loaded_) return result;

  for (int i = 0; i < 10; ++i) {
    double val = 0.0;
    for (int j = 0; j < 14; ++j) {
      val += seed_vec[j] * (1.0 / (1.0 + static_cast<double>(j)));
    }
    result[i] = std::clamp(val / 14.0, -1.0, 1.0);
  }

  return result;
}

auto SemanticBridge::steer_profile() const noexcept
    -> std::vector<SteerProfile> {
  return profiles_;
}

auto SemanticBridge::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["n_profiles"] = static_cast<int>(profiles_.size());
  j["loaded"]     = loaded_;
  return j;
}

}  // namespace nexus::psyche
