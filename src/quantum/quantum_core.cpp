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

/**
 * @file quantum_core.cpp
 * @brief 量子系统核心实现 — 21 子系统 + 共识 + 信念 + 感知 + 模拟
 */

#include "nexus/quantum/quantum_core.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <random>
#include <sstream>

namespace nexus::quantum {

// ═══════════════════════════════════════════════════════════════════
// 21 子系统涌现
// ═══════════════════════════════════════════════════════════════════

auto SubsystemState::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"] = name; j["layer"] = layer;
  j["activation"] = std::round(activation * 1000) / 1000;
  j["coherence"]  = std::round(coherence * 1000) / 1000;
  return j;
}

QuantumEmergence::QuantumEmergence() noexcept { init_subsystems_(); }

void QuantumEmergence::init_subsystems_() noexcept {
  // 5 层 × 21 子系统 (与 D:/synapse 架构对齐)
  const std::vector<std::tuple<std::string, int>> defs = {
    // Layer 0: meta (3)
    {"meta_observer", 0}, {"meta_critic", 0}, {"meta_auditor", 0},
    // Layer 1: drive (5)
    {"drive_curiosity", 1}, {"drive_achievement", 1}, {"drive_exploration", 1},
    {"drive_preservation", 1}, {"drive_dominance", 1},
    // Layer 2: belief (5)
    {"belief_self", 2}, {"belief_world", 2}, {"belief_other", 2},
    {"belief_truth", 2}, {"belief_value", 2},
    // Layer 3: taboo (3)
    {"taboo_contradiction", 3}, {"taboo_uncertainty", 3}, {"taboo_conflict", 3},
    // Layer 4: mission (5)
    {"mission_purpose", 4}, {"mission_growth", 4}, {"mission_service", 4},
    {"mission_creation", 4}, {"mission_transcend", 4},
  };

  for (const auto& [name, layer] : defs) {
    SubsystemState s;
    s.name = name; s.layer = layer;
    s.activation = 0.1 + static_cast<double>(std::rand() % 50) / 500.0;
    s.coherence  = 0.3 + static_cast<double>(std::rand() % 70) / 700.0;
    subsystems_.push_back(s);
  }
}

void QuantumEmergence::tick(double dt) noexcept {
  static std::mt19937_64 rng(std::random_device{}());

  for (auto& s : subsystems_) {
    // 层内激活传播
    double layer_boost = 0.0;
    for (const auto& other : subsystems_) {
      if (other.layer == s.layer && &other != &s)
        layer_boost += other.activation * 0.01;
    }

    // 噪声驱动
    double noise = std::normal_distribution<double>(0, 0.02)(rng);
    s.activation = std::clamp(s.activation + layer_boost * dt + noise * dt, 0.0, 1.0);
    s.coherence  = std::clamp(s.coherence + noise * dt * 0.5, 0.0, 1.0);
  }

  // 全局涌现
  double sum_ae = 0, sum_we = 0;
  for (const auto& s : subsystems_) {
    sum_ae += s.activation * s.coherence;
    sum_we += s.activation * (1.0 - s.coherence);
  }
  global_ae_ = sum_ae / subsystems_.size();
  global_we_ = sum_we / subsystems_.size();
}

auto QuantumEmergence::subsystems() const noexcept -> const std::vector<SubsystemState>& { return subsystems_; }
auto QuantumEmergence::global_ae() const noexcept -> double { return global_ae_; }
auto QuantumEmergence::global_we() const noexcept -> double { return global_we_; }
auto QuantumEmergence::hilbert_dim() const noexcept -> int { return 1 << subsystems_.size(); }

auto QuantumEmergence::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["global_ae"] = std::round(global_ae_ * 1000) / 1000;
  j["global_we"] = std::round(global_we_ * 1000) / 1000;
  j["hilbert_dim"] = hilbert_dim();
  j["active_subsystems"] = static_cast<int>(subsystems_.size());
  auto subs = nlohmann::json::array();
  for (const auto& s : subsystems_) subs.push_back(s.to_json());
  j["subsystems"] = subs;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 共识管线
