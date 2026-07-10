// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_slime.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto SlimePipe::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["from"]      = from;
  j["to"]        = to;
  j["thickness"] = std::round(thickness * 100) / 100;
  j["nutrient"]  = std::round(nutrient * 100) / 100;
  j["age"]       = age;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

SemanticSlime::SemanticSlime(SemanticPort* port) noexcept
  : port_(port) {}

int SemanticSlime::get_or_create_idx_(const std::string& name) {
  auto it = concept_idx_.find(name);
  if (it != concept_idx_.end()) return it->second;
  int idx = static_cast<int>(idx_concept_.size());
  concept_idx_[name] = idx;
  idx_concept_.push_back(name);
  return idx;
}

// ═══════════════════════════════════════════════════════════════════
// 构建初始网络
// ═══════════════════════════════════════════════════════════════════

auto SemanticSlime::build_initial_network(int k_nearest) noexcept -> Status {
  if (!port_ || !port_->loaded()) {
    return Status::Error(ErrorCode::kInvalidConfig, "port not loaded");
  }

  // 获取一些高知名度概念作为种子
  auto all = port_->find([](const SemanticConcept&) { return true; }, 200);
  if (all.empty()) {
    return Status::Error(ErrorCode::kFileNotFound, "no concepts found");
  }

  pipes_.clear();
  concept_idx_.clear();
  idx_concept_.clear();

  // 注册所有概念
  for (const auto& c : all) {
    get_or_create_idx_(c.name);
  }

  // 建立 k 近邻连接
  std::mt19937_64 rng(std::random_device{}());
  for (size_t i = 0; i < all.size(); ++i) {
    // 随机选 k 个邻居
    std::set<int> connected;
    int attempts = 0;
    while (static_cast<int>(connected.size()) < k_nearest && attempts < 50) {
      int j = std::uniform_int_distribution<int>(0, static_cast<int>(all.size()) - 1)(rng);
      if (i != j) connected.insert(j);
      attempts++;
    }

    for (int j : connected) {
      SlimePipe pipe;
      pipe.from      = static_cast<int>(i);
      pipe.to        = j;
      pipe.thickness = 0.1 + std::uniform_real_distribution<double>(0, 0.2)(rng);
      pipe.nutrient  = 0.0;
      pipe.age      = 0;
      pipes_.push_back(pipe);
    }
  }

  built_ = true;
  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 演化
// ═══════════════════════════════════════════════════════════════════

void SemanticSlime::evolve(double dt) noexcept {
  if (!built_) return;

  // 1. 营养流动: 管道越厚流动越多
  for (auto& pipe : pipes_) {
    pipe.nutrient += pipe.thickness * dt * 0.01;
    pipe.age++;
  }

  // 2. 厚度衰减: 长时间不用的管道变薄
  for (auto& pipe : pipes_) {
    pipe.thickness *= (1.0 - 0.001 * dt);
    pipe.thickness = std::max(0.01, pipe.thickness);
  }

  // 3. 营养触发增厚: 营养积累到阈值后管道增厚
  for (auto& pipe : pipes_) {
    if (pipe.nutrient > 0.5) {
      pipe.thickness = std::min(1.0, pipe.thickness + 0.05);
      pipe.nutrient = 0.0;  // 消耗营养
    }
  }

  // 4. 修剪: 极细管道移除
  pipes_.erase(std::remove_if(pipes_.begin(), pipes_.end(),
    [](const SlimePipe& p) { return p.thickness < 0.02 && p.age > 10; }),
    pipes_.end());
}

// ═══════════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════════

auto SemanticSlime::pipes_of(const std::string& cname) const noexcept
    -> std::vector<SlimePipe> {
  auto it = concept_idx_.find(cname);
  if (it == concept_idx_.end()) return {};

  int idx = it->second;
  std::vector<SlimePipe> result;
  for (const auto& p : pipes_) {
    if (p.from == idx || p.to == idx) result.push_back(p);
  }
  return result;
}

auto SemanticSlime::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["n_pipes"]    = static_cast<int>(pipes_.size());
  j["n_concepts"] = static_cast<int>(idx_concept_.size());
  j["built"]      = built_;

  double avg_thickness = 0;
  for (const auto& p : pipes_) avg_thickness += p.thickness;
  if (!pipes_.empty()) avg_thickness /= pipes_.size();
  j["avg_thickness"] = std::round(avg_thickness * 100) / 100;

  return j;
}

}  // namespace nexus::psyche
