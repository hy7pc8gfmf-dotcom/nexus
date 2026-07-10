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

#ifndef NEXUS_ALGO_ENGINES_BIMCMC_H
#define NEXUS_ALGO_ENGINES_BIMCMC_H

#include "nexus/algo/engine.h"
#include <cmath>
#include <random>

namespace nexus::algo::engines {

/// 双向 MCMC — 正向+反向链收敛评估
class BiMcmcEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return {"bimcmc_dual", "Bidirectional MCMC", "1.0.0",
            "双向 MCMC: 正向链 + 反向链收敛评估", {"sampling", "mcmc", "convergence"}};
  }
  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true; return Status::Ok();
  }
  auto execute(const nlohmann::json& input) noexcept -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    double target = input.value("target", 0.0);
    int n = input.value("n_samples", 500);
    std::mt19937_64 rng(42);
    std::normal_distribution<double> proposal(0, 0.5);

    // 正向链
    std::vector<double> forward;
    double f_current = target + 1.0;
    for (int i = 0; i < n; ++i) {
      double next = f_current + proposal(rng);
      double acceptance = std::exp(-0.5 * std::pow((next - target) / 0.5, 2)
                                 + 0.5 * std::pow((f_current - target) / 0.5, 2));
      if (std::uniform_real_distribution<double>(0, 1)(rng) < std::min(1.0, acceptance))
        f_current = next;
      forward.push_back(f_current);
    }

    // 反向链 (从另一端开始)
    std::vector<double> backward;
    double b_current = target - 1.0;
    for (int i = 0; i < n; ++i) {
      double next = b_current + proposal(rng);
      double acceptance = std::exp(-0.5 * std::pow((next - target) / 0.5, 2)
                                 + 0.5 * std::pow((b_current - target) / 0.5, 2));
      if (std::uniform_real_distribution<double>(0, 1)(rng) < std::min(1.0, acceptance))
        b_current = next;
      backward.push_back(b_current);
    }

    // 收敛评估
    double f_mean = 0, b_mean = 0;
    for (auto v : forward) f_mean += v;
    for (auto v : backward) b_mean += v;
    f_mean /= n; b_mean /= n;
    double convergence = 1.0 - std::abs(f_mean - b_mean) / 2.0;

    auto result = nlohmann::json::object();
    result["status"] = "ok"; result["forward_mean"] = f_mean;
    result["backward_mean"] = b_mean; result["convergence"] = convergence;
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
