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
 * @file native_bridge.cpp
 * @brief 原生推理桥接实现
 */

#include "nexus/core/native_bridge.h"

namespace nexus::core {

auto NativeCapability::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"] = name; j["description"] = description; j["available"] = available;
  return j;
}

NativeBridge::NativeBridge() noexcept {
  caps_ = {
    {"tokenize", "文本 → Token 序列转换", true},
    {"decode", "Token 序列 → 文本转换", true},
    {"embed", "文本向量化 (embedding)", true},
    {"classify", "文本分类", true},
    {"compare", "文本相似度比较", true},
    {"truncate", "文本截断到指定长度", true},
    {"detect_lang", "语言检测", true},
  };
}

auto NativeBridge::capabilities() const noexcept -> std::vector<NativeCapability> {
  return caps_;
}

auto NativeBridge::summary() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total"] = static_cast<int>(caps_.size());
  auto list = nlohmann::json::array();
  for (const auto& c : caps_) list.push_back(c.to_json());
  j["capabilities"] = list;
  return j;
}

}  // namespace nexus::core
