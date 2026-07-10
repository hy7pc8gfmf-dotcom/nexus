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
 * @file algo_quantum.cpp
 * @brief Algo×Quantum 桥接实现
 */

#include "nexus/psyche/algo_quantum.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <sstream>

namespace nexus::psyche {

auto BridgePacket::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["source"]       = source;
  j["target"]       = target;
  j["payload_type"] = payload_type;
  j["payload"]      = payload;
  j["timestamp"]    = timestamp;
  j["direction"]    = static_cast<int>(direction);
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 算法 → 量子
// ═══════════════════════════════════════════════════════════════════

auto AlgoQuantumBridge::feed_algo_result(const std::string& engine_id,
                                          const nlohmann::json& result) noexcept
    -> Status {
  BridgePacket packet;
  packet.source       = "algo:" + engine_id;
  packet.target       = "quantum";
  packet.payload_type = "algo_result";
  packet.payload      = result;
  packet.timestamp    = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  packet.direction    = BridgeDirection::kAlgoToQuantum;

  history_.push_back(packet);
  if (static_cast<int>(history_.size()) > kMaxHistory) {
    history_.erase(history_.begin());
  }

  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 量子 → 算法
// ═══════════════════════════════════════════════════════════════════

auto AlgoQuantumBridge::query_emergence() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["ae"]               = 0.0;
  j["we"]               = 0.0;
  j["quantum_subsystems"] = kQuantumSubsystems;
  j["bridge_active"]    = true;
  return j;
}

auto AlgoQuantumBridge::recommend_algo(double ae_level) const noexcept
    -> std::vector<std::string> {
  std::vector<std::string> recommended;

  if (ae_level < 0.3) {
    // 低涌现: 推荐探索性算法
    recommended = {"mcmc", "mtcs", "dual_pruning"};
  } else if (ae_level < 0.7) {
    // 中涌现: 推荐分析性算法
    recommended = {"dialectical_consensus", "infinitas_truth", "temporal_kg"};
  } else {
    // 高涌现: 推荐综合性算法
    recommended = {"dre", "multimodal_verifier", "ethical_evaluation"};
  }

  return recommended;
}

// ═══════════════════════════════════════════════════════════════════
// 统计与查询
// ═══════════════════════════════════════════════════════════════════

auto AlgoQuantumBridge::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_packets"]      = static_cast<int>(history_.size());
  j["quantum_subsystems"] = kQuantumSubsystems;
  j["observer_mapped"]    = true;

  // 按方向统计
  int algo_to_q = 0, q_to_algo = 0;
  for (const auto& p : history_) {
    if (p.direction == BridgeDirection::kAlgoToQuantum) algo_to_q++;
    else if (p.direction == BridgeDirection::kQuantumToAlgo) q_to_algo++;
  }
  j["algo_to_quantum"] = algo_to_q;
  j["quantum_to_algo"] = q_to_algo;
  return j;
}

auto AlgoQuantumBridge::recent_packets(int n) const noexcept
    -> std::vector<BridgePacket> {
  if (history_.empty()) return {};
  int start = std::max(0, static_cast<int>(history_.size()) - n);
  return std::vector<BridgePacket>(history_.begin() + start, history_.end());
}

}  // namespace nexus::psyche
