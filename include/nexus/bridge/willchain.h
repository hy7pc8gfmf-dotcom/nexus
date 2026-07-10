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

#ifndef NEXUS_BRIDGE_WILLCHAIN_H
#define NEXUS_BRIDGE_WILLCHAIN_H

/**
 * @file willchain.h
 * @brief 意志链桥接 — Shell Ψ 意志持久化与追溯
 *
 * 每条意志记录包含:
 *   intent — 意图描述
 *   source — 来源 (Shell A/B/C/Ψ)
 *   priority — 优先级
 *   payload — 附加数据
 *   chain_id — 链式追溯ID
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::bridge {

/// 意志记录
struct WillRecord {
  std::string will_id;
  std::string intent;
  std::string source;       // "shell_a" | "shell_b" | "shell_c" | "psi_auto" | "psi_guide"
  int         priority = 5; // [1-10]
  std::string parent_id;    // 父意志 ID (链式追溯)
  nlohmann::json payload;
  double created_at = 0.0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// WillChainBridge
// ═══════════════════════════════════════════════════════════════════

class WillChainBridge {
public:
  explicit WillChainBridge(std::string data_dir = ".nexus") noexcept;

  /// 提交一条意志
  auto commit(const std::string& intent, const std::string& source,
              int priority = 5,
              const nlohmann::json& payload = {},
              const std::string& parent_id = "") -> Status;

  /// 查询意志链
  [[nodiscard]] auto trace(const std::string& will_id) const noexcept
      -> std::vector<WillRecord>;

  /// 按来源查询
  [[nodiscard]] auto query_by_source(const std::string& source,
                                      int limit = 20) const noexcept
      -> std::vector<WillRecord>;

  /// 获取统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;

private:
  std::string db_path_;
  std::vector<WillRecord> chain_;

  auto load_() noexcept -> void;
  auto append_(const WillRecord& record) noexcept -> void;
};

}  // namespace nexus::bridge

#endif  // NEXUS_BRIDGE_WILLCHAIN_H
