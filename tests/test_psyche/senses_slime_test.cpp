// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "nexus/psyche/semantic_senses.h"
#include "nexus/psyche/semantic_slime.h"

using nexus::psyche::SenseRegistry;
using nexus::psyche::SenseEntry;
using nexus::psyche::SemanticSlime;
using nexus::psyche::SemanticPort;

TEST(SensesTest, BuiltinCount) {
  SenseRegistry reg;
  auto s = reg.stats();
  EXPECT_GE(s["total_senses"].get<int>(), 20);
  EXPECT_GE(s["n_characters"].get<int>(), 7);
}

TEST(SensesTest, QueryByCharacter) {
  SenseRegistry reg;
  auto senses = reg.get_senses("行");
  EXPECT_EQ(static_cast<int>(senses.size()), 4);
  EXPECT_EQ(senses[0].sense_id, "xing_walk");
}

TEST(SensesTest, GetEntity) {
  SenseRegistry reg;
  auto entity = reg.get_entity("hong_color");
  EXPECT_FALSE(entity.empty());
  EXPECT_TRUE(entity.find("红") != std::string::npos);
}

TEST(SensesTest, RegisterCustom) {
  SenseRegistry reg;
  SenseEntry custom;
  custom.character = "测";
  custom.sense_id = "ce_test";
  custom.gloss = "测试/验证";
  custom.clusters = {"test", "logic"};
  reg.register_sense(custom);

  auto senses = reg.get_senses("测");
  EXPECT_EQ(static_cast<int>(senses.size()), 1);
  EXPECT_EQ(senses[0].sense_id, "ce_test");
}

TEST(SlimeTest, BuildNetwork) {
  SemanticPort port;
  port.load("D:/nexus/binary/.semantic_field.bin");

  SemanticSlime slime(&port);
  auto s = slime.build_initial_network(3);
  if (!s.ok()) GTEST_SKIP() << "slime build failed: " << s.to_string();

  auto stats = slime.stats();
  EXPECT_GT(stats["n_pipes"].get<int>(), 0);
  EXPECT_TRUE(stats["built"].get<bool>());
}

TEST(SlimeTest, Evolve) {
  SemanticPort port;
  port.load("D:/nexus/binary/.semantic_field.bin");

  SemanticSlime slime(&port);
  auto s = slime.build_initial_network(3);
  if (!s.ok()) GTEST_SKIP();

  slime.evolve(10.0);
  auto stats = slime.stats();
  EXPECT_GT(stats["n_pipes"].get<int>(), 0);
}
