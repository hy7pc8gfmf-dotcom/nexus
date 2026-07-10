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

#ifndef NEXUS_CORE_SCHEDULER_H
#define NEXUS_CORE_SCHEDULER_H

/**
 * @file scheduler.h
 * @brief 推理调度器 — 单模型/合议/自适应三层调度
 *
 * 三层架构:
 *   Layer 1 (Single):  直接路由到指定模型
 *   Layer 2 (Consensus): 多模型推理后合议投票
 *   Layer 3 (Adaptive):  自动分析任务类型, 选择最优策略
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "nexus/types/status.h"
#include <nlohmann/json.hpp>

namespace nexus::core {

/// 任务类型
enum class TaskType : uint8_t {
  kReasoning = 0,
  kQuickLocal,
  kTranslate,
  kVision,
  kDialectic,
  kDebug,
  kCode,
  kUnknown = 255,
};

auto task_type_to_string(TaskType t) -> const char*;
auto parse_task_type(const std::string& s) -> TaskType;

/// 调度结果
struct ScheduleResult {
  std::string model_id;        // 选中的模型
  std::string strategy;        // "single" | "consensus" | "adaptive"
  int         n_models_used;   // 使用的模型数
  double      confidence;      // 置信度 [0, 1]
  std::string output;          // 推理输出

  auto to_json() const -> nlohmann::json;
};

/// 推理函数签名 (由 ExpertLoader 提供)
using InferFn = std::function<Result<std::string>(
  const std::string& model_id, const std::string& prompt)>;

// ═══════════════════════════════════════════════════════════════════
// 推理调度器
// ═══════════════════════════════════════════════════════════════════

class InferenceScheduler {
public:
  /// 注册一个可用模型
  auto register_model(const std::string& model_id,
                      InferFn infer_fn,
                      int priority = 5) noexcept -> Status;

  /// 执行推理 (自动选择策略)
  auto infer(const std::string& prompt,
             TaskType task_type = TaskType::kReasoning) noexcept
      -> Result<ScheduleResult>;

  /// 单模型推理
  auto infer_single(const std::string& model_id,
                    const std::string& prompt) noexcept
      -> Result<ScheduleResult>;

  /// 合议推理 (多模型投票)
  auto infer_consensus(const std::string& prompt,
                       const std::vector<std::string>& model_ids = {}) noexcept
      -> Result<ScheduleResult>;

  /// 获取当前策略配置
  [[nodiscard]] auto status() const noexcept -> nlohmann::json;

  /// 设置合议阈值
  void set_consensus_threshold(double t) noexcept { consensus_threshold_ = t; }

private:
  struct ModelSlot {
    std::string model_id;
    InferFn     infer_fn;
    int         priority;
  };
  std::vector<ModelSlot> models_;
  double consensus_threshold_ = 0.6;

  /// 根据任务类型选择最优模型
  [[nodiscard]] auto select_model_(TaskType task) const noexcept
      -> const ModelSlot*;

  /// 简单文本相似度 (用于合议投票)
  static double text_similarity_(const std::string& a, const std::string& b);
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_SCHEDULER_H
