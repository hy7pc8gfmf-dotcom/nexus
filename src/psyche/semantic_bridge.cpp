// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_bridge.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto SteerProfile::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]       = name;
  j["default"]    = default_val;
  j["min"]        = min_val;
  j["max"]        = max_val;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

SemanticBridge::SemanticBridge() noexcept {
  init_profiles_();
}

void SemanticBridge::init_profiles_() noexcept {
  // 8 维转向标量 (Python 版 STEER_DIM = 8)
  profiles_ = {
    {"formal",    0.3, 0.0, 1.0},  // 形式化/定理复杂度
    {"creative",  0.3, 0.0, 1.0},  // 创造性
    {"technical", 0.3, 0.0, 1.0},  // 工程技术
    {"quantum",   0.3, 0.0, 1.0},  // 量子
    {"safety",    0.5, 0.0, 1.0},  // 安全/信念
    {"diversity", 0.3, 0.0, 1.0},  // 多样性
    {"precision", 0.5, 0.0, 1.0},  // 精确性
    {"speed",     0.3, 0.0, 1.0},  // 速度
  };

  // 8D → 14D 投影映射 (Python 版 STEER_TO_SEED)
  steer_to_seed_map_ = {0, 2, 3, 6, 1, 4, 5, 7};

  // 加权欧氏距离的对角度规 (Python 版 _euclidean_14d)
  weight_ = {1.0, 0.3, 1.0, 1.0, 1.0, 1.2, 1.0, 1.0, 1.0, 1.0, 2.0, 1.5, 3.0, 2.0};
}

// ═══════════════════════════════════════════════════════════════════
// 加载
// ═══════════════════════════════════════════════════════════════════