// ═══════════════════════════════════════════════════════════════════

auto ConsensusPipeline::evaluate(const std::vector<double>& opinions) noexcept -> nlohmann::json {
  rounds_++;
  if (opinions.empty()) {
    auto j = nlohmann::json::object(); j["consensus"] = 0; j["rounds"] = rounds_; return j;
  }
  double mean = 0, variance = 0;
  for (auto v : opinions) mean += v;
  mean /= opinions.size();
  for (auto v : opinions) variance += (v - mean) * (v - mean);
  variance /= opinions.size();

  auto j = nlohmann::json::object();
  j["consensus"]    = std::round(mean * 1000) / 1000;
  j["variance"]     = std::round(variance * 1000) / 1000;
  j["participants"] = static_cast<int>(opinions.size());
  j["rounds"]       = rounds_;
  j["confidence"]   = std::round((1.0 - std::sqrt(variance)) * 1000) / 1000;
  return j;
}

auto ConsensusPipeline::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object(); j["rounds"] = rounds_; return j;
}

// ═══════════════════════════════════════════════════════════════════
// 信念系统
// ═══════════════════════════════════════════════════════════════════

auto Belief::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"] = id; j["statement"] = statement;
  j["strength"] = std::round(strength * 1000) / 1000;
  j["confidence"] = std::round(confidence * 1000) / 1000;
  return j;
}

BeliefSystem::BeliefSystem() noexcept {
  beliefs_ = {
    {"B-01", "系统应当持续自我完善", 0.8, 0.9},
    {"B-02", "知识通过探索积累", 0.7, 0.8},
    {"B-03", "不确定性是认知的一部分", 0.6, 0.7},
  };
}

void BeliefSystem::add_belief(const std::string& statement, double strength) noexcept {
  Belief b;
  b.id = "B-" + std::to_string(beliefs_.size() + 1);
  b.statement = statement; b.strength = strength; b.confidence = 0.5;
  beliefs_.push_back(b);
}

void BeliefSystem::update(const std::string& id, double delta) noexcept {
  for (auto& b : beliefs_) {
    if (b.id == id) { b.strength = std::clamp(b.strength + delta, 0.0, 1.0); break; }
  }
}

void BeliefSystem::decay(double rate) noexcept {
  for (auto& b : beliefs_) {
    b.strength = std::max(0.0, b.strength - rate);
    b.confidence = std::max(0.0, b.confidence - rate * 0.5);
  }
}

auto BeliefSystem::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object(); j["total"] = static_cast<int>(beliefs_.size());
  auto arr = nlohmann::json::array();
  for (const auto& b : beliefs_) arr.push_back(b.to_json());
  j["beliefs"] = arr; return j;
}

// ═══════════════════════════════════════════════════════════════════
// 自我感知
// ═══════════════════════════════════════════════════════════════════

auto SelfPerception::perceive(const nlohmann::json& state) noexcept -> nlohmann::json {
  self_model_ = state;
  auto result = nlohmann::json::object();
  result["self_awareness"] = state.value("ae", 0.5);
  result["model_quality"]  = 0.7;
  return result;
}

auto SelfPerception::model() const noexcept -> nlohmann::json { return self_model_; }

// ═══════════════════════════════════════════════════════════════════
// 沙盘模拟
// ═══════════════════════════════════════════════════════════════════

auto SimulationChamber::run(const std::string& scenario) noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["scenario"] = scenario;
  j["simulated_ae"] = 0.3 + static_cast<double>(std::rand() % 50) / 100.0;
  j["simulated_we"] = 0.2 + static_cast<double>(std::rand() % 40) / 100.0;
  j["confidence"]   = 0.6;
  simulations_.push_back(j);
  return j;
}

auto SimulationChamber::history() const noexcept -> std::vector<nlohmann::json> { return simulations_; }

// ═══════════════════════════════════════════════════════════════════
// 突触网络
// ═══════════════════════════════════════════════════════════════════

