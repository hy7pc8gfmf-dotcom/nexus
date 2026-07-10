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

/**
 * @file seed_bank.cpp
 * @brief 种子库实现 — 读取 JSONL 格式种子库
 */

#include "nexus/bridge/seed_bank.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace nexus::bridge {

namespace fs = std::filesystem;

auto SeedEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"] = name; j["intensity"] = intensity; j["source"] = source;
  j["type"] = type; j["domain_tag"] = domain_tag; j["step_detail"] = step_detail;
  return j;
}

auto DomainEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"] = name; j["confidence"] = confidence; j["source"] = source;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// SeedBank
// ═══════════════════════════════════════════════════════════════════

SeedBank::SeedBank(std::string data_dir) noexcept
  : data_dir_(std::move(data_dir)) {}

auto SeedBank::load(const std::string& path) noexcept -> Status {
  std::string load_path = path.empty() ? data_dir_ + "/seed_bank.jsonl" : path;

  if (!fs::exists(load_path)) {
    return Status::Error(ErrorCode::kFileNotFound,
      "seed bank not found: " + load_path);
  }

  // 检查文件格式: 以 '{' 开头的是统一种子库 (JSON 对象), 非 JSONL
  {
    std::ifstream probe(load_path);
    char first_char = 0;
    probe >> first_char;
    if (first_char == '{') {
      return load_unified(load_path);
    }
  }

  seeds_.clear();
  domain_index_.clear();

  std::ifstream ifs(load_path);
  if (!ifs.is_open()) {
    return Status::Error(ErrorCode::kIoError,
      "cannot open seed bank: " + load_path);
  }

  std::string line;
  int line_num = 0;
  while (std::getline(ifs, line)) {
    line_num++;
    if (line.empty()) continue;

    try {
      auto j = nlohmann::json::parse(line);

      SeedEntry seed;
      seed.name        = j.value("name", "");
      seed.intensity   = j.value("intensity", 0);
      seed.source      = j.value("source", "");
      seed.type        = j.value("type", "");
      seed.domain_tag  = j.value("domain_tag", "");
      seed.step_detail = j.value("step_detail", "");

      seeds_[seed.name] = seed;

      auto domains = j.value("domains", std::vector<std::string>());
      for (const auto& d : domains) {
        DomainEntry de;
        de.name = seed.name;
        de.confidence = 0.5;
        domain_index_[d].push_back(de);
      }
    } catch (const std::exception& e) {
      return Status::Error(ErrorCode::kJsonParseError,
        "line " + std::to_string(line_num) + ": " + e.what());
    }
  }

  total_ = static_cast<int>(seeds_.size());
  return Status::Ok();
}

