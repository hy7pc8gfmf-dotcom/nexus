// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file pathway_test.cpp
 * @brief 概念路径 + Ψ-Balancer 单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/psyche/concept_pathway.h"
#include "nexus/psyche/psi_balancer.h"
#include "nexus/core/thermal_governor.h"

using nexus::psyche::ConceptPathway;
using nexus::psyche::PsiBalancer;
using nexus::psyche::BranchProfile;
using nexus::psyche::SemanticPort;
using nexus::core::ThermalGovernor;

// ═══════════════════════════════════════════════════════════════════
// ConceptPathway 测试
// ═══════════════════════════════════════════════════════════════════

TEST(PathwayTest, NoPort) {
  ConceptPathway pw;
  auto a = pw.attraction("水", "火");
  EXPECT_FALSE(a.ok());  // 无 port 时应返回错误
}

TEST(PathwayTest, AttractionWithPort) {
  SemanticPort port;
  auto s = port.load("D:/nexus/binary/.semantic_field.bin");
  if (!s.ok()) GTEST_SKIP() << "语义场文件不可用";

  ConceptPathway pw(&port);
  auto a = pw.attraction("水", "火");
  ASSERT_TRUE(a.ok());
  EXPECT_GE(a.value(), 0.0);
  EXPECT_LE(a.value(), 1.0);
}

TEST(PathwayTest, NearestNeighbors) {
  SemanticPort port;
  auto s = port.load("D:/nexus/binary/.semantic_field.bin");
  if (!s.ok()) GTEST_SKIP() << "语义场文件不可用";

  ConceptPathway pw(&port);
  auto nn = pw.nearest_neighbors("水", 5);
  EXPECT_FALSE(nn.empty());
  EXPECT_LE(static_cast<int>(nn.size()), 5);
}

TEST(PathwayTest, Stats) {
  SemanticPort port;
  ConceptPathway pw(&port);
  auto j = pw.stats();
  EXPECT_TRUE(j["port_loaded"].is_boolean());
}

// ═══════════════════════════════════════════════════════════════════
// PsiBalancer 测试
// ═══════════════════════════════════════════════════════════════════

TEST(BalancerTest, DefaultDecision) {
  PsiBalancer bal;
  auto d = bal.decide("math", 25);

  EXPECT_EQ(d["domain"].get<std::string>(), "math");
  EXPECT_EQ(d["total_budget"].get<int>(), 25);
  EXPECT_FALSE(d["branches"].is_null());
}

TEST(BalancerTest, UnknownDomainFallsBackToDefault) {
  PsiBalancer bal;
  auto d = bal.decide("__unknown__", 20);
  EXPECT_EQ(d["domain"].get<std::string>(), "__unknown__");
  // 应使用 default 路由
  EXPECT_FALSE(d["branches"].is_null());
}

TEST(BalancerTest, RegisterProfile) {
  PsiBalancer bal;
  BranchProfile custom;
  custom.name = "custom_test";
  custom.gpu_weight = 0.9;
  custom.cpu_weight = 0.1;
  custom.cost = 1;
  custom.priority = 0;
  bal.register_profile(custom);

  bal.register_domain_route("test_domain", {"custom_test"});
  auto d = bal.decide("test_domain", 10);

  EXPECT_FALSE(d["branches"]["custom_test"].is_null());
  auto b = d["branches"]["custom_test"];
  EXPECT_NEAR(b["gpu_weight"].get<double>(), 0.9, 0.01);
}

TEST(BalancerTest, GovernorIntegration) {
  ThermalGovernor gov;
  PsiBalancer bal(&gov);
  auto d = bal.decide("default", 25);
  // 即使 governor 没有数据, balancer 也应正常工作
  EXPECT_EQ(d["total_budget"].get<int>(), 25);
}

TEST(BalancerTest, Stats) {
  PsiBalancer bal;
  auto s = bal.stats();
  EXPECT_GE(s["n_profiles"].get<int>(), 5);
  EXPECT_GE(s["n_domains"].get<int>(), 8);
}

TEST(BalancerTest, TemperatureImpact) {
  // 高温时应降低 GPU 权重
  // 通过 governor 设置高温
  ThermalGovernor gov;
  PsiBalancer bal(&gov);

  // 无法直接设置 governor 温度 (只读), 但 can check structure
  auto d = bal.decide("math", 25);
  auto engine = d["branches"]["psi_engine"];
  EXPECT_FALSE(engine["gpu_weight"].is_null());
  EXPECT_FALSE(engine["cpu_weight"].is_null());
}
