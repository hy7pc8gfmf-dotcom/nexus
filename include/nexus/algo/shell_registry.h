#ifndef NEXUS_ALGO_SHELL_REGISTRY_H
#define NEXUS_ALGO_SHELL_REGISTRY_H

/**
 * @file shell_registry.h
 * @brief Shell A/B/C 注册表 — 三壳模块注册与能力索引
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::algo {

struct ShellModule {
  std::string name;
  std::string shell;        // "A" | "B" | "C"
  std::string type;         // "algorithm" | "model" | "cloud"
  bool registered = false;

  auto to_json() const -> nlohmann::json;
};

class ShellRegistry {
public:
  ShellRegistry() noexcept;

  auto register_module(const std::string& name, const std::string& shell,
                       const std::string& type) noexcept -> void;
  [[nodiscard]] auto list(const std::string& shell) const noexcept -> std::vector<ShellModule>;
  [[nodiscard]] auto all() const noexcept -> std::vector<ShellModule>;
  [[nodiscard]] auto summary() const noexcept -> nlohmann::json;

private:
  std::vector<ShellModule> modules_;
};

}  // namespace nexus::algo

#endif
