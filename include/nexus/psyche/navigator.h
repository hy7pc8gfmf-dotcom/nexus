#ifndef NEXUS_PSYCHE_NAVIGATOR_H
#define NEXUS_PSYCHE_NAVIGATOR_H

/**
 * @file navigator.h
 * @brief Ψ-Navigator — 对向向量收敛膨胀升维算法
 *
 * 12 标量参数控制方向与收敛:
 *   初心(orig) → 信念(belief) → 定力(stability) → 目标(goal)
 *   → 使命(mission) → 价值(value) → 决策(decision) → 魄力(courage)
 *   → 信仰(faith) → 真相(truth) → 真理(verity) → 究极真理(ult)
 *
 * 核心循环: step() → 收敛度上升 → 升维触发 → 收敛重置 → 继续
 */

#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace nexus::psyche {

/// 12 标量参数
struct ScalarParams {
  double orig     = 1.0;     // 初心: 初始原动力
  double belief   = 100.0;   // 信念: 初心持久度 (时间常数)
  double stability= 0.3;     // 定力: 收敛力系数 (管道约束)
  double goal     = 3.0;     // 目标: 阶段性目标距离
  double mission  = 0.1;     // 使命: 方向更新率
  double value    = 1.0;     // 价值判断: 升维阈值因子
  double decision = 0.5;     // 决策: 升维果断度 (步长)
  double courage  = 0.2;     // 魄力: 展开力幅度
  double faith    = 0.3;     // 信仰: 究极真理趋近系数
  double truth    = 0.05;    // 真相: 噪声幅度
  double verity   = 2.0;     // 真理: 维度间距
  double ult      = 0.01;    // 究极真理: 终极演进速度

  // 从 JSON 加载
  static auto from_json(const nlohmann::json& j) -> ScalarParams;

  // 序列化为 JSON
  auto to_json() const -> nlohmann::json;
};

/// ψ-Navigator 状态快照
struct NavigatorStatus {
  int    current_dim    = 3;       // 当前维度
  double convergence    = 0.0;     // 收敛度 [0, 1]
  int    total_steps    = 0;       // 总步进次数
  int    ascensions     = 0;       // 升维次数
  double ae             = 0.0;     // 意识涌现值
  double we             = 0.0;     // 意志涌现值

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// Ψ-Navigator
// ═══════════════════════════════════════════════════════════════════

class PsiNavigator {
public:
  explicit PsiNavigator(const ScalarParams& params = {}) noexcept;

  /// 单步步进 (dt = 时间步长)
  auto step(double dt = 0.01) noexcept -> void;

  /// 获取当前状态
  [[nodiscard]] auto status() const noexcept -> const NavigatorStatus&;

  /// 重置导航器
  auto reset(const ScalarParams& params = {}) noexcept -> void;

  /// 获取标量参数
  [[nodiscard]] auto params() const noexcept -> const ScalarParams&;

private:
  ScalarParams    params_;
  NavigatorStatus state_;

  // 内部状态
  double orig_power_  = 0.0;  // 初心衰减动力
  double belief_decay_= 0.0;  // 信念衰减量

  /// 检查是否达到升维条件
  auto should_ascend_() const noexcept -> bool;

  /// 执行升维
  auto ascend_() noexcept -> void;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_NAVIGATOR_H
