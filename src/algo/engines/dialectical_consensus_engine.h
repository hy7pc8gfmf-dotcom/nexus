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

#ifndef NEXUS_ALGO_ENGINES_DIALECTICAL_CONSENSUS_H
#define NEXUS_ALGO_ENGINES_DIALECTICAL_CONSENSUS_H

#include "nexus/algo/engine.h"
#include <cmath>
#include <map>
#include <set>

namespace nexus::algo::engines {

/// 辩证共识引擎 — 多观点 → 共识度量化
class DialecticalConsensusEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return {"dialectical_consensus", "Dialectical Consensus", "1.0.0",
            "多观点辩证共识度计算", {"consensus", "dialectic", "agreement"}};
  }

  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true; return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");

    auto viewpoints = input.value("viewpoints", std::vector<std::string>());
    if (viewpoints.empty()) {
      return Status::Error(ErrorCode::kInvalidConfig, "viewpoints required");
    }

    // 关键词提取 + 共识分析
    std::map<std::string, int> keyword_freq;
    for (const auto& vp : viewpoints) {
      std::set<std::string> seen;
      std::string word;
      for (char c : vp) {
        if (std::isalnum(c) || c == '_') { word += c; }
        else if (!word.empty()) {
          if (word.size() > 1 && seen.insert(word).second) keyword_freq[word]++;
          word.clear();
        }
      }
      if (!word.empty() && word.size() > 1 && seen.insert(word).second) keyword_freq[word]++;
    }

    // 共识度 = 共享关键词比例
    double total_keywords = static_cast<double>(keyword_freq.size());
    double shared_keywords = 0;
    auto shared = nlohmann::json::array();
    for (const auto& [kw, count] : keyword_freq) {
      if (count == static_cast<int>(viewpoints.size())) {
        shared_keywords++;
        shared.push_back(kw);
      }
    }
    double consensus = total_keywords > 0 ? shared_keywords / total_keywords : 1.0;

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["viewpoint_count"] = static_cast<int>(viewpoints.size());
    result["consensus_score"] = std::round(consensus * 1000) / 1000;
    result["total_keywords"] = static_cast<int>(total_keywords);
    result["shared_keywords"] = static_cast<int>(shared_keywords);
    result["shared_terms"] = shared;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object(); j["initialized"] = initialized_; return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
};

} // namespace
#endif
