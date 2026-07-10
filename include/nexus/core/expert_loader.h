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

#ifndef NEXUS_CORE_EXPERT_LOADER_H
#define NEXUS_CORE_EXPERT_LOADER_H

/**
 * @file expert_loader.h
 * @brief 专家模型加载/推理/卸载引擎
 *
 * 封装 llama.cpp 的 C API，提供模型加载、推理、卸载接口。
 * 依赖: llama.cpp (通过 FetchContent 引入)
 */

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::core {

/// 推理参数
struct InferParams {
  int   n_predict = 512;     // 最大生成 token 数
  int   n_ctx     = 8192;    // 上下文长度
  int   n_batch   = 512;     // 批处理大小
  int   n_gpu_layers = -1;   // GPU 层数 (-1 = 全部)
  float temperature = 0.0f;  // 温度 (0 = 贪婪)
  int   seed = -1;           // 随机种子 (-1 = 随机)
};

/// 模型句柄 (不透明指针)
struct ModelHandle;

/// Token 回调: 每次生成一个 token 时调用
using TokenCallback = std::function<void(const std::string& token)>;

// ═══════════════════════════════════════════════════════════════════
// ExpertLoader — 专家模型加载器
// ═══════════════════════════════════════════════════════════════════

class ExpertLoader {
public:
  ExpertLoader() noexcept;
  ~ExpertLoader() noexcept;

  ExpertLoader(const ExpertLoader&) = delete;
  ExpertLoader& operator=(const ExpertLoader&) = delete;
  ExpertLoader(ExpertLoader&&) noexcept;
  ExpertLoader& operator=(ExpertLoader&&) noexcept;

  /// 加载模型
  /// @param gguf_path GGUF 模型文件路径
  /// @param params    推理参数
  auto load(const std::string& gguf_path, const InferParams& params = {}) noexcept
      -> Status;

  /// 卸载模型
  auto unload() noexcept -> Status;

  /// 推理 (同步，返回完整输出)
  auto infer(const std::string& prompt, const InferParams& params = {},
             TokenCallback on_token = nullptr) noexcept -> Result<std::string>;

  /// 是否已加载
  [[nodiscard]] auto is_loaded() const noexcept -> bool;

  /// 获取已加载的模型路径
  [[nodiscard]] auto model_path() const noexcept -> const std::string&;

private:
  std::unique_ptr<ModelHandle> handle_;
  std::string model_path_;
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_EXPERT_LOADER_H
