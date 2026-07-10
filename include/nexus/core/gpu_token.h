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

#ifndef NEXUS_CORE_GPU_TOKEN_H
#define NEXUS_CORE_GPU_TOKEN_H

/**
 * @file gpu_token.h
 * @brief GPU Token 注入 — 推理管线 GPU 加速
 *
 * 在推理管线中注入 EngineLogitsProcessor，将 logits 处理
 * 卸载到 GPU 执行，减少 CPU 负载。
 *
 * 编译条件: NEXUS_CUDA_ENABLED
 * 降级: CUDA 不可用时透明回退到 CPU
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::core {

/// GPU 注入配置
struct GpuInjectConfig {
  bool enabled = true;
  int  device_id = 0;
  int  batch_size = 512;
};

/// GPU 性能指标
struct GpuPerfMetrics {
  double kernel_time_ms = 0.0;
  double memcpy_time_ms = 0.0;
  int    tokens_processed = 0;
  double tokens_per_sec = 0.0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// GpuTokenInjector
// ═══════════════════════════════════════════════════════════════════

class GpuTokenInjector {
public:
  GpuTokenInjector() noexcept;
  ~GpuTokenInjector() noexcept;

  /// 初始化 CUDA 上下文
  auto initialize(int device_id = 0) noexcept -> Status;

  /// 注入 token 处理 kernel
  auto process_logits(float* logits, int n_tokens, int n_vocab) noexcept -> Status;

  /// 同步 CUDA 流
  auto synchronize() noexcept -> Status;

  /// 获取指标
  [[nodiscard]] auto metrics() const noexcept -> GpuPerfMetrics;

  /// 是否可用
  [[nodiscard]] auto available() const noexcept -> bool;

  /// 关闭
  auto shutdown() noexcept -> void;

private:
  bool initialized_ = false;
#ifdef NEXUS_CUDA_ENABLED
  void* cuda_stream_ = nullptr;   // cudaStream_t
  void* d_logits_    = nullptr;   // device buffer
  int   logits_size_ = 0;
#endif
  GpuPerfMetrics metrics_;
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_GPU_TOKEN_H
