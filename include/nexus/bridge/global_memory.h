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

#ifndef NEXUS_BRIDGE_GLOBAL_MEMORY_H
#define NEXUS_BRIDGE_GLOBAL_MEMORY_H

/**
 * @file global_memory.h
 * @brief 全局记忆 — 结构化知识仓库 (跨会话持久化)
 *
 * 存储结构:
 *   INDEX.json     → {topic: [mem_ids], type: [mem_ids]}
 *   keywords.json  → {keyword: [mem_ids]} 倒排索引
 *   timeline.jsonl → 按时间序追加
 *   entries/       → mem_0001.json ...
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::bridge {

/// 记忆条目
struct MemEntry {
  std::string id;
  std::string type;        // architectural_decision | insight | workaround | etc
  std::string topic;
  std::string content;
  std::vector<std::string> keywords;
  std::string component;   // core | algo | psyche | etc
  double created_at = 0.0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// GlobalMemory
// ═══════════════════════════════════════════════════════════════════

class GlobalMemory {
public:
  explicit GlobalMemory(std::string data_dir = ".nexus") noexcept;

  /// 存储一条记忆
  auto store(const std::string& type, const std::string& topic,
             const std::string& content,
             const std::vector<std::string>& keywords = {},
             const std::string& component = "") -> Status;

  /// 按关键词搜索
  [[nodiscard]] auto search(const std::string& query,
                            int limit = 10) const noexcept
      -> std::vector<MemEntry>;

  /// 按类型搜索
  [[nodiscard]] auto search_by_type(const std::string& type,
                                     int limit = 10) const noexcept
      -> std::vector<MemEntry>;

  /// 获取统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;
  [[nodiscard]] auto total_entries() const noexcept -> int;

private:
  std::string data_dir_;
  std::string index_path_;
  std::string kw_path_;
  std::string timeline_path_;
  std::string entries_dir_;

  mutable nlohmann::json index_;   // {topic:[id], type:[id]}
  mutable nlohmann::json kw_index_; // {keyword:[id]}
  mutable bool dirty_ = false;

  auto ensure_dirs_() noexcept -> void;
  auto load_indexes_() noexcept -> void;
  auto save_indexes_() noexcept -> void;
  auto generate_id_() const -> std::string;
};

}  // namespace nexus::bridge

#endif  // NEXUS_BRIDGE_GLOBAL_MEMORY_H
