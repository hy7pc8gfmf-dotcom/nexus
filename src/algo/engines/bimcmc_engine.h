// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_BIMCMC_H
#define NEXUS_ALGO_ENGINES_BIMCMC_H

/* 双向 MCMC v2 — Gelmman-Rubin 诊断 + 自适应步长 + 收敛判定 */

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

namespace nexus::algo::engines {

class BiMcmcEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id = "bimcmc_dual", .name = "Bidirectional MCMC v2",
      .version = "2.0.0",
      .description = "双向 MCMC: GR 诊断 + 自适应步长 + 收敛判定",
      .tags = {"sampling", "mcmc", "convergence", "bidirectional"},
    };
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    gr_threshold_ = config.value("gr_threshold", 1.1);
    adaptive_ = config.value("adaptive", true);
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    double target = input.value("target", 0.0);
    double sigma = input.value("sigma", 0.5);
    int n = input.value("n_samples", 500);

    std::mt19937_64 rng(42);

    // 多链 (4链: 2正向+2反向起始)
    struct ChainResult { double mean; double var; int accepted; };
    auto run_chain = [&](double start, double& step) -> ChainResult {
      double current = start;
      std::vector<double> samples;
      int accepted = 0;
      for (int i = 0; i < n; ++i) {
        std::normal_distribution<double> proposal(0, step);
        double next = current + proposal(rng);
        double ar = std::exp(-0.5 * std::pow((next - target) / sigma, 2)
                            + 0.5 * std::pow((current - target) / sigma, 2));
        if (std::uniform_real_distribution<>(0, 1)(rng) < std::min(1.0, ar)) {
          current = next; accepted++;
        }
        samples.push_back(current);
        if (adaptive_ && i > 50 && i % 50 == 0) {
          double rate = static_cast<double>(accepted) / (i + 1);
          step *= (rate > 0.4) ? 1.05 : (rate < 0.2) ? 0.95 : 1.0;
        }
      }
      double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
      double mean = sum / samples.size();
      double var = 0;
      for (auto v : samples) var += (v - mean) * (v - mean);
      return {mean, var / samples.size(), accepted};
    };

    double step_f = sigma * 0.5, step_b = sigma * 0.5;
    auto fwd = run_chain(target + sigma, step_f);
    auto bwd = run_chain(target - sigma, step_b);

    // 添加2条额外链增加诊断精度
    double step_f2 = sigma * 0.5, step_b2 = sigma * 0.5;
    auto fwd2 = run_chain(target + sigma * 2, step_f2);
    auto bwd2 = run_chain(target - sigma * 2, step_b2);

    std::vector<double> means = {fwd.mean, bwd.mean, fwd2.mean, bwd2.mean};
    std::vector<double> vars  = {fwd.var, bwd.var, fwd2.var, bwd2.var};

    // Gelman-Rubin R_hat
    int m = 4;
    double overall_mean = std::accumulate(means.begin(), means.end(), 0.0) / m;
    double B = 0;
    for (auto mu : means) B += (mu - overall_mean) * (mu - overall_mean);
    B *= static_cast<double>(n) / (m - 1);
    double W = std::accumulate(vars.begin(), vars.end(), 0.0) / m;
    double var_est = (1.0 - 1.0 / n) * W + B / n;
    double r_hat = (W > 0) ? std::sqrt(var_est / W) : 1.0;

    // 正反向收敛差异
    double fwd_avg = (fwd.mean + fwd2.mean) / 2;
    double bwd_avg = (bwd.mean + bwd2.mean) / 2;
    double divergence = std::abs(fwd_avg - bwd_avg);

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["gelman_rubin_rhat"] = std::round(r_hat * 1000) / 1000;
    result["forward_mean"] = std::round(fwd_avg * 1000) / 1000;
    result["backward_mean"] = std::round(bwd_avg * 1000) / 1000;
    result["divergence"] = std::round(divergence * 1000) / 1000;
    result["converged"] = r_hat < gr_threshold_;
    result["forward_acceptance"] = std::round(100.0 * fwd.accepted / n) / 100;
    result["backward_acceptance"] = std::round(100.0 * bwd.accepted / n) / 100;
    result["n_samples"] = n;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"] = initialized_;
    j["gr_threshold"] = gr_threshold_;
    j["adaptive"] = adaptive_;
    return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
  double gr_threshold_ = 1.1;
  bool adaptive_ = true;
};

} // namespace
#endif