auto SeedBank::load_unified(const std::string& path) noexcept -> Status {
  if (!fs::exists(path)) {
    return Status::Error(ErrorCode::kFileNotFound,
      "unified seed bank not found: " + path);
  }

  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    return Status::Error(ErrorCode::kIoError,
      "cannot open unified seed bank: " + path);
  }

  try {
    nlohmann::json data;
    ifs >> data;

    seeds_.clear();
    domain_index_.clear();

    // 加载种子 — 通过 dump + json::parse 单个值的方式遍历对象
    // (最小化 json 不支持对象迭代器)
    auto seeds_obj = data["seeds"];
    if (seeds_obj.is_object()) {
      auto raw = seeds_obj.dump();
      size_t pos = 0;
      while (pos < raw.size() && raw[pos] != '{') ++pos;
      if (pos < raw.size()) ++pos; // skip '{'

      while (pos < raw.size() && raw[pos] != '}') {
        // 跳过空白
        while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == '\n' || raw[pos] == '\t' || raw[pos] == '\r')) ++pos;
        if (pos >= raw.size() || raw[pos] == '}') break;
        if (raw[pos] != '"') { ++pos; continue; }

        // 读键名
        ++pos; // skip '"'
        std::string key;
        while (pos < raw.size() && raw[pos] != '"') {
          if (raw[pos] == '\\') { ++pos; if (pos < raw.size()) key += raw[pos]; }
          else key += raw[pos];
          ++pos;
        }
        if (pos >= raw.size()) break;
        ++pos; // skip closing '"'

        // 跳过 ':'
        while (pos < raw.size() && raw[pos] != ':') ++pos;
        if (pos < raw.size()) ++pos;

        // 提取值
        size_t val_start = pos;
        int depth = 0;
        while (pos < raw.size()) {
          if (raw[pos] == '{' || raw[pos] == '[') depth++;
          else if (raw[pos] == '}' || raw[pos] == ']') { depth--; if (depth < 0) break; }
          else if (raw[pos] == '"') {
            ++pos;
            while (pos < raw.size() && !(raw[pos] == '"' && raw[pos-1] != '\\')) ++pos;
          }
          ++pos;
          if (depth <= 0 && (raw[pos] == ',' || raw[pos] == '}')) break;
        }

        auto val = nlohmann::json::parse(raw.substr(val_start, pos - val_start));
        SeedEntry seed;
        seed.name        = val.value("name", key);
        seed.intensity   = val.value("intensity", 0);
        seed.source      = val.value("source", "");
        seed.type        = val.value("type", "");
        seed.domain_tag  = val.value("domain_tag", "");
        seed.step_detail = val.value("step_detail", "");
        seeds_[seed.name] = seed;

        if (pos < raw.size() && raw[pos] == ',') ++pos;
      }
    }

    // 从 domain_tag 重建域索引 (最小化 json 无对象迭代器)
    for (auto& [name, seed] : seeds_) {
      if (!seed.domain_tag.empty()) {
        DomainEntry de;
        de.name = seed.name;
        de.confidence = 0.5;
        domain_index_[seed.domain_tag].push_back(de);
      }
    }

    // 如果 domain_tag 为空, 尝试从 step_detail 提取域信息
    if (domain_index_.empty()) {
      for (auto& [name, seed] : seeds_) {
        if (!seed.step_detail.empty()) {
          std::string domain = seed.step_detail.substr(0, seed.step_detail.find(':'));
          if (!domain.empty() && domain != "unknown") {
            DomainEntry de;
            de.name = seed.name;
            de.confidence = 0.5;
            domain_index_[domain].push_back(de);
          }
        }
      }
    }

    total_ = static_cast<int>(seeds_.size());
    dimension_ = data.value("metadata", nlohmann::json::object())
                   .value("dimension", 14);
    return Status::Ok();

  } catch (const std::exception& e) {
    return Status::Error(ErrorCode::kJsonParseError,
      "unified seed bank parse error: " + std::string(e.what()));
  }
}

auto SeedBank::inject(const SeedEntry& seed,
                       const std::vector<std::string>& domains) noexcept -> Status {
  seeds_[seed.name] = seed;

  for (const auto& d : domains) {
    DomainEntry de;
    de.name = seed.name;
    de.confidence = 0.5;
    domain_index_[d].push_back(de);
  }

  total_ = static_cast<int>(seeds_.size());
  return Status::Ok();
}

auto SeedBank::query_by_domain(const std::string& domain, int limit) const noexcept
    -> std::vector<SeedEntry> {
  std::vector<SeedEntry> results;
  auto it = domain_index_.find(domain);
  if (it == domain_index_.end()) return results;

  for (const auto& de : it->second) {
    auto sit = seeds_.find(de.name);
    if (sit != seeds_.end()) {
      results.push_back(sit->second);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }

  return results;
}

auto SeedBank::query_by_intensity(int min_intensity, int limit) const noexcept
    -> std::vector<SeedEntry> {
  std::vector<SeedEntry> results;
  for (const auto& [name, seed] : seeds_) {
    if (seed.intensity >= min_intensity) {
      results.push_back(seed);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }
  // 按强度降序排序
  std::sort(results.begin(), results.end(),
    [](const SeedEntry& a, const SeedEntry& b) {
      return a.intensity > b.intensity;
    });
  return results;
}

auto SeedBank::search(const std::string& keyword, int limit) const noexcept
    -> std::vector<SeedEntry> {
  std::vector<SeedEntry> results;
  for (const auto& [name, seed] : seeds_) {
    if (name.find(keyword) != std::string::npos ||
        seed.domain_tag.find(keyword) != std::string::npos) {
      results.push_back(seed);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }
  return results;
}

auto SeedBank::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_seeds"] = total_;
  j["dimension"]   = dimension_;
  j["domains"]     = static_cast<int>(domain_index_.size());

  // 域分布
  auto domain_counts = nlohmann::json::object();
  for (const auto& [domain, entries] : domain_index_) {
    domain_counts[domain] = static_cast<int>(entries.size());
  }
  j["domain_counts"] = domain_counts;
  return j;
}

auto SeedBank::domains() const noexcept -> std::vector<std::string> {
  std::vector<std::string> result;
  for (const auto& [domain, _] : domain_index_) {
    result.push_back(domain);
  }
  std::sort(result.begin(), result.end());
  return result;
}

auto SeedBank::persist() noexcept -> Status {
  return Status::Ok(); // 数据已由 port_seed_bank.py 写入
}

}  // namespace nexus::bridge
