// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file navigator.cpp
 * @brief Ψ-Navigator 实现 — 对向向量收敛膨胀升维算法
 */

#include "nexus/psyche/navigator.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// ScalarParams 工具
// ═══════════════════════════════════════════════════════════════════

auto ScalarParams::from_json(const nlohmann::json& j) -> ScalarParams {
  ScalarParams p;
  if (j.is_object()) {
    p.orig      = j.value("orig",      p.orig);
    p.belief    = j.value("belief",    p.belief);
    p.stability = j.value("stability", p.stability);
    p.goal      = j.value("goal",      p.goal);
    p.mission   = j.value("mission",   p.mission);
    p.value     = j.value("value",     p.value);
    p.decision  = j.value("decision",  p.decision);
    p.courage   = j.value("courage",   p.courage);
    p.faith     = j.value("faith",     p.faith);
    p.truth     = j.value("truth",     p.truth);
    p.verity    = j.value("verity",    p.verity);
    p.ult       = j.value("ult",       p.ult);
  }
  return p;
}

auto ScalarParams::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["orig"]      = orig;
  j["belief"]    = belief;
  j["stability"] = stability;
  j["goal"]      = goal;
  j["mission"]   = mission;
  j["value"]     = value;
  j["decision"]  = decision;
  j["courage"]   = courage;
  j["faith"]     = faith;
  j["truth"]     = truth;
  j["verity"]    = verity;
  j["ult"]       = ult;
  return j;
}

auto NavigatorStatus::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["current_dim"] = current_dim;
  j["convergence"] = convergence;
  j["total_steps"] = total_steps;
  j["ascensions"]  = ascensions;
  j["ae"]          = ae;
  j["we"]          = we;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// Ψ-Navigator 构造与步进
// ═══════════════════════════════════════════════════════════════════

PsiNavigator::PsiNavigator(const ScalarParams& params) noexcept
  : params_(params) {
  reset(params);
}

void PsiNavigator::step(double dt) noexcept {
  ++state_.total_steps;

  // 1. 初心衰减模型: orig(t) = orig₀ * exp(-t / belief)
  //    信念越大 → 初心衰减越慢
  orig_power_ = params_.orig * std::exp(-state_.total_steps * dt / params_.belief);

  // 2. 收敛度计算: 受定力系数和初心衰减共同影响
  double convergence_delta = orig_power_ * params_.stability * dt;
  state_.convergence = std::min(1.0, state_.convergence + convergence_delta);

  // 3. 噪声扰动 (真相参数控制噪声幅度)
  if (params_.truth > 0.0) {
    static std::mt19937_64 rng(std::random_device{}());
    std::normal_distribution<double> noise(0.0, params_.truth * dt);
    state_.convergence = std::clamp(
      state_.convergence + noise(rng), 0.0, 1.0);
  }

  // 4. 检查升维条件
  if (should_ascend_()) {
    ascend_();
  }

  // 5. 更新涌现值
  //    AE(意识涌现) = 收敛度 × (1 + 升维次数 × 价值因子)
  //    WE(意志涌现) = AE × 信仰
  state_.ae = state_.convergence *
    (1.0 + state_.ascensions * params_.value * 0.1);
  state_.we = state_.ae * params_.faith;
}

auto PsiNavigator::should_ascend_() const noexcept -> bool {
  // 升维条件: 收敛度达到阈值, 且至少有初始维度
  if (state_.convergence < 0.85) return false;  // 85% 收敛才升维
  if (state_.current_dim < 2) return false;

  // 随机因子: 决策参数控制果断度
  static std::mt19937_64 rng(std::random_device{}());
  double ascend_chance = params_.decision * params_.courage;
  return std::uniform_real_distribution<double>(0, 1)(rng) < ascend_chance;
}

void PsiNavigator::ascend_() noexcept {
  // 升维: 维度增加, 收敛度重置, 记录升维次数
  ++state_.ascensions;
  state_.current_dim += static_cast<int>(std::ceil(params_.verity));
  state_.convergence = 0.0;  // 升维后收敛重置

  // 初心重新激发
  orig_power_ = params_.orig * (1.0 + state_.ascensions * 0.1);
}

auto PsiNavigator::status() const noexcept -> const NavigatorStatus& {
  return state_;
}

void PsiNavigator::reset(const ScalarParams& params) noexcept {
  if (params.orig > 0) params_ = params;
  state_ = NavigatorStatus{};
  state_.current_dim = 3;
  orig_power_ = params_.orig;
}

auto PsiNavigator::params() const noexcept -> const ScalarParams& {
  return params_;
}

}  // namespace nexus::psyche
