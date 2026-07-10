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

#ifndef NEXUS_CORE_NATIVE_BRIDGE_H
#define NEXUS_CORE_NATIVE_BRIDGE_H

/**
 * @file native_bridge.h
 * @brief 原生推理桥接 — 7 项原生推理能力
 *
 * 将 C++ 原生推理能力注册到神经系统。
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::core {

struct NativeCapability {
  std::string name;
  std::string description;
  bool available = true;

  auto to_json() const -> nlohmann::json;
};

class NativeBridge {
public:
  NativeBridge() noexcept;

  [[nodiscard]] auto capabilities() const noexcept -> std::vector<NativeCapability>;
  [[nodiscard]] auto summary() const noexcept -> nlohmann::json;

private:
  std::vector<NativeCapability> caps_;
};

}  // namespace nexus::core

#endif
