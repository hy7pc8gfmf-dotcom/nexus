// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "nexus/core/proof_cache.h"

using nexus::core::ProofCache;
using nexus::core::TheoremEntry;

TEST(ProofCacheTest, LoadIndex) {
  ProofCache cache;
  auto s = cache.load("D:/nexus/binary/.proof_index.json");
  if (!s.ok()) GTEST_SKIP() << "proof index not available: " << s.to_string();
  EXPECT_GT(cache.constructive_count(), 50000);
}

TEST(ProofCacheTest, SearchTheorem) {
  ProofCache cache;
  auto s = cache.load("D:/nexus/binary/.proof_index.json");
  if (!s.ok()) GTEST_SKIP() << "proof index not available";

  auto results = cache.search("plus_comm", 10);
  EXPECT_FALSE(results.empty());
  for (const auto& r : results) {
    EXPECT_TRUE(r.constructive);
  }
}

TEST(ProofCacheTest, SearchByDomain) {
  ProofCache cache;
  auto s = cache.load("D:/nexus/binary/.proof_index.json");
  if (!s.ok()) GTEST_SKIP();

  // Arith 域应有大量定理
  auto results = cache.search_by_domain("Arith", 10);
  EXPECT_FALSE(results.empty());
}

TEST(ProofCacheTest, SearchByModule) {
  ProofCache cache;
  auto s = cache.load("D:/nexus/binary/.proof_index.json");
  if (!s.ok()) GTEST_SKIP();

  // 搜索 Init 模块 (使用 search 按关键词匹配)
  auto results = cache.search("Init", 10);
  EXPECT_FALSE(results.empty());
}

TEST(ProofCacheTest, Stats) {
  ProofCache cache;
  auto s = cache.load("D:/nexus/binary/.proof_index.json");
  if (!s.ok()) GTEST_SKIP();

  auto j = cache.stats();
  EXPECT_GT(j["constructive_theorems"].get<int>(), 50000);
  EXPECT_GT(j["constructive_files"].get<int>(), 3000);
}

TEST(ProofCacheTest, InjectToSeedBank) {
  ProofCache cache;
  auto s = cache.load("D:/nexus/binary/.proof_index.json");
  if (!s.ok()) GTEST_SKIP();

  nexus::bridge::SeedBank bank(".nexus");
  auto loaded = bank.load("D:/synapse/logic_solve_engine/.unified_seed_bank.json");

  int before = bank.count();
  int injected = cache.inject_to_seed_bank(&bank, 100);
  EXPECT_GE(injected, 1);
  EXPECT_GE(bank.count(), before);
}
