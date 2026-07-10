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

#ifndef NEXUS_BRIDGE_CAPABILITY_H
#define NEXUS_BRIDGE_CAPABILITY_H

/**
 * @file capability.h
 * @brief 能力自认知 — 系统能力清单与自我评估
 *
 * 维护系统当前能力的完整清单, 并评估每项能力的成熟度。
 * 输出: agi_capability_brief.json
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::bridge {

/// 能力条目
struct CapabilityEntry {
  std::string id;
  std::string name;
  std::string category;   // "inference" | "algorithm" | "consciousness" | "bridge"
  std::string component;  // 所属 EXE
  int maturity = 5;       // [1-10]

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// CapabilitySelfAwareness
// ═══════════════════════════════════════════════════════════════════

class CapabilitySelfAwareness {
public:
  explicit CapabilitySelfAwareness(std::string data_dir = ".nexus") noexcept;

  /// 注册能力
  auto register_capability(const std::string& id, const std::string& name,
                           const std::string& category,
                           const std::string& component,
                           int maturity = 5) -> Status;

  /// 生成能力简报
  auto generate_brief() noexcept -> nlohmann::json;

  /// 按类别查询
  [[nodiscard]] auto query_by_category(const std::string& category) const noexcept
      -> std::vector<CapabilityEntry>;

  /// 获取统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;

private:
  std::string data_dir_;
  std::vector<CapabilityEntry> capabilities_;

  auto register_defaults_() noexcept -> void;
  auto persist_() noexcept -> void;
};

}  // namespace nexus::bridge

#endif  // NEXUS_BRIDGE_CAPABILITY_H
