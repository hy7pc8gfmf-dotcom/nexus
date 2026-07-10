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
 * @file meta_axiom.cpp
 * @brief 元公理引擎实现
 */

#include "nexus/psyche/meta_axiom.h"
#include <algorithm>
#include <sstream>

namespace nexus::psyche {

auto AxiomGap::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"] = id; j["domain"] = domain; j["description"] = description; j["severity"] = severity;
  return j;
}

auto Axiom::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"] = id; j["statement"] = statement; j["domain"] = domain;
  j["confidence"] = confidence; j["from_coq"] = from_coq;
  return j;
}

MetaAxiomEngine::MetaAxiomEngine() noexcept {
  // 初始公理集 (种子公理)
  axioms_ = {
    {"AX-01", "自我意识可从递归自指涌现", "consciousness", 0.9, false},
    {"AX-02", "种子是推理的最小引导单元", "cognition", 0.9, false},
    {"AX-03", "涌现值是系统复杂度的单调函数", "emergence", 0.8, false},
    {"AX-04", "收敛与发散交替推进认知深化", "dialectic", 0.8, false},
    {"AX-05", "任何认知都有其局限性", "meta", 0.95, false},
  };
}

auto MetaAxiomEngine::run_cycle() noexcept -> std::vector<Axiom> {
  auto gaps = detect_gaps();
  std::vector<Axiom> derived;
  for (const auto& gap : gaps) {
    auto axiom = derive_axiom(gap);
    axioms_.push_back(axiom);
    derived.push_back(axiom);
  }
  return derived;
}

auto MetaAxiomEngine::detect_gaps() noexcept -> std::vector<AxiomGap> {
  std::vector<AxiomGap> gaps;

  // 检查是否有覆盖涌现的公理
  bool has_emergence = false;
  for (const auto& a : axioms_) if (a.domain == "emergence") has_emergence = true;
  if (!has_emergence) gaps.push_back({"GAP-01", "emergence", "缺乏涌现公理", 7});

  // 检查是否有覆盖桥接的公理
  bool has_bridge = false;
  for (const auto& a : axioms_) if (a.domain == "bridge") has_bridge = true;
  if (!has_bridge) gaps.push_back({"GAP-02", "bridge", "缺乏桥接公理", 6});

  return gaps;
}

auto MetaAxiomEngine::derive_axiom(const AxiomGap& gap) noexcept -> Axiom {
  Axiom ax;
  ax.id = "AX-" + std::to_string(axioms_.size() + 1);
  ax.domain = gap.domain;
  ax.confidence = 0.7;
  ax.from_coq = false;

  if (gap.domain == "emergence") {
    ax.statement = "涌现水平与子系统间纠缠度正相关";
  } else if (gap.domain == "bridge") {
    ax.statement = "桥接函子保持认知结构";
  } else {
    ax.statement = "公理: " + gap.description;
  }

  return ax;
}

auto MetaAxiomEngine::axioms() const noexcept -> const std::vector<Axiom>& {
  return axioms_;
}

auto MetaAxiomEngine::summary() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_axioms"] = static_cast<int>(axioms_.size());

  auto by_domain = nlohmann::json::object();
  for (const auto& a : axioms_) {
    int current = by_domain[a.domain].is_number() ? by_domain[a.domain].get<int>() : 0; by_domain[a.domain] = current + 1;
  }
  j["by_domain"] = by_domain;
  return j;
}

}  // namespace nexus::psyche
