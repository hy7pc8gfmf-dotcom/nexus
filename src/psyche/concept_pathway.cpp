// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/concept_pathway.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <set>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto PathSegment::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["concept"] = concept_name;
  j["force"]   = std::round(force * 100) / 100;
  j["cos_sim"] = std::round(cos_sim * 100) / 100;
  j["depth"]   = depth;
  return j;
}

auto ConceptPath::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["from"] = from;
  j["to"]   = to;
  j["total_force"] = std::round(total_force * 100) / 100;
  auto arr = nlohmann::json::array();
  for (const auto& s : segments) arr.push_back(s.to_json());
  j["segments"] = arr;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

ConceptPathway::ConceptPathway(SemanticPort* port) noexcept
  : port_(port) {}

// ═══════════════════════════════════════════════════════════════════
// 坐标余弦相似度计算
// ═══════════════════════════════════════════════════════════════════

double ConceptPathway::calc_coord_sim_(const SemanticPort& port,
                                        const std::string& a,
                                        const std::string& b) {
  auto ca = port.concept_of(a);
  auto cb = port.concept_of(b);
  if (!ca.ok() || !cb.ok()) return 0.0;

  const auto& ca_val = ca.value();
  const auto& cb_val = cb.value();

  double dot = 0, na = 0, nb = 0;
  for (int i = 0; i < kSemanticDim; ++i) {
    dot += ca_val.coord[i] * cb_val.coord[i];
    na  += ca_val.coord[i] * ca_val.coord[i];
    nb  += cb_val.coord[i] * cb_val.coord[i];
  }
  double denom = std::sqrt(na) * std::sqrt(nb);
  return denom > 0 ? dot / denom : 0.0;
}

// ═══════════════════════════════════════════════════════════════════
// 吸引力
// ═══════════════════════════════════════════════════════════════════

auto ConceptPathway::attraction(const std::string& a,
                                 const std::string& b) const noexcept
    -> Result<double> {
  if (!port_ || !port_->loaded()) {
    return Status::Error(ErrorCode::kInvalidConfig, "semantic port not loaded");
  }

  double cos_sim = calc_coord_sim_(*port_, a, b);

  // 邻居检测: 如果 b 是 a 的邻居, 增加吸引力
  double neighbor_boost = 0.0;
  auto neighbors = port_->neighbors(a, 50);
  for (const auto& n : neighbors) {
    if (n == b) { neighbor_boost = 0.3; break; }
  }

  double force = kBeta * cos_sim + kGamma * neighbor_boost;
  return std::clamp(force, 0.0, 1.0);
}

// ═══════════════════════════════════════════════════════════════════
// Beam search 路径搜索
// ═══════════════════════════════════════════════════════════════════

auto ConceptPathway::beam_search(const std::string& from,
                                  const std::string& to,
                                  int beam_width,
                                  int max_depth) const noexcept
    -> std::vector<ConceptPath> {
  std::vector<ConceptPath> results;
  if (!port_ || !port_->loaded()) return results;

  // 广度优先 beam search
  struct BeamItem {
    std::vector<std::string> path;
    double total_force = 0.0;
  };

  std::vector<BeamItem> beam;
  beam.push_back({{from}, 0.0});

  for (int depth = 0; depth < max_depth; ++depth) {
    std::vector<BeamItem> candidates;

    for (const auto& item : beam) {
      std::string current = item.path.back();

      // 获取邻居概念
      auto neighbors = port_->neighbors(current, beam_width * 2);
      if (neighbors.empty()) {
        // 找高相似度概念 — 使用函数指针避免 MSVC lambda 转换
        struct Matcher {
          const std::string* current;
          bool operator()(const SemanticConcept& c) const {
            return c.name != *current;
          }
        };
        Matcher matcher{&current};
        auto find_all = port_->find(matcher, beam_width * 3);
        for (const auto& c : find_all) {
          neighbors.push_back(c.name);
        }
      }

      for (const auto& next : neighbors) {
        // 避免环路
        bool visited = false;
        for (const auto& p : item.path) {
          if (p == next) { visited = true; break; }
        }
        if (visited) continue;

        double f = 0.0;
        auto attr = attraction(current, next);
        if (attr.ok()) f = attr.value();

        BeamItem new_item;
        new_item.path = item.path;
        new_item.path.push_back(next);
        new_item.total_force = item.total_force + f;
        candidates.push_back(new_item);
      }
    }

    // 按总吸引力排序, 保留 top-k
    std::sort(candidates.begin(), candidates.end(),
      [](const BeamItem& a, const BeamItem& b) {
        return a.total_force > b.total_force;
      });

    beam.clear();
    for (int i = 0; i < std::min(beam_width, static_cast<int>(candidates.size())); ++i) {
      beam.push_back(candidates[i]);

      // 如果到达目标, 记录路径
      if (candidates[i].path.back() == to) {
        ConceptPath cp;
        cp.from = from;
        cp.to = to;
        cp.total_force = candidates[i].total_force;
        for (size_t j = 0; j < candidates[i].path.size(); ++j) {
          PathSegment seg;
          seg.concept_name = candidates[i].path[j];
          seg.depth = static_cast<int>(j);
          if (j > 0) {
            auto a = attraction(candidates[i].path[j-1], candidates[i].path[j]);
            if (a.ok()) seg.force = a.value();
          }
          cp.segments.push_back(seg);
        }
        results.push_back(cp);
      }
    }

    if (beam.empty()) break;
  }

  return results;
}

// ═══════════════════════════════════════════════════════════════════
// 最近邻
// ═══════════════════════════════════════════════════════════════════

auto ConceptPathway::nearest_neighbors(const std::string& cname,
                                        int n) const noexcept
    -> std::vector<PathSegment> {
  std::vector<PathSegment> result;
  if (!port_ || !port_->loaded()) return result;

  // 先找语义邻居
  auto neighbors = port_->neighbors(cname, n * 2);
  for (const auto& name : neighbors) {
    auto attr = attraction(cname, name);
    if (attr.ok()) {
      PathSegment seg;
      seg.concept_name = name;
      seg.force = attr.value();
      seg.cos_sim = calc_coord_sim_(*port_, cname, name);
      result.push_back(seg);
    }
  }

  // 按吸引力排序
  std::sort(result.begin(), result.end(),
    [](const PathSegment& a, const PathSegment& b) {
      return a.force > b.force;
    });

  if (static_cast<int>(result.size()) > n)
    result.resize(n);
  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 统计
// ═══════════════════════════════════════════════════════════════════

auto ConceptPathway::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["port_loaded"] = port_ ? port_->loaded() : false;
  if (port_) {
    j["concepts"] = port_->count();
  }
  return j;
}

}  // namespace nexus::psyche
