/**
 * @file experience.cpp
 * @brief 经验引擎实现
 */

#include "nexus/core/experience.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace nexus::core {

auto ExperienceCard::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"] = id; j["prompt_preview"] = prompt_preview;
  j["output_preview"] = output_preview; j["duration_ms"] = duration_ms;
  j["quality_score"] = quality_score; j["created_at"] = created_at;
  return j;
}

void ExperienceEngine::store(const std::string& prompt, const std::string& output,
                              double duration_ms, double quality) noexcept {
  if (static_cast<int>(cards_.size()) >= kMaxCards) cards_.erase(cards_.begin());
  ExperienceCard card;
  card.id = "exp_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count());
  card.prompt_hash = compute_hash_(prompt);
  card.prompt_preview = prompt.substr(0, 64);
  card.output_preview = output.substr(0, 128);
  card.duration_ms = duration_ms;
  card.quality_score = std::clamp(quality, 0.0, 1.0);
  card.created_at = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  cards_.push_back(card);
}

auto ExperienceEngine::lookup(const std::string& prompt, int limit) const noexcept
    -> std::vector<ExperienceCard> {
  auto hash = compute_hash_(prompt);
  std::vector<ExperienceCard> results;
  for (const auto& c : cards_) {
    if (c.prompt_hash == hash) results.push_back(c);
  }
  if (static_cast<int>(results.size()) > limit) results.resize(limit);
  // 按质量排序
  std::sort(results.begin(), results.end(),
    [](const auto& a, const auto& b) { return a.quality_score > b.quality_score; });
  return results;
}

auto ExperienceEngine::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_cards"] = static_cast<int>(cards_.size());
  j["max_cards"] = kMaxCards;
  if (!cards_.empty()) {
    double avg = 0;
    for (const auto& c : cards_) avg += c.quality_score;
    j["avg_quality"] = std::round(avg / cards_.size() * 100) / 100;
  }
  return j;
}

auto ExperienceEngine::count() const noexcept -> int {
  return static_cast<int>(cards_.size());
}

auto ExperienceEngine::compute_hash_(const std::string& s) -> std::string {
  // 简化哈希: 取前 32 字符 + 长度
  return s.substr(0, 32) + "_" + std::to_string(s.size());
}

}  // namespace nexus::core