auto SemanticBridge::load(bridge::SeedBank* bank) noexcept -> Status {
  if (!bank) {
    return Status::Error(ErrorCode::kInvalidConfig, "seed bank is null");
  }
  bank_ = bank;
  loaded_ = bank->count() > 0;
  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 8D 转向 → 14D 种子 (显式映射, 与 Python 版一致)
// ═══════════════════════════════════════════════════════════════════

auto SemanticBridge::steer_to_seed(const std::vector<double>& steer_vec) const noexcept
    -> std::vector<double> {
  std::vector<double> result(kSeedDim, 0.5);
  if (steer_vec.empty()) return result;

  size_t n = std::min(steer_vec.size(), steer_to_seed_map_.size());
  for (size_t i = 0; i < n; ++i) {
    int dim = steer_to_seed_map_[i];
    if (dim >= 0 && dim < kSeedDim) {
      result[dim] = std::clamp(steer_vec[i], 0.0, 1.0);
    }
  }

  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 14D 种子 → 8D 转向 (逆映射)
// ═══════════════════════════════════════════════════════════════════

auto SemanticBridge::seed_to_steer(const std::vector<double>& seed_vec) const noexcept
    -> std::vector<double> {
  std::vector<double> result(kSteerDim, 0.3);
  if (seed_vec.size() < kSeedDim) return result;

  for (size_t i = 0; i < steer_to_seed_map_.size() && i < result.size(); ++i) {
    int dim = steer_to_seed_map_[i];
    if (dim >= 0 && dim < static_cast<int>(seed_vec.size())) {
      result[i] = std::clamp(seed_vec[dim], 0.0, 1.0);
    }
  }

  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 加权欧氏距离 (Python 版 _euclidean_14d)
// ═══════════════════════════════════════════════════════════════════

double SemanticBridge::weighted_distance_(
    const std::vector<double>& a, const std::vector<double>& b) const noexcept {
  if (a.size() < kSeedDim || b.size() < kSeedDim) return 1.0;

  double sq_sum = 0.0;
  double w_sum = 0.0;
  for (int d = 0; d < kSeedDim; ++d) {
    double diff = a[d] - b[d];
    double w = (d < static_cast<int>(weight_.size())) ? weight_[d] : 1.0;
    sq_sum += w * diff * diff;
    w_sum += w;
  }

  return (w_sum > 0) ? std::sqrt(sq_sum / w_sum) : 1.0;
}

// ═══════════════════════════════════════════════════════════════════
// 跨系统查询 (Python 版 query)
// ═══════════════════════════════════════════════════════════════════

auto SemanticBridge::query(const std::string& section, int top_k) const noexcept
    -> nlohmann::json {
  auto result = nlohmann::json::object();

  // P 向量 (8D) — 简化: 用 section 的哈希生成确定性的向量
  std::vector<double> p8(kSteerDim, 0.3);
  size_t h = std::hash<std::string>{}(section);
  for (int i = 0; i < kSteerDim; ++i) {
    p8[i] = 0.2 + static_cast<double>((h >> (i * 4)) & 0xF) / 20.0;
  }

  // 投影到 14D
  auto p14 = steer_to_seed(p8);

  // 8D 转向查询结果
  result["p_vector_8d"] = nlohmann::json::array();
  for (double v : p8) result["p_vector_8d"].push_back(v);

  result["p_vector_14d"] = nlohmann::json::array();
  for (double v : p14) result["p_vector_14d"].push_back(v);

  // 种子检索
  auto seeds_arr = nlohmann::json::array();
  if (bank_) {
    // 按强度查询候选种子
    auto candidates = bank_->query_by_intensity(3, 200);
    std::vector<std::pair<double, bridge::SeedEntry>> scored;
    for (const auto& c : candidates) {
      // 用种子的 intensity 作为近似的坐标值
      std::vector<double> seed_coord(kSeedDim, 0.5);
      seed_coord[0] = static_cast<double>(c.intensity) / 10.0;
      double dist = weighted_distance_(p14, seed_coord);
      scored.emplace_back(dist, c);
    }
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    for (int i = 0; i < std::min(top_k, static_cast<int>(scored.size())); ++i) {
      auto entry = nlohmann::json::object();
      entry["name"]     = scored[i].second.name;
      entry["distance"] = std::round(scored[i].first * 1000) / 1000;
      entry["intensity"] = scored[i].second.intensity;
      seeds_arr.push_back(entry);
    }
  }
  result["seeds"] = seeds_arr;

  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 空缺检测 + 补齐建议 (Python 版 suggest_fill)
// ═══════════════════════════════════════════════════════════════════

auto SemanticBridge::suggest_fill(const std::string& section,
                                   double threshold) const noexcept -> nlohmann::json {
  auto result = query(section);

  auto suggestions = nlohmann::json::array();

  // 检查种子覆盖
  bool has_seed = false;
  auto& seeds_ref = result["seeds"];
  if (seeds_ref.is_array() && !seeds_ref.empty()) {
    double best_dist = seeds_ref[0].value("distance", 1.0);
    has_seed = best_dist < threshold;
  }

  if (!has_seed) {
    auto sugg = nlohmann::json::object();
    sugg["system"] = "seed";
    sugg["action"] = "inject_seed";
    auto& p14 = result["p_vector_14d"];
    if (p14.is_array()) {
      sugg["p_vector_14d"] = p14;
    }
    sugg["suggested_keyword"] = section;
    sugg["source"] = "P=N_gap_detection";
    sugg["priority"] = "high";
    suggestions.push_back(sugg);
  }

  result["suggestions"] = suggestions;
  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════════

auto SemanticBridge::steer_profile() const noexcept
    -> std::vector<SteerProfile> {
  return profiles_;
}

auto SemanticBridge::steer_to_seed_map() const noexcept
    -> std::vector<int> {
  return steer_to_seed_map_;
}

auto SemanticBridge::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["n_profiles"] = static_cast<int>(profiles_.size());
  j["loaded"]     = loaded_;
  j["k_steer_dim"] = kSteerDim;
  j["k_seed_dim"]  = kSeedDim;
  if (bank_) {
    j["seed_count"] = bank_->count();
  }
  return j;
}

}  // namespace nexus::psyche
