// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file integration_test.cpp
 * @brief Phase 7 — 端到端集成测试
 *
 * 验证 C++ 模块与 Python 系统的等价性。
 * 需要本地数据文件 (D:/synapse/, D:/nexus/binary/)。
 * CI 环境跳过 (GTEST_SKIP)。
 */

#include <gtest/gtest.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include "nexus/bridge/seed_bank.h"
#include "nexus/ipc/mmap_ringbuf.h"
#include "nexus/ipc/state_file.h"
#include "nexus/psyche/semantic_port.h"
#include "nexus/types/component_state.h"
#include "nexus/types/status.h"

namespace fs = std::filesystem;
using nexus::Status;
using nexus::Result;
using nexus::ComponentStateBase;
using nexus::ComponentStatus;
using nexus::ipc::MmapRingBuffer;
using nexus::bridge::SeedBank;
using nexus::psyche::SemanticPort;

// ── 数据文件路径 (开发环境) ──
static const char* kSeedBankPath = "D:/synapse/logic_solve_engine/.unified_seed_bank.json";
static const char* kSemanticPath = "D:/nexus/binary/.semantic_field.bin";

#define REQUIRE_DATA_FILE(path) \
  if (!fs::exists(path)) GTEST_SKIP() << "数据文件不存在 (仅开发环境): " << path

// ═══════════════════════════════════════════════════════════════════
// 1. 种子库端到端
// ═══════════════════════════════════════════════════════════════════

TEST(IntegrationTest, SeedBank_LoadUnified) {
  REQUIRE_DATA_FILE(kSeedBankPath);

  SeedBank bank(".nexus");
  auto status = bank.load(kSeedBankPath);
  ASSERT_TRUE(status.ok()) << status.to_string();
  EXPECT_GT(bank.count(), 30000) << "种子数不足 (预期 >30000, 实际 " << bank.count() << ")";
}

TEST(IntegrationTest, SeedBank_QueryByDomain) {
  REQUIRE_DATA_FILE(kSeedBankPath);

  SeedBank bank(".nexus");
  auto status = bank.load(kSeedBankPath);
  ASSERT_TRUE(status.ok());

  // 按强度查询 (不依赖域索引)
  auto strong_seeds = bank.query_by_intensity(5, 5);
  EXPECT_FALSE(strong_seeds.empty()) << "强度>=5 的种子应存在";
}

TEST(IntegrationTest, SeedBank_Search) {
  REQUIRE_DATA_FILE(kSeedBankPath);

  SeedBank bank(".nexus");
  bank.load(kSeedBankPath);
  auto results = bank.search("theorem", 5);
  EXPECT_FALSE(results.empty());
}

TEST(IntegrationTest, SeedBank_Inject) {
  REQUIRE_DATA_FILE(kSeedBankPath);

  SeedBank bank(".nexus");
  bank.load(kSeedBankPath);

  int before = bank.count();
  nexus::bridge::SeedEntry seed;
  seed.name = "test_integration_seed";
  seed.intensity = 7;
  seed.source = "integration_test";
  seed.type = "test";
  seed.domain_tag = "test";

  auto status = bank.inject(seed, {"test", "integration"});
  ASSERT_TRUE(status.ok());
  EXPECT_EQ(bank.count(), before + 1);
}

// ═══════════════════════════════════════════════════════════════════
// 2. 语义场端到端
// ═══════════════════════════════════════════════════════════════════

TEST(IntegrationTest, SemanticPort_LoadBinary) {
  REQUIRE_DATA_FILE(kSemanticPath);

  SemanticPort port;
  auto status = port.load(kSemanticPath);
  ASSERT_TRUE(status.ok());
  EXPECT_GT(port.count(), 50000);
  EXPECT_TRUE(port.loaded());
}

