// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file thermal_test.cpp
 * @brief 热敏算力治理引擎单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/core/thermal_governor.h"

using nexus::core::ThermalGovernor;
using nexus::core::ThermalStatus;

TEST(ThermalTest, InitialState) {
  ThermalGovernor gov;
  auto s = gov.status();
  EXPECT_EQ(s.gpu_temp, 0.0);
  EXPECT_EQ(s.cpu_temp, 0.0);
  EXPECT_FALSE(s.throttled);
  EXPECT_FALSE(s.cooling);
}

TEST(ThermalTest, CheckDoesNotCrash) {
  ThermalGovernor gov;
  auto s = gov.check();
  // 温度读取可能返回 0 (无 GPU), 但不应该崩溃
  EXPECT_GE(s.gpu_temp, 0.0);
  EXPECT_GE(s.cpu_temp, 0.0);
}

TEST(ThermalTest, WaitIfHot) {
  ThermalGovernor gov;
  // 如果没有过热, wait_if_hot 应直接返回
  bool waited = gov.wait_if_hot();
  // 不关心返回值, 只确保不阻塞
  SUCCEED();
}

TEST(ThermalTest, StatusToJson) {
  ThermalGovernor gov;
  gov.check();
  auto s = gov.status();
  auto j = s.to_json();
  EXPECT_TRUE(j["gpu_temp"].is_number());
  EXPECT_TRUE(j["throttled"].is_boolean());
  EXPECT_TRUE(j["cooling"].is_boolean());
}

TEST(ThermalTest, BackgroundMonitor) {
  ThermalGovernor gov;
  gov.start_background_monitor();
  // 给线程一点时间启动
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // 不应该崩溃
  gov.stop_background_monitor();
  SUCCEED();
}

TEST(ThermalTest, ConfigurableThresholds) {
  ThermalGovernor gov;
  gov.set_gpu_throttle(70.0);
  gov.set_gpu_cooldown(75.0);
  // 验证默认状态
  auto s = gov.status();
  EXPECT_FALSE(s.throttled);
}
