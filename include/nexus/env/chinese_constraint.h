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
