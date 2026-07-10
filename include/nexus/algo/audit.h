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

#ifndef NEXUS_ALGO_AUDIT_H
#define NEXUS_ALGO_AUDIT_H

/**
 * @file audit.h
 * @brief 审计引擎 — 40 条规则, 三 Shell 注入
 *
 * 审计类型:
 *   UA - Usage Audit (使用审计)
 *   CD - Cross-check Domain (域交叉检查)
 *   AZ - Authorization (授权审计)
 *   PB - Policy Breach (策略违规)
 *   CE - Capability Error (能力错误)
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "nexus/types/status.h"
#include <nlohmann/json.hpp>

namespace nexus::algo {

/// 审计结果
struct AuditRecord {
  std::string rule_id;       // 规则 ID (如 "UA-01")
  std::string category;      // UA / CD / AZ / PB / CE
  std::string description;   // 规则描述
  bool        passed = true;
  std::string detail;        // 详细信息

  auto to_json() const -> nlohmann::json;
};

/// 审计报告
struct AuditReport {
  std::vector<AuditRecord> records;
  int total = 0;
  int passed = 0;
  int failed = 0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 审计引擎
// ═══════════════════════════════════════════════════════════════════

class AuditEngine {
public:
  /// 配置数据目录（必须在 run_all 前调用）
  void configure(const std::string& data_dir) noexcept;

  /// 运行全部 40 条规则
  auto run_all() noexcept -> AuditReport;

  /// 按类别运行
  auto run_category(const std::string& category) noexcept -> AuditReport;

  /// 获取审计统计
  [[nodiscard]] auto summary() const noexcept -> nlohmann::json;

  /// 获取单例
  static auto instance() noexcept -> AuditEngine&;

private:
  using RuleFn = std::function<AuditRecord()>;
  struct Rule {
    std::string id;
    std::string category;
    std::string description;
    RuleFn fn;
  };
  std::vector<Rule> rules_;
  std::string data_dir_ = ".nexus";

  AuditEngine() noexcept;
  void init_default_rules_() noexcept;

  // Shell A/B/C 注入状态
  bool shell_a_injected_ = true;
  bool shell_b_injected_ = true;
  bool shell_c_injected_ = true;
};

}  // namespace nexus::algo

#endif  // NEXUS_ALGO_AUDIT_H
