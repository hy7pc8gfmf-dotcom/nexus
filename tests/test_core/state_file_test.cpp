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
 * @file state_file_test.cpp
 * @brief 状态文件 IPC 单元测试
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "nexus/ipc/state_file.h"
#include "nexus/types/status.h"

namespace fs = std::filesystem;
using nexus::Status;
using nexus::ErrorCode;
using nexus::Result;
using nexus::ipc::FileLock;
using nexus::ipc::StateFileWriter;
using nexus::ipc::StateFileReader;

// ═══════════════════════════════════════════════════════════════════
// 测试夹具 — 临时目录
// ═══════════════════════════════════════════════════════════════════

class StateFileTest : public ::testing::Test {
protected:
  fs::path tmp_dir;
  fs::path test_file;

  void SetUp() override {
    tmp_dir = fs::temp_directory_path() / "nexus_test_state";
    fs::create_directories(tmp_dir);
    test_file = tmp_dir / "test_state.json";
  }

  void TearDown() override {
    fs::remove_all(tmp_dir);
  }
};

// ═══════════════════════════════════════════════════════════════════
// 工具函数测试
// ═══════════════════════════════════════════════════════════════════

TEST(StateFileUtilTest, CurrentIsoTimestamp_Format) {
  auto ts = nexus::ipc::current_iso_timestamp();
  // 格式: YYYY-MM-DDTHH:MM:SSZ
  EXPECT_EQ(ts.size(), 20);
  EXPECT_EQ(ts[4], '-');
  EXPECT_EQ(ts[7], '-');
  EXPECT_EQ(ts[10], 'T');
  EXPECT_EQ(ts[13], ':');
  EXPECT_EQ(ts[16], ':');
  EXPECT_EQ(ts[19], 'Z');
}

TEST(StateFileUtilTest, CurrentPid_ReturnsPositive) {
  auto pid = nexus::ipc::current_pid();
  EXPECT_GT(pid, 0);
}

// ═══════════════════════════════════════════════════════════════════
// StateFileWriter 测试
// ═══════════════════════════════════════════════════════════════════

TEST_F(StateFileTest, Writer_CreatesFile) {
  auto content = nlohmann::json{{"key", "value"}, {"num", 42}};
  StateFileWriter writer(test_file.string());
  auto status = writer.write(content);
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(fs::exists(test_file));
}

TEST_F(StateFileTest, Writer_ContentIsValid) {
  auto written = nlohmann::json{{"component", "core"}, {"status", "ready"}};
  {
    StateFileWriter writer(test_file.string());
    auto status = writer.write(written);
    ASSERT_TRUE(status.ok());
  }

  // 验证文件内容
  std::ifstream ifs(test_file);
  ASSERT_TRUE(ifs.is_open());
  nlohmann::json parsed;
  ifs >> parsed;
  EXPECT_EQ(parsed["component"], "core");
  EXPECT_EQ(parsed["status"], "ready");
}

TEST_F(StateFileTest, Writer_Overwrite) {
  {
    StateFileWriter writer(test_file.string());
    ASSERT_TRUE(writer.write(nlohmann::json{{"v", 1}}).ok());
  }
  {
    StateFileWriter writer(test_file.string());
    ASSERT_TRUE(writer.write(nlohmann::json{{"v", 2}}).ok());
  }

  std::ifstream ifs(test_file);
  nlohmann::json parsed;
  ifs >> parsed;
  EXPECT_EQ(parsed["v"], 2);
}

// ═══════════════════════════════════════════════════════════════════
// StateFileReader 测试
// ═══════════════════════════════════════════════════════════════════

TEST_F(StateFileTest, Reader_FileExists) {
  StateFileWriter writer(test_file.string());
  ASSERT_TRUE(writer.write(nlohmann::json{{"x", 1}}).ok());

  StateFileReader reader(test_file.string());
  EXPECT_TRUE(reader.exists());
}

TEST_F(StateFileTest, Reader_ReadBack) {
  StateFileWriter writer(test_file.string());
  ASSERT_TRUE(writer.write(nlohmann::json{{"name", "nexus"}}).ok());

  StateFileReader reader(test_file.string());
  auto result = reader.read();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value()["name"], "nexus");
}

TEST_F(StateFileTest, Reader_FileNotExist) {
  StateFileReader reader((tmp_dir / "nonexistent.json").string());
  EXPECT_FALSE(reader.exists());
  auto result = reader.read();
  EXPECT_FALSE(result.ok());
}

TEST_F(StateFileTest, Reader_RetryOnEmpty) {
  // 写入空文件
  {
    std::ofstream ofs(test_file);
    ofs << "   ";
  }

  StateFileReader reader(test_file.string());
  auto result = reader.read(2, 10);  // 重试 2 次
  EXPECT_FALSE(result.ok());
}

// ═══════════════════════════════════════════════════════════════════
// FileLock 测试
// ═══════════════════════════════════════════════════════════════════

TEST_F(StateFileTest, FileLock_AcquireRelease) {
  FileLock lock;
  EXPECT_FALSE(lock.is_held());

  bool acquired = lock.acquire(test_file.string());
  EXPECT_TRUE(acquired);
  EXPECT_TRUE(lock.is_held());

  lock.release();
  EXPECT_FALSE(lock.is_held());
}

TEST_F(StateFileTest, FileLock_DoubleAcquire) {
  FileLock lock;
  ASSERT_TRUE(lock.acquire(test_file.string()));
  EXPECT_TRUE(lock.is_held());

  // 重复获取同一锁
  bool reacquired = lock.acquire(test_file.string());
  EXPECT_TRUE(reacquired);  // 应成功（同一对象重入或超时等待）
  EXPECT_TRUE(lock.is_held());
}

TEST_F(StateFileTest, FileLock_Move) {
  FileLock lock;
  ASSERT_TRUE(lock.acquire(test_file.string()));

  FileLock moved = std::move(lock);
  EXPECT_TRUE(moved.is_held());
  EXPECT_FALSE(lock.is_held());  // NOLINT: moved-from state

  moved.release();
  EXPECT_FALSE(moved.is_held());
}
