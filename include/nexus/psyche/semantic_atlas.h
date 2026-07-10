// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_ATLAS_H
#define NEXUS_PSYCHE_SEMANTIC_ATLAS_H

/**
 * @file semantic_atlas.h
 * @brief 语义图集 — 概念坐标 + 语义层级 + 知识分类
 *
 * 移植自 D:/synapse/semantic_atlas.py (381行)
 *
 * 管理概念在 14D 语义空间中的坐标映射,
 * 以及语义层级的分类标注 (axiom/classical/nonconstructive 等)
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>

#include "nexus/psyche/semantic_port.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 语义层级
// ═══════════════════════════════════════════════════════════════════

struct SemanticTier {
  std::string name;
  double      weight     = 1.0;
  std::vector<std::string> tags;
  double      classical  = 0.0;   // 经典逻辑含量 [0,1]
  double      nonconstructive = 0.0; // 非构造性含量 [0,1]

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 语义图集
// ═══════════════════════════════════════════════════════════════════

class SemanticAtlas {
public:
  SemanticAtlas() noexcept;

  /// 从语义端口构建图集
  auto build(SemanticPort* port) noexcept -> Status;

  /// 查询概念坐标
  auto concept_coord(const std::string& name) const noexcept
      -> Result<std::vector<float>>;

  /// 获取概念所在的层级
  auto concept_tier(const std::string& name) const noexcept -> std::string;

  /// 注册自定义层级
  void register_tier(const SemanticTier& tier) noexcept;

  /// 保存/加载缓存
  auto save(const std::string& path) const noexcept -> Status;
  auto load(const std::string& path) noexcept -> Status;

  /// 统计
  auto stats() const noexcept -> nlohmann::json;

private:
  SemanticPort* port_ = nullptr;
  std::vector<SemanticTier> tiers_;
  bool built_ = false;
  mutable std::unordered_map<std::string, std::vector<float>> coord_cache_;

  void build_default_tiers_() noexcept;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_ATLAS_H
