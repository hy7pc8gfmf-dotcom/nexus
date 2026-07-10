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
 * @file scheduler.cpp
 * @brief 推理调度器实现
 */

#include "nexus/core/scheduler.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <sstream>

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// 任务类型工具
// ═══════════════════════════════════════════════════════════════════

auto task_type_to_string(TaskType t) -> const char* {
  switch (t) {
    case TaskType::kReasoning:  return "reasoning";
    case TaskType::kQuickLocal: return "quick_local";
    case TaskType::kTranslate:  return "translate";
    case TaskType::kVision:     return "vision";
    case TaskType::kDialectic:  return "dialectic";
    case TaskType::kDebug:      return "debug";
    case TaskType::kCode:       return "code";
    default: return "unknown";
  }
}

auto parse_task_type(const std::string& s) -> TaskType {
  static const std::map<std::string, TaskType> m = {
    {"reasoning", TaskType::kReasoning},
    {"quick_local", TaskType::kQuickLocal},
    {"translate", TaskType::kTranslate},
    {"vision", TaskType::kVision},
    {"dialectic", TaskType::kDialectic},
    {"debug", TaskType::kDebug},
    {"code", TaskType::kCode},
  };
  auto it = m.find(s);
  return it != m.end() ? it->second : TaskType::kUnknown;
}

auto ScheduleResult::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["model_id"]      = model_id;
  j["strategy"]      = strategy;
  j["n_models_used"] = n_models_used;
  j["confidence"]    = std::round(confidence * 1000) / 1000;
  j["output"]        = output;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 推理调度器
// ═══════════════════════════════════════════════════════════════════

auto InferenceScheduler::register_model(const std::string& model_id,
                                         InferFn infer_fn,
                                         int priority) noexcept -> Status {
  if (!infer_fn) {
    return Status::Error(ErrorCode::kInvalidConfig,
      "null infer_fn for " + model_id);
  }
  models_.push_back({model_id, std::move(infer_fn), priority});
  return Status::Ok();
}

auto InferenceScheduler::infer(const std::string& prompt,
                                TaskType task_type) noexcept
    -> Result<ScheduleResult> {
  if (models_.empty()) {
    return Status::Error(ErrorCode::kModelNotFound,
      "no models registered");
  }

  // 自适应策略选择
  if (task_type == TaskType::kReasoning || task_type == TaskType::kDialectic) {
    // 复杂任务: 合议
    return infer_consensus(prompt);
  }

  // 简单任务: 单模型
  auto* slot = select_model_(task_type);
  if (!slot) {
    // fallback: 选优先级最高的
    slot = &models_[0];
    for (const auto& m : models_) {
      if (m.priority < slot->priority) slot = &m;
    }
  }

  auto result = slot->infer_fn(slot->model_id, prompt);
  if (!result.ok()) {
    return result.error();
  }

  ScheduleResult sr;
  sr.model_id    = slot->model_id;
  sr.strategy    = "single";
  sr.n_models_used = 1;
  sr.confidence = 0.8;
  sr.output     = result.value();
  return sr;
}

auto InferenceScheduler::infer_single(const std::string& model_id,
                                       const std::string& prompt) noexcept
    -> Result<ScheduleResult> {
  for (const auto& m : models_) {
    if (m.model_id == model_id) {
      auto result = m.infer_fn(model_id, prompt);
      if (!result.ok()) return result.error();

      ScheduleResult sr;
      sr.model_id    = model_id;
      sr.strategy    = "single";
      sr.n_models_used = 1;
      sr.confidence = 0.8;
      sr.output     = result.value();
      return sr;
    }
  }
  return Status::Error(ErrorCode::kFileNotFound,
    "model not registered: " + model_id);
}

