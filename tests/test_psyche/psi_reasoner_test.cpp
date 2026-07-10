// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file psi_reasoner_test.cpp
 * @brief Ψ 推理引擎单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/psyche/psi_reasoner.h"
#include "nexus/types/status.h"

using nexus::Status;
using nexus::ErrorCode;
using nexus::Result;
using nexus::psyche::PsiReasoner;
using nexus::psyche::InferCallback;
using nexus::psyche::ReasoningStep;
using nexus::psyche::ReasoningReport;
using nexus::psyche::StepType;

// ═══════════════════════════════════════════════════════════════════
// 模拟推理回调 — 全局状态 + 静态函数 (兼容 MSVC lambda→std::function)
// ═══════════════════════════════════════════════════════════════════

struct MockState {
  int step;
  int steps_before;
};

static auto mock_infer_fn(const std::string& problem,
                           const std::string&,
                           void* state) -> Result<std::pair<std::string, double>> {
  auto* ms = static_cast<MockState*>(state);
  ms->step++;
  double conf = std::min(0.95, 0.3 + ms->step * 0.1);
  std::string resp;
  if (ms->step <= 2) {
    resp = "分析 " + problem + " 的初步推理";
  } else if (ms->step <= ms->steps_before) {
    resp = "逐步推理: 推导出新的结论 (步 " + std::to_string(ms->step) + ")";
  } else {
    resp = "最终结论: 推理链闭合";
    conf = 0.92;
  }
  return Result<std::pair<std::string, double>>(
    std::pair<std::string, double>(resp, conf));
}

static MockState g_mock_state{0, 10};
static auto g_mock_cb = [](const std::string& p, const std::string& c) {
  return mock_infer_fn(p, c, &g_mock_state);
};

static auto low_conf_infer_fn(const std::string& problem,
                               const std::string&,
                               void* state) -> Result<std::pair<std::string, double>> {
  auto* step = static_cast<int*>(state);
  (*step)++;
  double conf = std::max(0.1, 0.3 - *step * 0.02);
  return Result<std::pair<std::string, double>>(
    std::pair<std::string, double>(
      "不确定推理: " + problem + " 步 " + std::to_string(*step), conf));
}

static int g_low_step = 0;
static auto g_low_cb = [](const std::string& p, const std::string& c) {
  return low_conf_infer_fn(p, c, &g_low_step);
};

// ═══════════════════════════════════════════════════════════════════
// 基础推理测试
// ═══════════════════════════════════════════════════════════════════

TEST(PsiReasonerTest, BasicReasoning) {
  g_mock_state = {0, 8};

  PsiReasoner::Config cfg;
  cfg.max_steps = 20;
  cfg.enable_observer = false;
  cfg.enable_seeds = false;

  PsiReasoner reasoner(cfg);
  reasoner.set_infer_callback(InferCallback(g_mock_cb));

  auto result = reasoner.reason("测试问题", "math");
  ASSERT_TRUE(result.ok());

  auto report = result.value();
  EXPECT_TRUE(report.converged);
  EXPECT_GE(report.total_steps, 3);
  EXPECT_LE(report.total_steps, 20);
  EXPECT_GE(report.final_confidence, 0.5);
  EXPECT_FALSE(report.problem.empty());
}

TEST(PsiReasonerTest, ReportStructure) {
  g_mock_state = {0, 8};

  PsiReasoner::Config cfg;
  cfg.max_steps = 5;
  cfg.enable_observer = false;

  PsiReasoner reasoner(cfg);
  reasoner.set_infer_callback(InferCallback(g_mock_cb));

  auto result = reasoner.reason("test", "");
  ASSERT_TRUE(result.ok());

  auto report = result.value();
  EXPECT_GT(report.total_steps, 0);
  EXPECT_FALSE(report.steps.empty());

  EXPECT_TRUE(report.steps[0].type == StepType::kInitial);
  EXPECT_TRUE(report.steps.back().type == StepType::kAdvance ||
              report.steps.back().type == StepType::kConclusion);

  auto j = report.to_json();
  EXPECT_EQ(j["problem"].get<std::string>(), "test");
  EXPECT_TRUE(j["steps"].is_array());
  EXPECT_GT(j["steps"].size(), 0);
}

TEST(PsiReasonerTest, StepConfidenceIncreases) {
  g_mock_state = {0, 10};

  PsiReasoner reasoner;
  reasoner.set_infer_callback(InferCallback(g_mock_cb));

  auto result = reasoner.reason("递增置信度测试", "");
  ASSERT_TRUE(result.ok());

  double prev_conf = 0.0;
  int advances = 0;
  for (const auto& s : result.value().steps) {
    if (s.type == StepType::kAdvance) {
      EXPECT_GE(s.confidence, prev_conf - 0.01) << "步 " << s.id << " 置信度下降";
      prev_conf = s.confidence;
      advances++;
    }
  }
  EXPECT_GT(advances, 0);
}

// ═══════════════════════════════════════════════════════════════════
// 分支与修正测试
// ═══════════════════════════════════════════════════════════════════

