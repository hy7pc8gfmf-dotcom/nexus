/**
 * @file holographic_core.cpp
 * @brief 全息系统实现
 */

#include "nexus/holographic/holographic_core.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <sstream>

namespace nexus::holographic {

auto Hotspot::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"] = name; j["intensity"] = std::round(intensity * 1000) / 1000;
  j["neighbors"] = neighbors; j["domain"] = domain;
  return j;
}

auto Pathway::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["from"] = from; j["to"] = to;
  j["strength"] = std::round(strength * 1000) / 1000;
  j["bridges"] = bridges;
  return j;
}

auto GeneratedSentence::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["subject"] = subject; j["type"] = template_type;
  j["sentence"] = sentence; j["language"] = language;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 语义场
// ═══════════════════════════════════════════════════════════════════

SemanticField::SemanticField() noexcept { build_default(); }

void SemanticField::build_default() noexcept {
  const std::vector<std::tuple<std::string, double, int, std::string>> defs = {
    {"consciousness", 0.95, 6, "meta"}, {"entropy", 0.88, 4, "physics"},
    {"emergence", 0.92, 5, "meta"},     {"recursion", 0.85, 4, "math"},
    {"symmetry", 0.78, 3, "physics"},   {"information", 0.82, 5, "physics"},
    {"causality", 0.75, 3, "physics"},  {"duality", 0.70, 3, "math"},
    {"coherence", 0.72, 4, "meta"},     {"complexity", 0.80, 4, "meta"},
    {"self", 0.90, 5, "meta"},          {"knowledge", 0.85, 4, "meta"},
    {"truth", 0.88, 4, "logic"},        {"belief", 0.82, 3, "psyche"},
    {"time", 0.76, 3, "physics"},       {"space", 0.74, 3, "physics"},
    {"entropy", 0.88, 4, "physics"},    {"quantum", 0.80, 4, "physics"},
    {"logic", 0.85, 4, "logic"},        {"proof", 0.78, 3, "logic"},
  };

  for (const auto& [name, intensity, neighbors, domain] : defs) {
    hotspots_[name] = {name, intensity, neighbors, domain};
  }

  pathways_ = {
    {"consciousness", "emergence", 0.90, 3}, {"consciousness", "self", 0.85, 2},
    {"consciousness", "entropy", 0.65, 2},   {"emergence", "complexity", 0.80, 2},
    {"emergence", "coherence", 0.75, 2},     {"entropy", "information", 0.88, 3},
    {"entropy", "symmetry", 0.70, 2},        {"recursion", "self", 0.82, 3},
    {"recursion", "proof", 0.72, 2},         {"symmetry", "duality", 0.78, 2},
    {"information", "knowledge", 0.85, 3},   {"causality", "time", 0.80, 2},
    {"truth", "belief", 0.76, 2},            {"knowledge", "truth", 0.80, 3},
    {"complexity", "emergence", 0.82, 3},    {"quantum", "entropy", 0.75, 2},
    {"logic", "proof", 0.85, 2},             {"proof", "truth", 0.80, 2},
  };
}

auto SemanticField::query(const std::string& name) noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  auto it = hotspots_.find(name);
  if (it == hotspots_.end()) { j["found"] = false; return j; }

  j["found"] = true;
  j["hotspot"] = it->second.to_json();

  auto conn = nlohmann::json::array();
  for (const auto& p : pathways_) {
    if (p.from == name || p.to == name) conn.push_back(p.to_json());
  }
  j["connections"] = conn;
  return j;
}

auto SemanticField::neighbors(const std::string& name, int depth) noexcept
    -> std::vector<Hotspot> {
  std::vector<Hotspot> result;
  std::set<std::string> visited;
  std::queue<std::pair<std::string, int>> q;
  q.push({name, 0});
  visited.insert(name);

  while (!q.empty()) {
    auto [current, d] = q.front(); q.pop();
    if (d >= depth) continue;

    for (const auto& p : pathways_) {
      std::string next;
      if (p.from == current) next = p.to;
      else if (p.to == current) next = p.from;
      else continue;

      if (visited.insert(next).second) {
        auto it = hotspots_.find(next);
        if (it != hotspots_.end()) result.push_back(it->second);
        q.push({next, d + 1});
      }
    }
  }
  return result;
}

auto SemanticField::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["hotspots"] = static_cast<int>(hotspots_.size());

  auto by_domain = nlohmann::json::object();
  for (const auto& [n, h] : hotspots_) {
    int current = by_domain[h.domain].is_number() ? by_domain[h.domain].get<int>() : 0;
    by_domain[h.domain] = current + 1;
  }
  j["by_domain"] = by_domain;

  j["pathways"] = static_cast<int>(pathways_.size());
  j["connectivity"] = hotspots_.empty() ? 0 :
    std::round(100.0 * pathways_.size() / (hotspots_.size() * 2)) / 100;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 概念路径引擎
// ═══════════════════════════════════════════════════════════════════

ConceptPathway::ConceptPathway(SemanticField* field) noexcept : field_(field) {}

auto ConceptPathway::explore(const std::string& from, const std::string& to) noexcept
    -> std::vector<Pathway> {
  std::vector<Pathway> result;
  if (!field_) return result;

  for (const auto& p : field_->stats()) {
    (void)p;
  }
  return result;
}

auto ConceptPathway::beam_search(const std::string& start, int width, int depth) noexcept
    -> std::vector<std::vector<Pathway>> {
  std::vector<std::vector<Pathway>> results;
  if (!field_) return results;
  return results;
}

// ═══════════════════════════════════════════════════════════════════
// 流桥接
// ═══════════════════════════════════════════════════════════════════

FlowBridge::FlowBridge(SemanticField* field) noexcept : field_(field) {}

auto FlowBridge::generate(const std::string& topic, const std::string& lang) noexcept
    -> std::vector<GeneratedSentence> {
  std::vector<GeneratedSentence> results;
  if (!field_) return results;

  std::vector<std::string> templates = {"definition", "causal", "relation", "example"};
  for (const auto& t : templates) {
    GeneratedSentence gs;
    gs.subject = topic;
    gs.template_type = t;
    gs.language = lang;

    if (t == "definition")      gs.sentence = topic + " is a " + topic + " topic";
    else if (t == "causal")     gs.sentence = topic + " leads to emergence";
    else if (t == "relation")   gs.sentence = topic + " relates to information";
    else                        gs.sentence = "example: " + topic + " in context";

    results.push_back(gs);
  }
  return results;
}

auto FlowBridge::calibrate(const std::string& source, const std::string& target_lang) noexcept
    -> std::string {
  return source + " [" + target_lang + "]";
}

auto FlowBridge::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["field_loaded"] = field_ != nullptr;
  return j;
}

}  // namespace nexus::holographic
