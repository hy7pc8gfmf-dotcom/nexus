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
 * @file coq_kernel.cpp
 * @brief Coq 提取的元内核 DLL 加载器
 *
 * 加载 D:/synapse 中的 Coq 验证 DLL:
 *   meta_kernel_rules.dll  — 5 条元规则 (Coq 证明 + OCaml 提取 + C 翻译)
 *   constructive_rules.dll — 构造性规则 (duality check)
 */

#include "nexus/core/coq_kernel.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <sstream>

namespace nexus::core {

auto CoqRuleResult::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["rule_name"] = rule_name; j["result"] = result;
  j["task"] = task; j["passed"] = passed;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// CoqMetaKernel
// ═══════════════════════════════════════════════════════════════════

CoqMetaKernel::CoqMetaKernel() noexcept = default;

CoqMetaKernel::~CoqMetaKernel() noexcept {
  if (dll_handle_) {
    FreeLibrary(static_cast<HMODULE>(dll_handle_));
    dll_handle_ = nullptr;
  }
}

auto CoqMetaKernel::load(const std::string& dll_path) noexcept -> Status {
  HMODULE h = LoadLibraryA(dll_path.c_str());
  if (!h) {
    return Status::Error(ErrorCode::kFileNotFound,
      "Coq DLL not found: " + dll_path);
  }
  dll_handle_ = h;
  return resolve_symbols_();
}

auto CoqMetaKernel::resolve_symbols_() noexcept -> Status {
  auto h = static_cast<HMODULE>(dll_handle_);

  decide_ = reinterpret_cast<decltype(decide_)>(
    GetProcAddress(h, "decide"));
  check_eternity_ = reinterpret_cast<decltype(check_eternity_)>(
    GetProcAddress(h, "check_eternity"));
  full_chain_equivalence_ = reinterpret_cast<decltype(full_chain_equivalence_)>(
    GetProcAddress(h, "full_chain_equivalence"));

  if (!decide_ || !check_eternity_ || !full_chain_equivalence_) {
    return Status::Error(ErrorCode::kEngineFailed,
      "Coq DLL missing exported functions");
  }
  return Status::Ok();
}

auto CoqMetaKernel::evaluate(const std::string& task) noexcept
    -> std::vector<CoqRuleResult> {
  std::vector<CoqRuleResult> results;
  if (!dll_handle_) return results;

  int r = decide_(task.c_str());
  results.push_back({"decide", r, task, r >= 0});

  int e = check_eternity_(task.c_str());
  results.push_back({"check_eternity", e, task, e >= 0});

  int f = full_chain_equivalence_();
  results.push_back({"full_chain_equivalence", f, "all", f >= 0});

  return results;
}

auto CoqMetaKernel::is_loaded() const noexcept -> bool {
  return dll_handle_ != nullptr;
}

// ═══════════════════════════════════════════════════════════════════
// ConstructiveRules
// ═══════════════════════════════════════════════════════════════════

ConstructiveRules::ConstructiveRules() noexcept = default;

ConstructiveRules::~ConstructiveRules() noexcept {
  if (dll_handle_) {
    FreeLibrary(static_cast<HMODULE>(dll_handle_));
    dll_handle_ = nullptr;
  }
}

auto ConstructiveRules::load(const std::string& dll_path) noexcept -> Status {
  HMODULE h = LoadLibraryA(dll_path.c_str());
  if (!h) {
    return Status::Error(ErrorCode::kFileNotFound,
      "constructive DLL not found: " + dll_path);
  }
  dll_handle_ = h;

  duality001_check_ = reinterpret_cast<DualityFn>(
    GetProcAddress(h, "duality001_check"));
  duality002_check_ = reinterpret_cast<DualityFn>(
    GetProcAddress(h, "duality002_check"));

  if (!duality001_check_ || !duality002_check_) {
    return Status::Error(ErrorCode::kEngineFailed,
      "constructive DLL missing duality exports");
  }

  return Status::Ok();
}

auto ConstructiveRules::check_duality(int level) noexcept -> bool {
  if (!dll_handle_) return false;
  if (level == 1 && duality001_check_) return duality001_check_() != 0;
  if (level == 2 && duality002_check_) return duality002_check_() != 0;
  return false;
}

auto ConstructiveRules::is_loaded() const noexcept -> bool {
  return dll_handle_ != nullptr;
}

}  // namespace nexus::core
