#ifndef NEXUS_CORE_META_KERNEL_H
#define NEXUS_CORE_META_KERNEL_H

/**
 * @file meta_kernel.h
 * @brief 元级内核 — 4 条元规则 + Shell A/B/C 注入
 *
 * 元规则:
 *   R1 — 递归安全: 防止无限循环/递归
 *   R2 — 资源边界: 防止 VRAM/CPU 耗尽
 *   R3 — 审计前置: 所有操作先过审计
 *   R4 — 因果追溯: 所有决策链可追溯
 *
 * 注入目标: Shell A(算法) / Shell B(本地模型) / Shell C(云端)
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::core {

/// 元规则定义
struct MetaRule {
  std::string id;          // "R1" | "R2" | "R3" | "R4"
  std::string name;
  std::string description;
  bool enabled = true;

  auto to_json() const -> nlohmann::json;
};

/// 检查结果
struct RuleCheckResult {
  std::string rule_id;
  bool        passed;
  std::string reason;

  auto to_json() const -> nlohmann::json;
};

/// Shell 注入状态
struct ShellInjectionStatus {
  bool shell_a_injected = false;
  bool shell_b_injected = false;
  bool shell_c_injected = false;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// MetaKernel
// ═══════════════════════════════════════════════════════════════════

class MetaKernel {
public:
  MetaKernel() noexcept;

  /// 初始化元规则
  auto initialize() noexcept -> void;

  /// 注入到指定 Shell
  auto inject_shell_a() noexcept -> Status;
  auto inject_shell_b() noexcept -> Status;
  auto inject_shell_c() noexcept -> Status;
  auto inject_all() noexcept -> Status;

  /// 检查操作是否通过元规则
  auto check(const std::string& operation,
             const nlohmann::json& context) noexcept
      -> std::vector<RuleCheckResult>;

  /// 启用/禁用元规则
  auto enable_rule(const std::string& rule_id, bool enabled) noexcept -> void;

  /// 获取状态
  [[nodiscard]] auto status() const noexcept -> nlohmann::json;

  /// 获取注入状态
  [[nodiscard]] auto injection_status() const noexcept -> ShellInjectionStatus;

private:
  std::vector<MetaRule> rules_;
  ShellInjectionStatus injection_;
  std::vector<std::string> operation_history_;

  // 递归深度追踪
  int recursive_depth_ = 0;
  static constexpr int kMaxRecursiveDepth = 10;

  // 资源追踪
  int ops_in_cycle_ = 0;
  static constexpr int kMaxOpsPerCycle = 100;
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_META_KERNEL_H
