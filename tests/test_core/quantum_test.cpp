// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file quantum_test.cpp
 * @brief 量子涌现系统单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/quantum/quantum_core.h"

using nexus::quantum::QuantumEmergence;

TEST(QuantumTest, SubsystemCount) {
  QuantumEmergence qe;
  EXPECT_EQ(static_cast<int>(qe.subsystems().size()), 21);
}

TEST(QuantumTest, InitialState) {
  QuantumEmergence qe;
  for (const auto& s : qe.subsystems()) {
    EXPECT_GE(s.activation, 0.0);
    EXPECT_LE(s.activation, 1.0);
    EXPECT_GE(s.coherence, 0.0);
    EXPECT_LE(s.coherence, 1.0);
  }
}

TEST(QuantumTest, TickStability) {
  QuantumEmergence qe;
  for (int i = 0; i < 100; ++i) qe.tick(0.05);

  // After 100 ticks, values should still be in valid range
  for (const auto& s : qe.subsystems()) {
    EXPECT_GE(s.activation, 0.0);
    EXPECT_LE(s.activation, 1.0);
  }
}

TEST(QuantumTest, GlobalAE_WE) {
  QuantumEmergence qe;
  // Run enough ticks for stabilization
  for (int i = 0; i < 50; ++i) qe.tick(0.02);

  double ae = qe.global_ae();
  double we = qe.global_we();
  EXPECT_GE(ae, 0.0);
  EXPECT_LE(ae, 1.0);
  EXPECT_GE(we, 0.0);
  EXPECT_LE(we, 1.0);
}

TEST(QuantumTest, ToJson) {
  QuantumEmergence qe;
  for (int i = 0; i < 10; ++i) qe.tick(0.01);
  auto j = qe.to_json();
  EXPECT_GE(j.value("global_ae", -1.0), 0.0);
  EXPECT_GE(j.value("global_we", -1.0), 0.0);
  EXPECT_TRUE(j["subsystems"].is_array());
  EXPECT_EQ(j["subsystems"].size(), 21);
}

TEST(QuantumTest, HilbertDim) {
  QuantumEmergence qe;
  EXPECT_EQ(qe.hilbert_dim(), 1 << 21);  // 2^21 = 2097152
}
