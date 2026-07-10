// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_SENSES_H
#define NEXUS_PSYCHE_SEMANTIC_SENSES_H

/**
 * @file semantic_senses.h
 * @brief 同形异义拆分 — 多义字按义项拆分独立实体
 *
 * 移植自 D:/synapse/semantic_senses.py (718行)
 *
 * 每个义项有独立的14D坐标、团归属和语义管道。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>

#include "nexus/psyche/semantic_port.h"
#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 义项条目
// ═══════════════════════════════════════════════════════════════════

struct SenseEntry {
  std::string character;     // 原字 (如 "行")
  std::string sense_id;      // 义项ID (如 "xing_walk")
  std::string gloss;         // 释义 (如 "行走/行动")
  std::vector<std::string> clusters;  // 归属团列表
  float offset[kSemanticDim]; // 坐标偏移

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 义项注册器
// ═══════════════════════════════════════════════════════════════════

class SenseRegistry {
public:
  SenseRegistry() noexcept;

  /// 注册内置义项
  void register_builtin() noexcept;

  /// 注册自定义义项
  void register_sense(const SenseEntry& entry) noexcept;

  /// 查询某字的所有义项
  auto get_senses(const std::string& character) const noexcept
      -> std::vector<SenseEntry>;

  /// 查询某义项的完整实体名
  auto get_entity(const std::string& sense_id) const noexcept
      -> std::string;

  /// 统计
  auto stats() const noexcept -> nlohmann::json;

private:
  std::unordered_map<std::string, std::vector<SenseEntry>> senses_;
  int total_ = 0;

  void add_sense_(const std::string& ch, const std::string& id,
                  const std::string& gloss,
                  const std::vector<std::string>& clusters,
                  const std::vector<float>& offset);
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_SENSES_H
