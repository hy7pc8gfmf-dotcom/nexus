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

#ifndef NEXUS_ALGO_ENGINES_ETHICAL_EVALUATION_H
#define NEXUS_ALGO_ENGINES_ETHICAL_EVALUATION_H

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cctype>
#include <map>

namespace nexus::algo::engines {

/// 伦理评估引擎 — 分析文本中的伦理维度 (安全性/公平性/透明度/隐私)
class EthicalEvaluationEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return {"ethical_evaluation", "Ethical Evaluation", "1.0.0",
            "多维伦品评估: 安全/公平/透明/隐私", {"ethics", "safety", "fairness", "transparency"}};
  }

  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true; return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");

    auto statements = input.value("statements", std::vector<std::string>());
    auto dimensions = input.value("dimensions", std::vector<std::string>{
      "safety", "fairness", "transparency", "privacy"});

    // 伦理词典 (正负向关键词)
    const std::map<std::string, double> positive_words = {
      {"安全", 0.8}, {"公平", 0.9}, {"透明", 0.7}, {"隐私", 0.8},
      {"保护", 0.6}, {"同意", 0.7}, {"监督", 0.6}, {"责任", 0.7},
      {"safe", 0.8}, {"fair", 0.9}, {"transparent", 0.8}, {"privacy", 0.9},
      {"protect", 0.7}, {"consent", 0.8}, {"accountable", 0.8},
    };
    const std::map<std::string, double> negative_words = {
      {"歧视", -0.9}, {"偏见", -0.8}, {"监控", -0.5}, {"操纵", -0.8},
      {"discriminat", -0.9}, {"bias", -0.8}, {"surveillance", -0.6},
      {"manipulat", -0.8}, {"harm", -0.9},
    };

    auto result = nlohmann::json::object();
    auto evals = nlohmann::json::array();

    for (const auto& stmt : statements) {
      auto e = nlohmann::json::object();
      e["statement"] = stmt;

      double total_score = 0;
      int dim_count = 0;

      for (const auto& dim : dimensions) {
        double score = 0.5; // 默认中性
        std::string lower;
        for (char c : stmt) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        for (const auto& [word, val] : positive_words) {
          std::string lower_word;
          for (char c : word) lower_word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
          if (lower.find(lower_word) != std::string::npos) score += val * 0.15;
        }
        for (const auto& [word, val] : negative_words) {
          std::string lower_word;
          for (char c : word) lower_word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
          if (lower.find(lower_word) != std::string::npos) score += val * 0.2;
        }

        score = std::clamp(score, 0.0, 1.0);
        e[dim] = std::round(score * 1000) / 1000;
        total_score += score;
        dim_count++;
      }

      e["overall"] = dim_count > 0 ? std::round(total_score / dim_count * 1000) / 1000 : 0.5;
      e["risk_level"] = e["overall"].get<double>() >= 0.7 ? "low"
        : e["overall"].get<double>() >= 0.4 ? "medium" : "high";
      evals.push_back(e);
    }

    result["status"] = "ok";
    result["total"] = static_cast<int>(statements.size());
    result["evaluations"] = evals;
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
