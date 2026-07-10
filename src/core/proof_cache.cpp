// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/core/proof_cache.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto TheoremEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]    = name;
  j["kind"]    = kind;
  j["module"]  = module;
  j["file"]    = file;
  j["constructive"] = constructive;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 加载
// ═══════════════════════════════════════════════════════════════════

auto ProofCache::load(const std::string& path) noexcept -> Status {
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    return Status::Error(ErrorCode::kFileNotFound,
      "proof index not found: " + path);
  }

  try {
    nlohmann::json data;
    ifs >> data;

    auto& coq_lib = data["libraries"]["coq"];
    auto& files = coq_lib["files"];

    for (const auto& f : files) {
      for (const auto& t : f["theorems"]) {
        TheoremEntry entry;
        entry.name    = t["name"].get<std::string>();
        entry.kind    = t["kind"].get<std::string>();
        entry.module  = t["module"].get<std::string>();
        entry.file    = t["file"].get<std::string>();
        entry.constructive = f["constructive"].get<bool>();
        theorems_.push_back(entry);
      }
    }

    // 经典文件中的定理
    auto& classical_files = coq_lib["classical_files"];
    for (const auto& f : classical_files) {
      for (const auto& t : f["theorems"]) {
        TheoremEntry entry;
        entry.name    = t["name"].get<std::string>();
        entry.kind    = t["kind"].get<std::string>();
        entry.module  = t["module"].get<std::string>();
        entry.file    = t["file"].get<std::string>();
        entry.constructive = false;
        classical_theorems_.push_back(entry);
      }
    }

    total_files_ = data["stats"]["total_files"].get<int>();
    constructive_files_ = data["stats"]["constructive_files"].get<int>();

    return Status::Ok();

  } catch (const std::exception& e) {
    return Status::Error(ErrorCode::kJsonParseError,
      "proof index parse error: " + std::string(e.what()));
  }
}

// ═══════════════════════════════════════════════════════════════════
// 搜索
// ═══════════════════════════════════════════════════════════════════

auto ProofCache::search(const std::string& keyword, int limit) const noexcept
    -> std::vector<TheoremEntry> {
  std::vector<TheoremEntry> results;

  for (const auto& t : theorems_) {
    if (t.name.find(keyword) != std::string::npos ||
        t.module.find(keyword) != std::string::npos) {
      results.push_back(t);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }

  // 如果构造性不够, 补充经典定理
  if (static_cast<int>(results.size()) < limit) {
    for (const auto& t : classical_theorems_) {
      if (t.name.find(keyword) != std::string::npos) {
        results.push_back(t);
        if (static_cast<int>(results.size()) >= limit) break;
      }
    }
  }

  return results;
}

auto ProofCache::search_by_domain(const std::string& domain, int limit) const noexcept
    -> std::vector<TheoremEntry> {
  std::vector<TheoremEntry> results;

  for (const auto& t : theorems_) {
    if (t.module.find(domain) != std::string::npos) {
      results.push_back(t);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }

  return results;
}

auto ProofCache::search_by_module(const std::string& module_name, int limit) const noexcept
    -> std::vector<TheoremEntry> {
  std::vector<TheoremEntry> results;

  for (const auto& t : theorems_) {
    if (t.module == module_name) {
      results.push_back(t);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }

  return results;
}

// ═══════════════════════════════════════════════════════════════════
// 统计 & 种子注入
// ═══════════════════════════════════════════════════════════════════

auto ProofCache::constructive_count() const noexcept -> int {
  return static_cast<int>(theorems_.size());
}

auto ProofCache::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["constructive_theorems"] = static_cast<int>(theorems_.size());
  j["classical_theorems"]    = static_cast<int>(classical_theorems_.size());
  j["total_files"]    = total_files_;
  j["constructive_files"] = constructive_files_;
  return j;
}

auto ProofCache::inject_to_seed_bank(bridge::SeedBank* bank, int limit) noexcept
    -> int {
  if (!bank) return 0;

  int count = 0;
  for (const auto& t : theorems_) {
    if (count >= limit) break;

    bridge::SeedEntry seed;
    seed.name        = t.name;
    seed.intensity   = 5;
    seed.source      = "proof_library:coq";
    seed.type        = "formal_theorem";
    seed.domain_tag  = t.module.substr(0, t.module.find('.'));
    seed.step_detail = t.module + ":" + t.kind;

    bank->inject(seed, {seed.domain_tag});
    count++;
  }

  return count;
}

}  // namespace nexus::core
