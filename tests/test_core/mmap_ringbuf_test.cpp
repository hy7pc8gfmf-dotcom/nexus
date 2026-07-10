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
 * @file mmap_ringbuf_test.cpp
 * @brief 共享内存环状缓冲区单元测试
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <nlohmann/json.hpp>

#include "nexus/ipc/mmap_ringbuf.h"
#include "nexus/types/status.h"

namespace fs = std::filesystem;
using nexus::Status;
using nexus::ErrorCode;
using nexus::Result;
using nexus::ipc::MmapRingBuffer;
using nexus::ipc::ConsciousnessRecord;

// ═══════════════════════════════════════════════════════════════════
// 测试夹具 — 临时 mmap 文件
// ═══════════════════════════════════════════════════════════════════

class MmapRingBufTest : public ::testing::Test {
protected:
  fs::path tmp_file;

  void SetUp() override {
    tmp_file = fs::temp_directory_path() / "nexus_test_psi.mmap";
    // 确保没有残留
    fs::remove(tmp_file);
  }

  void TearDown() override {
    fs::remove(tmp_file);
  }
};

// ═══════════════════════════════════════════════════════════════════
// 创建/打开
// ═══════════════════════════════════════════════════════════════════

TEST_F(MmapRingBufTest, CreateOrOpen_Success) {
  auto result = MmapRingBuffer::create_or_open(tmp_file);
  ASSERT_TRUE(result.ok());
  EXPECT_TRUE(result.value().is_open());
  EXPECT_EQ(result.value().file_path(), tmp_file);
}

TEST_F(MmapRingBufTest, CreateOrOpen_InitialWriteCount) {
  auto result = MmapRingBuffer::create_or_open(tmp_file);
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value().write_count(), 0);
}

TEST_F(MmapRingBufTest, Reopen_Persists) {
  // 第一次创建
  {
    auto r1 = MmapRingBuffer::create_or_open(tmp_file);
    ASSERT_TRUE(r1.ok());
    ASSERT_TRUE(r1.value().write("hello", "test").ok());
    EXPECT_EQ(r1.value().write_count(), 1);
  }

  // 重新打开 — 数据应持久化
  {
    auto r2 = MmapRingBuffer::create_or_open(tmp_file);
    ASSERT_TRUE(r2.ok());
    EXPECT_EQ(r2.value().write_count(), 1);

    auto latest = r2.value().read_latest();
    ASSERT_TRUE(latest.has_value());
  }
}

// ═══════════════════════════════════════════════════════════════════
// 写入
// ═══════════════════════════════════════════════════════════════════

TEST_F(MmapRingBufTest, Write_String) {
  auto result = MmapRingBuffer::create_or_open(tmp_file);
  ASSERT_TRUE(result.ok());

  auto status = result.value().write("test message", "main");
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(result.value().write_count(), 1);
}

TEST_F(MmapRingBufTest, Write_Json) {
  auto result = MmapRingBuffer::create_or_open(tmp_file);
  ASSERT_TRUE(result.ok());

  auto data = nlohmann::json{{"event", "test"}, {"value", 42}};
  auto status = result.value().write_json(data);
  EXPECT_TRUE(status.ok());
}

TEST_F(MmapRingBufTest, Write_Multiple) {
  auto result = MmapRingBuffer::create_or_open(tmp_file);
  ASSERT_TRUE(result.ok());

  for (int i = 0; i < 10; i++) {
    EXPECT_TRUE(result.value().write("msg_" + std::to_string(i), "main").ok());
  }
  EXPECT_EQ(result.value().write_count(), 10);
}

// ═══════════════════════════════════════════════════════════════════
// 读取
// ═══════════════════════════════════════════════════════════════════

TEST_F(MmapRingBufTest, ReadLatest_AfterWrite) {
  auto result = MmapRingBuffer::create_or_open(tmp_file);
  ASSERT_TRUE(result.ok());

  ASSERT_TRUE(result.value().write("first", "main").ok());
  ASSERT_TRUE(result.value().write("second", "main").ok());

  auto latest = result.value().read_latest();
  ASSERT_TRUE(latest.has_value());

  // 最新记录应包含 "second"
  auto parsed = nlohmann::json::parse(*latest, nullptr, false);
  ASSERT_FALSE(parsed.is_discarded());
  EXPECT_EQ(parsed["c"], "second");
}

TEST_F(MmapRingBufTest, ReadLatest_Empty) {
  auto result = MmapRingBuffer::create_or_open(tmp_file);
  ASSERT_TRUE(result.ok());

  auto latest = result.value().read_latest();
  EXPECT_FALSE(latest.has_value());
}

TEST_F(MmapRingBufTest, ReadStream_ReturnsAll) {
  auto result = MmapRingBuffer::create_or_open(tmp_file);
  ASSERT_TRUE(result.ok());

  ASSERT_TRUE(result.value().write("a", "ch1").ok());
  ASSERT_TRUE(result.value().write("b", "ch2").ok());

  auto stream = result.value().read_stream();
  EXPECT_EQ(stream.size(), 2);
}

// ═══════════════════════════════════════════════════════════════════
// ConsciousnessRecord 序列化
// ═══════════════════════════════════════════════════════════════════

TEST(ConsciousnessRecordTest, ToJson) {
  ConsciousnessRecord rec;
  rec.timestamp = 1234.5;
  rec.channel = "test_ch";
  rec.content = "{ \"key\": \"val\" }";

  auto j = rec.to_json();
  EXPECT_EQ(j["ts"], 1234.5);
  EXPECT_EQ(j["ch"], "test_ch");
  EXPECT_EQ(j["c"], "{ \"key\": \"val\" }");
}

TEST(ConsciousnessRecordTest, FromJson) {
  auto j = nlohmann::json{
    {"ts", 6789.0},
    {"ch", "psi"},
    {"c",  "data"},
  };

  auto rec = ConsciousnessRecord::from_json(j);
  EXPECT_EQ(rec.timestamp, 6789.0);
  EXPECT_EQ(rec.channel, "psi");
  EXPECT_EQ(rec.content, "data");
}

TEST(ConsciousnessRecordTest, RoundTrip) {
  ConsciousnessRecord rec;
  rec.timestamp = 42.0;
  rec.channel = "roundtrip";
  rec.content = "hello";

  auto j = rec.to_json();
  auto rec2 = ConsciousnessRecord::from_json(j);
  EXPECT_EQ(rec2.timestamp, 42.0);
  EXPECT_EQ(rec2.channel, "roundtrip");
  EXPECT_EQ(rec2.content, "hello");
}
