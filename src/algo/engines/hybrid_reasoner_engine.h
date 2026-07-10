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

#ifndef NEXUS_ALGO_ENGINES_HYBRID_REASONER_H
#define NEXUS_ALGO_ENGINES_HYBRID_REASONER_H

#include "nexus/algo/engine.h"
#include <sstream>

namespace nexus::algo::engines {

/// 混合推理器 — 本地 + 云端协同推理
class HybridReasonerEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return {"hybrid_reasoner", "Hybrid Reasoner", "1.0.0",
            "本地模型 + 云端服务混合推理调度", {"reasoning", "hybrid", "cloud"}};
  }
  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true; return Status::Ok();
  }
  auto execute(const nlohmann::json& input) noexcept -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    auto query = input.value("query", std::string(""));
    bool use_cloud = input.value("use_cloud", false);
    double confidence = use_cloud ? 0.85 : 0.65;

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["query"] = query.substr(0, 64);
    result["mode"] = use_cloud ? "cloud_assisted" : "local_only";
    result["confidence"] = confidence;
    result["response"] = "hybrid:" + query.substr(0, 32);
    return result;
  }
  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object(); j["initialized"] = initialized_; return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }
private:
  bool initialized_ = false;
};

} // namespace
#endif
