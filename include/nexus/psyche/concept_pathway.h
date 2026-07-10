// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_CONCEPT_PATHWAY_H
#define NEXUS_PSYCHE_CONCEPT_PATHWAY_H

/**
 * @file concept_pathway.h
 * @brief 确定性概念路径引擎 — 语义场路径搜索
 *
 * 移植自 D:/synapse/semantic_pathway.py
 *
 * 吸引力公式:
 *   force = α·pipe_thickness + β·coord_sim + γ·cluster_boost
 *
 * 搜索: beam search 保留 top-k 路径
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/psyche/semantic_port.h"
#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 路径段
// ═══════════════════════════════════════════════════════════════════

struct PathSegment {
  std::string concept_name;
  double      force   = 0.0;   // 吸引力
  double      cos_sim = 0.0;   // 余弦相似度
  int         depth   = 0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 概念路径
// ═══════════════════════════════════════════════════════════════════

struct ConceptPath {
  std::vector<PathSegment> segments;
  double total_force = 0.0;
  std::string from;
  std::string to;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 概念路径引擎
// ═══════════════════════════════════════════════════════════════════

class ConceptPathway {
public:
  explicit ConceptPathway(SemanticPort* port = nullptr) noexcept;

  /// 设置语义场端口
  void set_port(SemanticPort* port) noexcept { port_ = port; }

  /// 计算概念间吸引力
  auto attraction(const std::string& a, const std::string& b) const noexcept
      -> Result<double>;

  /// Beam search 搜索概念路径
  auto beam_search(const std::string& from, const std::string& to,
                   int beam_width = 5, int max_depth = 10) const noexcept
      -> std::vector<ConceptPath>;

  /// 最近邻 (按吸引力排序)
  auto nearest_neighbors(const std::string& cname,
                          int n = 10) const noexcept -> std::vector<PathSegment>;

  /// 获取统计
  auto stats() const noexcept -> nlohmann::json;

private:
  SemanticPort* port_ = nullptr;

  // 吸引力权重
  static constexpr double kAlpha = 0.5;   // pipe_thickness
  static constexpr double kBeta  = 0.3;   // coord_sim
  static constexpr double kGamma = 0.2;   // cluster_boost

  static double calc_coord_sim_(const SemanticPort& port,
                                 const std::string& a, const std::string& b);
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_CONCEPT_PATHWAY_H
