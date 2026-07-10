// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file meta_axiom_test.cpp
 * @brief 元公理引擎单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/core/meta_axiom_engine.h"

using nexus::core::MetaAxiomEngine;
using nexus::core::KnowledgeGap;
using nexus::core::AxiomRule;

TEST(MetaAxiomTest, InitialState) {
  MetaAxiomEngine engine;
  EXPECT_TRUE(engine.is_loaded());
  auto s = engine.stats();
  EXPECT_GE(s["builtin_rules"].get<int>(), 8);
  EXPECT_GE(s["total"].get<int>(), 8);
}

TEST(MetaAxiomTest, DetectContradictionGap) {
  MetaAxiomEngine engine;
  std::vector<nlohmann::json> steps;
  auto step = nlohmann::json::object();
  step["content"] = "结论 A 成立, 但是结论 B 也成立 然而矛盾";
  step["confidence"] = 0.8;
  step["type"] = "advance";
  step["domain"] = "logic";
  steps.push_back(step);

  auto gaps = engine.detect_gaps(steps);
  EXPECT_FALSE(gaps.empty());

  bool found_contradiction = false;
  for (const auto& g : gaps) {
    if (g.type == KnowledgeGap::Type::kContradiction) {
      found_contradiction = true;
      break;
    }
  }
  EXPECT_TRUE(found_contradiction);
}

TEST(MetaAxiomTest, DetectUncertaintyGap) {
  MetaAxiomEngine engine;
  std::vector<nlohmann::json> steps;
  auto step = nlohmann::json::object();
  step["content"] = "不确定的推理结果";
  step["confidence"] = 0.15;
  step["type"] = "advance";
  step["domain"] = "math";
  steps.push_back(step);

  auto gaps = engine.detect_gaps(steps);
  bool found_uncertainty = false;
  for (const auto& g : gaps) {
    if (g.type == KnowledgeGap::Type::kUncertainty) {
      found_uncertainty = true;
      break;
    }
  }
  EXPECT_TRUE(found_uncertainty);
}

TEST(MetaAxiomTest, MatchRulesForContradiction) {
  MetaAxiomEngine engine;
  KnowledgeGap gap;
  gap.type = KnowledgeGap::Type::kContradiction;
  gap.description = "P 和 not-P 同时出现";
  gap.severity = 0.8;
  gap.domain = "logic";

  auto rules = engine.match_rules(gap);
  EXPECT_FALSE(rules.empty());

  // 应匹配 contradiction_avoid (B-04) 或 duality 规则
  bool has_relevant = false;
  for (const auto& r : rules) {
    if (r.id == "B-04" || r.id == "COQ-CON-01") {
      has_relevant = true;
      break;
    }
  }
  EXPECT_TRUE(has_relevant);
}

TEST(MetaAxiomTest, GapToJson) {
  KnowledgeGap gap;
  gap.type = KnowledgeGap::Type::kMissingPremise;
  gap.description = "缺少前提 P";
  gap.severity = 0.6;
  gap.domain = "math";
  gap.related_terms = {"P", "Q"};

  auto j = gap.to_json();
  EXPECT_EQ(j["type"].get<std::string>(), "missing_premise");
  EXPECT_EQ(j["description"].get<std::string>(), "缺少前提 P");
  EXPECT_NEAR(j["severity"].get<double>(), 0.6, 0.01);
}

TEST(MetaAxiomTest, NoCoqDLL) {
  MetaAxiomEngine engine;
  // 不加载 DLL 也能用内建规则
  EXPECT_TRUE(engine.is_loaded());

  auto s = engine.stats();
  EXPECT_EQ(s["coq_rules"].get<int>(), 0);
  EXPECT_FALSE(s["coq_loaded"].get<bool>());
}

TEST(MetaAxiomTest, RegisterBuiltin) {
  MetaAxiomEngine engine;
  AxiomRule custom;
  custom.id = "CUSTOM-01";
  custom.name = "custom_rule";
  custom.description = "用户自定义规则";
  custom.domains = {"test"};
  custom.min_intensity = 2;

  engine.register_builtin(custom);
  auto s = engine.stats();
  EXPECT_GE(s["builtin_rules"].get<int>(), 9);
}
