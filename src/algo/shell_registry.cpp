/**
 * @file shell_registry.cpp
 * @brief Shell A/B/C 注册表实现
 */

#include "nexus/algo/shell_registry.h"
#include <algorithm>

namespace nexus::algo {

auto ShellModule::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"] = name; j["shell"] = shell; j["type"] = type; j["registered"] = registered;
  return j;
}

ShellRegistry::ShellRegistry() noexcept {
  // Shell A — 算法引擎
  std::vector<std::pair<std::string, std::string>> algos = {
    {"mcmc","algorithm"},{"dre","algorithm"},{"dual_pruning","algorithm"},
    {"mtcs","algorithm"},{"infinitas_truth","algorithm"},
    {"dialectical_consensus","algorithm"},{"multimodal_verifier","algorithm"},
    {"temporal_kg","algorithm"},{"ethical_evaluation","algorithm"},
    {"paper_generation","algorithm"},
  };
  for (const auto& [n,t] : algos) modules_.push_back({n, "A", t, true});

  // Shell B — 本地模型
  modules_.push_back({"qwythos_9b", "B", "model", true});
  modules_.push_back({"grm2", "B", "model", true});
  modules_.push_back({"vibethinker", "B", "model", true});

  // Shell C — 云端服务
  modules_.push_back({"agi_capability", "C", "cloud", true});
  modules_.push_back({"deepseek", "C", "cloud", false});
  modules_.push_back({"claude", "C", "cloud", false});
}

auto ShellRegistry::register_module(const std::string& name, const std::string& shell,
                                     const std::string& type) noexcept -> void {
  for (auto& m : modules_) { if (m.name == name) { m.registered = true; return; } }
  modules_.push_back({name, shell, type, true});
}

auto ShellRegistry::list(const std::string& shell) const noexcept -> std::vector<ShellModule> {
  std::vector<ShellModule> result;
  for (const auto& m : modules_) if (m.shell == shell) result.push_back(m);
  return result;
}

auto ShellRegistry::all() const noexcept -> std::vector<ShellModule> { return modules_; }

auto ShellRegistry::summary() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["shell_a"] = static_cast<int>(list("A").size());
  j["shell_b"] = static_cast<int>(list("B").size());
  j["shell_c"] = static_cast<int>(list("C").size());
  j["total"] = static_cast<int>(modules_.size());
  return j;
}

}  // namespace nexus::algo
