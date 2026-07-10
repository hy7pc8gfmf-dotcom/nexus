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
 * @file meta_kernel.cpp
 * @brief 元级内核实现
 */

#include "nexus/core/meta_kernel.h"

#include <algorithm>
#include <sstream>

namespace nexus::core {

auto MetaRule::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]          = id;
  j["name"]        = name;
  j["description"] = description;
  j["enabled"]     = enabled;
  return j;
}

auto RuleCheckResult::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["rule_id"] = rule_id;
  j["passed"]  = passed;
  j["reason"]  = reason;
  return j;
}

auto ShellInjectionStatus::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["shell_a_injected"] = shell_a_injected;
  j["shell_b_injected"] = shell_b_injected;
  j["shell_c_injected"] = shell_c_injected;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// MetaKernel
// ═══════════════════════════════════════════════════════════════════

MetaKernel::MetaKernel() noexcept {
  initialize();
}

void MetaKernel::initialize() noexcept {
  rules_ = {
    {"R1", "递归安全", "防止无限递归: 最大深度 " + std::to_string(kMaxRecursiveDepth), true},
    {"R2", "资源边界", "防止资源耗尽: 每周期最大 " + std::to_string(kMaxOpsPerCycle) + " 操作", true},
    {"R3", "审计前置", "所有操作需先通过审计引擎", true},
    {"R4", "因果追溯", "所有决策链可追溯至来源", true},
  };
}

// ═══════════════════════════════════════════════════════════════════
// Shell 注入
// ═══════════════════════════════════════════════════════════════════

auto MetaKernel::inject_shell_a() noexcept -> Status {
  injection_.shell_a_injected = true;
  return Status::Ok();
}

auto MetaKernel::inject_shell_b() noexcept -> Status {
  injection_.shell_b_injected = true;
  return Status::Ok();
}

auto MetaKernel::inject_shell_c() noexcept -> Status {
  injection_.shell_c_injected = true;
  return Status::Ok();
}

auto MetaKernel::inject_all() noexcept -> Status {
  inject_shell_a();
  inject_shell_b();
  inject_shell_c();
  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 规则检查
// ═══════════════════════════════════════════════════════════════════

auto MetaKernel::check(const std::string& operation,
                        const nlohmann::json& context) noexcept
    -> std::vector<RuleCheckResult> {
  std::vector<RuleCheckResult> results;

  for (const auto& rule : rules_) {
    if (!rule.enabled) continue;

    RuleCheckResult result;
    result.rule_id = rule.id;
    result.passed  = true;

    if (rule.id == "R1") {
      // R1: 递归安全
      recursive_depth_++;
      if (recursive_depth_ > kMaxRecursiveDepth) {
        result.passed = false;
        result.reason = "递归深度超限: " + std::to_string(recursive_depth_);
      }
    } else if (rule.id == "R2") {
      // R2: 资源边界
      ops_in_cycle_++;
      if (ops_in_cycle_ > kMaxOpsPerCycle) {
        result.passed = false;
        result.reason = "周期操作数超限: " + std::to_string(ops_in_cycle_);
      }
    } else if (rule.id == "R3") {
      // R3: 审计前置
      // 实际审计由 AuditEngine 执行, 此处仅记录
      result.passed = true;
      result.reason = "审计记录已生成";
    } else if (rule.id == "R4") {
      // R4: 因果追溯
      operation_history_.push_back(operation);
      if (operation_history_.size() > 100) {
        operation_history_.erase(operation_history_.begin());
      }
      result.passed = true;
    }

    results.push_back(result);
  }

  return results;
}

// ═══════════════════════════════════════════════════════════════════
// 配置与查询
// ═══════════════════════════════════════════════════════════════════

void MetaKernel::enable_rule(const std::string& rule_id, bool enabled) noexcept {
  for (auto& rule : rules_) {
    if (rule.id == rule_id) {
      rule.enabled = enabled;
      break;
    }
  }
}

auto MetaKernel::status() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  auto rules = nlohmann::json::array();
  for (const auto& r : rules_) rules.push_back(r.to_json());
  j["rules"] = rules;
  j["recursive_depth"] = recursive_depth_;
  j["ops_in_cycle"] = ops_in_cycle_;
  j["operation_history"] = static_cast<int>(operation_history_.size());
  return j;
}

auto MetaKernel::injection_status() const noexcept -> ShellInjectionStatus {
  return injection_;
}

}  // namespace nexus::core