TEST(PsiReasonerTest, BranchLimit) {
  g_low_step = 0;

  PsiReasoner::Config cfg;
  cfg.max_steps = 30;
  cfg.max_branches = 3;
  cfg.branch_threshold = 0.5;
  cfg.enable_observer = false;

  PsiReasoner reasoner(cfg);
  reasoner.set_infer_callback(InferCallback(g_low_cb));

  auto result = reasoner.reason("分支测试", "");
  ASSERT_TRUE(result.ok());

  // total_branches 包含主路径 (branch 0), 所以最多 max_branches + 1
  EXPECT_LE(result.value().total_branches, cfg.max_branches + 1);
}

TEST(PsiReasonerTest, RevisionLimit) {
  g_low_step = 0;

  PsiReasoner::Config cfg;
  cfg.max_steps = 20;
  cfg.max_revisions = 3;
  cfg.revision_threshold = 0.25;
  cfg.enable_observer = false;

  PsiReasoner reasoner(cfg);
  reasoner.set_infer_callback(InferCallback(g_low_cb));

  auto result = reasoner.reason("修正测试", "");
  ASSERT_TRUE(result.ok());

  EXPECT_LE(result.value().total_revisions, cfg.max_revisions);
}

// ═══════════════════════════════════════════════════════════════════
// 边界条件测试
// ═══════════════════════════════════════════════════════════════════

TEST(PsiReasonerTest, NoInferCallback) {
  PsiReasoner reasoner;
  auto result = reasoner.reason("test", "");
  EXPECT_FALSE(result.ok());
}

TEST(PsiReasonerTest, EmptyProblem) {
  g_mock_state = {0, 5};

  PsiReasoner reasoner;
  reasoner.set_infer_callback(InferCallback(g_mock_cb));

  auto result = reasoner.reason("", "");
  ASSERT_TRUE(result.ok());
  EXPECT_GT(result.value().total_steps, 0);
}

TEST(PsiReasonerTest, ConfigCustomization) {
  g_mock_state = {0, 10};

  PsiReasoner::Config cfg;
  cfg.max_steps = 3;
  cfg.converge_threshold = 0.99;
  cfg.enable_observer = false;

  PsiReasoner reasoner(cfg);
  reasoner.set_infer_callback(InferCallback(g_mock_cb));

  auto result = reasoner.reason("配置测试", "");
  ASSERT_TRUE(result.ok());

  EXPECT_LE(result.value().total_steps, cfg.max_steps + 1);
}

// ═══════════════════════════════════════════════════════════════════
// 状态查询测试
// ═══════════════════════════════════════════════════════════════════

TEST(PsiReasonerTest, QueryBeforeReason) {
  PsiReasoner reasoner;
  EXPECT_EQ(reasoner.total_steps(), 0);
  EXPECT_EQ(reasoner.current_step(), 0);
  EXPECT_FALSE(reasoner.is_running());
  EXPECT_EQ(reasoner.current_confidence(), 0.0);
}

TEST(PsiReasonerTest, QueryAfterReason) {
  g_mock_state = {0, 8};

  PsiReasoner reasoner;
  reasoner.set_infer_callback(InferCallback(g_mock_cb));

  auto result = reasoner.reason("查询测试", "");
  ASSERT_TRUE(result.ok());

  EXPECT_GT(reasoner.total_steps(), 0);
  EXPECT_FALSE(reasoner.is_running());
  EXPECT_GT(reasoner.current_confidence(), 0.0);
}

// ═══════════════════════════════════════════════════════════════════
// 序列化测试
// ═══════════════════════════════════════════════════════════════════

TEST(PsiReasonerTest, StepToJson) {
  ReasoningStep step;
  step.id = 1;
  step.parent_id = 0;
  step.type = StepType::kAdvance;
  step.content = "测试内容";
  step.confidence = 0.85;
  step.branch_id = 0;
  step.verified = true;
  step.timestamp = 1234.5;

  auto j = step.to_json();
  EXPECT_EQ(j["id"].get<int>(), 1);
  EXPECT_EQ(j["type"].get<std::string>(), "advance");
  EXPECT_EQ(j["content"].get<std::string>(), "测试内容");
  EXPECT_NEAR(j["confidence"].get<double>(), 0.85, 0.001);
  EXPECT_TRUE(j["verified"].get<bool>());
}

TEST(PsiReasonerTest, ReportToJson) {
  g_mock_state = {0, 5};

  PsiReasoner reasoner;
  reasoner.set_infer_callback(InferCallback(g_mock_cb));

  auto result = reasoner.reason("序列化测试", "");
  ASSERT_TRUE(result.ok());

  auto j = result.value().to_json();
  EXPECT_EQ(j["problem"].get<std::string>(), "序列化测试");
  EXPECT_TRUE(j["converged"].is_boolean());
  EXPECT_TRUE(j["steps"].is_array());
  EXPECT_FALSE(j["steps"].empty());
}
