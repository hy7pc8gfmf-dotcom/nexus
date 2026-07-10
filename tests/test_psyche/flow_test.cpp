// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file flow_test.cpp
 * @brief 语义流桥 + 聚类 + 自演进触发器 单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/psyche/flow_bridge.h"
#include "nexus/psyche/semantic_cluster.h"
#include "nexus/psyche/self_evolve_trigger.h"

using nexus::psyche::FlowBridge;
using nexus::psyche::SemanticPort;
using nexus::psyche::SemanticClusterEngine;
using nexus::psyche::SemanticClusterDef;
using nexus::psyche::EmergenceMonitor;

// ═══════════════════════════════════════════════════════════════════
// FlowBridge
// ═══════════════════════════════════════════════════════════════════

TEST(FlowTest, DefineSentence) {
  FlowBridge fb;
  auto s = fb.define("entropy");
  EXPECT_FALSE(s.empty());
  EXPECT_TRUE(s.find("熵") != std::string::npos);
}

TEST(FlowTest, CausalSentence) {
  FlowBridge fb;
  auto s = fb.causal("water", "fire");
  EXPECT_FALSE(s.empty());
}

TEST(FlowTest, RelateSentence) {
  FlowBridge fb;
  auto s = fb.relate("time", "space");
  EXPECT_FALSE(s.empty());
}

TEST(FlowTest, PathToSentences) {
  FlowBridge fb;
  auto sentences = fb.path_to_sentences({"water", "fire", "energy"});
  EXPECT_GE(static_cast<int>(sentences.size()), 2);
}

TEST(FlowTest, Generate) {
  SemanticPort port;
  port.load("D:/nexus/binary/.semantic_field.bin");
  FlowBridge fb(&port);
  auto sentences = fb.generate("水", 3);
  EXPECT_FALSE(sentences.empty());
}

// ═══════════════════════════════════════════════════════════════════
// SemanticCluster
// ═══════════════════════════════════════════════════════════════════

TEST(ClusterTest, DefaultClusters) {
  SemanticClusterEngine engine;
  auto cls = engine.clusters();
  EXPECT_GE(static_cast<int>(cls.size()), 5);
}

TEST(ClusterTest, Stats) {
  SemanticClusterEngine engine;
  auto s = engine.stats();
  EXPECT_GE(s["n_clusters"].get<int>(), 5);
}

TEST(ClusterTest, RegisterCluster) {
  SemanticClusterEngine engine;
  SemanticClusterDef custom;
  custom.name = "custom_cluster";
  custom.label = "自定义";
  custom.radius = 0.3f;
  engine.register_cluster(custom);

  auto cls = engine.clusters();
  EXPECT_EQ(static_cast<int>(cls.size()), 6);
}

// ═══════════════════════════════════════════════════════════════════
// EmergenceMonitor
// ═══════════════════════════════════════════════════════════════════

TEST(MonitorTest, InitialState) {
  EmergenceMonitor mon;
  auto s = mon.snapshot();
  EXPECT_FALSE(s["stable"].get<bool>());
  EXPECT_EQ(s["total_rounds"].get<int>(), 0);
}

TEST(MonitorTest, RecordAndStability) {
  EmergenceMonitor mon;
  for (int i = 0; i < 20; ++i) {
    mon.record(0.5, nlohmann::json::object());
  }
  auto s = mon.snapshot();
  EXPECT_GT(s["total_rounds"].get<int>(), 0);
}

TEST(MonitorTest, Reset) {
  EmergenceMonitor mon;
  mon.record(0.5, nlohmann::json::object());
  mon.reset();
  auto s = mon.snapshot();
  EXPECT_EQ(s["total_rounds"].get<int>(), 0);
}

TEST(MonitorTest, StabilityCheck) {
  EmergenceMonitor mon;
  // 相同 AE 值应提高稳定性
  for (int i = 0; i < 60; ++i) {
    mon.record(0.42, nlohmann::json::object());
  }
  auto j = mon.stability_check();
  EXPECT_TRUE(j["stable"].get<bool>());
}
