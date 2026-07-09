/**
 * @file willchain.cpp
 * @brief 意志链桥接实现
 */

#include "nexus/bridge/willchain.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace nexus::bridge {

namespace fs = std::filesystem;

auto WillRecord::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["will_id"]    = will_id;
  j["intent"]     = intent;
  j["source"]     = source;
  j["priority"]   = priority;
  j["parent_id"]  = parent_id;
  j["payload"]    = payload;
  j["created_at"] = created_at;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// WillChainBridge
// ═══════════════════════════════════════════════════════════════════

WillChainBridge::WillChainBridge(std::string data_dir) noexcept
  : db_path_(data_dir + "/willchain_db.jsonl") {
  load_();
}

auto WillChainBridge::commit(const std::string& intent,
                              const std::string& source,
                              int priority,
                              const nlohmann::json& payload,
                              const std::string& parent_id) -> Status {
  WillRecord record;
  record.will_id = "will_" + std::to_string(std::chrono::duration_cast<
    std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
  record.intent    = intent;
  record.source    = source;
  record.priority  = std::clamp(priority, 1, 10);
  record.parent_id = parent_id;
  record.payload   = payload;
  record.created_at = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();

  chain_.push_back(record);
  append_(record);
  return Status::Ok();
}

auto WillChainBridge::trace(const std::string& will_id) const noexcept
    -> std::vector<WillRecord> {
  std::vector<WillRecord> trail;

  // 正向查找
  for (const auto& r : chain_) {
    if (r.will_id == will_id) {
      trail.push_back(r);
      // 递归查找子意志
      for (const auto& child : chain_) {
        if (child.parent_id == will_id) {
          trail.push_back(child);
        }
      }
      break;
    }
  }

  return trail;
}

auto WillChainBridge::query_by_source(const std::string& source,
                                       int limit) const noexcept
    -> std::vector<WillRecord> {
  std::vector<WillRecord> results;
  for (const auto& r : chain_) {
    if (r.source == source) {
      results.push_back(r);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }
  return results;
}

auto WillChainBridge::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_wills"] = static_cast<int>(chain_.size());

  // 按来源统计
  auto by_source = nlohmann::json::object();
  for (const auto& r : chain_) {
    auto key = r.source;
    by_source[key] = by_source.value(key, 0) + 1;
  }
  j["by_source"] = by_source;
  return j;
}

void WillChainBridge::load_() noexcept {
  if (!fs::exists(db_path_)) return;
  std::ifstream ifs(db_path_);
  if (!ifs.is_open()) return;

  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty()) continue;
    try {
      auto data = nlohmann::json::parse(line);
      WillRecord r;
      r.will_id    = data.value("will_id", "");
      r.intent     = data.value("intent", "");
      r.source     = data.value("source", "");
      r.priority   = data.value("priority", 5);
      r.parent_id  = data.value("parent_id", "");
      r.payload    = data.value("payload", nlohmann::json::object());
      r.created_at = data.value("created_at", 0.0);
      chain_.push_back(r);
    } catch (...) {}
  }
}

void WillChainBridge::append_(const WillRecord& record) noexcept {
  try {
    fs::create_directories(fs::path(db_path_).parent_path());
    std::ofstream ofs(db_path_, std::ios::app);
    if (ofs.is_open()) ofs << record.to_json().dump() << "\n";
  } catch (...) {}
}

}  // namespace nexus::bridge
