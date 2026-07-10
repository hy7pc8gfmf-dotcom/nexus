// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_CORE_PROOF_CACHE_H
#define NEXUS_CORE_PROOF_CACHE_H

/**
 * @file proof_cache.h
 * @brief 构造性证明库索引缓存
 *
 * 加载 index_proofs.py 输出的 .proof_index.json,
 * 提供定理搜索和种子注入功能。
 *
 * 用法:
 *   ProofCache cache;
 *   cache.load("D:/nexus/binary/.proof_index.json");
 *   auto theorems = cache.search("induction");
 *   cache.inject_to_seed_bank(&seed_bank);
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/bridge/seed_bank.h"
#include "nexus/types/status.h"

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// 定理条目
// ═══════════════════════════════════════════════════════════════════

struct TheoremEntry {
  std::string name;
  std::string kind;       // Theorem | Lemma | Corollary | Definition | etc.
  std::string module;
  std::string file;
  bool        constructive = true;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 证明库索引缓存
// ═══════════════════════════════════════════════════════════════════

class ProofCache {
public:
  ProofCache() noexcept = default;

  /// 加载索引 JSON
  auto load(const std::string& path) noexcept -> Status;

  /// 按名称搜索定理
  auto search(const std::string& keyword, int limit = 50) const noexcept
      -> std::vector<TheoremEntry>;

  /// 按域搜索
  auto search_by_domain(const std::string& domain, int limit = 100) const noexcept
      -> std::vector<TheoremEntry>;

  /// 按模块搜索
  auto search_by_module(const std::string& module_name, int limit = 50) const noexcept
      -> std::vector<TheoremEntry>;

  /// 获取构造性定理总数
  [[nodiscard]] auto constructive_count() const noexcept -> int;

  /// 获取统计
  auto stats() const noexcept -> nlohmann::json;

  /// 注入构造性定理到种子库
  auto inject_to_seed_bank(bridge::SeedBank* bank, int limit = 5000) noexcept -> int;

private:
  std::vector<TheoremEntry> theorems_;
  std::vector<TheoremEntry> classical_theorems_;
  int total_files_ = 0;
  int constructive_files_ = 0;
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_PROOF_CACHE_H
