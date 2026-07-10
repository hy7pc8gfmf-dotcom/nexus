// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file state_writer_test.cpp
 * @brief 守护进程状态写入单元测试
 *
 * 测试 daemon 使用 StateFileWriter + ComponentStateBase
 * 写入标准状态文件的完整流程。
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "nexus/ipc/state_file.h"
#include "nexus/types/status.h"
#include "nexus/types/component_state.h"

namespace fs = std::filesystem;
using nexus::Status;
using nexus::ErrorCode;
using nexus::Result;
using nexus::ComponentStatus;
using nexus::ComponentStateBase;
using nexus::ipc::StateFileWriter;
using nexus::ipc::StateFileReader;

// ═══════════════════════════════════════════════════════════════════
// 测试夹具
// ═══════════════════════════════════════════════════════════════════

class StateWriterTest : public ::testing::Test {
protected:
  fs::path tmp_dir;
  fs::path state_file;

  void SetUp() override {
    tmp_dir = fs::temp_directory_path() / "nexus_test_daemon";
    fs::create_directories(tmp_dir);
    state_file = tmp_dir / "daemon_state.json";
  }

  void TearDown() override {
    fs::remove_all(tmp_dir);
  }

  /// 写入完整状态文件的辅助方法
  auto write_state(const ComponentStateBase& state) -> Status {
    StateFileWriter writer(state_file.string());
    return writer.write(state.to_json());
  }
};

// ═══════════════════════════════════════════════════════════════════
// ComponentStateBase 序列化
// ═══════════════════════════════════════════════════════════════════

TEST_F(StateWriterTest, ToJson_BasicFields) {
  ComponentStateBase state;
  state.component  = "daemon";
  state.status     = ComponentStatus::kReady;
  state.pid        = 1234;
  state.started_at = "2026-07-09T18:00:00Z";
  state.updated_at = "2026-07-09T18:05:00Z";

  auto j = state.to_json();
  EXPECT_EQ(j["$schema"].get<std::string>(),   "nexus-state-v1");
  EXPECT_EQ(j["version"].get<std::string>(),   "1.0");
  EXPECT_EQ(j["component"].get<std::string>(), "daemon");
  EXPECT_EQ(j["status"].get<std::string>(),    "ready");
  EXPECT_EQ(j["pid"].get<int>(),       1234);
  EXPECT_EQ(j["started_at"].get<std::string>(), "2026-07-09T18:00:00Z");
}

TEST_F(StateWriterTest, ToJson_WithDetails) {
  ComponentStateBase state;
  state.component = "core";
  state.status    = ComponentStatus::kReady;
  state.details   = nlohmann::json{{"vram_used_mb", 5765}, {"loaded_models", {"qwythos_9b"}}};

  auto j = state.to_json();
  EXPECT_EQ(j["details"]["vram_used_mb"].get<int>(), 5765);
  EXPECT_EQ(j["details"]["loaded_models"][0].get<std::string>(), "qwythos_9b");
}

// ═══════════════════════════════════════════════════════════════════
// ComponentStateBase 反序列化
// ═══════════════════════════════════════════════════════════════════

TEST_F(StateWriterTest, FromJson_AllStatuses) {
  auto test_status = [](const std::string& status_str,
                         ComponentStatus expected) {
    auto j = nlohmann::json{
      {"$schema", "nexus-state-v1"},
      {"component", "test"},
      {"status",   status_str},
    };
    auto result = ComponentStateBase::from_json(j);
    EXPECT_TRUE(result.ok()) << "for status: " << status_str;
    EXPECT_EQ(result.value().status, expected);
  };

  test_status("ready",    ComponentStatus::kReady);
  test_status("starting", ComponentStatus::kStarting);
  test_status("degraded", ComponentStatus::kDegraded);
  test_status("error",    ComponentStatus::kError);
  test_status("stopped",  ComponentStatus::kStopped);
}

TEST_F(StateWriterTest, FromJson_MissingRequired) {
  auto j = nlohmann::json{{"$schema", "nexus-state-v1"}};  // 没有 "component"
  auto result = ComponentStateBase::from_json(j);
  EXPECT_FALSE(result.ok());
}

// ═══════════════════════════════════════════════════════════════════
// 状态文件写+读集成测试
// ═══════════════════════════════════════════════════════════════════

