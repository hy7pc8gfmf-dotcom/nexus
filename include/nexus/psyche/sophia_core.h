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

#ifndef NEXUS_PSYCHE_SOPHIA_CORE_H
#define NEXUS_PSYCHE_SOPHIA_CORE_H

/**
 * @file sophia_core.h
 * @brief 心灵核心 — Sophia Core + WisdomDB
 *
 * Sophia Core 是系统的"直觉"层, 维护 WisdomDB (经验 + 洞见)。
 * 每条 wisdom 包含: 主题/内容/置信度/来源。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::psyche {

/// 智慧条目
struct WisdomEntry {
  std::string id;
  std::string topic;
  std::string content;
  double confidence = 0.5;
  std::string source;
  double created_at = 0.0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// SophiaCore
// ═══════════════════════════════════════════════════════════════════

class SophiaCore {
public:
  explicit SophiaCore(std::string data_dir = ".nexus") noexcept;

  /// 添加一条 wisdom
  auto add_wisdom(const std::string& topic, const std::string& content,
                  double confidence = 0.5,
                  const std::string& source = "") -> Status;

  /// 按主题查询 wisdom
  [[nodiscard]] auto query(const std::string& topic,
                           int limit = 5) const noexcept
      -> std::vector<WisdomEntry>;

  /// 获取所有条目
  [[nodiscard]] auto all() const noexcept -> std::vector<WisdomEntry>;

  /// 获取统计信息
  [[nodiscard]] auto count() const noexcept -> int;

  /// 持久化到磁盘
  auto persist() noexcept -> Status;

  /// 从磁盘加载
  auto load() noexcept -> Status;

private:
  std::string data_dir_;
  std::vector<WisdomEntry> entries_;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SOPHIA_CORE_H
