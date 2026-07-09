#ifndef NEXUS_ALGO_ENGINES_TEMPORAL_KG_H
#define NEXUS_ALGO_ENGINES_TEMPORAL_KG_H

#include "nexus/algo/engine.h"
#include <cmath>
#include <ctime>
#include <map>

namespace nexus::algo::engines {

/// 知识图谱时序衰减 — 按时间衰减实体/关系权重
class TemporalKgEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return {"temporal_kg", "Temporal Knowledge Graph", "1.0.0",
            "时序知识图谱: 按时间衰减实体权重, 识别过时信息", {"knowledge-graph", "temporal", "decay"}};
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    decay_rate_ = config.value("decay_rate", 0.1);
    initialized_ = true; return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");

    auto entities_raw = input.value("entities", std::vector<nlohmann::json>());
    double current_time = input.value("current_time", 1000.0);

    auto result = nlohmann::json::object();
    auto processed = nlohmann::json::array();
    int decayed = 0;

    for (const auto& e : entities_raw) {
      auto entry = nlohmann::json::object();
      std::string name = e.value("name", std::string("unknown"));
      double created = e.value("created_at", 0.0);
      double weight = e.value("weight", 1.0);

      // 时序衰减: w(t) = w₀ * exp(-λ * Δt)
      double delta_t = current_time - created;
      double decayed_weight = weight * std::exp(-decay_rate_ * std::max(0.0, delta_t));

      entry["name"] = name;
      entry["original_weight"] = std::round(weight * 1000) / 1000;
      entry["decayed_weight"] = std::round(decayed_weight * 1000) / 1000;
      entry["age"] = std::round(delta_t * 10) / 10;
      entry["decay_factor"] = delta_t > 0
        ? std::round(std::exp(-decay_rate_ * delta_t) * 1000) / 1000 : 1.0;

      if (decayed_weight < 0.5 * weight) decayed++;
      processed.push_back(entry);
    }

    result["status"] = "ok";
    result["total_entities"] = static_cast<int>(entities_raw.size());
    result["decayed_entities"] = decayed;
    result["decay_rate"] = decay_rate_;
    result["entities"] = processed;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"] = initialized_; j["decay_rate"] = decay_rate_; return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
  double decay_rate_ = 0.1;
};

} // namespace
#endif