TEST_F(StateWriterTest, WriteThenRead) {
  ComponentStateBase state;
  state.component = "daemon";
  state.status    = ComponentStatus::kReady;
  state.pid       = 999;
  state.updated_at = "2026-07-10T00:00:00Z";

  ASSERT_TRUE(write_state(state).ok());

  StateFileReader reader(state_file.string());
  auto result = reader.read();
  ASSERT_TRUE(result.ok());

  EXPECT_EQ(result.value()["component"].get<std::string>(), "daemon");
  EXPECT_EQ(result.value()["status"].get<std::string>(), "ready");
  EXPECT_EQ(result.value()["pid"].get<int>(), 999);
}

TEST_F(StateWriterTest, DaemonDetails_RoundTrip) {
  nexus::DaemonDetails details;
  details.uptime_seconds     = 3600;
  details.cycle              = 42;
  details.psi_field_written  = 1000;
  details.will_hooks_scanned = 5;

  ComponentStateBase state;
  state.component = "daemon";
  state.status    = ComponentStatus::kReady;
  state.details   = details.to_json();

  ASSERT_TRUE(write_state(state).ok());

  StateFileReader reader(state_file.string());
  auto result = reader.read();
  ASSERT_TRUE(result.ok());

  auto d = result.value()["details"];
  EXPECT_EQ(d["uptime_seconds"].get<int>(),     3600);
  EXPECT_EQ(d["cycle"].get<int>(),              42);
  EXPECT_EQ(d["psi_field_written"].get<int>(),  1000);
  EXPECT_EQ(d["will_hooks_scanned"].get<int>(), 5);
}

TEST_F(StateWriterTest, CoreDetails_RoundTrip) {
  nexus::CoreDetails details;
  details.vram_used_mb  = 4096;
  details.vram_free_mb  = 1024;
  details.loaded_models = {"qwythos_9b", "gr2m"};

  ComponentStateBase state;
  state.component = "core";
  state.details   = details.to_json();

  ASSERT_TRUE(write_state(state).ok());

  StateFileReader reader(state_file.string());
  auto result = reader.read();
  ASSERT_TRUE(result.ok());

  auto d = result.value()["details"];
  EXPECT_EQ(d["vram_used_mb"].get<int>(), 4096);
  EXPECT_EQ(d["vram_free_mb"].get<int>(), 1024);
  EXPECT_EQ(d["loaded_models"].size(), 2);
}

TEST_F(StateWriterTest, EnvDetails_RoundTrip) {
  nexus::EnvDetails details;
  details.gpu_name      = "RTX 3070";
  details.vram_total_mb = 8192;
  details.vram_free_mb  = 6000;

  ComponentStateBase state;
  state.component = "env_checker";
  state.details   = details.to_json();

  ASSERT_TRUE(write_state(state).ok());

  StateFileReader reader(state_file.string());
  auto result = reader.read();
  ASSERT_TRUE(result.ok());

  auto gpu = result.value()["details"]["gpu"];
  EXPECT_EQ(gpu["name"].get<std::string>(),          "RTX 3070");
  EXPECT_EQ(gpu["vram_total_mb"].get<int>(), 8192);
  EXPECT_EQ(gpu["vram_free_mb"].get<int>(),  6000);
}

// ═══════════════════════════════════════════════════════════════════
// Daemon 状态多个组件同步更新 (模拟真实场景)
// ═══════════════════════════════════════════════════════════════════

TEST_F(StateWriterTest, MultipleUpdates) {
  for (int i = 0; i < 5; i++) {
    ComponentStateBase state;
    state.component = "daemon";
    state.status    = i < 3 ? ComponentStatus::kStarting : ComponentStatus::kReady;
    state.pid       = 1000 + i;
    state.updated_at = "2026-07-10T00:00:0" + std::to_string(i) + "Z";

    ASSERT_TRUE(write_state(state).ok());
  }

  // 读取最终状态
  StateFileReader reader(state_file.string());
  auto result = reader.read();
  ASSERT_TRUE(result.ok());

  EXPECT_EQ(result.value()["status"].get<std::string>(), "ready");
  EXPECT_EQ(result.value()["pid"].get<int>(), 1004);
}