TEST(IntegrationTest, SemanticPort_QueryConcept) {
  REQUIRE_DATA_FILE(kSemanticPath);

  SemanticPort port;
  port.load(kSemanticPath);

  auto result = port.concept_of("水");
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value().name, "水");
  EXPECT_EQ(result.value().lang, "zh");
}

TEST(IntegrationTest, SemanticPort_Neighbors) {
  REQUIRE_DATA_FILE(kSemanticPath);

  SemanticPort port;
  port.load(kSemanticPath);

  auto neighbors = port.neighbors("水");
  EXPECT_FALSE(neighbors.empty());
  EXPECT_FALSE(neighbors[0].empty());
}

TEST(IntegrationTest, SemanticPort_Stats) {
  REQUIRE_DATA_FILE(kSemanticPath);

  SemanticPort port;
  port.load(kSemanticPath);

  auto s = port.stats();
  EXPECT_GE(s["concepts"].get<int>(), 50000);
  EXPECT_GE(s["edges"].get<int>(), 1000000);
  EXPECT_TRUE(s["loaded"].get<bool>());
}

TEST(IntegrationTest, SemanticPort_Distance) {
  REQUIRE_DATA_FILE(kSemanticPath);

  SemanticPort port;
  port.load(kSemanticPath);

  auto d = port.distance("水", "火");
  ASSERT_TRUE(d.ok());
  EXPECT_GT(d.value(), 0.0);
  EXPECT_LT(d.value(), 4.0);
}

TEST(IntegrationTest, SemanticPort_NotFound) {
  REQUIRE_DATA_FILE(kSemanticPath);

  SemanticPort port;
  port.load(kSemanticPath);

  auto result = port.concept_of("__nonexistent__");
  EXPECT_FALSE(result.ok());
}

// ═══════════════════════════════════════════════════════════════════
// 3. 状态文件格式兼容性
// ═══════════════════════════════════════════════════════════════════

TEST(IntegrationTest, ComponentState_Format) {
  ComponentStateBase state;
  state.component  = "integration_test";
  state.status     = ComponentStatus::kReady;
  state.pid        = 12345;
  state.started_at = "2026-07-10T00:00:00Z";
  state.updated_at = "2026-07-10T00:05:00Z";

  auto j = state.to_json();
  EXPECT_EQ(j["$schema"].get<std::string>(), "nexus-state-v1");
  EXPECT_EQ(j["component"].get<std::string>(), "integration_test");
  EXPECT_EQ(j["status"].get<std::string>(), "ready");

  auto result = ComponentStateBase::from_json(j);
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value().component, "integration_test");
  EXPECT_EQ(result.value().status, ComponentStatus::kReady);
}

TEST(IntegrationTest, StateFile_RoundTrip) {
  auto tmp = fs::temp_directory_path() / "nexus_int_test_state.json";

  {
    nexus::ipc::StateFileWriter writer(tmp.string());
    auto data = nlohmann::json::object();
    data["component"] = "test";
    data["status"] = "running";
    data["value"] = 42;
    ASSERT_TRUE(writer.write(data).ok());
  }
  {
    nexus::ipc::StateFileReader reader(tmp.string());
    auto result = reader.read();
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value()["component"].get<std::string>(), "test");
    EXPECT_EQ(result.value()["value"].get<int>(), 42);
  }

  fs::remove(tmp);
}

// ═══════════════════════════════════════════════════════════════════
// 4. IPC mmap 环状缓冲区
// ═══════════════════════════════════════════════════════════════════

TEST(IntegrationTest, MmapRingBuffer_WriteRead) {
  auto tmp = fs::temp_directory_path() / "nexus_int_test_psi.mmap";
  fs::remove(tmp);

  auto result = MmapRingBuffer::create_or_open(tmp);
  ASSERT_TRUE(result.ok());

  auto& buf = result.value();
  ASSERT_TRUE(buf.write("集成测试消息", "test").ok());

  auto latest = buf.read_latest();
  ASSERT_TRUE(latest.has_value());

  fs::remove(tmp);
}
