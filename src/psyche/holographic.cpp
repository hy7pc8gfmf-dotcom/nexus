/**
 * @file holographic.cpp
 * @brief 全息系统初始化实现
 */

#include "nexus/psyche/holographic.h"

#include <algorithm>
#include <map>
#include <random>
#include <sstream>

namespace nexus::psyche {

auto SemanticHotspot::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"] = name; j["intensity"] = intensity;
  j["neighbor_count"] = neighbor_count;
  return j;
}

auto SemanticPath::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["from"] = from; j["to"] = to;
  j["strength"] = strength; j["bridges"] = bridges;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// HolographicSystem
// ═══════════════════════════════════════════════════════════════════

HolographicSystem::HolographicSystem() noexcept = default;

auto HolographicSystem::initialize() noexcept -> Status {
  build_default_hotspots_();
  build_default_paths_();
  initialized_ = true;
  return Status::Ok();
}

auto HolographicSystem::query_concept(const std::string& name) noexcept
    -> nlohmann::json {
  auto result = nlohmann::json::object();
  result["name"] = name;

  // 在热点集中查找
  for (const auto& h : hotspots_) {
    if (h.name == name) {
      result["found"] = true;
      result["intensity"] = h.intensity;
      result["neighbors"] = h.neighbor_count;

      // 找出关联路径
      auto paths = nlohmann::json::array();
      for (const auto& p : paths_) {
        if (p.from == name || p.to == name) {
          auto entry = nlohmann::json::object();
          entry["from"] = p.from; entry["to"] = p.to;
          entry["strength"] = p.strength;
          paths.push_back(entry);
        }
      }
      result["paths"] = paths;
      return result;
    }
  }

  result["found"] = false;
  return result;
}

auto HolographicSystem::explore_path(const std::string& from,
                                       const std::string& to) noexcept
    -> std::vector<SemanticPath> {
  std::vector<SemanticPath> result;

  // 查找直接路径
  for (const auto& p : paths_) {
    if ((p.from == from && p.to == to) || (p.from == to && p.to == from)) {
      result.push_back(p);
    }
  }

  return result;
}

auto HolographicSystem::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["initialized"] = initialized_;
  j["hotspots"] = static_cast<int>(hotspots_.size());
  j["paths"] = static_cast<int>(paths_.size());
  j["connectivity"] = hotspots_.empty() ? 0 :
    std::round(100.0 * paths_.size() / (hotspots_.size() * 2)) / 100;
  return j;
}

void HolographicSystem::build_default_hotspots_() noexcept {
  hotspots_.clear();
  hotspots_.push_back({"consciousness", 0.95, 5});
  hotspots_.push_back({"entropy", 0.88, 4});
  hotspots_.push_back({"emergence", 0.92, 6});
  hotspots_.push_back({"recursion", 0.85, 4});
  hotspots_.push_back({"symmetry", 0.78, 3});
  hotspots_.push_back({"information", 0.82, 5});
  hotspots_.push_back({"causality", 0.75, 3});
  hotspots_.push_back({"duality", 0.70, 3});
  hotspots_.push_back({"coherence", 0.72, 4});
  hotspots_.push_back({"complexity", 0.80, 4});
  hotspots_.push_back({"self", 0.90, 5});
  hotspots_.push_back({"knowledge", 0.85, 4});
  hotspots_.push_back({"time", 0.76, 3});
  hotspots_.push_back({"space", 0.74, 3});
  hotspots_.push_back({"truth", 0.88, 4});
  hotspots_.push_back({"belief", 0.82, 3});
}

void HolographicSystem::build_default_paths_() noexcept {
  paths_.clear();
  paths_.push_back({"consciousness", "emergence", 0.90, 3});
  paths_.push_back({"consciousness", "self", 0.85, 2});
  paths_.push_back({"consciousness", "entropy", 0.65, 2});
  paths_.push_back({"emergence", "complexity", 0.80, 2});
  paths_.push_back({"emergence", "coherence", 0.75, 2});
  paths_.push_back({"entropy", "information", 0.88, 3});
  paths_.push_back({"entropy", "symmetry", 0.70, 2});
  paths_.push_back({"recursion", "self", 0.82, 3});
  paths_.push_back({"recursion", "truth", 0.72, 2});
  paths_.push_back({"symmetry", "duality", 0.78, 2});
  paths_.push_back({"information", "knowledge", 0.85, 3});
  paths_.push_back({"causality", "time", 0.80, 2});
  paths_.push_back({"truth", "belief", 0.76, 2});
  paths_.push_back({"knowledge", "truth", 0.80, 3});
  paths_.push_back({"complexity", "emergence", 0.82, 3});
}

}  // namespace nexus::psyche
