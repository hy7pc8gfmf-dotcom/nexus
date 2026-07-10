// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_atlas.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto SemanticTier::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]    = name;
  j["weight"]  = weight;
  j["classical"] = std::round(classical * 100) / 100;
  j["nonconstructive"] = std::round(nonconstructive * 100) / 100;
  auto tags_arr = nlohmann::json::array();
  for (const auto& t : tags) tags_arr.push_back(t);
  j["tags"] = tags_arr;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

SemanticAtlas::SemanticAtlas() noexcept {
  build_default_tiers_();
}

void SemanticAtlas::build_default_tiers_() noexcept {
  tiers_ = {
    {"axiom",         1.0, {"公理", "基础"}, 0.5, 0.0},
    {"classical",    0.8, {"经典逻辑"},       1.0, 0.0},
    {"nonconstructive", 0.7, {"非构造性"},    0.8, 0.8},
    {"type_level",   0.9, {"类型论", "Coq"},  0.3, 0.0},
    {"constructive", 0.8, {"构造性"},          0.0, 0.0},
  };
}

// ═══════════════════════════════════════════════════════════════════
// 构建
// ═══════════════════════════════════════════════════════════════════

auto SemanticAtlas::build(SemanticPort* port) noexcept -> Status {
  if (!port) {
    return Status::Error(ErrorCode::kInvalidConfig, "port is null");
  }
  port_ = port;
  built_ = true;
  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════════

auto SemanticAtlas::concept_coord(const std::string& name) const noexcept
    -> Result<std::vector<float>> {
  if (!port_ || !port_->loaded()) {
    return Status::Error(ErrorCode::kInvalidConfig, "port not loaded");
  }

  // 从缓存或实时查询
  auto it = coord_cache_.find(name);
  if (it != coord_cache_.end()) return it->second;

  auto result = port_->concept_of(name);
  if (!result.ok()) {
    return Status::Error(ErrorCode::kFileNotFound, "concept not found: " + name);
  }

  const auto& c = result.value();
  std::vector<float> coord(c.coord, c.coord + kSemanticDim);
  coord_cache_[name] = coord;
  return coord;
}

auto SemanticAtlas::concept_tier(const std::string& name) const noexcept
    -> std::string {
  // 简单规则: 根据概念名中的关键词判断层级
  for (const auto& tier : tiers_) {
    for (const auto& tag : tier.tags) {
      if (name.find(tag) != std::string::npos) {
        return tier.name;
      }
    }
  }
  return "constructive";  // 默认为构造性
}

void SemanticAtlas::register_tier(const SemanticTier& tier) noexcept {
  tiers_.push_back(tier);
}

// ═══════════════════════════════════════════════════════════════════
// 持久化
// ═══════════════════════════════════════════════════════════════════

auto SemanticAtlas::save(const std::string& path) const noexcept -> Status {
  auto data = nlohmann::json::object();
  auto tiers_arr = nlohmann::json::array();
  for (const auto& t : tiers_) tiers_arr.push_back(t.to_json());
  data["version"] = 1;
  data["tiers"] = tiers_arr;
  data["n_cached"] = static_cast<int>(coord_cache_.size());
  data["built"] = built_;

  std::ofstream ofs(path);
  if (!ofs.is_open()) {
    return Status::Error(ErrorCode::kIoError, "cannot write: " + path);
  }
  ofs << data.dump(2);
  return Status::Ok();
}

auto SemanticAtlas::load(const std::string& path) noexcept -> Status {
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    return Status::Error(ErrorCode::kFileNotFound, "not found: " + path);
  }
  nlohmann::json data;
  ifs >> data;
  built_ = data.value("built", false);
  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 统计
// ═══════════════════════════════════════════════════════════════════

auto SemanticAtlas::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["n_tiers"]     = static_cast<int>(tiers_.size());
  j["n_cached"]    = static_cast<int>(coord_cache_.size());
  j["built"]       = built_;
  j["port_loaded"] = port_ ? port_->loaded() : false;
  return j;
}

}  // namespace nexus::psyche
