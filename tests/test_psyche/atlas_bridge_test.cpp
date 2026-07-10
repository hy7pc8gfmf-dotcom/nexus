// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include <gtest/gtest.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "nexus/psyche/semantic_atlas.h"
#include "nexus/psyche/semantic_bridge.h"

using nexus::psyche::SemanticAtlas;
using nexus::psyche::SemanticTier;
using nexus::psyche::SemanticBridge;

TEST(AtlasTest, DefaultTiers) {
  SemanticAtlas atlas;
  auto s = atlas.stats();
  EXPECT_GE(s["n_tiers"].get<int>(), 5);
}

TEST(AtlasTest, RegisterCustomTier) {
  SemanticAtlas atlas;
  SemanticTier t;
  t.name = "test_tier";
  t.weight = 0.5;
  t.tags = {"test"};
  atlas.register_tier(t);

  auto tier = atlas.concept_tier("test_concept");
  EXPECT_EQ(tier, "test_tier");
}

TEST(AtlasTest, ConceptTierDefault) {
  SemanticAtlas atlas;
  auto tier = atlas.concept_tier("__unknown__");
  EXPECT_FALSE(tier.empty());
}

TEST(AtlasTest, SaveLoad) {
  SemanticAtlas atlas;
  auto tmp = std::filesystem::temp_directory_path() / "nexus_atlas_test.json";
  auto s = atlas.save(tmp.string());
  EXPECT_TRUE(s.ok());

  SemanticAtlas atlas2;
  auto s2 = atlas2.load(tmp.string());
  EXPECT_TRUE(s2.ok());

  std::error_code ec;
  std::filesystem::remove(tmp, ec);
}

TEST(BridgeTest, DefaultProfiles) {
  SemanticBridge bridge;
  auto profiles = bridge.steer_profile();
  EXPECT_GE(static_cast<int>(profiles.size()), 8);
}

TEST(BridgeTest, SteerToSeed) {
  SemanticBridge bridge;
  auto result = bridge.steer_to_seed({0.5, 0.3, 0.0, -0.5, 1.0});
  EXPECT_EQ(static_cast<int>(result.size()), 14);
}

TEST(BridgeTest, SeedToSteer) {
  SemanticBridge bridge;
  std::vector<double> seed(14, 0.3);
  auto result = bridge.seed_to_steer(seed);
  EXPECT_EQ(static_cast<int>(result.size()), 8);
}

TEST(BridgeTest, Stats) {
  SemanticBridge bridge;
  auto s = bridge.stats();
  EXPECT_GE(s["n_profiles"].get<int>(), 8);
}
