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

#ifndef NEXUS_ALGO_ENGINES_MCMC_H
#define NEXUS_ALGO_ENGINES_MCMC_H

/**
 * @file mcmc_engine.h
 * @brief 双向 MCMC 引擎 — Markov Chain Monte Carlo 推理
 *
 * 使用 Metropolis-Hastings 算法进行概率推理。
 * 用于: 种子空间采样、不确定性量化、推理路径探索。
 */

#include "nexus/algo/engine.h"

#include <random>

namespace nexus::algo::engines {

class McmcEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id          = "mcmc",
      .name        = "Bidirectional MCMC",
      .version     = "1.0.0",
      .description = "Metropolis-Hastings MCMC 采样引擎",
      .tags        = {"sampling", "uncertainty", "inference"},
    };
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    if (config.is_object()) {
      n_chains_ = config.value("n_chains", 4);
      n_samples_ = config.value("n_samples", 1000);
      warmup_ = config.value("warmup", 100);
    }
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) {
      return Status::Error(ErrorCode::kEngineFailed,
        "MCMC engine not initialized");
    }

    // Metropolis-Hastings 采样
    auto target_mean = input.value("mean", 0.0);
    auto target_std  = input.value("std", 1.0);
    auto n_samples   = input.value("n_samples", static_cast<int>(n_samples_));
    auto n_chains    = input.value("n_chains", static_cast<int>(n_chains_));

    std::mt19937_64 rng(std::random_device{}());
    std::normal_distribution<double> proposal(0.0, target_std * 0.5);

    // 运行多链采样
    nlohmann::json chains = nlohmann::json::array();
    for (int c = 0; c < n_chains; ++c) {
      double current = target_mean + target_std * 0.1 * (c - n_chains/2);
      double current_likelihood = -0.5 * std::pow((current - target_mean) / target_std, 2);

      nlohmann::json samples = nlohmann::json::array();
      for (int i = 0; i < n_samples; ++i) {
        // 提议新状态
        double proposal_val = current + proposal(rng);
        double proposal_likelihood =
          -0.5 * std::pow((proposal_val - target_mean) / target_std, 2);

        // Metropolis 接受/拒绝
        double acceptance_ratio = std::exp(proposal_likelihood - current_likelihood);
        if (std::uniform_real_distribution<double>(0, 1)(rng) < acceptance_ratio) {
          current = proposal_val;
          current_likelihood = proposal_likelihood;
        }

        auto sample = nlohmann::json::object();
        sample["step"]   = i;
        sample["value"]  = current;
        sample["accept"] = acceptance_ratio;
        if (i >= static_cast<int>(warmup_)) {
          samples.push_back(sample);
        }
      }

      auto chain = nlohmann::json::object();
      chain["chain_id"] = c;
      chain["samples"]  = samples;
      chains.push_back(chain);
    }

    auto result = nlohmann::json::object();
    result["chains"]   = chains;
    result["n_chains"] = n_chains;
    result["n_samples"]= n_samples;
    result["status"]   = "ok";

    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"] = initialized_;
    j["n_chains"]    = n_chains_;
    j["n_samples"]   = n_samples_;
    j["warmup"]      = warmup_;
    return j;
  }

  auto is_initialized() const noexcept -> bool override {
    return initialized_;
  }

private:
  bool initialized_ = false;
  uint32_t n_chains_  = 4;
  uint32_t n_samples_ = 1000;
  uint32_t warmup_    = 100;
};

}  // namespace nexus::algo::engines

#endif  // NEXUS_ALGO_ENGINES_MCMC_H
