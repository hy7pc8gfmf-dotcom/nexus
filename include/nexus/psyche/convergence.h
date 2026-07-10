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

#ifndef NEXUS_PSYCHE_CONVERGENCE_H
#define NEXUS_PSYCHE_CONVERGENCE_H

/**
 * @file convergence.h
 * @brief Ψ-ConvergenceNavigator — 工程级收敛导航器
 *
 * 曾用名: DarkSphereRefactored (暗球重构版)
 * 与 Ψ-Navigator 的关系:
 *   PsiNavigator = 纯净框架 (12 标量 + 升维)
 *   ConvergenceNavigator = 工程引擎 (运行时调参 + 多目标优化)
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::psyche {

/// 收敛状态
struct ConvergenceState {
  double orig_power    = 0.0;  // 初心动力
  double belief_reserve= 0.0;  // 信念储备
  double field_strength= 0.0;  // 场强 [0, 1]
  double convergence   = 0.0;  // 收敛度 [0, 1]
  double divergence    = 0.0;  // 发散度 [0, 1]
  int    dimension     = 3;    // 当前维度
  int    total_steps   = 0;    // 总步数
  int    ascensions    = 0;    // 升维次数

  auto to_json() const -> nlohmann::json;
};

/// 标量参数 (与 PsiNavigator 不同, ConvergenceNavigator 支持动态调参)
struct ConvergenceParams {
  double orig      = 2.0;
  double belief    = 80.0;
  double stability = 0.3;
  double goal      = 4.0;
  double mission   = 0.05;
  double value     = 0.8;
  double decision  = 0.6;
  double courage   = 0.3;
  double faith     = 0.3;
  double truth     = 0.05;
  double verity    = 2.0;
  double ult       = 0.01;

  auto to_json() const -> nlohmann::json;
  static auto from_json(const nlohmann::json& j) -> ConvergenceParams;
};

// ═══════════════════════════════════════════════════════════════════
// Ψ-ConvergenceNavigator
// ═══════════════════════════════════════════════════════════════════

class ConvergenceNavigator {
public:
  explicit ConvergenceNavigator(const ConvergenceParams& params = {}) noexcept;

  /// 单步步进
  auto step(double dt = 0.01) noexcept -> void;

  /// 动态调参 (运行时调整标量)
  auto adjust_params(const ConvergenceParams& params) noexcept -> void;

  /// 获取当前状态
  [[nodiscard]] auto status() const noexcept -> const ConvergenceState&;

  /// 获取当前参数
  [[nodiscard]] auto params() const noexcept -> const ConvergenceParams&;

  /// 重置
  auto reset(const ConvergenceParams& params = {}) noexcept -> void;

private:
  ConvergenceParams params_;
  ConvergenceState  state_;

  // 内部振荡器 (用于发散/收敛交替)
  double oscillator_phase_ = 0.0;

  /// 对向向量收敛计算
  auto compute_convergence_(double dt) noexcept -> void;

  /// 检查并执行升维
  auto check_ascend_(double dt) noexcept -> void;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_CONVERGENCE_H
