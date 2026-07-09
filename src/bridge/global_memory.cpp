/**
 * @file global_memory.cpp
 * @brief 全局记忆实现
 */

#include "nexus/bridge/global_memory.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace nexus::bridge {

namespace fs = std::filesystem;

auto MemEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]         = id;
  j["type"]       = type;
  j["topic"]      = topic;
  j["content"]    = content.size() > 512 ? content.substr(0, 512) : content;
  j["keywords"]   = keywords;
  j["component"]  = component;
  j["created_at"] = created_at;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// GlobalMemory
// ═══════════════════════════════════════════════════════════════════

GlobalMemory::GlobalMemory(std::string data_dir) noexcept
  : data_dir_(std::move(data_dir))
  , index_path_(data_dir_ + "/memory/INDEX.json")
  , kw_path_(data_dir_ + "/memory/keywords.json")
  , timeline_path_(data_dir_ + "/memory/timeline.jsonl")
  , entries_dir_(data_dir_ + "/memory/entries/") {
  ensure_dirs_();
  load_indexes_();
}

auto GlobalMemory::store(const std::string& type, const std::string& topic,
                          const std::string& content,
                          const std::vector<std::string>& keywords,
                          const std::string& component) -> Status {
  ensure_dirs_();

  MemEntry entry;
  entry.id         = generate_id_();
  entry.type       = type;
  entry.topic      = topic;
  entry.content    = content;
  entry.keywords   = keywords;
  entry.component  = component;
  entry.created_at = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();

  // 写入条目文件
  std::string entry_path = entries_dir_ + entry.id + ".json";
  {
    std::ofstream ofs(entry_path);
    if (!ofs.is_open()) return Status::Error(ErrorCode::kIoError, "write entry failed");
    ofs << entry.to_json().dump(2);
  }

  // 更新 INDEX
  if (!index_["topic"].is_object()) index_["topic"] = nlohmann::json::object();
  if (!index_["type"].is_object())  index_["type"]  = nlohmann::json::object();
  index_["topic"][topic].push_back(entry.id);
  index_["type"][type].push_back(entry.id);

  // 更新关键词索引
  for (const auto& kw : keywords) {
    kw_index_[kw].push_back(entry.id);
  }

  // 追加到时间线
  {
    std::ofstream ofs(timeline_path_, std::ios::app);
    if (ofs.is_open()) ofs << entry.to_json().dump() << "\n";
  }

  dirty_ = true;
  save_indexes_();
  return Status::Ok();
}

auto GlobalMemory::search(const std::string& query, int limit) const noexcept
    -> std::vector<MemEntry> {
  std::set<std::string> matched_ids;

  // 关键词索引匹配
  auto kw_val = kw_index_[query];
  if (!kw_val.is_null() && kw_val.is_array()) {
    for (size_t i = 0; i < kw_val.size(); ++i) {
      matched_ids.insert(kw_val[i].get<std::string>());
    }
  }

  // topic 模糊匹配
  auto topics = index_["topic"];
  if (topics.is_object()) {
    // 由于我们的 json 不支持对象迭代, 我们直接用 query 精确匹配
    auto topic_val = topics[query];
    if (!topic_val.is_null() && topic_val.is_array()) {
      for (size_t i = 0; i < topic_val.size(); ++i) {
        matched_ids.insert(topic_val[i].get<std::string>());
      }
    }
  }

  // 加载匹配的条目
  std::vector<MemEntry> results;
  for (const auto& id : matched_ids) {
    std::string entry_path = entries_dir_ + id + ".json";
    if (!fs::exists(entry_path)) continue;

    std::ifstream ifs(entry_path);
    if (!ifs.is_open()) continue;

    nlohmann::json data;
    ifs >> data;

    MemEntry entry;
    entry.id        = data.value("id", "");
    entry.type      = data.value("type", "");
    entry.topic     = data.value("topic", "");
    entry.content   = data.value("content", "");
    entry.keywords  = data.value("keywords", std::vector<std::string>());
    entry.component = data.value("component", "");
    results.push_back(entry);

    if (static_cast<int>(results.size()) >= limit) break;
  }

  return results;
}

auto GlobalMemory::search_by_type(const std::string& type, int limit) const noexcept
    -> std::vector<MemEntry> {
  std::vector<MemEntry> results;
  auto type_idx = index_["type"][type];
  if (!type_idx.is_null() && type_idx.is_array()) {
    for (size_t i = 0; i < type_idx.size(); ++i) {
      auto id = type_idx[i].get<std::string>();
      std::string entry_path = entries_dir_ + id + ".json";
      if (!fs::exists(entry_path)) continue;
      std::ifstream ifs(entry_path);
      if (!ifs.is_open()) continue;
      nlohmann::json data;
      ifs >> data;
      MemEntry entry;
      entry.id        = data.value("id", "");
      entry.type      = data.value("type", "");
      entry.topic     = data.value("topic", "");
      entry.content   = data.value("content", "");
      entry.keywords  = data.value("keywords", std::vector<std::string>());
      entry.component = data.value("component", "");
      results.push_back(entry);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }
  return results;
}

auto GlobalMemory::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_entries"] = total_entries();
  j["keywords"] = static_cast<int>(kw_index_.size());
  return j;
}

auto GlobalMemory::total_entries() const noexcept -> int {
  // 从 entries 目录统计
  if (!fs::exists(entries_dir_)) return 0;
  int count = 0;
  for (const auto& entry : fs::directory_iterator(entries_dir_)) {
    if (entry.path().extension() == ".json") count++;
  }
  return count;
}

// ═══════════════════════════════════════════════════════════════════
// 私有
// ═══════════════════════════════════════════════════════════════════

void GlobalMemory::ensure_dirs_() noexcept {
  fs::create_directories(entries_dir_);
}

void GlobalMemory::load_indexes_() noexcept {
  auto load_json = [](const std::string& path) -> nlohmann::json {
    if (!fs::exists(path)) return nlohmann::json::object();
    std::ifstream ifs(path);
    if (!ifs.is_open()) return nlohmann::json::object();
    nlohmann::json j;
    try { ifs >> j; } catch (...) { return nlohmann::json::object(); }
    return j;
  };

  index_   = load_json(index_path_);
  kw_index_ = load_json(kw_path_);
}

void GlobalMemory::save_indexes_() noexcept {
  if (!dirty_) return;

  auto save_json = [](const std::string& path, const nlohmann::json& j) {
    std::ofstream ofs(path);
    if (!ofs.is_open()) return;
    ofs << j.dump(2);
  };

  save_json(index_path_, index_);
  save_json(kw_path_, kw_index_);
  dirty_ = false;
}

auto GlobalMemory::generate_id_() const -> std::string {
  static int counter = 0;
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  return "mem_" + std::to_string(now) + "_" + std::to_string(counter++);
}

}  // namespace nexus::bridge
