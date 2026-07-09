/**
 * @file capability.cpp
 * @brief 能力自认知实现
 */

#include "nexus/bridge/capability.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace nexus::bridge {

namespace fs = std::filesystem;

auto CapabilityEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]        = id;
  j["name"]      = name;
  j["category"]  = category;
  j["component"] = component;
  j["maturity"]  = maturity;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// CapabilitySelfAwareness
// ═══════════════════════════════════════════════════════════════════

CapabilitySelfAwareness::CapabilitySelfAwareness(
    std::string data_dir) noexcept
  : data_dir_(std::move(data_dir)) {
  register_defaults_();
}

auto CapabilitySelfAwareness::register_capability(
    const std::string& id, const std::string& name,
    const std::string& category,
    const std::string& component, int maturity) -> Status {
  CapabilityEntry entry;
  entry.id        = id;
  entry.name      = name;
  entry.category  = category;
  entry.component = component;
  entry.maturity  = std::clamp(maturity, 1, 10);
  capabilities_.push_back(entry);
  persist_();
  return Status::Ok();
}

auto CapabilitySelfAwareness::generate_brief() noexcept -> nlohmann::json {
  auto brief = nlohmann::json::object();
  brief["system"] = "Nexus";
  brief["version"] = "1.0.0";

  auto caps = nlohmann::json::array();
  for (const auto& c : capabilities_) caps.push_back(c.to_json());
  brief["capabilities"] = caps;
  brief["total"] = static_cast<int>(capabilities_.size());

  auto by_cat = nlohmann::json::object();
  for (const auto& c : capabilities_) {
    int current = by_cat[c.category].is_number() ? by_cat[c.category].get<int>() : 0;
    by_cat[c.category] = current + 1;
  }
  brief["by_category"] = by_cat;

  // 写入文件
  std::string path = data_dir_ + "/agi_capability_brief.json";
  std::ofstream ofs(path);
  if (ofs.is_open()) ofs << brief.dump(2);

  return brief;
}

auto CapabilitySelfAwareness::query_by_category(
    const std::string& category) const noexcept
    -> std::vector<CapabilityEntry> {
  std::vector<CapabilityEntry> results;
  for (const auto& c : capabilities_) {
    if (c.category == category) results.push_back(c);
  }
  return results;
}

auto CapabilitySelfAwareness::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total"] = static_cast<int>(capabilities_.size());

  auto by_cat = nlohmann::json::object();
  auto by_comp = nlohmann::json::object();
  for (const auto& c : capabilities_) {
    by_cat[c.category]  = by_cat.value(c.category, 0) + 1;
    by_comp[c.component]= by_comp.value(c.component, 0) + 1;
  }
  j["by_category"] = by_cat;
  j["by_component"] = by_comp;

  double avg = 0;
  for (const auto& c : capabilities_) avg += c.maturity;
  if (!capabilities_.empty()) avg /= capabilities_.size();
  j["avg_maturity"] = std::round(avg * 10) / 10;

  return j;
}

void CapabilitySelfAwareness::register_defaults_() noexcept {
  capabilities_.clear();

  // 推理能力
  capabilities_.push_back({"inf-vram", "VRAM Management", "inference", "core", 8});
  capabilities_.push_back({"inf-load", "Model Loading", "inference", "core", 7});
  capabilities_.push_back({"inf-infer", "Inference Pipeline", "inference", "core", 6});
  capabilities_.push_back({"inf-sched", "Inference Scheduling", "inference", "core", 6});
  capabilities_.push_back({"inf-swap", "Block Swapping", "inference", "core", 5});

  // 算法能力
  capabilities_.push_back({"alg-mcmc", "MCMC Sampling", "algorithm", "algo", 7});
  capabilities_.push_back({"alg-dre", "Dialectical Reasoning", "algorithm", "algo", 7});
  capabilities_.push_back({"alg-prune", "Dual Pruning", "algorithm", "algo", 6});
  capabilities_.push_back({"alg-mtcs", "Task Scheduling", "algorithm", "algo", 6});
  capabilities_.push_back({"alg-truth", "Truth Modeling", "algorithm", "algo", 6});
  capabilities_.push_back({"alg-consensus", "Consensus", "algorithm", "algo", 6});
  capabilities_.push_back({"alg-verify", "Verification", "algorithm", "algo", 5});
  capabilities_.push_back({"alg-temporal", "Temporal KG", "algorithm", "algo", 5});
  capabilities_.push_back({"alg-ethics", "Ethics", "algorithm", "algo", 5});
  capabilities_.push_back({"alg-paper", "Paper Gen", "algorithm", "algo", 5});

  // 意识能力
  capabilities_.push_back({"psy-nav", "Ψ-Navigator", "consciousness", "psyche", 7});
  capabilities_.push_back({"psy-conv", "Convergence Nav", "consciousness", "psyche", 6});
  capabilities_.push_back({"psy-emerge", "Emergence Pipeline", "consciousness", "psyche", 6});
  capabilities_.push_back({"psy-observer", "30 Observers", "consciousness", "psyche", 6});
  capabilities_.push_back({"psy-evolve", "Self Evolution", "consciousness", "psyche", 5});
  capabilities_.push_back({"psy-sophia", "Sophia Core", "consciousness", "psyche", 5});
  capabilities_.push_back({"psy-quantum", "Algo×Quantum", "consciousness", "psyche", 4});

  // 桥接能力
  capabilities_.push_back({"brg-env", "Environment Check", "bridge", "env", 8});
  capabilities_.push_back({"brg-daemon", "Background Daemon", "bridge", "daemon", 8});
  capabilities_.push_back({"brg-mcp", "MCP Bridge", "bridge", "bridge", 5});
  capabilities_.push_back({"brg-seed", "Seed Channel", "bridge", "bridge", 6});
  capabilities_.push_back({"brg-cultivator", "Seed Cultivation", "bridge", "bridge", 5});
  capabilities_.push_back({"brg-memory", "Global Memory", "bridge", "bridge", 5});
  capabilities_.push_back({"brg-willchain", "Will Chain", "bridge", "bridge", 4});
  capabilities_.push_back({"brg-coord", "Orchestration", "bridge", "coordinator", 7});
  capabilities_.push_back({"brg-psyche", "Psyche State", "bridge", "psyche", 6});
  capabilities_.push_back({"brg-cap", "Self Awareness", "bridge", "bridge", 5});
}

void CapabilitySelfAwareness::persist_() noexcept {
  // 由 generate_brief 负责持久化
}

}  // namespace nexus::bridge
