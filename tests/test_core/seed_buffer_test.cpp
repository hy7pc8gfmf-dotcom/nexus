// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file seed_buffer_test.cpp
 * @brief 种子缓冲区 + 语义合成器单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/bridge/seed_buffer.h"
#include "nexus/bridge/seed_bank.h"

using nexus::bridge::SeedBuffer;
using nexus::bridge::SemanticSynthesizer;
using nexus::bridge::SeedBank;

TEST(SeedBufferTest, AddAndSize) {
  SeedBuffer buf(10);
  EXPECT_EQ(buf.size(), 0);

  buf.add("test_seed", 5.0, "test", "logic", "测试种子");
  EXPECT_EQ(buf.size(), 1);
}

TEST(SeedBufferTest, FlushEmpty) {
  SeedBuffer buf;
  SeedBank bank(".nexus");
  int count = buf.flush(&bank);
  EXPECT_EQ(count, 0);
}

TEST(SeedBufferTest, FlushToBank) {
  SeedBuffer buf(10);
  buf.add("test_flush", 7.0, "test", "math", "批量写入测试");
  buf.add("test_flush_2", 3.0, "test", "logic", "第二条");

  // 需要先加载种子库才能注入
  SeedBank bank(".nexus");
  bank.load("D:/synapse/logic_solve_engine/.unified_seed_bank.json");

  int before = bank.count();
  int count = buf.flush(&bank);
  EXPECT_EQ(count, 2);
}

TEST(SeedBufferTest, Clear) {
  SeedBuffer buf(5);
  buf.add("s1", 1.0, "t", "d", "d1");
  buf.add("s2", 2.0, "t", "d", "d2");
  EXPECT_EQ(buf.size(), 2);

  buf.clear();
  EXPECT_EQ(buf.size(), 0);
}

TEST(SemanticSynthTest, DomainName) {
  auto name = SemanticSynthesizer::domain_name("logic");
  EXPECT_FALSE(name.empty());
}

TEST(SemanticSynthTest, ExtractKeyTerms) {
  auto terms = SemanticSynthesizer::extract_key_terms(
    "ZFC和PA在一致性强度上的比较", 5);
  EXPECT_FALSE(terms.empty());
}

TEST(SemanticSynthTest, Synthesize) {
  auto result = SemanticSynthesizer::synthesize(
    "seed:logic", "形式逻辑的完备性", "logic",
    "seed:meta", "元认知的自指结构", "meta", 0.85);

  EXPECT_FALSE(result.empty());
  EXPECT_TRUE(result.find("[引理]") != std::string::npos);
  EXPECT_TRUE(result.find("置信度:0.85") != std::string::npos);
}

TEST(SemanticSynthTest, SynthesizeWithTemplate) {
  // logic + meta 有模板
  auto result = SemanticSynthesizer::synthesize(
    "", "", "logic", "", "", "meta", 0.9);
  EXPECT_FALSE(result.empty());
}

TEST(SemanticSynthTest, ExtractEmpty) {
  auto terms = SemanticSynthesizer::extract_key_terms("", 5);
  EXPECT_TRUE(terms.empty());
}
