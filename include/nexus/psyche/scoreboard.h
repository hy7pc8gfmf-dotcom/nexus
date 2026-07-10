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

#ifndef NEXUS_PSYCHE_SCOREBOARD_H
#define NEXUS_PSYCHE_SCOREBOARD_H

/**
 * @file scoreboard.h
 * @brief 竞技记分板 — 记录算法引擎和历史对局的评分
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::psyche {

struct MatchRecord {
  std::string  match_id;
  std::string  engine_a;
  std::string  engine_b;
  double       score_a = 0;
  double       score_b = 0;
  std::string  winner;
  double       timestamp = 0;

  auto to_json() const -> nlohmann::json;
};

class Scoreboard {
public:
  Scoreboard() noexcept = default;

  auto record_match(const std::string& engine_a, const std::string& engine_b,
                    double score_a, double score_b) noexcept -> void;
  [[nodiscard]] auto leaderboard(int top_n = 5) const noexcept -> nlohmann::json;
  [[nodiscard]] auto total_matches() const noexcept -> int;

private:
  std::vector<MatchRecord> matches_;
  std::vector<std::pair<std::string, double>> compute_scores_() const noexcept;
};

}  // namespace nexus::psyche

#endif
