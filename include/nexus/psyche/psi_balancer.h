// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_PSI_BALANCER_H
#define NEXUS_PSYCHE_PSI_BALANCER_H

/**
 * @file psi_balancer.h
 * @brief Ψ-Balancer — GPU/CPU 动态负载均衡
 *
 * 移植自 D:/synapse/engines/psi_guard.py 中的 PsiBalancer
 *
 * 按 GPU 温度 + 问题域 动态分配推理分支计算资源
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/core/thermal_governor.h"
#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 分支配置
// ═══════════════════════════════════════════════════════════════════

struct BranchProfile {
  std::string name;
  double gpu_weight;   // GPU 分配权重 [0,1]
  double cpu_weight;   // CPU 分配权重 [0,1]
  int     cost;        // 计算开销
  int     priority;    // 优先级 (0=最高)

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// Ψ-Balancer
// ═══════════════════════════════════════════════════════════════════

class PsiBalancer {
public:
  explicit PsiBalancer(core::ThermalGovernor* governor = nullptr) noexcept;

  /// 设置热敏治理引用
  void set_governor(core::ThermalGovernor* governor) noexcept { gov_ = governor; }

  /// 获取当前负载分配决策
  auto decide(const std::string& domain = "default",
              int total_budget = 25) const noexcept -> nlohmann::json;

  /// 注册自定义分支配置
  void register_profile(const BranchProfile& profile) noexcept;

  /// 注册域路由
  void register_domain_route(const std::string& domain,
                              const std::vector<std::string>& branches) noexcept;

  /// 获取统计
  auto stats() const noexcept -> nlohmann::json;

private:
  core::ThermalGovernor* gov_ = nullptr;
  std::vector<BranchProfile> profiles_;
  std::vector<std::pair<std::string, std::vector<std::string>>> domain_routes_;

  void init_defaults_() noexcept;
  auto get_profile_(const std::string& name) const noexcept -> const BranchProfile*;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_PSI_BALANCER_H
