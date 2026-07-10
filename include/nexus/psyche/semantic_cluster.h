// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_CLUSTER_H
#define NEXUS_PSYCHE_SEMANTIC_CLUSTER_H

/**
 * @file semantic_cluster.h
 * @brief 语义团锚点系统 — 概念聚类 + 质心计算
 *
 * 移植自 D:/synapse/semantic_clusters.py (1113行)
 *
 * 在 14D 语义空间上建立团级抽象:
 *   团 = 密集语义区域, 质心坐标 + 辐射半径
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/psyche/semantic_port.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 语义团
// ═══════════════════════════════════════════════════════════════════

struct SemanticClusterDef {
  std::string  name;
  std::string  label;
  float        centroid[kSemanticDim] = {};  // 质心 14D
  float        radius     = 0.0;     // 辐射半径
  float        density    = 0.0;     // 密度
  std::vector<std::string> members;  // 成员概念

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 聚类器
// ═══════════════════════════════════════════════════════════════════

class SemanticClusterEngine {
public:
  explicit SemanticClusterEngine(SemanticPort* port = nullptr) noexcept;

  void set_port(SemanticPort* port) noexcept { port_ = port; }

  /// 计算指定概念的归属团
  auto cluster_of(const std::string& cname) const noexcept -> std::string;

  /// 获取所有团
  auto clusters() const noexcept -> std::vector<SemanticClusterDef>;

  /// 注册自定义团
  void register_cluster(const SemanticClusterDef& cluster) noexcept;

  /// 自动构建团 (从概念坐标聚类)
  auto build_clusters(int n_clusters = 10) noexcept -> Status;

  /// 统计
  auto stats() const noexcept -> nlohmann::json;

private:
  SemanticPort* port_ = nullptr;
  std::vector<SemanticClusterDef> clusters_;

  void build_default_clusters_() noexcept;
  static float centroid_distance_(const float* a, const float* b);
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_CLUSTER_H
