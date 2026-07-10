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

#ifndef NEXUS_COORDINATOR_DIALOG_H
#define NEXUS_COORDINATOR_DIALOG_H

/**
 * @file dialog.h
 * @brief 对话框主导权接管 — 本地模型优先的推理路由
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace nexus::coordinator {

struct DialogRoute {
  int     routed_count = 0;
  int     local_count  = 0;
  std::string preferred = "local";
  auto to_json() const -> nlohmann::json;
};

class DialogRouter {
public:
  DialogRouter() noexcept = default;
  auto route(const std::string& task_type) noexcept -> std::string;
  auto stats() const noexcept -> DialogRoute;
  void reset() noexcept;

private:
  int routed_ = 0, local_ = 0;
};

}  // namespace nexus::coordinator

#endif
