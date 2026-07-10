// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_CORE_MEMORY_INDEX_H
#define NEXUS_CORE_MEMORY_INDEX_H

/**
 * @file memory_index.h
 * @brief 全局记忆索引 — 持久化文件存储 + 关键词检索
 *
 * 移植自 D:/synapse/neural_memory_index.py (774行) — Shell A 核心
 *
 * Shell A (纯算法): JSON文件存储 + 倒排索引 + 时间线
 * 不依赖任何模型，始终可用。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::core {

/// 记忆条目
struct MemoryEntry {
  std::string id;
  std::string type;        // architectural_decision, insight, milestone, ...
  std::string topic;
  std::string summary;
  std::vector<std::string> keywords;
  std::vector<std::string> related_components;
  int importance = 5;      // 1-10
  std::string created;
  std::string data_dir;    // 存储路径

  auto to_json() const -> nlohmann::json;
  static auto from_json(const nlohmann::json& j) -> MemoryEntry;
};

/// 全局记忆索引 — Shell A (纯算法)
class MemoryIndex {
public:
  explicit MemoryIndex(const std::string& data_dir = ".nexus") noexcept;

  /// 存储一条记忆
  auto store(const MemoryEntry& entry) noexcept -> std::string;

  /// 关键词检索
  auto search(const std::string& query, int limit = 10) noexcept -> std::vector<MemoryEntry>;

  /// 获取单条
  auto retrieve(const std::string& mem_id) noexcept -> MemoryEntry;

  /// 最近 N 条
  auto recent(int limit = 10) noexcept -> std::vector<MemoryEntry>;

  /// 统计
  auto stats() const noexcept -> nlohmann::json;

private:
  std::string data_dir_;
  int next_id_ = 1;

  std::string entries_dir_() const noexcept;
  std::string index_path_() const noexcept;
  std::string kw_path_() const noexcept;
  std::string timeline_path_() const noexcept;

  auto load_index_() -> nlohmann::json;
  auto load_keywords_() -> nlohmann::json;
  void save_index_(const nlohmann::json& index);
  void save_keywords_(const nlohmann::json& keywords);
  void append_timeline_(const std::string& mem_id, const MemoryEntry& entry);
};

}  // namespace nexus::core

#endif
