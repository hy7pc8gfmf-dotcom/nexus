// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_SLIME_H
#define NEXUS_PSYCHE_SEMANTIC_SLIME_H

/**
 * @file semantic_slime.h
 * @brief 语义黏菌网络 — 自演化连接拓扑
 *
 * 移植自 D:/synapse/semantic_slime.py (520行)
 *
 * 模拟黏菌生长: 概念间管道厚度随使用频率变化,
 * 低使用管道逐渐萎缩 (prune), 高频管道增厚 (thicken)
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>

#include "nexus/psyche/semantic_port.h"
#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 黏菌管道
// ═══════════════════════════════════════════════════════════════════

struct SlimePipe {
  int    from;      // 概念索引
  int    to;         // 概念索引
  double thickness = 0.1;  // 管道厚度
  double nutrient  = 0.0;  // 营养值
  int    age       = 0;    // 生存周期

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 黏菌网络
// ═══════════════════════════════════════════════════════════════════

class SemanticSlime {
public:
  explicit SemanticSlime(SemanticPort* port = nullptr) noexcept;

  void set_port(SemanticPort* port) noexcept { port_ = port; }

  /// 构建初始网络 (k近邻连接)
  auto build_initial_network(int k_nearest = 5) noexcept -> Status;

  /// 演化一步
  auto evolve(double dt = 1.0) noexcept -> void;

  /// 获取某个概念的管道
  auto pipes_of(const std::string& cname) const noexcept
      -> std::vector<SlimePipe>;

  /// 统计
  auto stats() const noexcept -> nlohmann::json;

private:
  SemanticPort* port_ = nullptr;
  std::vector<SlimePipe> pipes_;
  bool built_ = false;

  // 概念名 → 索引
  std::unordered_map<std::string, int> concept_idx_;
  std::vector<std::string> idx_concept_;

  int get_or_create_idx_(const std::string& name);
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_SLIME_H
