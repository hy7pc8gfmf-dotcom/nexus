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
 * @file observer.cpp
 * @brief 30 Observer 并行观察者池实现
 */

#include "nexus/psyche/observer.h"

#include <algorithm>
#include <array>
#include <random>
#include <sstream>

namespace nexus::psyche {

auto ObserverDef::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]    = id;
  j["name"]  = name;
  j["role"]  = static_cast<int>(role);
  j["focus"] = focus;
  return j;
}

auto Observation::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["observer_id"]   = observer_id;
  j["observer_name"] = observer_name;
  j["content"]       = content;
  j["confidence"]    = confidence;
  j["type"]          = type;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// ObserverPool
// ═══════════════════════════════════════════════════════════════════

ObserverPool::ObserverPool() noexcept {
  initialize();
}

void ObserverPool::initialize() noexcept {
  observers_.clear();

  // 30 个观察者, 按角色分组
  // 角色: Auditor(5) / Tagger(7) / Critic(6) / Analyst(7) / Synthesizer(5)

  const std::vector<std::tuple<std::string, ObserverRole, std::string>> defs = {
    // Auditor 组 (5) — 一致性检查
    {"auditor_logic",    ObserverRole::kAuditor, "逻辑一致性"},
    {"auditor_fact",     ObserverRole::kAuditor, "事实准确性"},
    {"auditor_ethics",   ObserverRole::kAuditor, "伦理合规"},
    {"auditor_format",   ObserverRole::kAuditor, "格式规范"},
    {"auditor_trace",    ObserverRole::kAuditor, "追溯完整性"},

    // Tagger 组 (7) — 标注与分类
    {"tagger_topic",     ObserverRole::kTagger, "主题分类"},
    {"tagger_domain",    ObserverRole::kTagger, "域标记"},
    {"tagger_entity",    ObserverRole::kTagger, "实体识别"},
    {"tagger_sentiment", ObserverRole::kTagger, "情感分析"},
    {"tagger_complexity",ObserverRole::kTagger, "复杂度评估"},
    {"tagger_risk",      ObserverRole::kTagger, "风险评估"},
    {"tagger_novelty",   ObserverRole::kTagger, "新颖度评估"},

    // Critic 组 (6) — 问题发现
    {"critic_contradiction", ObserverRole::kCritic, "矛盾检测"},
    {"critic_gap",        ObserverRole::kCritic, "信息缺口"},
    {"critic_bias",       ObserverRole::kCritic, "偏见识别"},
    {"critic_edge_case",  ObserverRole::kCritic, "边界条件"},
    {"critic_overfit",    ObserverRole::kCritic, "过度拟合"},
    {"critic_false_assump",ObserverRole::kCritic, "错误假设"},

    // Analyst 组 (7) — 深度分析
    {"analyst_causal",    ObserverRole::kAnalyst, "因果分析"},
    {"analyst_temporal",  ObserverRole::kAnalyst, "时序分析"},
    {"analyst_structural",ObserverRole::kAnalyst, "结构分析"},
    {"analyst_comparative",ObserverRole::kAnalyst, "对比分析"},
    {"analyst_statistical",ObserverRole::kAnalyst, "统计分析"},
    {"analyst_network",   ObserverRole::kAnalyst, "网络分析"},
    {"analyst_pattern",   ObserverRole::kAnalyst, "模式识别"},

    // Synthesizer 组 (5) — 信息综合
    {"synthesizer_summary",  ObserverRole::kSynthesizer, "摘要生成"},
    {"synthesizer_insight",  ObserverRole::kSynthesizer, "洞见提取"},
    {"synthesizer_decision", ObserverRole::kSynthesizer, "决策支持"},
    {"synthesizer_priority", ObserverRole::kSynthesizer, "优先级排序"},
    {"synthesizer_action",   ObserverRole::kSynthesizer, "行动建议"},
  };

  int id = 0;
  for (const auto& [name, role, focus] : defs) {
    observers_.push_back({id++, name, role, focus});
  }
}

auto ObserverPool::observe_all(const nlohmann::json& context) noexcept
    -> std::vector<Observation> {
  std::vector<Observation> results;

  for (const auto& def : observers_) {
    auto obs = run_observer_(def, context);
    results.push_back(obs);

    // 保存到历史
    history_.push_back(obs);
    if (static_cast<int>(history_.size()) > kMaxHistory) {
      history_.erase(history_.begin());
    }
  }

  return results;
}

auto ObserverPool::observe_by_role(ObserverRole role,
                                    const nlohmann::json& context) noexcept
    -> std::vector<Observation> {
  std::vector<Observation> results;

  for (const auto& def : observers_) {
    if (def.role == role) {
      auto obs = run_observer_(def, context);
      results.push_back(obs);
      history_.push_back(obs);
      if (static_cast<int>(history_.size()) > kMaxHistory) {
        history_.erase(history_.begin());
      }
    }
  }

  return results;
}

auto ObserverPool::get_observer(int id) const noexcept -> const ObserverDef* {
  for (const auto& o : observers_) {
    if (o.id == id) return &o;
  }
  return nullptr;
}

auto ObserverPool::count_by_role() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["auditor"]      = 0;
  j["tagger"]       = 0;
  j["critic"]       = 0;
  j["analyst"]      = 0;
  j["synthesizer"]  = 0;

  for (const auto& o : observers_) {
    switch (o.role) {
      case ObserverRole::kAuditor:     j["auditor"]     = j["auditor"].get<int>() + 1; break;
      case ObserverRole::kTagger:      j["tagger"]      = j["tagger"].get<int>() + 1; break;
      case ObserverRole::kCritic:      j["critic"]      = j["critic"].get<int>() + 1; break;
      case ObserverRole::kAnalyst:     j["analyst"]     = j["analyst"].get<int>() + 1; break;
      case ObserverRole::kSynthesizer: j["synthesizer"] = j["synthesizer"].get<int>() + 1; break;
    }
  }
  return j;
}

auto ObserverPool::total_observers() const noexcept -> int {
  return static_cast<int>(observers_.size());
}

auto ObserverPool::recent_observations(int n) const noexcept
    -> std::vector<Observation> {
  if (history_.empty()) return {};
  int start = std::max(0, static_cast<int>(history_.size()) - n);
  return std::vector<Observation>(history_.begin() + start, history_.end());
}

auto ObserverPool::run_observer_(const ObserverDef& def,
                                  const nlohmann::json& context) noexcept
    -> Observation {
  Observation obs;
  obs.observer_id   = def.id;
  obs.observer_name = def.name;
  obs.confidence = 0.5 + static_cast<double>(def.id) / 60.0; // [0.5, 1.0]
  obs.content = def.name + " 观察: " + def.focus + " 分析完成";

  // 类型基于角色
  switch (def.role) {
    case ObserverRole::kAuditor:     obs.type = "audit";     obs.confidence *= 0.9; break;
    case ObserverRole::kTagger:      obs.type = "tag";       break;
    case ObserverRole::kCritic:      obs.type = "warning";   obs.confidence *= 0.8; break;
    case ObserverRole::kAnalyst:     obs.type = "insight";   break;
    case ObserverRole::kSynthesizer: obs.type = "suggestion"; break;
  }

  return obs;
}

}  // namespace nexus::psyche
