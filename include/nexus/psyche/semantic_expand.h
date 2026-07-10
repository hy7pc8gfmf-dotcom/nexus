// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_EXPAND_H
#define NEXUS_PSYCHE_SEMANTIC_EXPAND_H

/**
 * @file semantic_expand.h
 * @brief 概念扩展 — 名称→14D 坐标生成
 *
 * 移植自 D:/synapse/semantic_expand.py (945行)
 *
 * 为新概念生成确定性 14D 语义坐标,
 * 基于名称哈希 + 种子, 保证同一名称始终得到相同坐标。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/psyche/semantic_port.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 概念扩展器
// ═══════════════════════════════════════════════════════════════════

class SemanticExpander {
public:
  explicit SemanticExpander(SemanticPort* port = nullptr) noexcept;

  void set_port(SemanticPort* port) noexcept { port_ = port; }

  /// 名称 → 14D 坐标 (确定性, 基于名称哈希 + 种子)
  static auto name_to_coord(const std::string& name, int seed = 42)
      -> std::vector<float>;

  /// 检查概念是否已存在于语义场
  auto exists(const std::string& name) const noexcept -> bool;

  /// 生成新概念并注册到语义场
  auto expand(const std::string& name, int seed = 42) noexcept -> Status;

  /// 批量生成
  auto expand_many(const std::vector<std::string>& names,
                    int seed = 42) noexcept -> int;

  /// 统计
  auto stats() const noexcept -> nlohmann::json;

private:
  SemanticPort* port_ = nullptr;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_EXPAND_H
