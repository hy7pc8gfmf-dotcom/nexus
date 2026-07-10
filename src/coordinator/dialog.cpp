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
 * @file dialog.cpp
 * @brief 对话框主导权接管实现
 */

#include "nexus/coordinator/dialog.h"

namespace nexus::coordinator {

auto DialogRoute::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["routed_count"] = routed_count; j["local_count"] = local_count;
  j["preferred"] = preferred;
  return j;
}

auto DialogRouter::route(const std::string& task_type) noexcept -> std::string {
  routed_++;
  if (task_type == "reasoning" || task_type == "dialectic") {
    local_++;
    return "local";  // 本地模型优先
  }
  return "cloud";
}

auto DialogRouter::stats() const noexcept -> DialogRoute {
  DialogRoute r; r.routed_count = routed_; r.local_count = local_; return r;
}

void DialogRouter::reset() noexcept { routed_ = 0; local_ = 0; }

}  // namespace nexus::coordinator
