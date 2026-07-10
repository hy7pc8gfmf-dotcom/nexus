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

#ifndef NEXUS_BRIDGE_SEED_BANK_H
#define NEXUS_BRIDGE_SEED_BANK_H

/**
 * @file seed_bank.h
 * @brief 种子库 — 统一种子存储与查询
 *
 * 兼容 D:/synapse/logic_solve_engine/.unified_seed_bank.json 格式。
 * 支持 14D 坐标、域索引、按域/强度/类型查询。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>

#include "nexus/types/status.h"

namespace nexus::bridge {

/// 单颗种子
struct SeedEntry {
  std::string name;
  int         intensity  = 0;
  std::string source;
  std::string type;
  std::string domain_tag;
  std::string step_detail;

  auto to_json() const -> nlohmann::json;
};

/// 域索引条目
struct DomainEntry {
  std::string name;
  double      confidence = 0.0;
  std::string source;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// SeedBank
// ═══════════════════════════════════════════════════════════════════

class SeedBank {
public:
  explicit SeedBank(std::string data_dir = ".nexus") noexcept;

  /// 从 JSON 文件加载种子库
  auto load(const std::string& path = "") noexcept -> Status;

  /// 按域查询
  [[nodiscard]] auto query_by_domain(const std::string& domain,
                                      int limit = 50) const noexcept
      -> std::vector<SeedEntry>;

  /// 按强度过滤
  [[nodiscard]] auto query_by_intensity(int min_intensity = 5,
                                         int limit = 100) const noexcept
      -> std::vector<SeedEntry>;

  /// 搜索种子名
  [[nodiscard]] auto search(const std::string& keyword,
                             int limit = 20) const noexcept
      -> std::vector<SeedEntry>;

  /// 获取统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;

  /// 获取数量
  [[nodiscard]] auto count() const noexcept -> int { return total_; }

  /// 获取域列表
  [[nodiscard]] auto domains() const noexcept -> std::vector<std::string>;

  /// 持久化到 Nexus 格式
  auto persist() noexcept -> Status;

private:
  std::string data_dir_;
  std::unordered_map<std::string, SeedEntry> seeds_;
  std::unordered_map<std::string, std::vector<DomainEntry>> domain_index_;
  int total_ = 0;
  int dimension_ = 14;
};

}  // namespace nexus::bridge

#endif  // NEXUS_BRIDGE_SEED_BANK_H
