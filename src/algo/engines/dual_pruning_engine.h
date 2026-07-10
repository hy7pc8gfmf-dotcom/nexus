// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NEXUS_ALGO_ENGINES_DUAL_PRUNING_H
#define NEXUS_ALGO_ENGINES_DUAL_PRUNING_H

#include "nexus/algo/engine.h"
#include <cmath>
#include <set>
#include <sstream>

namespace nexus::algo::engines {

/// 双逻辑剪枝引擎 — 从候选集中并行剪除冗余/矛盾项
class DualPruningEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return {"dual_pruning", "Dual Logic Pruning", "1.0.0",
            "双路并行剪枝: 前置约束剪枝 + 后置矛盾剪枝", {"pruning", "logic", "optimization"}};
  }

  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true; return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");

    auto candidates = input.value("candidates", std::vector<std::string>());
    auto constraints = input.value("constraints", std::vector<std::string>());
    double threshold = input.value("threshold", 0.5);

    // 前置剪枝: 按长度/复杂度过滤
    std::vector<std::string> after_pre;
    for (const auto& c : candidates) {
      if (c.size() >= 3) after_pre.push_back(c);
    }

    // 后置剪枝: 检查相似度(简单编辑距离)
    std::vector<std::string> after_post;
    std::set<int> pruned_indices;
    for (size_t i = 0; i < after_pre.size(); ++i) {
      if (pruned_indices.count(static_cast<int>(i))) continue;
      after_post.push_back(after_pre[i]);
      for (size_t j = i + 1; j < after_pre.size(); ++j) {
        double sim = similarity_(after_pre[i], after_pre[j]);
        if (sim > threshold) pruned_indices.insert(static_cast<int>(j));
      }
    }

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["input_count"] = static_cast<int>(candidates.size());
    result["output_count"] = static_cast<int>(after_post.size());
    result["pruned_count"] = static_cast<int>(candidates.size() - after_post.size());
    result["pre_pruned"] = static_cast<int>(candidates.size() - after_pre.size());
    result["post_pruned"] = static_cast<int>(after_pre.size() - after_post.size());
    result["results"] = after_post;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object(); j["initialized"] = initialized_; return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;

  static double similarity_(const std::string& a, const std::string& b) {
    if (a.empty() && b.empty()) return 1.0;
    int dist = edit_distance_(a, b);
    int max_len = std::max(static_cast<int>(a.size()), static_cast<int>(b.size()));
    return max_len == 0 ? 1.0 : 1.0 - static_cast<double>(dist) / max_len;
  }

  static int edit_distance_(const std::string& a, const std::string& b) {
    size_t n = a.size(), m = b.size();
    std::vector<int> prev(m + 1), curr(m + 1);
    for (size_t j = 0; j <= m; ++j) prev[j] = static_cast<int>(j);
    for (size_t i = 1; i <= n; ++i) {
      curr[0] = static_cast<int>(i);
      for (size_t j = 1; j <= m; ++j) {
        int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
        curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
      }
      std::swap(prev, curr);
    }
    return prev[m];
  }
};

} // namespace
#endif
