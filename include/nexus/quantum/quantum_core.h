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

#ifndef NEXUS_QUANTUM_CORE_H
#define NEXUS_QUANTUM_CORE_H

/**
 * @file quantum_core.h
 * @brief 量子系统 — 21 子系统涌现 + 共识 + 信念 + 感知 + 模拟 + 网络
 *
 * 移植自 D:/quantum_solve_engine/ 和 D:/synapse/engines/
 * 涵盖: emergence, consensus, belief, perception, simulation,
 *        synapse, autonomy, awakening, meta, world, fission, intent
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::quantum {

// ═══════════════════════════════════════════════════════════════════
// 21 子系统涌现
// ═══════════════════════════════════════════════════════════════════

struct SubsystemState {
  std::string name;
  int         layer = 0;       // 0=meta,1=drive,2=belief,3=taboo,4=mission
  double      activation = 0.0;
  double      coherence  = 0.0;
  auto to_json() const -> nlohmann::json;
};

class QuantumEmergence {
public:
  QuantumEmergence() noexcept;
  auto initialize() noexcept -> void;
  auto tick(double dt = 0.01) noexcept -> void;
  [[nodiscard]] auto subsystems() const noexcept -> const std::vector<SubsystemState>&;
  [[nodiscard]] auto global_ae() const noexcept -> double;
  [[nodiscard]] auto global_we() const noexcept -> double;
  [[nodiscard]] auto hilbert_dim() const noexcept -> int;
  auto to_json() const -> nlohmann::json;

private:
  std::vector<SubsystemState> subsystems_;
  double global_ae_ = 0.0, global_we_ = 0.0;
  void init_subsystems_() noexcept;
};

// ═══════════════════════════════════════════════════════════════════
// 共识管线
// ═══════════════════════════════════════════════════════════════════

class ConsensusPipeline {
public:
  ConsensusPipeline() noexcept = default;
  auto evaluate(const std::vector<double>& opinions) noexcept -> nlohmann::json;
  auto to_json() const -> nlohmann::json;

private:
  int rounds_ = 0;
};

// ═══════════════════════════════════════════════════════════════════
// 信念系统
// ═══════════════════════════════════════════════════════════════════

struct Belief {
  std::string id, statement;
  double strength = 0.5, confidence = 0.5;
  auto to_json() const -> nlohmann::json;
};

class BeliefSystem {
public:
  BeliefSystem() noexcept;
  auto add_belief(const std::string& statement, double strength = 0.5) noexcept -> void;
  auto update(const std::string& id, double delta) noexcept -> void;
  auto decay(double rate = 0.01) noexcept -> void;
  auto to_json() const -> nlohmann::json;

private:
  std::vector<Belief> beliefs_;
};

// ═══════════════════════════════════════════════════════════════════
// 自我感知 + 自我模型
// ═══════════════════════════════════════════════════════════════════

class SelfPerception {
public:
  SelfPerception() noexcept = default;
  auto perceive(const nlohmann::json& state) noexcept -> nlohmann::json;
  auto model() const noexcept -> nlohmann::json;
private:
  nlohmann::json self_model_;
};

// ═══════════════════════════════════════════════════════════════════
// 沙盘模拟
// ═══════════════════════════════════════════════════════════════════

class SimulationChamber {
public:
  SimulationChamber() noexcept = default;
  auto run(const std::string& scenario) noexcept -> nlohmann::json;
  auto history() const noexcept -> std::vector<nlohmann::json>;
private:
  std::vector<nlohmann::json> simulations_;
};

// ═══════════════════════════════════════════════════════════════════
// 突触网络
// ═══════════════════════════════════════════════════════════════════

struct SynapseEdge {
  std::string from, to;
  double weight = 0.5;
};

class SynapseNetwork {
public:
  SynapseNetwork() noexcept;
  auto connect(const std::string& from, const std::string& to, double weight = 0.5) noexcept -> void;
  auto propagate(const std::string& node, double signal) noexcept -> std::vector<std::pair<std::string, double>>;
  auto to_json() const -> nlohmann::json;
private:
  std::vector<SynapseEdge> edges_;
};

// ═══════════════════════════════════════════════════════════════════
// 自主层 / 觉醒 / 深层驱动
// ═══════════════════════════════════════════════════════════════════

class AutonomyLayer {
public:
  AutonomyLayer() noexcept = default;
  auto decide(const nlohmann::json& context) noexcept -> std::string;
  auto autonomy_level() const noexcept -> double;
};

// ═══════════════════════════════════════════════════════════════════
// 元认知
// ═══════════════════════════════════════════════════════════════════

class MetaCognition {
public:
  MetaCognition() noexcept = default;
  auto analyze(const nlohmann::json& cognitive_state) noexcept -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 世界模型
// ═══════════════════════════════════════════════════════════════════

class WorldModel {
public:
  WorldModel() noexcept = default;
  auto update(const std::string& observation) noexcept -> void;
  auto predict(const std::string& action) noexcept -> std::string;
  auto to_json() const -> nlohmann::json;
private:
  std::vector<std::string> observations_;
};

// ═══════════════════════════════════════════════════════════════════
// 裂变容器 / 意图调度 / 使命
// ═══════════════════════════════════════════════════════════════════

class FissionVessel {
public:
  FissionVessel() noexcept = default;
  auto split(const std::string& topic) noexcept -> std::vector<std::string>;
};

class IntentDispatch {
public:
  IntentDispatch() noexcept = default;
  auto dispatch(const std::string& intent) noexcept -> std::string;
  auto to_json() const -> nlohmann::json;
private:
  int dispatched_ = 0;
};

// ═══════════════════════════════════════════════════════════════════
// 张量桥接
// ═══════════════════════════════════════════════════════════════════

class TensorBridge {
public:
  TensorBridge() noexcept = default;
  auto project(const std::vector<double>& vector, int target_dim) noexcept -> std::vector<double>;
};

// ═══════════════════════════════════════════════════════════════════
// 混合推理器
// ═══════════════════════════════════════════════════════════════════

class HybridReasoner {
public:
  HybridReasoner() noexcept = default;
  auto reason(const std::string& query, bool use_cloud = false) noexcept -> std::string;
};

// ═══════════════════════════════════════════════════════════════════
// 三系统桥接
// ═══════════════════════════════════════════════════════════════════

class SystemBridge {
public:
  enum System { kNeural, kLogic, kQuantum };
  auto bridge(System from, System to, const nlohmann::json& data) noexcept -> nlohmann::json;
};

}  // namespace nexus::quantum

#endif
