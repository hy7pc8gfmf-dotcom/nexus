/**
 * @file convergence.cpp
 * @brief Ψ-ConvergenceNavigator 实现
 */

#include "nexus/psyche/convergence.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 工具函数
// ═══════════════════════════════════════════════════════════════════

auto ConvergenceParams::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["orig"]      = orig;      j["belief"]    = belief;
  j["stability"] = stability; j["goal"]      = goal;
  j["mission"]   = mission;   j["value"]     = value;
  j["decision"]  = decision;  j["courage"]   = courage;
  j["faith"]     = faith;     j["truth"]     = truth;
  j["verity"]    = verity;    j["ult"]       = ult;
  return j;
}

auto ConvergenceParams::from_json(const nlohmann::json& j) -> ConvergenceParams {
  ConvergenceParams p;
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

auto ConvergenceState::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["orig_power"]     = std::round(orig_power * 1000) / 1000;
  j["belief_reserve"] = std::round(belief_reserve * 1000) / 1000;
  j["field_strength"] = std::round(field_strength * 1000) / 1000;
  j["convergence"]    = std::round(convergence * 1000) / 1000;
  j["divergence"]     = std::round(divergence * 1000) / 1000;
  j["dimension"]      = dimension;
  j["total_steps"]    = total_steps;
  j["ascensions"]     = ascensions;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造与步进
// ═══════════════════════════════════════════════════════════════════

ConvergenceNavigator::ConvergenceNavigator(
    const ConvergenceParams& params) noexcept {
  reset(params);
}

void ConvergenceNavigator::step(double dt) noexcept {
  state_.total_steps++;

  // 1. 初心衰减与信念储备
  state_.orig_power = params_.orig *
    std::exp(-state_.total_steps * dt / params_.belief);
  state_.belief_reserve += params_.belief * dt * 0.01;
  if (state_.belief_reserve > 100.0) state_.belief_reserve = 100.0;

  // 2. 振荡器 (发散/收敛交替)
  oscillator_phase_ += dt * params_.mission;
  if (oscillator_phase_ > 2 * std::numbers::pi_v<double>) oscillator_phase_ -= 2 * std::numbers::pi_v<double>;

  // 3. 场强 = 初心动力 × 信念因子
  state_.field_strength = state_.orig_power *
    (1.0 + state_.belief_reserve / 100.0 * params_.faith);

  // 4. 收敛/发散计算
  compute_convergence_(dt);

  // 5. 升维检查
  check_ascend_(dt);
}

void ConvergenceNavigator::compute_convergence_(double dt) noexcept {
  // 对向向量模型:
  //   收敛力 = 场强 × 稳定系数 × (1 - 发散)
  //   发散力 = 振荡器 × 魄力
  double conv_force = state_.field_strength * params_.stability *
    (1.0 - state_.divergence);
  double div_force  = (std::sin(oscillator_phase_) + 1.0) * 0.5 *
    params_.courage;

  // 更新收敛/发散度
  state_.convergence += conv_force * dt;
  state_.divergence  += div_force * dt;

  // 相互抑制
  double total = state_.convergence + state_.divergence;
  if (total > 1.0) {
    double scale = 1.0 / total;
    state_.convergence *= scale;
    state_.divergence  *= scale;
  }

  // 噪声
  if (params_.truth > 0.0) {
    static std::mt19937_64 rng(std::random_device{}());
    state_.convergence += std::normal_distribution<double>(0, params_.truth * dt)(rng);
    state_.divergence  += std::normal_distribution<double>(0, params_.truth * dt * 0.5)(rng);
    state_.convergence = std::clamp(state_.convergence, 0.0, 1.0);
    state_.divergence  = std::clamp(state_.divergence, 0.0, 1.0);
  }
}

void ConvergenceNavigator::check_ascend_(double dt) noexcept {
  // 升维条件: 收敛 > 0.85 且 发散 < 0.3
  if (state_.convergence < 0.85) return;
  if (state_.divergence > 0.3) return;

  // 决策因子
  static std::mt19937_64 rng(std::random_device{}());
  double ascend_chance = params_.decision * params_.courage *
    (1.0 + state_.ascensions * 0.05);

  if (std::uniform_real_distribution<double>(0, 1)(rng) < ascend_chance) {
    state_.ascensions++;
    state_.dimension += static_cast<int>(std::ceil(params_.verity));

    // 收敛重置, 发散保持 (升维后发散用于探索新维度)
    state_.convergence = state_.divergence * 0.5;
    oscillator_phase_ = std::numbers::pi_v<double>;  // 重置到发散相位

    // 初心重新激发
    state_.orig_power = params_.orig * (1.0 + state_.ascensions * 0.15);
  }
}

// ═══════════════════════════════════════════════════════════════════
// 调参与查询
// ═══════════════════════════════════════════════════════════════════

void ConvergenceNavigator::adjust_params(const ConvergenceParams& params) noexcept {
  if (params.orig > 0)      params_.orig      = params.orig;
  if (params.belief > 0)    params_.belief    = params.belief;
  if (params.stability > 0) params_.stability = params.stability;
  if (params.goal > 0)      params_.goal      = params.goal;
  if (params.mission > 0)   params_.mission   = params.mission;
  if (params.value > 0)     params_.value     = params.value;
  if (params.decision > 0)  params_.decision  = params.decision;
  if (params.courage > 0)   params_.courage   = params.courage;
  if (params.faith > 0)     params_.faith     = params.faith;
  if (params.truth > 0)     params_.truth     = params.truth;
  if (params.verity > 0)    params_.verity    = params.verity;
  if (params.ult > 0)       params_.ult       = params.ult;
}

auto ConvergenceNavigator::status() const noexcept
    -> const ConvergenceState& {
  return state_;
}

auto ConvergenceNavigator::params() const noexcept
    -> const ConvergenceParams& {
  return params_;
}

void ConvergenceNavigator::reset(const ConvergenceParams& params) noexcept {
  if (params.orig > 0) params_ = params;
  state_ = ConvergenceState{};
  state_.dimension = 5;  // ConvergenceNavigator 从更高维度起步
  oscillator_phase_ = 0.0;
}

}  // namespace nexus::psyche
