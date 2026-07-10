// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_TEMPORAL_KG_H
#define NEXUS_ALGO_ENGINES_TEMPORAL_KG_H

/* 时序知识图谱 v2 — Ebbinghaus 遗忘曲线 + 语义衰减 + 近期增益 */

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <vector>

namespace nexus::algo::engines {

class TemporalKgEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id = "temporal_kg", .name = "Temporal Knowledge Graph v2",
      .version = "2.0.0",
      .description = "Ebbinghaus 遗忘 + 语义衰减 + 近期增益",
      .tags = {"knowledge-graph", "temporal", "forgetting", "decay"},
    };
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    decay_rate_ = config.value("decay_rate", 0.1);
    recency_boost_ = config.value("recency_boost", 0.3);
    enable_forgetting_curve_ = config.value("forgetting_curve", true);
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    auto entities_raw = input.value("entities", std::vector<nlohmann::json>());
    double current_time = input.value("current_time", 1000.0);

    auto result = nlohmann::json::object();
    auto processed = nlohmann::json::array();
    int decayed = 0, boosted = 0;

    for (const auto& e : entities_raw) {
      auto entry = nlohmann::json::object();
      std::string name = e.value("name", std::string("unknown"));
      double created = e.value("created_at", 0.0);
      double weight = e.value("weight", 1.0);
      int access_count = e.value("access_count", 0);
      double last_access = e.value("last_access", created);

      double delta_t = current_time - created;
      double age = std::max(0.0, delta_t);

      // 指数衰减: w(t) = w₀ * exp(-λ * Δt)
      double decayed_weight = weight * std::exp(-decay_rate_ * age);

      // Ebbinghaus 遗忘曲线: 基于访问次数加强记忆
      if (enable_forgetting_curve_) {
        // 每次访问强化: w *= (1 + 0.1 * sqrt(access_count))
        double recall_boost = 1.0 + 0.1 * std::sqrt(static_cast<double>(access_count));
        decayed_weight *= std::min(2.0, recall_boost);
      }

      // 近期增益: 最近访问过的实体权重重置
      double recency = current_time - last_access;
      if (recency < 50.0) {
        double recency_factor = recency_boost_ * (1.0 - recency / 50.0);
        decayed_weight += recency_factor;
        boosted++;
      }

      decayed_weight = std::clamp(decayed_weight, 0.01, 2.0);

      if (decayed_weight < 0.5 * weight) decayed++;

      entry["name"] = name;
      entry["original_weight"] = std::round(weight * 1000) / 1000;
      entry["decayed_weight"] = std::round(decayed_weight * 1000) / 1000;
      entry["age"] = std::round(age * 10) / 10;
      entry["access_count"] = access_count;
      entry["recency_boosted"] = recency < 50.0;

      processed.push_back(entry);
    }

    result["status"] = "ok";
    result["decay_model"] = enable_forgetting_curve_ ? "ebbinghaus" : "exponential";
    result["total_entities"] = static_cast<int>(entities_raw.size());
    result["decayed_entities"] = decayed;
    result["recency_boosted"] = boosted;
    result["decay_rate"] = decay_rate_;
    result["entities"] = processed;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"] = initialized_;
    j["decay_rate"] = decay_rate_;
    j["forgetting_curve"] = enable_forgetting_curve_;
    return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
  double decay_rate_ = 0.1;
  double recency_boost_ = 0.3;
  bool enable_forgetting_curve_ = true;
};

} // namespace
#endif
