#ifndef NEXUS_ENV_CHINESE_CONSTRAINT_H
#define NEXUS_ENV_CHINESE_CONSTRAINT_H

/**
 * @file chinese_constraint.h
 * @brief 中文硬约束 — 强制推理过程使用中文
 *
 * 加载中文约束规则, 注入到 Shell B (本地模型)。
 * 约束类型: zero-tolerance (零容忍英文推理)
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::env {

/// 中文约束规则
struct ChineseConstraint {
  std::string id;
  std::string description;
  bool enforced = true;  // 是否强制

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// ChineseConstraintLoader
// ═══════════════════════════════════════════════════════════════════

class ChineseConstraintLoader {
public:
  ChineseConstraintLoader() noexcept;

  /// 加载约束规则
  auto load() noexcept -> std::vector<ChineseConstraint>;

  /// 注入到 Shell B
  auto inject() noexcept -> Status;

  /// 获取加载的约束
  [[nodiscard]] auto constraints() const noexcept
      -> const std::vector<ChineseConstraint>&;

private:
  std::vector<ChineseConstraint> constraints_;
  auto init_defaults_() noexcept -> void;
};

}  // namespace nexus::env

#endif  // NEXUS_ENV_CHINESE_CONSTRAINT_H
