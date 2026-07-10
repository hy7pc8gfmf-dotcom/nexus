// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/psi_balancer.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto BranchProfile::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]       = name;
  j["gpu_weight"] = std::round(gpu_weight * 100) / 100;
  j["cpu_weight"] = std::round(cpu_weight * 100) / 100;
  j["cost"]       = cost;
  j["priority"]   = priority;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

PsiBalancer::PsiBalancer(core::ThermalGovernor* governor) noexcept
  : gov_(governor) {
  init_defaults_();
}

// ═══════════════════════════════════════════════════════════════════
// 默认配置
// ═══════════════════════════════════════════════════════════════════

void PsiBalancer::init_defaults_() noexcept {
  profiles_ = {
    {"psi_engine",   0.7, 0.3, 5, 1},
    {"arbiter",      0.3, 0.7, 4, 2},
    {"tri_agi",      0.2, 0.8, 3, 3},
    {"psi_guard",    0.5, 0.5, 6, 0},
    {"cross_domain", 0.6, 0.4, 7, 4},
  };

  domain_routes_ = {
    {"math",    {"psi_engine", "psi_guard", "arbiter"}},
    {"logic",   {"psi_engine", "psi_guard", "arbiter"}},
    {"game",    {"tri_agi", "arbiter", "cross_domain"}},
    {"meta",    {"tri_agi", "psi_guard", "cross_domain"}},
    {"crypto",  {"psi_engine", "arbiter", "cross_domain"}},
    {"rl",      {"psi_engine", "tri_agi", "cross_domain"}},
    {"causal",  {"psi_engine", "psi_guard", "tri_agi"}},
    {"default", {"psi_engine", "arbiter", "tri_agi", "psi_guard", "cross_domain"}},
  };
}

// ═══════════════════════════════════════════════════════════════════
// 辅助
// ═══════════════════════════════════════════════════════════════════

auto PsiBalancer::get_profile_(const std::string& name) const noexcept
    -> const BranchProfile* {
  for (const auto& p : profiles_) {
    if (p.name == name) return &p;
  }
  return nullptr;
}

void PsiBalancer::register_profile(const BranchProfile& profile) noexcept {
  // 替换已存在的或追加
  for (auto& p : profiles_) {
    if (p.name == profile.name) { p = profile; return; }
  }
  profiles_.push_back(profile);
}

void PsiBalancer::register_domain_route(
    const std::string& domain,
    const std::vector<std::string>& branches) noexcept {
  for (auto& [d, b] : domain_routes_) {
    if (d == domain) { b = branches; return; }
  }
  domain_routes_.emplace_back(domain, branches);
}

// ═══════════════════════════════════════════════════════════════════
// 决策
// ═══════════════════════════════════════════════════════════════════

auto PsiBalancer::decide(const std::string& domain,
                          int total_budget) const noexcept -> nlohmann::json {
  // 查找域路由
  std::vector<std::string> branches;
  for (const auto& [d, b] : domain_routes_) {
    if (d == domain) { branches = b; break; }
  }
  if (branches.empty()) {
    for (const auto& [d, b] : domain_routes_) {
      if (d == "default") { branches = b; break; }
    }
  }

  // 获取 GPU 温度
  double gpu_temp = 65.0;
  if (gov_) {
    auto s = gov_->status();
    gpu_temp = s.gpu_temp;
    if (gpu_temp < 1.0) gpu_temp = 65.0;  // 无传感器时默认
  }

  // 分配
  auto result = nlohmann::json::object();
  result["domain"] = domain;
  result["gpu_temp"] = gpu_temp;
  result["total_budget"] = total_budget;

  auto branches_arr = nlohmann::json::object();
  for (const auto& br : branches) {
    const auto* profile = get_profile_(br);
    if (!profile) continue;

    double gpu = profile->gpu_weight;
    // 温度修正
    if (gpu_temp > 80) {
      gpu *= std::max(0.1, 1.0 - (gpu_temp - 80.0) / 50.0);
    }

    auto entry = nlohmann::json::object();
    entry["name"]       = profile->name;
    entry["gpu_weight"] = std::round(gpu * 100) / 100;
    entry["cpu_weight"] = std::round((1.0 - gpu) * 100) / 100;
    entry["cost"]       = profile->cost;
    entry["budget"]     = std::max(1, total_budget / static_cast<int>(branches.size()));
    entry["priority"]   = profile->priority;

    branches_arr[profile->name] = entry;
  }
  result["branches"] = branches_arr;

  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 统计
// ═══════════════════════════════════════════════════════════════════

auto PsiBalancer::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["n_profiles"] = static_cast<int>(profiles_.size());
  j["n_domains"]  = static_cast<int>(domain_routes_.size());
  return j;
}

}  // namespace nexus::psyche
