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

#ifndef NEXUS_CORE_EXPERIENCE_H
#define NEXUS_CORE_EXPERIENCE_H

/**
 * @file experience.h
 * @brief 经验引擎 — 推理经验卡缓存与索引
 *
 * 每次推理后生成经验卡, 包含输入/输出/耗时/评分。
 * 后续推理可参考相似经验卡。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::core {

struct ExperienceCard {
  std::string id;
  std::string prompt_hash;
  std::string prompt_preview;
  std::string output_preview;
  double      duration_ms = 0;
  double      quality_score = 0.5;
  double      created_at = 0.0;

  auto to_json() const -> nlohmann::json;
};

class ExperienceEngine {
public:
  ExperienceEngine() noexcept = default;

  auto store(const std::string& prompt, const std::string& output,
             double duration_ms, double quality = 0.5) noexcept -> void;
  [[nodiscard]] auto lookup(const std::string& prompt, int limit = 3) const noexcept
      -> std::vector<ExperienceCard>;
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;
  [[nodiscard]] auto count() const noexcept -> int;

private:
  std::vector<ExperienceCard> cards_;
  static constexpr int kMaxCards = 100;
  static auto compute_hash_(const std::string& s) -> std::string;
};

}  // namespace nexus::core

#endif
