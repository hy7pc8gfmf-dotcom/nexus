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
 * @file logic_core.cpp
 * @brief 逻辑求解引擎实现
 */

#include "nexus/logic/logic_core.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <sstream>

namespace nexus::logic {

namespace fs = std::filesystem;

auto LogicSeed::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"] = name; j["intensity"] = intensity;
  j["conductance"] = std::round(conductance * 1000) / 1000;
  j["domain"] = domain;
  auto coords = nlohmann::json::array();
  for (auto c : coordinates) coords.push_back(c);
  j["coordinates"] = coords;
  return j;
}

auto ProofStep::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["seed_name"] = seed_name; j["rule"] = rule;
  j["confidence"] = std::round(confidence * 1000) / 1000;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 种子空间
// ═══════════════════════════════════════════════════════════════════

SeedSpace::SeedSpace() noexcept = default;

auto SeedSpace::load(const std::string& path) noexcept -> Status {
  if (!fs::exists(path)) {
    return Status::Error(ErrorCode::kFileNotFound, "seed file: " + path);
  }

  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    return Status::Error(ErrorCode::kIoError, "cannot open: " + path);
  }

  seeds_.clear();
  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty()) continue;
    try {
      auto j = nlohmann::json::parse(line);
      LogicSeed seed;
      seed.name       = j.value("name", "");
      seed.intensity  = j.value("intensity", 0);
      seed.conductance = 0.5;
      seed.domain     = j.value("domain_tag", "");
      if (seed.domain.empty()) {
        auto domains = j.value("domains", std::vector<std::string>());
        if (!domains.empty()) seed.domain = domains[0];
      }
      // 14D 坐标 (占位)
      for (int i = 0; i < 14; ++i) seed.coordinates.push_back(0);
      seeds_.push_back(seed);
    } catch (...) {}
  }

  loaded_ = true;
  return Status::Ok();
}

auto SeedSpace::query(const std::string& domain, int limit) noexcept
    -> std::vector<LogicSeed> {
  std::vector<LogicSeed> results;
  for (const auto& s : seeds_) {
    if (s.domain == domain) results.push_back(s);
    if (static_cast<int>(results.size()) >= limit) break;
  }
  return results;
}

auto SeedSpace::search(const std::string& keyword, int limit) noexcept
    -> std::vector<LogicSeed> {
  std::vector<LogicSeed> results;
  for (const auto& s : seeds_) {
    if (s.name.find(keyword) != std::string::npos ||
        s.domain.find(keyword) != std::string::npos) {
      results.push_back(s);
      if (static_cast<int>(results.size()) >= limit) break;
    }
  }
  return results;
}

auto SeedSpace::conduct(const std::string& from, const std::string& to) noexcept
    -> double {
  for (const auto& s : seeds_) {
    if (s.name == from || s.name == to) return s.conductance;
  }
  return 0.0;
}

auto SeedSpace::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_seeds"] = static_cast<int>(seeds_.size());
  j["loaded"] = loaded_;

  std::map<std::string, int> domain_counts;
  for (const auto& s : seeds_) domain_counts[s.domain]++;
  auto dc = nlohmann::json::object();
  for (const auto& [d, c] : domain_counts) dc[d] = c;
  j["domains"] = dc;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 证明搜索
// ═══════════════════════════════════════════════════════════════════

ProofSearch::ProofSearch(SeedSpace* space) noexcept : space_(space) {}

auto ProofSearch::search(const std::string& goal, int max_depth) noexcept
    -> std::vector<ProofStep> {
  std::vector<ProofStep> proof;

  if (!space_) return proof;

  auto seeds = space_->search(goal, 10);
  if (seeds.empty()) {
    seeds = space_->query("meta", 5);
  }

  for (const auto& s : seeds) {
    ProofStep step;
    step.seed_name  = s.name;
    step.rule       = "seed_match";
    step.confidence = s.conductance;
    proof.push_back(step);
    if (static_cast<int>(proof.size()) >= max_depth) break;
  }

  return proof;
}

auto ProofSearch::validate(const std::vector<ProofStep>& proof) noexcept -> bool {
  return !proof.empty();
}

auto ProofSearch::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["space_loaded"] = space_ != nullptr;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 逻辑推理器
// ═══════════════════════════════════════════════════════════════════

auto LogicReasoner::evaluate(const std::string& proposition) noexcept
    -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["proposition"] = proposition;
  j["length"] = static_cast<int>(proposition.size());
  j["has_negation"] = proposition.find("¬") != std::string::npos ||
                      proposition.find("not") != std::string::npos;
  j["has_conjunction"] = proposition.find("∧") != std::string::npos ||
                         proposition.find("and") != std::string::npos;
  j["has_disjunction"] = proposition.find("∨") != std::string::npos ||
                         proposition.find("or") != std::string::npos;
  j["has_implication"] = proposition.find("→") != std::string::npos ||
                         proposition.find("if") != std::string::npos;
  return j;
}

auto LogicReasoner::entail(const std::string& premise,
                            const std::string& conclusion) noexcept -> bool {
  // 简化: 检查结论关键词是否出现在前提中
  auto words = [](const std::string& s) {
    std::set<std::string> w;
    std::string cur;
    for (char c : s) {
      if (std::isalnum(static_cast<unsigned char>(c))) cur += c;
      else { if (!cur.empty()) { w.insert(cur); cur.clear(); } }
    }
    if (!cur.empty()) w.insert(cur);
    return w;
  };

  auto pw = words(premise), cw = words(conclusion);
  for (const auto& c : cw) {
    if (pw.find(c) != pw.end()) return true;
  }
  return false;
}

auto LogicReasoner::verify(const std::string& formula) noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["formula"] = formula;
  j["well_formed"] = !formula.empty();
  j["has_quantifier"] = formula.find("∀") != std::string::npos ||
                        formula.find("∃") != std::string::npos;
  return j;
}

}  // namespace nexus::logic
