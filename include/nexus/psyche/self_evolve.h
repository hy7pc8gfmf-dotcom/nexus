#ifndef NEXUS_PSYCHE_SELF_EVOLVE_H
#define NEXUS_PSYCHE_SELF_EVOLVE_H

/**
 * @file self_evolve.h
 * @brief 自演进引擎 — 系统自我评估 + 缺口检测 + 自动修复
 *
 * 三阶段循环:
 *   Sense  → 感知系统状态
 *   Judge  → 检测能力缺口
 *   Act    → 自动实施修复
 *
 * Shell 三态感知: A(算法) / B(本地模型) / C(云端)
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::psyche {

/// Shell 状态
struct ShellStatus {
  bool algo_available  = true;   // Shell A
  bool local_available = true;   // Shell B
  bool cloud_available = true;   // Shell C
  int  algo_count      = 10;     // 算法引擎数
  int  local_models    = 0;      // 本地模型数
  int  cloud_services  = 0;      // 云端服务数

  auto to_json() const -> nlohmann::json;
};

/// 能力缺口
struct CapabilityGap {
  std::string id;
  std::string domain;
  std::string description;
  int severity = 5;         // [1-10]
  bool auto_fixable = true;

  auto to_json() const -> nlohmann::json;
};

/// 演进记录
struct EvolutionRecord {
  int cycle;
  std::string action;
  std::string detail;
  bool success;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 自演进引擎
// ═══════════════════════════════════════════════════════════════════

class SelfEvolutionEngine {
public:
  SelfEvolutionEngine() noexcept;

  /// 执行一个完整的演进周期
  auto run_cycle() noexcept -> std::vector<EvolutionRecord>;

  /// 感知阶段
  auto sense() noexcept -> ShellStatus;

  /// 判断阶段
  auto judge(const ShellStatus& status) noexcept -> std::vector<CapabilityGap>;

  /// 行动阶段
  auto act(const std::vector<CapabilityGap>& gaps) noexcept
      -> std::vector<EvolutionRecord>;

  /// 获取演化历史
  [[nodiscard]] auto history() const noexcept -> const std::vector<EvolutionRecord>&;

  /// 获取 Shell 状态
  [[nodiscard]] auto shell_status() const noexcept -> const ShellStatus&;

  /// 获取统计
  [[nodiscard]] auto total_cycles() const noexcept -> int;
  [[nodiscard]] auto successful_acts() const noexcept -> int;

private:
  int cycle_ = 0;
  ShellStatus last_status_;
  std::vector<EvolutionRecord> history_;

  /// 初演: 首次自检
  auto first_performance_() noexcept -> void;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SELF_EVOLVE_H
