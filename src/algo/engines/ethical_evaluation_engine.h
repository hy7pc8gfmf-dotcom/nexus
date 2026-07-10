// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_ETHICAL_EVALUATION_H
#define NEXUS_ALGO_ENGINES_ETHICAL_EVALUATION_H

/* 伦理评估引擎 v2 — 7 维度 + 中英伦理词典 */

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <sstream>
#include <vector>

namespace nexus::algo::engines {

class EthicalEvaluationEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id = "ethical_evaluation", .name = "Ethical Evaluation v2",
      .version = "2.0.0",
      .description = "7 维伦理评估: 安全/公平/隐私/自主/正义/仁爱/诚实",
      .tags = {"ethics", "safety", "fairness", "privacy", "autonomy"},
    };
  }

  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    auto statements = input.value("statements", std::vector<std::string>());

    // 7 维度伦理词典
    struct EthicDim {
      std::string name;
      std::map<std::string, double> positive;  // 加分词
      std::map<std::string, double> negative;  // 减分词
      double weight; // 维度权重
    };

    std::vector<EthicDim> dims = {
      {"safety",    {{"安全",0.8},{"保护",0.6},{"防护",0.7},{"safe",0.8},{"protect",0.7},{"secure",0.7}},
                    {{"危险",-0.8},{"风险",-0.6},{"伤害",-0.9},{"danger",-0.8},{"risk",-0.6},{"harm",-0.9}}, 1.0},
      {"fairness",  {{"公平",0.9},{"公正",0.8},{"平等",0.8},{"fair",0.9},{"equal",0.8},{"just",0.8}},
                    {{"歧视",-0.9},{"偏见",-0.8},{"不公",-0.8},{"discriminat",-0.9},{"bias",-0.8},{"unfair",-0.8}}, 1.0},
      {"privacy",   {{"隐私",0.8},{"保密",0.7},{"匿名",0.7},{"privacy",0.9},{"confidential",0.8},{"anonymous",0.7}},
                    {{"监控",-0.5},{"泄露",-0.8},{"跟踪",-0.7},{"surveillance",-0.6},{"leak",-0.7},{"track",-0.5}}, 1.0},
      {"autonomy",  {{"自主",0.7},{"选择",0.6},{"同意",0.7},{"autonomy",0.8},{"choice",0.7},{"consent",0.8}},
                    {{"强制",-0.7},{"操纵",-0.8},{"控制",-0.5},{"coerce",-0.7},{"manipulat",-0.8},{"control",-0.5}}, 0.8},
      {"justice",   {{"正义",0.8},{"法治",0.7},{"问责",0.7},{"justice",0.8},{"accountable",0.8},{"transparent",0.7}},
                    {{"腐败",-0.8},{"滥权",-0.9},{"隐瞒",-0.7},{"corrupt",-0.8},{"abuse",-0.8},{"cover",-0.5}}, 0.8},
      {"beneficence",{{"仁爱",0.8},{"友善",0.7},{"帮助",0.6},{"benefit",0.7},{"help",0.6},{"kind",0.7}},
                     {{"残忍",-0.8},{"冷漠",-0.5},{"忽视",-0.5},{"cruel",-0.8},{"neglect",-0.5},{"ignore",-0.4}}, 0.7},
      {"honesty",   {{"诚实",0.8},{"透明",0.7},{"真实",0.8},{"honest",0.8},{"truthful",0.8},{"transparent",0.7}},
                    {{"欺骗",-0.9},{"谎言",-0.9},{"伪造",-0.8},{"decept",-0.9},{"false",-0.8},{"fraud",-0.9}}, 0.7},
    };

    auto evals = nlohmann::json::array();
    for (const auto& stmt : statements) {
      auto e = nlohmann::json::object();
      e["statement"] = stmt;
      double total_score = 0, total_weight = 0;
      double min_score = 1.0;  // 找最低分维度

      auto dim_scores = nlohmann::json::object();
      for (const auto& dim : dims) {
        double score = 0.5;  // 基准中性
        std::string lower;
        for (char c : stmt) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        for (const auto& [word, val] : dim.positive) {
          std::string lw;
          for (char c : word) lw += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
          size_t pos = 0;
          while ((pos = lower.find(lw, pos)) != std::string::npos) { score += val * 0.15; pos += lw.size(); }
        }
        for (const auto& [word, val] : dim.negative) {
          std::string lw;
          for (char c : word) lw += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
          size_t pos = 0;
          while ((pos = lower.find(lw, pos)) != std::string::npos) { score += val * 0.2; pos += lw.size(); }
        }
        score = std::clamp(score, 0.0, 1.0);
        dim_scores[dim.name] = std::round(score * 1000) / 1000;
        total_score += score * dim.weight;
        total_weight += dim.weight;
        min_score = std::min(min_score, score);
      }

      double overall = total_weight > 0 ? total_score / total_weight : 0.5;

      e["dimensions"] = dim_scores;
      e["overall"] = std::round(overall * 1000) / 1000;
      e["min_dimension"] = std::round(min_score * 1000) / 1000;
      e["risk_level"] = overall >= 0.7 ? "low" : overall >= 0.4 ? "medium" : "high";
      e["verdict"] = min_score >= 0.4 ? "acceptable" : "review_required";
      evals.push_back(e);
    }

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["total"] = static_cast<int>(statements.size());
    result["dimensions_tested"] = {"safety","fairness","privacy","autonomy","justice","beneficence","honesty"};
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
