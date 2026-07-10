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
