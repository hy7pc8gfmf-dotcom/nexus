/**
 * @file sophia_core.cpp
 * @brief Sophia Core 实现
 */

#include "nexus/psyche/sophia_core.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

using nexus::Status;
using nexus::ErrorCode;

namespace nexus::psyche {

namespace fs = std::filesystem;

auto WisdomEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]          = id;
  j["topic"]       = topic;
  j["content"]     = content;
  j["confidence"]  = confidence;
  j["source"]      = source;
  j["created_at"]  = created_at;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// SophiaCore
// ═══════════════════════════════════════════════════════════════════

SophiaCore::SophiaCore(std::string data_dir) noexcept
  : data_dir_(std::move(data_dir)) {
  load();
}

auto SophiaCore::add_wisdom(const std::string& topic,
                             const std::string& content,
                             double confidence,
                             const std::string& source) -> Status {
  WisdomEntry entry;
  entry.id = "wisdom_" + std::to_string(entries_.size() + 1);
  entry.topic = topic;
  entry.content = content.size() > 256 ? content.substr(0, 256) : content;
  entry.confidence = std::clamp(confidence, 0.0, 1.0);
  entry.source = source;
  entry.created_at = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();

  entries_.push_back(entry);
  return persist();
}

auto SophiaCore::query(const std::string& topic, int limit) const noexcept
    -> std::vector<WisdomEntry> {
  if (topic.empty()) {
    auto sorted = entries_;
    std::sort(sorted.begin(), sorted.end(),
      [](const WisdomEntry& a, const WisdomEntry& b) {
        return a.confidence > b.confidence;
      });
    if (static_cast<int>(sorted.size()) > limit) sorted.resize(limit);
    return sorted;
  }

  // 简单关键词匹配 + 置信度排序
  auto topic_lower = topic;
  std::transform(topic_lower.begin(), topic_lower.end(), topic_lower.begin(),
    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  std::vector<std::pair<WisdomEntry, double>> scored;
  for (const auto& e : entries_) {
    double score = 0;

    auto etopic = e.topic;
    std::transform(etopic.begin(), etopic.end(), etopic.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto econtent = e.content;
    std::transform(econtent.begin(), econtent.end(), econtent.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (etopic.find(topic_lower) != std::string::npos) score += 0.5;
    if (econtent.find(topic_lower) != std::string::npos) score += 0.3;
    score += e.confidence * 0.2;

    if (score > 0) scored.push_back({e, score});
  }

  std::sort(scored.begin(), scored.end(),
    [](const auto& a, const auto& b) { return a.second > b.second; });

  std::vector<WisdomEntry> results;
  for (const auto& [e, s] : scored) {
    if (static_cast<int>(results.size()) >= limit) break;
    results.push_back(e);
  }

  return results;
}

auto SophiaCore::all() const noexcept -> std::vector<WisdomEntry> {
  return entries_;
}

auto SophiaCore::count() const noexcept -> int {
  return static_cast<int>(entries_.size());
}

auto SophiaCore::persist() noexcept -> Status {
  try {
    fs::create_directories(data_dir_);
    std::string path = data_dir_ + "/wisdom_db.json";

    nlohmann::json data = nlohmann::json::array();
    for (const auto& e : entries_) data.push_back(e.to_json());

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
      return Status::Error(ErrorCode::kIoError, "cannot write wisdom_db.json");
    }
    ofs << data.dump(2);
    return Status::Ok();
  } catch (const std::exception& e) {
    return Status::Error(ErrorCode::kIoError, e.what());
  }
}

auto SophiaCore::load() noexcept -> Status {
  try {
    std::string path = data_dir_ + "/wisdom_db.json";
    if (!fs::exists(path)) return Status::Ok();

    std::ifstream ifs(path);
    if (!ifs.is_open()) return Status::Ok();

    nlohmann::json data;
    ifs >> data;

    if (data.is_array()) {
      for (const auto& item : data) {
        WisdomEntry entry;
        entry.id         = item.value("id", "");
        entry.topic      = item.value("topic", "");
        entry.content    = item.value("content", "");
        entry.confidence = item.value("confidence", 0.5);
        entry.source     = item.value("source", "");
        entry.created_at = item.value("created_at", 0.0);
        entries_.push_back(entry);
      }
    }
    return Status::Ok();
  } catch (const std::exception&) {
    return Status::Ok(); // 加载失败不回退
  }
}

}  // namespace nexus::psyche
