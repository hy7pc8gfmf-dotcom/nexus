// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_english.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <sstream>
#include <unordered_map>

#include "nexus/psyche/semantic_expand.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 解析 "中文:权重,中文:权重" 字符串
// ═══════════════════════════════════════════════════════════════════

struct ZhPair {
  std::string name;
  double weight;
};

static std::vector<ZhPair> parse_zh_pairs(const std::string& input) {
  std::vector<ZhPair> result;
  size_t start = 0, end;
  while ((end = input.find(',', start)) != std::string::npos) {
    auto pair_str = input.substr(start, end - start);
    auto colon = pair_str.find(':');
    if (colon != std::string::npos) {
      ZhPair p;
      p.name   = pair_str.substr(0, colon);
      p.weight = std::stod(pair_str.substr(colon + 1));
      result.push_back(p);
    }
    start = end + 1;
  }
  // 最后一个
  if (start < input.size()) {
    auto pair_str = input.substr(start);
    auto colon = pair_str.find(':');
    if (colon != std::string::npos) {
      ZhPair p;
      p.name   = pair_str.substr(0, colon);
      p.weight = std::stod(pair_str.substr(colon + 1));
      result.push_back(p);
    }
  }
  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 英→中映射表 → JSON
// ═══════════════════════════════════════════════════════════════════

auto en2zh_to_json() -> nlohmann::json {
  auto arr = nlohmann::json::array();
  for (int i = 0; i < kEn2ZhCount; ++i) {
    auto j = nlohmann::json::object();
    j["en"] = kEn2Zh[i].en;

    auto pairs = parse_zh_pairs(kEn2Zh[i].zh_pairs);
    auto pairs_arr = nlohmann::json::array();
    for (const auto& p : pairs) {
      auto pair_arr = nlohmann::json::array();
      pair_arr.push_back(p.name);
      pair_arr.push_back(p.weight);
      pairs_arr.push_back(pair_arr);
    }
    j["zh_pairs"] = pairs_arr;
    arr.push_back(j);
  }
  return arr;
}

// ═══════════════════════════════════════════════════════════════════
// 生成英文概念坐标
// ═══════════════════════════════════════════════════════════════════

auto rebuild_english_to_json(double sigma, int seed) -> nlohmann::json {
  // 中文概念 → 坐标的缓存
  std::unordered_map<std::string, std::vector<float>> zh_coords;

  // 确定性噪声生成器
  std::mt19937_64 rng(static_cast<uint64_t>(seed));
  std::normal_distribution<double> normal(0.0, sigma);

  auto arr = nlohmann::json::array();

  for (int i = 0; i < kEn2ZhCount; ++i) {
    auto pairs = parse_zh_pairs(kEn2Zh[i].zh_pairs);

    // 加权平均
    double sum_x[14] = {0};
    double total_w = 0;

    for (const auto& p : pairs) {
      // 获取或生成中文概念坐标
      auto it = zh_coords.find(p.name);
      if (it == zh_coords.end()) {
        auto coord = SemanticExpander::name_to_coord(p.name, 0);
        zh_coords[p.name] = coord;
        it = zh_coords.find(p.name);
      }
      const auto& coord = it->second;
      for (int d = 0; d < 14 && d < static_cast<int>(coord.size()); ++d) {
        sum_x[d] += coord[d] * p.weight;
      }
      total_w += p.weight;
    }

    if (total_w <= 0) continue;

    // 生成最终向量 (加权平均 + 噪声)
    auto vec_arr = nlohmann::json::array();
    for (int d = 0; d < 14; ++d) {
      double v = sum_x[d] / total_w;
      v += normal(rng);  // 加噪声
      v = std::clamp(v, 0.0, 1.0);
      vec_arr.push_back(std::round(v * 1e6) / 1e6);
    }

    auto j = nlohmann::json::object();
    j["en"]   = kEn2Zh[i].en;
    j["coord"] = vec_arr;
    j["lang"] = "en";
    arr.push_back(j);
  }

  return arr;
}

}  // namespace nexus::psyche
