#include <gtest/gtest.h>
#include <gtest/gtest-death-test.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "nexus/types/status.h"
#include "nexus/types/component_state.h"

// ─── Status 测试 ─────────────────────────────────────────────
TEST(StatusTest, Ok) {
  auto s = nexus::Status::Ok();
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(s.to_string(), "OK");
}

TEST(StatusTest, Error) {
  auto s = nexus::Status::Error(nexus::ErrorCode::kFileNotFound, "env.json not found");
  EXPECT_FALSE(s.ok());
  EXPECT_EQ(s.code, nexus::ErrorCode::kFileNotFound);
  EXPECT_TRUE(s.to_string().find("FILE_NOT_FOUND") != std::string::npos);
}

// ─── Result 测试 ─────────────────────────────────────────────
TEST(ResultTest, Value) {
  auto r = nexus::Result<int>(42);
  EXPECT_TRUE(r.ok());
  EXPECT_EQ(r.value(), 42);
}

TEST(ResultTest, Error) {
  auto r = nexus::Result<int>(nexus::Status::Error(nexus::ErrorCode::kInternal, "test"));
  EXPECT_FALSE(r.ok());
  EXPECT_EQ(r.error().code, nexus::ErrorCode::kInternal);
}

TEST(ResultTest, ValueOr) {
  auto r = nexus::Result<int>(nexus::Status::Error(nexus::ErrorCode::kInternal, "test"));
  EXPECT_EQ(r.value_or(-1), -1);
}

// ─── ComponentState 测试 ─────────────────────────────────────
TEST(ComponentStateTest, ToJson) {
  nexus::ComponentStateBase state;
  state.component   = "core";
  state.status      = nexus::ComponentStatus::kReady;
  state.pid         = 1234;
  state.started_at  = "2026-07-09T18:00:00Z";
  state.updated_at  = "2026-07-09T18:05:00Z";
  state.details     = {{"vram_used_mb", 5765}};

  auto j = state.to_json();
  EXPECT_EQ(j["component"], "core");
  EXPECT_EQ(j["status"], "ready");
  EXPECT_EQ(j["pid"], 1234);
  EXPECT_EQ(j["details"]["vram_used_mb"], 5765);
}

TEST(ComponentStateTest, FromJson) {
  auto j = nlohmann::json{
    {"$schema", "nexus-state-v1"},
    {"version", "1.0"},
    {"component", "daemon"},
    {"status", "running"},
    {"pid", 5678},
    {"details", {{"uptime_seconds", 300}}},
  };

  auto result = nexus::ComponentStateBase::from_json(j);
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value().component, "daemon");
  EXPECT_EQ(result.value().status, nexus::ComponentStatus::kReady); // running → ready
  EXPECT_EQ(result.value().pid, 5678);
}

TEST(ComponentStateTest, ErrorCodeToString) {
  EXPECT_STREQ(nexus::error_code_to_string(nexus::ErrorCode::kOk), "OK");
  EXPECT_STREQ(nexus::error_code_to_string(nexus::ErrorCode::kGpuUnavailable), "GPU_UNAVAILABLE");
  EXPECT_STREQ(nexus::error_code_to_string(nexus::ErrorCode::kIpcTimeout), "IPC_TIMEOUT");
  EXPECT_STREQ(nexus::error_code_to_string(nexus::ErrorCode::kInternal), "INTERNAL_ERROR");
}
