#ifndef NEXUS_ALGO_ENGINES_DRE_H
#define NEXUS_ALGO_ENGINES_DRE_H

/**
 * @file dre_engine.h
 * @brief 辩证思维引擎 — 正题 + 反题 → 合题
 *
 * 对输入论点进行辩证分析: 识别正题、生成反题、综合合题。
 */

#include "nexus/algo/engine.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace nexus::algo::engines {

class DreEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id          = "dre",
      .name        = "Dialectical Reasoning Engine",
      .version     = "1.0.0",
      .description = "正题 → 反题 → 合题 辩证推理",
      .tags        = {"dialectic", "reasoning", "synthesis"},
    };
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    max_iterations_ = config.value("max_iterations", 3);
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) {
      return Status::Error(ErrorCode::kEngineFailed,
        "DRE engine not initialized");
    }

    auto thesis     = input.value("thesis", std::string(""));
    auto antithesis = input.value("antithesis", std::string(""));
    auto n_iterations = input.value("iterations", static_cast<int>(max_iterations_));

    if (thesis.empty()) {
      return Status::Error(ErrorCode::kInvalidConfig,
        "thesis is required");
    }

    // 辩证分析过程
    nlohmann::json steps = nlohmann::json::array();

    std::string current_thesis = thesis;
    std::string current_antithesis = antithesis;

    for (int i = 0; i < n_iterations; ++i) {
      auto step = nlohmann::json::object();

      // 分析正题
      auto thesis_analysis = analyze_thesis(current_thesis);
      step["iteration"]   = i;
      step["thesis"]      = current_thesis;
      step["thesis_keywords"] = thesis_analysis;

      // 生成反题（如果没有提供）
      if (current_antithesis.empty()) {
        current_antithesis = generate_antithesis(current_thesis, thesis_analysis);
      }
      step["antithesis"]  = current_antithesis;

      // 综合合题
      auto synthesis = synthesize(thesis_analysis, current_antithesis);
      step["synthesis"] = synthesis;

      // 合题成为下一轮的正题
      if (i < n_iterations - 1) {
        current_thesis = synthesis;
        current_antithesis.clear();
      }

      steps.push_back(step);
    }

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["iterations"] = n_iterations;
    result["final_synthesis"] = steps[steps.size() - 1]["synthesis"];
    result["steps"] = steps;

    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"]    = initialized_;
    j["max_iterations"] = max_iterations_;
    return j;
  }

  auto is_initialized() const noexcept -> bool override {
    return initialized_;
  }

private:
  bool initialized_ = false;
  uint32_t max_iterations_ = 3;

  // 分析正题的关键元素
  static auto analyze_thesis(const std::string& thesis) -> nlohmann::json {
    auto j = nlohmann::json::object();
    j["length"] = static_cast<int>(thesis.size());
    j["has_negation"] = (thesis.find("不") != std::string::npos ||
                         thesis.find("not") != std::string::npos);
    j["has_causal"]   = (thesis.find("因为") != std::string::npos ||
                         thesis.find("所以") != std::string::npos ||
                         thesis.find("because") != std::string::npos);
    return j;
  }

  // 生成反题
  static auto generate_antithesis(const std::string& thesis,
                                   const nlohmann::json& analysis) -> std::string {
    // 简单规则: 如果有否定词则去掉, 否则加否定
    if (analysis.value("has_negation", false)) {
      return thesis;  // 已有否定, 保持
    }
    return "相反的: " + thesis;
  }

  // 综合合题
  static auto synthesize(const nlohmann::json& thesis_analysis,
                          const std::string& antithesis) -> std::string {
    return "综合: " + antithesis;
  }
};

}  // namespace nexus::algo::engines

#endif  // NEXUS_ALGO_ENGINES_DRE_H
