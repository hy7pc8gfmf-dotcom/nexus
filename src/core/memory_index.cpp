// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/core/memory_index.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace fs = std::filesystem;

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// MemoryEntry 序列化
// ═══════════════════════════════════════════════════════════════════

auto MemoryEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]       = id;
  j["type"]     = type;
  j["topic"]    = topic;
  j["summary"]  = summary;
  j["importance"] = importance;
  j["created"]  = created;

  auto kws = nlohmann::json::array();
  for (const auto& k : keywords) kws.push_back(k);
  j["keywords"] = kws;

  auto comps = nlohmann::json::array();
  for (const auto& c : related_components) comps.push_back(c);
  j["related_components"] = comps;

  return j;
}

auto MemoryEntry::from_json(const nlohmann::json& j) -> MemoryEntry {
  MemoryEntry e;
  e.id       = j.value("id", std::string(""));
  e.type     = j.value("type", std::string("insight"));
  e.topic    = j.value("topic", std::string(""));
  e.summary  = j.value("summary", std::string(""));
  e.importance = j.value("importance", 5);
  e.created  = j.value("created", std::string(""));

  auto kws = j["keywords"];
  if (kws.is_array()) {
    for (const auto& k : kws) e.keywords.push_back(k.get<std::string>());
  }
  auto comps = j["related_components"];
  if (comps.is_array()) {
    for (const auto& c : comps) e.related_components.push_back(c.get<std::string>());
  }

  return e;
}

// ═══════════════════════════════════════════════════════════════════
// MemoryIndex 实现
// ═══════════════════════════════════════════════════════════════════

MemoryIndex::MemoryIndex(const std::string& data_dir) noexcept
  : data_dir_(data_dir) {
  // 确保目录存在
  fs::create_directories(entries_dir_());
  // 读取已有索引确认 next_id
  auto index = load_index_();
  if (index.is_object()) {
    // 从现有文件推算 next_id
    int max_id = 0;
    try {
      for (auto& p : fs::directory_iterator(entries_dir_())) {
        auto fn = p.path().filename().string();
        if (fn.size() > 8 && fn.substr(0, 4) == "mem_") {
          int id = std::stoi(fn.substr(4, 4));
          max_id = std::max(max_id, id);
        }
      }
    } catch (...) {}
    next_id_ = std::max(1, max_id + 1);
  }
}

auto MemoryIndex::entries_dir_() const noexcept -> std::string {
  return data_dir_ + "/memory/entries";
}

auto MemoryIndex::index_path_() const noexcept -> std::string {
  return data_dir_ + "/memory/INDEX.json";
}

auto MemoryIndex::kw_path_() const noexcept -> std::string {
  return data_dir_ + "/memory/keywords.json";
}

auto MemoryIndex::timeline_path_() const noexcept -> std::string {
  return data_dir_ + "/memory/timeline.jsonl";
}

// ── 文件 I/O 辅助 ──

static auto read_json_file(const std::string& path) -> nlohmann::json {
  std::ifstream ifs(path);
  if (!ifs.is_open()) return nlohmann::json();
  try {
    nlohmann::json j;
    ifs >> j;
    return j;
  } catch (...) { return nlohmann::json(); }
}

static void write_json_file(const std::string& path, const nlohmann::json& data) {
  // 原子写入: tmp + rename
  std::string tmp = path + ".tmp";
  {
    std::ofstream ofs(tmp);
    if (!ofs.is_open()) return;
    ofs << data.dump(2) << std::endl;
  }
  std::error_code ec;
  fs::rename(tmp, path, ec);
}

// ── 索引加载/保存 ──

auto MemoryIndex::load_index_() -> nlohmann::json {
  auto j = read_json_file(index_path_());
  if (!j.is_object()) {
    j = nlohmann::json::object();
    j["by_topic"] = nlohmann::json::object();
    j["by_type"]  = nlohmann::json::object();
    j["by_component"] = nlohmann::json::object();
  }
  return j;
}

auto MemoryIndex::load_keywords_() -> nlohmann::json {
  auto j = read_json_file(kw_path_());
  return j.is_object() ? j : nlohmann::json::object();
}

void MemoryIndex::save_index_(const nlohmann::json& index) {
  write_json_file(index_path_(), index);
}

void MemoryIndex::save_keywords_(const nlohmann::json& keywords) {
  write_json_file(kw_path_(), keywords);
}

void MemoryIndex::append_timeline_(const std::string& mem_id, const MemoryEntry& entry) {
  auto j = nlohmann::json::object();
  j["id"] = mem_id;
  j["type"] = entry.type;
  j["topic"] = entry.topic;
  j["importance"] = entry.importance;
  j["created"] = entry.created;
  auto kws = nlohmann::json::array();
  for (const auto& k : entry.keywords) kws.push_back(k);
  j["keywords"] = kws;

  std::ofstream ofs(timeline_path_(), std::ios::app);
  if (ofs.is_open()) {
    ofs << j.dump() << std::endl;
  }
}

// ═══════════════════════════════════════════════════════════════════
// store
// ═══════════════════════════════════════════════════════════════════

