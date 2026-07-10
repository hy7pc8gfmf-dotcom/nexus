// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_HYBRID_REASONER_H
#define NEXUS_ALGO_ENGINES_HYBRID_REASONER_H

/* 混合推理器 v2 — 降级链 + 延迟预算 + 模型能力注册 */

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <vector>

namespace nexus::algo::engines {

class HybridReasonerEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id = "hybrid_reasoner", .name = "Hybrid Reasoner v2",
      .version = "2.0.0",
      .description = "本地/云端降级链 + 延迟预算 + 能力匹配",
      .tags = {"reasoning", "hybrid", "fallback", "latency"},
    };
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    local_confidence_ = config.value("local_confidence", 0.65);
    cloud_confidence_ = config.value("cloud_confidence", 0.85);
    latency_budget_ms_ = config.value("latency_budget_ms", 5000);
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    auto query = input.value("query", std::string(""));
    bool prefer_cloud = input.value("prefer_cloud", false);
    auto task_type = input.value("task_type", std::string("general"));

    // 模型能力注册表 (本地可处理的任务类型与置信度)
    const std::map<std::string, double> local_capabilities = {
      {"general", 0.65}, {"math", 0.70}, {"code", 0.60},
      {"translation", 0.55}, {"summarization", 0.70},
      {"qa", 0.65}, {"reasoning", 0.60},
    };

    // 云端能力
    const std::map<std::string, double> cloud_capabilities = {
      {"general", 0.85}, {"math", 0.90}, {"code", 0.88},
      {"translation", 0.90}, {"summarization", 0.85},
      {"qa", 0.88}, {"reasoning", 0.85}, {"creative", 0.80},
    };

    // 降级链决策
    struct FallbackStep {
      std::string mode;
      double confidence;
      int latency_ms;
      std::string model;
    };

    std::vector<FallbackStep> fallback_chain;
    double local_conf = local_confidence_;
    auto lc = local_capabilities.find(task_type);
    if (lc != local_capabilities.end()) local_conf = lc->second;

    double cloud_conf = cloud_confidence_;
    auto cc = cloud_capabilities.find(task_type);
    if (cc != cloud_capabilities.end()) cloud_conf = cc->second;

    // Step 1: 首选模式
    if (prefer_cloud && cloud_conf > 0.7) {
      fallback_chain.push_back({"cloud", cloud_conf, 2000, "ds-v4-pro"});
      fallback_chain.push_back({"local", local_conf, 500, "qwythos-9b"});
    } else {
      fallback_chain.push_back({"local", local_conf, 500, "qwythos-9b"});
      if (local_conf < 0.8) {
        fallback_chain.push_back({"cloud", cloud_conf, 2000, "ds-v4-pro"});
      }
    }

    // Step 3: 兜底 (轻量)
    if (local_conf < 0.5) {
      fallback_chain.push_back({"lightweight", 0.4, 100, "grm2-3b"});
    }

    // 选择第一个可用模式
    auto chosen = fallback_chain[0];
    double total_latency = 0;
    for (const auto& step : fallback_chain) total_latency += step.latency_ms;

    // 检查延迟预算
    bool within_budget = total_latency <= latency_budget_ms_;

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["query"] = query.substr(0, 64);
    result["task_type"] = task_type;
    result["chosen_mode"] = chosen.mode;
    result["confidence"] = std::round(chosen.confidence * 1000) / 1000;
    result["estimated_latency_ms"] = static_cast<int>(chosen.latency_ms);
    result["within_budget"] = within_budget;
    result["total_fallback_steps"] = static_cast<int>(fallback_chain.size());

    auto steps = nlohmann::json::array();
    for (const auto& step : fallback_chain) {
      auto s = nlohmann::json::object();
      s["mode"] = step.mode;
      s["confidence"] = std::round(step.confidence * 1000) / 1000;
      s["latency_ms"] = step.latency_ms;
      steps.push_back(s);
    }
    result["fallback_chain"] = steps;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"] = initialized_;
    j["local_confidence"] = local_confidence_;
    j["cloud_confidence"] = cloud_confidence_;
    return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
  double local_confidence_ = 0.65;
  double cloud_confidence_ = 0.85;
  int latency_budget_ms_ = 5000;
};

} // namespace
#endif