auto InferenceScheduler::infer_consensus(const std::string& prompt,
                                          const std::vector<std::string>& model_ids) noexcept
    -> Result<ScheduleResult> {
  // 选择参与合议的模型
  std::vector<const ModelSlot*> participants;
  std::vector<ModelSlot> sorted;  // 保持生命周期与 participants 一致
  if (model_ids.empty()) {
    // 默认: 取优先级最高的 2 个模型
    sorted = models_;
    std::sort(sorted.begin(), sorted.end(),
      [](const ModelSlot& a, const ModelSlot& b) { return a.priority < b.priority; });
    int n = std::min(2, static_cast<int>(sorted.size()));
    for (int i = 0; i < n; ++i) participants.push_back(&sorted[i]);
  } else {
    for (const auto& id : model_ids) {
      for (const auto& m : models_) {
        if (m.model_id == id) { participants.push_back(&m); break; }
      }
    }
  }

  if (participants.empty()) {
    return Status::Error(ErrorCode::kModelNotFound, "no models for consensus");
  }
  if (participants.size() == 1) {
    // 只有一个模型, 降级为单模型
    return infer_single(participants[0]->model_id, prompt);
  }

  // 并行推理
  struct InferResult {
    std::string model_id;
    std::string output;
    bool ok;
  };
  std::vector<InferResult> results;
  for (const auto* p : participants) {
    auto r = p->infer_fn(p->model_id, prompt);
    results.push_back({p->model_id, r.ok() ? r.value() : "", r.ok()});
  }

  // 合议: 逐个比较, 找最一致的结果
  int best_idx = 0;
  double best_agreement = 0;
  for (size_t i = 0; i < results.size(); ++i) {
    if (!results[i].ok) continue;
    double agreement = 0;
    int comparisons = 0;
    for (size_t j = 0; j < results.size(); ++j) {
      if (i == j || !results[j].ok) continue;
      agreement += text_similarity_(results[i].output, results[j].output);
      comparisons++;
    }
    double avg_agreement = comparisons > 0 ? agreement / comparisons : 0;
    if (avg_agreement > best_agreement) {
      best_agreement = avg_agreement;
      best_idx = static_cast<int>(i);
    }
  }

  auto& chosen = results[best_idx];
  ScheduleResult sr;
  sr.model_id    = chosen.model_id;
  sr.strategy    = "consensus";
  sr.n_models_used = static_cast<int>(participants.size());
  sr.confidence  = best_agreement;
  sr.output      = chosen.output;
  return sr;
}

auto InferenceScheduler::status() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  auto models = nlohmann::json::array();
  for (const auto& m : models_) {
    auto entry = nlohmann::json::object();
    entry["model_id"] = m.model_id;
    entry["priority"] = m.priority;
    models.push_back(entry);
  }
  j["models"]    = models;
  j["n_models"]  = static_cast<int>(models_.size());
  j["consensus_threshold"] = consensus_threshold_;
  return j;
}

auto InferenceScheduler::select_model_(TaskType task) const noexcept
    -> const ModelSlot* {
  // 简单路由: 按任务类型选模型
  // 实际应该基于 env.json 的路由表
  for (const auto& m : models_) {
    if (m.priority <= 2) return &m;
  }
  return models_.empty() ? nullptr : &models_[0];
}

double InferenceScheduler::text_similarity_(const std::string& a,
                                             const std::string& b) {
  if (a.empty() && b.empty()) return 1.0;
  if (a.empty() || b.empty()) return 0.0;

  // 关键词重叠度
  auto words = [](const std::string& s) {
    std::set<std::string> w;
    std::string cur;
    for (char c : s) {
      if (std::isalnum(static_cast<unsigned char>(c))) { cur += c; }
      else if (!cur.empty()) { if (cur.size() > 2) w.insert(cur); cur.clear(); }
    }
    if (!cur.empty() && cur.size() > 2) w.insert(cur);
    return w;
  };

  auto wa = words(a), wb = words(b);
  if (wa.empty() && wb.empty()) return 1.0;

  std::set<std::string> intersection;
  for (const auto& w : wa) {
    if (wb.count(w)) intersection.insert(w);
  }

  double union_size = static_cast<double>(wa.size() + wb.size() - intersection.size());
  return union_size > 0 ? intersection.size() / union_size : 0;
}

}  // namespace nexus::core
