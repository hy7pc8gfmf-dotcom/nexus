// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_BRIDGE_H
#define NEXUS_PSYCHE_SEMANTIC_BRIDGE_H

/**
 * @file semantic_bridge.h
 * @brief 语义桥接 — 转向向量 ↔ 种子向量映射
 *
 * 移植自 D:/synapse/semantic_bridge.py (246行)
 *
 * 桥接 steering_tokens (88 标量) 和 seed_bank (14D 向量):
 *   steer_to_seed: 转向向量 → 种子空间投影
 *   seed_to_steer: 种子 → 转向空间投影
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/bridge/seed_bank.h"
#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 转向标量配置
// ═══════════════════════════════════════════════════════════════════

struct SteerProfile {
  std::string name;
  double      default_val = 0.0;
  double      min_val = -1.0;
  double      max_val = 1.0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 语义桥接器
// ═══════════════════════════════════════════════════════════════════

class SemanticBridge {
public:
  static constexpr int kSteerDim = 8;
  static constexpr int kSeedDim  = 14;

  SemanticBridge() noexcept;

  /// 从种子库加载桥接矩阵
  auto load(bridge::SeedBank* bank) noexcept -> Status;

  /// 转向向量 → 种子空间投影 (8D → 14D)
  auto steer_to_seed(const std::vector<double>& steer_vec) const noexcept
      -> std::vector<double>;

  /// 种子向量 → 转向空间投影 (14D → 8D)
  auto seed_to_steer(const std::vector<double>& seed_vec) const noexcept
      -> std::vector<double>;

  /// 跨系统查询 — 返回转向向量 + 相邻种子
  auto query(const std::string& section, int top_k = 5) const noexcept
      -> nlohmann::json;

  /// 空缺检测 + 补齐建议
  auto suggest_fill(const std::string& section,
                    double threshold = 0.3) const noexcept -> nlohmann::json;

  /// 查询转向标量
  auto steer_profile() const noexcept -> std::vector<SteerProfile>;

  /// 获取 8D→14D 映射表
  auto steer_to_seed_map() const noexcept -> std::vector<int>;

  /// 统计
  auto stats() const noexcept -> nlohmann::json;

private:
  bridge::SeedBank* bank_ = nullptr;
  std::vector<SteerProfile> profiles_;
  std::vector<int> steer_to_seed_map_;
  std::vector<double> weight_;
  bool loaded_ = false;

  void init_profiles_() noexcept;
  double weighted_distance_(
      const std::vector<double>& a, const std::vector<double>& b) const noexcept;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_BRIDGE_H
