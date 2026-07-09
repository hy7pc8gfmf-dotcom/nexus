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
