// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file psi_guard_test.cpp
 * @brief Ψ-Guard 自审计管线单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/psyche/psi_guard.h"

using nexus::psyche::PsiGuard;
using nexus::psyche::AuditResult;
using nexus::psyche::AuditIssue;

TEST(PsiGuardTest, CleanClaim) {
  PsiGuard guard;
  auto result = guard.audit("ZFC is consistent", "test", "logic");
  EXPECT_TRUE(result.passed);
}

TEST(PsiGuardTest, UnprovenAssertion) {
  PsiGuard guard;
  auto result = guard.audit("This is obviously true", "test", "meta");
  EXPECT_FALSE(result.passed);

  bool found = false;
  for (const auto& i : result.issues) {
    if (i.type == "unproven_assertion") { found = true; break; }
  }
  EXPECT_TRUE(found);
}

TEST(PsiGuardTest, CoCMisstatement) {
  PsiGuard guard;
  auto result = guard.audit("CoC is known inconsistent", "test", "logic");
  EXPECT_FALSE(result.passed);

  bool found = false;
  for (const auto& i : result.issues) {
    if (i.type == "consistency_misstatement") { found = true; break; }
  }
  EXPECT_TRUE(found);
}

TEST(PsiGuardTest, AutoCorrectSomethingHuge) {
  PsiGuard guard;
  auto result = guard.audit("ordinal = something huge", "test", "math");
  EXPECT_FALSE(result.passed);

  auto [corrected, applied] = guard.auto_correct("ordinal = something huge", result);
  EXPECT_FALSE(applied.empty());
  EXPECT_TRUE(corrected.find("galactic") != std::string::npos);
}

TEST(PsiGuardTest, AutoCorrectCoC) {
  PsiGuard guard;
  auto result = guard.audit("CoC is known inconsistent", "test", "logic");

  auto [corrected, applied] = guard.auto_correct("CoC is known inconsistent", result);
  EXPECT_TRUE(corrected.find("Coquand-Huet") != std::string::npos);
}

TEST(PsiGuardTest, CrossQuestionConsistency) {
  PsiGuard guard;
  // First claim: consistent
  guard.audit("CoC is consistent", "Q1", "logic");
  // Second claim: contradicts
  auto result = guard.audit("CoC is inconsistent", "Q2", "logic");

  bool found = false;
  for (const auto& i : result.issues) {
    if (i.type == "consistency_contradiction") { found = true; break; }
  }
  EXPECT_TRUE(found);
  EXPECT_FALSE(result.passed);
}

TEST(PsiGuardTest, AuditResultToJson) {
  PsiGuard guard;
  auto result = guard.audit("test obviously wrong", "src", "meta");

  auto j = result.to_json();
  EXPECT_FALSE(j["passed"].get<bool>());
  EXPECT_TRUE(j["issues"].is_array());
  EXPECT_GT(j["n_errors"].get<int>(), 0);
}

TEST(PsiGuardTest, MultipleIssues) {
  PsiGuard guard;
  // Multiple issues in one claim
  auto result = guard.audit("something huge and obviously wrong with CoC inconsistent",
                             "test", "meta");
  EXPECT_FALSE(result.passed);
  EXPECT_GT(static_cast<int>(result.issues.size()), 1);
}

TEST(PsiGuardTest, ResetHistory) {
  PsiGuard guard;
  guard.audit("CoC is consistent", "Q1", "logic");
  guard.reset_history();
  // After reset, no history exists
  auto result = guard.audit("CoC is inconsistent", "Q2", "logic");
  // Should pass because no history to compare against
  bool found = false;
  for (const auto& i : result.issues) {
    if (i.type == "consistency_contradiction") { found = true; break; }
  }
  EXPECT_FALSE(found);
}

TEST(PsiGuardTest, UndefinedTerm) {
  PsiGuard guard;
  auto result = guard.audit("XXXXXX_undefined_symbol_XXXXXX is important",
                             "test", "meta");
  bool found = false;
  for (const auto& i : result.issues) {
    if (i.type == "undefined_term") { found = true; break; }
  }
  EXPECT_TRUE(found);
}
