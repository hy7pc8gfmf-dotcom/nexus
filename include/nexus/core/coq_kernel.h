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

#ifndef NEXUS_CORE_COQ_KERNEL_H
#define NEXUS_CORE_COQ_KERNEL_H

/**
 * @file coq_kernel.h
 * @brief Coq 提取的元内核规则 — 加载 constructive_rules.dll + meta_kernel_rules.dll
 *
 * 这些 DLL 来自 D:/synapse 的 Coq 形式化证明提取:
 *   meta_kernel_rules.dll    — 5 条元规则 (open_completeness/fissure/foundation/recursion/observer)
 *   constructive_rules.dll   — 构造性规则 (duality check, ground chain, 等价集)
 *
 * Coq 证明 → OCaml 提取 → C 翻译 → DLL, 保证逻辑正确性。
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::core {

/// Coq 验证的元规则结果
struct CoqRuleResult {
  std::string rule_name;
  int         result;     // Coq 验证返回值
  std::string task;
  bool        passed;     // result >= 0

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// CoqMetaKernel — 加载 meta_kernel_rules.dll 的 5 条元规则
// ═══════════════════════════════════════════════════════════════════

class CoqMetaKernel {
public:
  CoqMetaKernel() noexcept;
  ~CoqMetaKernel() noexcept;

  /// 加载 DLL
  auto load(const std::string& dll_path = "D:/synapse/meta_kernel_rules.dll") noexcept -> Status;

  /// 运行 5 条 Coq 验证的元规则
  auto evaluate(const std::string& task) noexcept -> std::vector<CoqRuleResult>;

  /// 是否已加载
  [[nodiscard]] auto is_loaded() const noexcept -> bool;

private:
  void* dll_handle_ = nullptr;  // HMODULE

  // 函数指针 (对应 meta_kernel_rules.c 的导出)
  using DecideFn = int (*)(const char*);
  using EternityFn = int (*)(const char*);
  using ChainFn = int (*)();

  DecideFn decide_ = nullptr;
  EternityFn check_eternity_ = nullptr;
  ChainFn full_chain_equivalence_ = nullptr;

  auto resolve_symbols_() noexcept -> Status;
};

// ═══════════════════════════════════════════════════════════════════
// ConstructiveRules — 加载 constructive_rules.dll
// ═══════════════════════════════════════════════════════════════════

class ConstructiveRules {
public:
  ConstructiveRules() noexcept;
  ~ConstructiveRules() noexcept;

  auto load(const std::string& dll_path = "D:/synapse/constructive_rules.dll") noexcept -> Status;

  /// duality001_check / duality002_check — Coq 验证的对偶性检查
  auto check_duality(int level = 1) noexcept -> bool;

  /// 是否已加载
  [[nodiscard]] auto is_loaded() const noexcept -> bool;

private:
  void* dll_handle_ = nullptr;
  using DualityFn = int (*)();
  DualityFn duality001_check_ = nullptr;
  DualityFn duality002_check_ = nullptr;
};

}  // namespace nexus::core

#endif
