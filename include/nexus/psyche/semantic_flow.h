// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_FLOW_H
#define NEXUS_PSYCHE_SEMANTIC_FLOW_H

/**
 * @file semantic_flow.h
 * @brief 语义流句子生成器 — 主谓宾骨架 + 涌现加权选词
 *
 * 移植自 D:/synapse/semantic_flow.py (574行)
 *
 * 基于语义团 + 黏菌管道 + 涌现剖面的中文句子生成器。
 * 50+ 句法模板覆盖 15 种句式类型。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nexus/psyche/semantic_cluster.h"
#include "nexus/psyche/semantic_slime.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 语义流生成器
// ═══════════════════════════════════════════════════════════════════

class SemanticFlow {
public:
  /// 构造: 需要团定义 + 黏菌管道 + 涌现剖面
  SemanticFlow(
      const std::vector<SemanticClusterDef>& clusters,
      const std::unordered_map<std::string, double>& pipe_thickness,
      const nlohmann::json& emergence_profile) noexcept;

  /// 生成一个句子 (seed 控制随机性)
  auto generate_sentence(int seed = 0) -> std::string;

  /// 生成段落
  auto generate(int paragraphs = 3, int sentences_per = 2, int seed = 42)
      -> std::string;

  /// 批量生成
  auto batch(int n = 3, int paragraphs = 2, int sentences_per = 2)
      -> std::vector<std::string>;

  /// 统计
  auto stats() const noexcept -> nlohmann::json;

private:
  std::vector<SemanticClusterDef> clusters_;
  std::unordered_map<std::string, double> pipe_thickness_;
  nlohmann::json emergence_;

  // 团名 → 成员集合 (预计算加速)
  std::unordered_map<std::string, std::vector<std::string>> cluster_members_;

  void build_member_index_() noexcept;

  /// 检查字符串是否为已知角色名
  static bool is_role_slot_(const std::string& token) noexcept;

  /// 检查字符串是否为连接词
  static bool is_connector_(const std::string& token) noexcept;

  /// 按角色取词池
  auto role_pool_(const std::string& role,
                   const std::string* prev_word,
                   const std::unordered_set<std::string>& used_words,
                   std::mt19937_64& rng) const -> std::string;

  /// 涌现加权选模板
  auto pick_template_(std::mt19937_64& rng) const
      -> std::vector<const char*>;

  /// 展开时态标记
  static auto expand_aspect_(const std::vector<const char*>& pattern,
                              std::mt19937_64& rng)
      -> std::vector<const char*>;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_FLOW_H