auto MemoryIndex::store(const MemoryEntry& entry) noexcept -> std::string {
  auto e = entry;

  // 生成 ID
  char id_buf[16];
  std::snprintf(id_buf, sizeof(id_buf), "mem_%04d", next_id_++);
  e.id = id_buf;

  // 写入条目文件
  auto entry_path = entries_dir_() + "/" + e.id + ".json";
  write_json_file(entry_path, e.to_json());

  // 更新索引
  auto index = load_index_();

  // by_topic
  if (!e.topic.empty()) {
    auto& topics = index["by_topic"];
    if (topics[e.topic].is_null()) topics[e.topic] = nlohmann::json::array();
    topics[e.topic].push_back(e.id);
  }

  // by_type
  auto& types = index["by_type"];
  if (types[e.type].is_null()) types[e.type] = nlohmann::json::array();
  types[e.type].push_back(e.id);

  // by_component
  for (const auto& comp : e.related_components) {
    auto& comps = index["by_component"];
    if (comps[comp].is_null()) comps[comp] = nlohmann::json::array();
    comps[comp].push_back(e.id);
  }

  save_index_(index);

  // 更新关键词索引
  auto keywords = load_keywords_();
  for (const auto& kw : e.keywords) {
    std::string lower;
    for (char c : kw) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (keywords[lower].is_null()) keywords[lower] = nlohmann::json::array();
    keywords[lower].push_back(e.id);
  }
  save_keywords_(keywords);

  // 追加时间线
  append_timeline_(e.id, e);

  return e.id;
}

// ═══════════════════════════════════════════════════════════════════
// search
// ═══════════════════════════════════════════════════════════════════

auto MemoryIndex::search(const std::string& query, int limit) noexcept
    -> std::vector<MemoryEntry> {
  if (query.empty()) return recent(limit);

  std::string q;
  for (char c : query) q += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

  // 读取索引文件为字符串，做子串匹配 (避免 json.hpp 对象迭代限制)
  std::string index_str, kw_str;
  {
    std::ifstream ifs(index_path_());
    if (ifs.is_open()) {
      std::ostringstream ss; ss << ifs.rdbuf();
      index_str = ss.str();
    }
  }
  {
    std::ifstream ifs(kw_path_());
    if (ifs.is_open()) {
      std::ostringstream ss; ss << ifs.rdbuf();
      kw_str = ss.str();
    }
  }

  // 在关键字索引中搜索匹配的 ID
  std::map<std::string, int> candidate_scores;

  // 简化: 使用 timeline 搜索 (始终可用)
  std::ifstream tfs(timeline_path_());
  if (tfs.is_open()) {
    std::string line;
    while (std::getline(tfs, line)) {
      // 行级子串匹配
      std::string lower;
      for (char c : line) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      if (lower.find(q) != std::string::npos) {
        try {
          auto j = nlohmann::json::parse(line);
          std::string id = j.value("id", std::string(""));
          if (!id.empty()) candidate_scores[id]++;
        } catch (...) {}
      }
    }
  }

  if (candidate_scores.empty()) return recent(std::min(limit, 5));

  // 按得分排序
  std::vector<std::pair<int, std::string>> sorted;
  for (const auto& [id, score] : candidate_scores)
    sorted.emplace_back(score, id);
  std::sort(sorted.begin(), sorted.end(),
    [](const auto& a, const auto& b) { return a.first > b.first; });

  std::vector<MemoryEntry> results;
  for (const auto& [score, id] : sorted) {
    auto entry = retrieve(id);
    if (!entry.id.empty()) {
      results.push_back(entry);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }
  return results;
}

// ═══════════════════════════════════════════════════════════════════
// retrieve / recent
// ═══════════════════════════════════════════════════════════════════

auto MemoryIndex::retrieve(const std::string& mem_id) noexcept -> MemoryEntry {
  auto path = entries_dir_() + "/" + mem_id + ".json";
  auto j = read_json_file(path);
  if (!j.is_object()) return MemoryEntry{};
  return MemoryEntry::from_json(j);
}

auto MemoryIndex::recent(int limit) noexcept -> std::vector<MemoryEntry> {
  // 从时间线读取最新的条目
  std::ifstream ifs(timeline_path_());
  if (!ifs.is_open()) return {};

  std::vector<std::string> ids;
  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty()) continue;
    try {
      auto j = nlohmann::json::parse(line);
      ids.push_back(j.value("id", std::string("")));
    } catch (...) {}
  }

  // 取最后 limit 条
  std::vector<MemoryEntry> results;
  int start = std::max(0, static_cast<int>(ids.size()) - limit);
  for (int i = start; i < static_cast<int>(ids.size()); ++i) {
    auto entry = retrieve(ids[i]);
    if (!entry.id.empty()) results.push_back(entry);
  }

  return results;
}

// ═══════════════════════════════════════════════════════════════════
// stats
// ═══════════════════════════════════════════════════════════════════

auto MemoryIndex::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["next_id"] = next_id_;

  int entry_count = 0;
  try {
    for (auto& p : fs::directory_iterator(entries_dir_())) {
      if (p.path().extension() == ".json") entry_count++;
    }
  } catch (...) {}
  j["entry_count"] = entry_count;
  j["topic_count"] = 0;  // counted from directory
  j["data_dir"] = data_dir_ + "/memory";
  return j;
}

}  // namespace nexus::core
