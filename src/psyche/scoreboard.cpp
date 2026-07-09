/**
 * @file scoreboard.cpp
 * @brief 竞技记分板实现
 */

#include "nexus/psyche/scoreboard.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <sstream>

namespace nexus::psyche {

auto MatchRecord::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["match_id"] = match_id; j["engine_a"] = engine_a; j["engine_b"] = engine_b;
  j["score_a"] = score_a; j["score_b"] = score_b; j["winner"] = winner; j["timestamp"] = timestamp;
  return j;
}

void Scoreboard::record_match(const std::string& engine_a, const std::string& engine_b,
                               double score_a, double score_b) noexcept {
  MatchRecord mr;
  mr.match_id = "match_" + std::to_string(matches_.size() + 1);
  mr.engine_a = engine_a; mr.engine_b = engine_b;
  mr.score_a = score_a; mr.score_b = score_b;
  mr.winner = score_a > score_b ? engine_a : (score_b > score_a ? engine_b : "tie");
  mr.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  matches_.push_back(mr);
}

auto Scoreboard::leaderboard(int top_n) const noexcept -> nlohmann::json {
  auto scores = compute_scores_();
  std::sort(scores.begin(), scores.end(),
    [](const auto& a, const auto& b) { return a.second > b.second; });

  auto j = nlohmann::json::object();
  j["total_matches"] = static_cast<int>(matches_.size());
  auto entries = nlohmann::json::array();
  for (int i = 0; i < std::min(top_n, static_cast<int>(scores.size())); ++i) {
    auto e = nlohmann::json::object();
    e["rank"] = i + 1; e["engine"] = scores[i].first; e["score"] = scores[i].second;
    entries.push_back(e);
  }
  j["leaderboard"] = entries;
  return j;
}

auto Scoreboard::total_matches() const noexcept -> int {
  return static_cast<int>(matches_.size());
}

auto Scoreboard::compute_scores_() const noexcept
    -> std::vector<std::pair<std::string, double>> {
  std::map<std::string, double> totals;
  std::map<std::string, int> counts;

  for (const auto& m : matches_) {
    totals[m.engine_a] += m.score_a; counts[m.engine_a]++;
    totals[m.engine_b] += m.score_b; counts[m.engine_b]++;
  }

  std::vector<std::pair<std::string, double>> result;
  for (const auto& [name, total] : totals) {
    result.emplace_back(name, counts[name] > 0 ? total / counts[name] : 0);
  }
  return result;
}

}  // namespace nexus::psyche
