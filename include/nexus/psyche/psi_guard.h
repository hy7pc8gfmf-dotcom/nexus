// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_PSI_GUARD_H
#define NEXUS_PSYCHE_PSI_GUARD_H

/**
 * @file psi_guard.h
 * @brief Ψ-Guard 自审计管线 — 三重验证闭包
 *
 * 移植自 D:/synapse/engines/psi_guard.py
 *
 * 三重验证:
 *   Phase 1 — 形式验证 (未定义项/未证明断言/量纲分析)
 *   Phase 2 — 跨题一致性 (与历史断言对比)
 *   Phase 3 — 种子库一致性 (按域检索引申冲突)
 *
 * 用法:
 *   PsiGuard guard;
 *   auto result = guard.audit("断言文本", "Q23", "math");
 *   if (!result.passed) { auto fix = guard.auto_correct(result); }
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/bridge/seed_bank.h"
#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 审计问题
// ═══════════════════════════════════════════════════════════════════

struct AuditIssue {
  std::string type;        // "undefined_term" | "unproven_assertion" | etc.
  std::string detail;      // 问题描述
  std::string severity;    // "error" | "warning" | "info"
  std::string correction;  // 建议修正 (可选)

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 审计结果
// ═══════════════════════════════════════════════════════════════════

struct AuditResult {
  bool        passed    = true;
  std::string source;
  std::string domain;
  std::vector<AuditIssue> issues;
  std::vector<std::string> corrections;

  [[nodiscard]] auto errors() const noexcept -> std::vector<AuditIssue>;
  [[nodiscard]] auto warnings() const noexcept -> std::vector<AuditIssue>;
  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// Ψ-Guard 自审计引擎
// ═══════════════════════════════════════════════════════════════════

class PsiGuard {
public:
  PsiGuard() noexcept;

  /// 加载已知术语库 (含种子库种子名)
  void load_known_terms(bridge::SeedBank* bank = nullptr) noexcept;

  /// 三重审计入口
  auto audit(const std::string& claim,
             const std::string& source = "unknown",
             const std::string& domain = "meta") noexcept -> AuditResult;

  /// 基于审计结果自动修正
  auto auto_correct(const std::string& claim,
                    const AuditResult& result) noexcept
      -> std::pair<std::string, std::vector<std::string>>;

  /// 重置断言历史 (Phase 2)
  void reset_history() noexcept { claim_history_.clear(); }

private:
  // 已知术语集
  std::vector<std::string> known_terms_;
  bool terms_loaded_ = false;

  // 断言历史 (Phase 2)
  struct ClaimRecord {
    std::string claim;
    std::string source;
    double      timestamp;
  };
  std::vector<ClaimRecord> claim_history_;

  // ── Phase 1: 形式验证 ──
  auto phase1_formal_(const std::string& claim) noexcept -> std::vector<AuditIssue>;

  // ── Phase 2: 跨题一致性 ──
  auto phase2_cross_question_(const std::string& claim,
                               const std::string& source) noexcept
      -> std::vector<AuditIssue>;

  // ── Phase 3: 种子库一致性 ──
  auto phase3_seed_consistency_(const std::string& claim,
                                 const std::string& domain,
                                 bridge::SeedBank* bank) noexcept
      -> std::vector<AuditIssue>;

  // 辅助
  static std::vector<std::string> extract_symbols_(const std::string& text);
  static bool contains_word_(const std::string& text, const std::string& word);
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_PSI_GUARD_H
