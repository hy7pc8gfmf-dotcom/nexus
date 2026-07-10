// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_PORT_H
#define NEXUS_PSYCHE_SEMANTIC_PORT_H

/**
 * @file semantic_port.h
 * @brief 语义场访问器 — 50K 热点 × 1.1M 边语义网络
 *
 * 二进制数据格式:
 *   HEADER: magic[4]="NSEM" ver[4] n_concept[4] n_edge[4] dim[4] = 20 bytes
 *   CONCEPTS: n × (name_len[2] name[name_len] coord[dim×f32] padding)
 *   EDGES: n × (concept_idx[4] edge_count[4] edge_idx[edge_count×4])
 *
 * Python 导出: export_semantic.py → .semantic_field.bin
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>

#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 常量
// ═══════════════════════════════════════════════════════════════════

constexpr int kSemanticDim = 14;           // 语义空间维度
constexpr uint32_t kSemanticMagic = 0x4D45534E;  // "NSEM" little-endian
constexpr int kSemanticVersion = 1;

// ═══════════════════════════════════════════════════════════════════
// 语义概念
// ═══════════════════════════════════════════════════════════════════

struct SemanticConcept {
  int         index = 0;
  std::string name;
  float       coord[kSemanticDim] = {};     // 14D 语义坐标
  std::vector<int> neighbor_indices;        // 邻居概念索引
  float       intensity = 0.0f;
  std::string lang = "zh";

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 语义场访问器
// ═══════════════════════════════════════════════════════════════════

class SemanticPort {
public:
  SemanticPort() noexcept = default;

  /// 加载二进制语义场数据
  auto load(const std::string& path) noexcept -> Status;

  /// 查询概念
  auto concept_of(const std::string& name) const noexcept
      -> Result<SemanticConcept>;

  /// 查询邻居概念名
  auto neighbors(const std::string& name, int limit = 20) const noexcept
      -> std::vector<std::string>;

  /// 遍历所有满足谓词的概念
  auto find(std::function<bool(const SemanticConcept&)> pred,
            int limit = 100) const noexcept -> std::vector<SemanticConcept>;

  /// 概念间语义距离 (cosine 距离)
  auto distance(const std::string& a, const std::string& b) const noexcept
      -> Result<double>;

  /// 获取统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;

  /// 是否已加载
  [[nodiscard]] auto loaded() const noexcept -> bool { return !concepts_.empty(); }

  /// 概念数量
  [[nodiscard]] auto count() const noexcept -> int {
    return static_cast<int>(concepts_.size());
  }

private:
  std::vector<SemanticConcept> concepts_;
  std::unordered_map<std::string, int> name_to_idx_;
  std::vector<std::pair<int, int>> edges_;   // (from_idx, to_idx)

  /// 归一化的坐标指针 (用于计算)
  [[nodiscard]] auto coord_ptr_(int idx) const noexcept -> const float*;
  static float cosine_sim_(const float* a, const float* b);
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_PORT_H
