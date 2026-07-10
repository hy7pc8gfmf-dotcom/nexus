// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_MCMC_H
#define NEXUS_ALGO_ENGINES_MCMC_H

/**
 * @file mcmc_engine.h
 * @brief 双向 MCMC 引擎 v2 — 自适应提议 + 多链 + 收敛诊断
 */

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

namespace nexus::algo::engines {

class McmcEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id = "mcmc", .name = "Bidirectional MCMC v2",
      .version = "2.0.0",
      .description = "Metropolis-Hastings + 自适应提议 + GR 诊断",
      .tags = {"sampling", "mcmc", "convergence", "adaptive"},
    };
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    if (config.is_object()) {
      n_chains_ = config.value("n_chains", 4);
      n_samples_ = config.value("n_samples", 1000);
      warmup_ = config.value("warmup", 100);
      adapt_step_ = config.value("adaptive", true);
    }
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");

    auto target_mean = input.value("mean", 0.0);
    auto target_std  = input.value("std", 1.0);
    auto n_samples  = input.value("n_samples", static_cast<int>(n_samples_));
    auto n_chains   = input.value("n_chains", static_cast<int>(n_chains_));
    auto warmup     = input.value("warmup", static_cast<int>(warmup_));

    std::mt19937_64 rng(42);

    // 多链采样
    nlohmann::json chains = nlohmann::json::array();
    std::vector<double> chain_means;
    std::vector<double> chain_vars;

    for (int c = 0; c < n_chains; ++c) {
      double step = target_std * 0.5;
      double current = target_mean + target_std * 0.5 * (c - n_chains / 2);
      std::vector<double> samples;
      int accepted = 0;

      for (int i = 0; i < n_samples; ++i) {
        std::normal_distribution<double> proposal(0.0, step);
        double next = current + proposal(rng);
        double ratio = std::exp(
            -0.5 * std::pow((next - target_mean) / target_std, 2)
            + 0.5 * std::pow((current - target_mean) / target_std, 2));
        if (std::uniform_real_distribution<>(0, 1)(rng) < std::min(1.0, ratio)) {
          current = next;
          accepted++;
        }
        if (i >= warmup) samples.push_back(current);

        // 自适应步长 (优化接受率 ~0.234)
        if (adapt_step_ && i > 100 && i % 50 == 0) {
          double ar = static_cast<double>(accepted) / (i + 1);
          step *= (ar > 0.3) ? 1.05 : (ar < 0.15) ? 0.95 : 1.0;
        }
      }

      // 链统计
      double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
      double mean = sum / samples.size();
      double var = 0;
      for (auto v : samples) var += (v - mean) * (v - mean);
      var /= samples.size();

      chain_means.push_back(mean);
      chain_vars.push_back(var);

      auto chain = nlohmann::json::object();
      chain["chain_id"] = c;
      chain["mean"] = std::round(mean * 1000) / 1000;
      chain["std"] = std::round(std::sqrt(var) * 1000) / 1000;
      chain["acceptance_rate"] = std::round(100.0 * accepted / n_samples) / 100;
      chains.push_back(chain);
    }

    // Gelman-Rubin 收敛诊断: R_hat
    double overall_mean = std::accumulate(chain_means.begin(), chain_means.end(), 0.0) / n_chains;
    double between_var = 0;
    for (auto m : chain_means) between_var += (m - overall_mean) * (m - overall_mean);
    between_var *= static_cast<double>(n_samples - warmup) / (n_chains - 1);
    double within_var = std::accumulate(chain_vars.begin(), chain_vars.end(), 0.0) / n_chains;
    double r_hat = (within_var > 0)
        ? std::sqrt((between_var / within_var + 1.0) * static_cast<double>(n_samples - warmup) / (n_samples - warmup - 1))
        : 1.0;

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["n_chains"] = n_chains;
    result["n_samples"] = n_samples;
    result["chains"] = chains;
    result["gelman_rubin_rhat"] = std::round(r_hat * 1000) / 1000;
    result["converged"] = r_hat < 1.1;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"] = initialized_;
    j["n_chains"] = n_chains_;
    j["n_samples"] = n_samples_;
    j["adaptive"] = adapt_step_;
    return j;
  }

  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
  uint32_t n_chains_ = 4;
  uint32_t n_samples_ = 1000;
  uint32_t warmup_ = 100;
  bool adapt_step_ = true;
};

}  // namespace nexus::algo::engines
#endif