SynapseNetwork::SynapseNetwork() noexcept {
  connect("perception", "belief", 0.8);
  connect("belief", "decision", 0.7);
  connect("decision", "action", 0.9);
  connect("action", "perception", 0.3);
}

void SynapseNetwork::connect(const std::string& from, const std::string& to, double w) noexcept {
  edges_.push_back({from, to, w});
}

auto SynapseNetwork::propagate(const std::string& node, double signal) noexcept
    -> std::vector<std::pair<std::string, double>> {
  std::vector<std::pair<std::string, double>> results;
  for (const auto& e : edges_) {
    if (e.from == node) results.emplace_back(e.to, signal * e.weight);
  }
  return results;
}

auto SynapseNetwork::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  auto arr = nlohmann::json::array();
  for (const auto& e : edges_) {
    auto ej = nlohmann::json::object();
    ej["from"] = e.from; ej["to"] = e.to; ej["weight"] = e.weight;
    arr.push_back(ej);
  }
  j["edges"] = arr; return j;
}

// ═══════════════════════════════════════════════════════════════════
// 自主层
// ═══════════════════════════════════════════════════════════════════

auto AutonomyLayer::decide(const nlohmann::json& context) noexcept -> std::string {
  double ae = context.value("ae", 0.0);
  if (ae > 0.7) return "autonomous";
  if (ae > 0.4) return "guided";
  return "supervised";
}

auto AutonomyLayer::autonomy_level() const noexcept -> double { return 0.65; }

// ═══════════════════════════════════════════════════════════════════
// 元认知
// ═══════════════════════════════════════════════════════════════════

auto MetaCognition::analyze(const nlohmann::json& state) noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["cognitive_load"] = state.value("ae", 0.5);
  j["requires_revision"] = state.value("ae", 0) < 0.2;
  j["meta_awareness"] = 0.6;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 世界模型
// ═══════════════════════════════════════════════════════════════════

void WorldModel::update(const std::string& observation) noexcept {
  observations_.push_back(observation);
  if (observations_.size() > 100) observations_.erase(observations_.begin());
}

auto WorldModel::predict(const std::string& action) noexcept -> std::string {
  if (observations_.empty()) return "unknown";
  return "predicted_" + action;
}

auto WorldModel::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["observations"] = static_cast<int>(observations_.size());
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 裂变容器
// ═══════════════════════════════════════════════════════════════════

auto FissionVessel::split(const std::string& topic) noexcept -> std::vector<std::string> {
  return {topic + "_A", topic + "_B", topic + "_C"};
}

// ═══════════════════════════════════════════════════════════════════
// 意图调度
// ═══════════════════════════════════════════════════════════════════

auto IntentDispatch::dispatch(const std::string& intent) noexcept -> std::string {
  dispatched_++;
  return "routed:" + intent;
}

auto IntentDispatch::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object(); j["dispatched"] = dispatched_; return j;
}

// ═══════════════════════════════════════════════════════════════════
// 张量桥接
// ═══════════════════════════════════════════════════════════════════

auto TensorBridge::project(const std::vector<double>& vector, int target_dim) noexcept
    -> std::vector<double> {
  std::vector<double> result(target_dim, 0);
  for (int i = 0; i < std::min(static_cast<int>(vector.size()), target_dim); ++i)
    result[i] = vector[i];
  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 混合推理器
// ═══════════════════════════════════════════════════════════════════

auto HybridReasoner::reason(const std::string& query, bool use_cloud) noexcept -> std::string {
  std::ostringstream os;
  os << "hybrid_reason(" << query.substr(0, 32) << ")";
  if (use_cloud) os << "+cloud";
  return os.str();
}

// ═══════════════════════════════════════════════════════════════════
// 三系统桥接
// ═══════════════════════════════════════════════════════════════════

auto SystemBridge::bridge(System from, System to, const nlohmann::json& data) noexcept
    -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["from"] = static_cast<int>(from);
  j["to"]   = static_cast<int>(to);
  j["data"] = data;
  j["bridged"] = true;
  return j;
}

}  // namespace nexus::quantum
