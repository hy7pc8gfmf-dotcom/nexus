// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file observer_test.cpp
 * @brief 30 Observer 池单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/psyche/observer.h"

using nexus::psyche::ObserverPool;
using nexus::psyche::ObserverRole;

TEST(ObserverTest, PoolInitialization) {
  ObserverPool pool;
  EXPECT_EQ(pool.total_observers(), 30);

  auto roles = pool.count_by_role();
  EXPECT_EQ(roles["auditor"].get<int>(), 5);
  EXPECT_EQ(roles["tagger"].get<int>(), 7);
  EXPECT_EQ(roles["critic"].get<int>(), 6);
  EXPECT_EQ(roles["analyst"].get<int>(), 7);
  EXPECT_EQ(roles["synthesizer"].get<int>(), 5);
}

TEST(ObserverTest, ObserveAll) {
  ObserverPool pool;
  auto ctx = nlohmann::json::object();
  ctx["content"] = "测试推理内容";
  ctx["confidence"] = 0.8;

  auto results = pool.observe_all(ctx);
  EXPECT_EQ(static_cast<int>(results.size()), 30);

  // 验证角色类型分布
  int critics = 0, auditors = 0;
  for (const auto& obs : results) {
    if (obs.type == "warning") critics++;
    if (obs.type == "audit") auditors++;
  }
  EXPECT_EQ(critics, 6);   // 6 critics
  EXPECT_EQ(auditors, 5);  // 5 auditors
}

TEST(ObserverTest, ObserveByRole) {
  ObserverPool pool;
  auto ctx = nlohmann::json::object();
  ctx["content"] = "content with contradiction 但是 uncertainty";
  ctx["confidence"] = 0.3;

  auto critics = pool.observe_by_role(ObserverRole::kCritic, ctx);
  EXPECT_FALSE(critics.empty());
  for (const auto& c : critics) {
    EXPECT_EQ(c.type, "warning");
    EXPECT_GT(c.confidence, 0.0);
  }
}

TEST(ObserverTest, CriticContradiction) {
  ObserverPool pool;
  auto ctx = nlohmann::json::object();
  ctx["content"] = "可能成立, 但一定不成立 然而不一定";
  ctx["confidence"] = 0.5;

  auto critics = pool.observe_by_role(ObserverRole::kCritic, ctx);
  bool found_contradiction = false;
  for (const auto& a : critics) {
    if (a.content.find("矛盾") != std::string::npos) {
      found_contradiction = true;
      EXPECT_LT(a.confidence, 0.7);
    }
  }
  EXPECT_TRUE(found_contradiction);
}

TEST(ObserverTest, AnalystWordCount) {
  ObserverPool pool;
  auto ctx = nlohmann::json::object();
  ctx["content"] = "这是一个包含多个词语的长句子。用于测试分析。";
  ctx["confidence"] = 0.7;

  auto analysts = pool.observe_by_role(ObserverRole::kAnalyst, ctx);
  EXPECT_FALSE(analysts.empty());
  for (const auto& a : analysts) {
    EXPECT_EQ(a.type, "insight");
    EXPECT_TRUE(a.content.find("个词") != std::string::npos ||
                a.content.find("句") != std::string::npos);
  }
}

TEST(ObserverTest, LowConfidenceCritic) {
  ObserverPool pool;
  auto ctx = nlohmann::json::object();
  ctx["content"] = "简短";
  ctx["confidence"] = 0.1;

  auto critics = pool.observe_by_role(ObserverRole::kCritic, ctx);
  bool found_issue = false;
  for (const auto& c : critics) {
    if (c.content.find("置信度过低") != std::string::npos ||
        c.content.find("内容过短") != std::string::npos) {
      found_issue = true;
      break;
    }
  }
  EXPECT_TRUE(found_issue);
}

TEST(ObserverTest, RecentHistory) {
  ObserverPool pool;
  auto ctx = nlohmann::json::object();
  ctx["content"] = "历史记录测试";
  ctx["confidence"] = 0.5;

  pool.observe_all(ctx);
  auto recent = pool.recent_observations(5);
  EXPECT_EQ(static_cast<int>(recent.size()), 5);

  auto all_recent = pool.recent_observations(100);
  EXPECT_EQ(static_cast<int>(all_recent.size()), 30);
}

TEST(ObserverTest, ContextConfidenceImpact) {
  ObserverPool pool;

  // 高置信上下文
  auto high_ctx = nlohmann::json::object();
  high_ctx["content"] = "高置信度推理结论";
  high_ctx["confidence"] = 0.95;
  auto high_results = pool.observe_by_role(ObserverRole::kSynthesizer, high_ctx);

  // 低置信上下文
  auto low_ctx = nlohmann::json::object();
  low_ctx["content"] = "不确定";
  low_ctx["confidence"] = 0.15;
  auto low_results = pool.observe_by_role(ObserverRole::kSynthesizer, low_ctx);

  // 高置信应得到更高的综合评分
  double high_conf = 0, low_conf = 0;
  for (const auto& r : high_results) high_conf += r.confidence;
  for (const auto& r : low_results) low_conf += r.confidence;
  EXPECT_GT(high_conf, low_conf);
}
