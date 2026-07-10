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

#ifndef NEXUS_ALGO_ENGINE_H
#define NEXUS_ALGO_ENGINE_H

/**
 * @file engine.h
 * @brief 算法引擎接口 — 所有算法引擎的基类
 *
 * 插件架构: 每个算法引擎是一个独立类, 注册到 EngineRegistry。
 * 生命周期: construct → initialize → execute (多次) → destroy
 */

#include <memory>
#include <string>
#include <vector>

#include "nexus/types/status.h"
#include <nlohmann/json.hpp>

namespace nexus::algo {

/// 算法引擎元信息
struct EngineInfo {
  std::string id;           // 唯一标识 (如 "mcmc", "dre")
  std::string name;         // 人类可读名称
  std::string version;      // 版本号
  std::string description;  // 简要描述
  std::vector<std::string> tags;  // 能力标签
};

// ═══════════════════════════════════════════════════════════════════
// 算法引擎基类
// ═══════════════════════════════════════════════════════════════════

class AlgorithmEngine {
public:
  virtual ~AlgorithmEngine() noexcept = default;

  /// 获取引擎信息
  [[nodiscard]] virtual auto info() const noexcept -> EngineInfo = 0;

  /// 初始化引擎
  virtual auto initialize(const nlohmann::json& config = {}) noexcept -> Status = 0;

  /// 执行算法
  /// @param input  输入参数 (JSON)
  /// @param output 输出结果 (JSON)
  virtual auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> = 0;

  /// 获取引擎状态
  [[nodiscard]] virtual auto status() const noexcept -> nlohmann::json = 0;

  /// 是否已初始化
  [[nodiscard]] virtual auto is_initialized() const noexcept -> bool = 0;
};

// ═══════════════════════════════════════════════════════════════════
// 算法引擎注册表
// ═══════════════════════════════════════════════════════════════════

class EngineRegistry {
public:
  /// 注册一个算法引擎
  auto register_engine(std::unique_ptr<AlgorithmEngine> engine) noexcept -> Status;

  /// 获取指定引擎
  [[nodiscard]] auto get(const std::string& id) noexcept -> AlgorithmEngine*;

  /// 获取引擎（const）
  [[nodiscard]] auto get(const std::string& id) const noexcept -> const AlgorithmEngine*;

  /// 列出所有已注册引擎的 ID
  [[nodiscard]] auto list_ids() const noexcept -> std::vector<std::string>;

  /// 列出所有已注册引擎的详情
  [[nodiscard]] auto list_details() const noexcept -> std::vector<EngineInfo>;

  /// 获取引擎数量
  [[nodiscard]] auto count() const noexcept -> size_t;

  /// 初始化所有引擎
  auto initialize_all(const nlohmann::json& config = {}) noexcept -> Status;

  /// 获取单例实例
  static auto instance() noexcept -> EngineRegistry&;

private:
  std::vector<std::unique_ptr<AlgorithmEngine>> engines_;
};

}  // namespace nexus::algo

#endif  // NEXUS_ALGO_ENGINE_H
