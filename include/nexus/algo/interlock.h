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

#ifndef NEXUS_ALGO_INTERLOCK_H
#define NEXUS_ALGO_INTERLOCK_H

/**
 * @file interlock.h
 * @brief 三反射互锁 — Shell A/B/C 三向验证
 *
 * 每次操作需经过三壳互锁验证:
 *   A(算法) → B(模型) → C(云端) 逐层确认
 */

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::algo {

struct InterlockResult {
  bool shell_a_approved = false;
  bool shell_b_approved = false;
  bool shell_c_approved = false;
  std::string detail;
  auto to_json() const -> nlohmann::json;
};

class TripleInterlock {
public:
  TripleInterlock() noexcept = default;

  auto verify(const std::string& operation) noexcept -> InterlockResult;
  auto reset() noexcept -> void;

private:
  int violation_count_ = 0;
  static constexpr int kMaxViolations = 5;
};

}  // namespace nexus::algo

#endif
