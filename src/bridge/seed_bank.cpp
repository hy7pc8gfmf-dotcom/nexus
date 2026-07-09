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

      // 构建域索引
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
