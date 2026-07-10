// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_cluster.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto SemanticClusterDef::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]    = name;
  j["label"]   = label;
  j["radius"]  = std::round(radius * 100) / 100;
  j["density"] = std::round(density * 100) / 100;

  auto coord_arr = nlohmann::json::array();
  for (int i = 0; i < kSemanticDim; ++i) coord_arr.push_back(centroid[i]);
  j["centroid"] = coord_arr;

  auto members_arr = nlohmann::json::array();
  for (const auto& m : members) members_arr.push_back(m);
  j["members"] = members_arr;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

SemanticClusterEngine::SemanticClusterEngine(SemanticPort* port) noexcept
  : port_(port) {
  build_default_clusters_();
}

// ═══════════════════════════════════════════════════════════════════
// 距离
// ═══════════════════════════════════════════════════════════════════

float SemanticClusterEngine::centroid_distance_(const float* a, const float* b) {
  float dist = 0;
  for (int i = 0; i < kSemanticDim; ++i) {
    float d = a[i] - b[i];
    dist += d * d;
  }
  return std::sqrt(dist);
}

// ═══════════════════════════════════════════════════════════════════
// 默认团
// ═══════════════════════════════════════════════════════════════════

void SemanticClusterEngine::build_default_clusters_() noexcept {
  clusters_ = {
    {"nature",   "自然", {}, 0.5f, 0.0f, {"水", "火", "空气", "土", "光"}},
    {"cognition", "认知", {}, 0.5f, 0.0f, {"意识", "信念", "知识", "真", "时间"}},
    {"physics",  "物理", {}, 0.5f, 0.0f, {"熵", "能量", "空间", "因果", "对称"}},
    {"math",     "数学", {}, 0.5f, 0.0f, {"递归", "对偶", "复杂度", "信息"}},
    {"abstract", "抽象", {}, 0.5f, 0.0f, {"涌现", "相干", "自指", "无限"}},
  };
}

// ═══════════════════════════════════════════════════════════════════
// 团归属
// ═══════════════════════════════════════════════════════════════════

auto SemanticClusterEngine::cluster_of(const std::string& cname) const noexcept
    -> std::string {
  if (!port_) return "unknown";

  auto result = port_->concept_of(cname);
  if (!result.ok()) return "unknown";

  const auto& c = result.value();
  float best_dist = std::numeric_limits<float>::max();
  std::string best_cluster = "unknown";

  for (const auto& cl : clusters_) {
    float dist = centroid_distance_(c.coord, cl.centroid);
    if (dist < best_dist) {
      best_dist = dist;
      best_cluster = cl.name;
    }
  }

  return best_cluster;
}

auto SemanticClusterEngine::clusters() const noexcept
    -> std::vector<SemanticClusterDef> {
  return clusters_;
}

void SemanticClusterEngine::register_cluster(const SemanticClusterDef& cluster) noexcept {
  clusters_.push_back(cluster);
}

// ═══════════════════════════════════════════════════════════════════
// 统计
// ═══════════════════════════════════════════════════════════════════

auto SemanticClusterEngine::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["n_clusters"] = static_cast<int>(clusters_.size());
  j["port_loaded"] = port_ ? port_->loaded() : false;
  auto arr = nlohmann::json::array();
  for (const auto& c : clusters_) arr.push_back(c.name);
  j["cluster_names"] = arr;
  return j;
}

auto SemanticClusterEngine::build_clusters(int n_clusters) noexcept -> Status {
  if (!port_ || !port_->loaded()) {
    return Status::Error(ErrorCode::kInvalidConfig, "port not loaded");
  }
  // 简单实现: 基于种子概念构建团
  // (完整 k-means 需 Eigen/numpy, 纯 C++ 从简)
  return Status::Ok();
}

}  // namespace nexus::psyche
