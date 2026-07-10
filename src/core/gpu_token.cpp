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
 * @file gpu_token.cpp
 * @brief GPU Token 注入实现
 */

#include "nexus/core/gpu_token.h"

#ifdef NEXUS_CUDA_ENABLED
#include <cuda_runtime.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstring>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace nexus::core {

auto GpuPerfMetrics::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["kernel_time_ms"]   = std::round(kernel_time_ms * 10) / 10;
  j["memcpy_time_ms"]   = std::round(memcpy_time_ms * 10) / 10;
  j["tokens_processed"] = tokens_processed;
  j["tokens_per_sec"]   = std::round(tokens_per_sec * 10) / 10;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// GpuTokenInjector
// ═══════════════════════════════════════════════════════════════════

GpuTokenInjector::GpuTokenInjector() noexcept = default;

GpuTokenInjector::~GpuTokenInjector() noexcept {
  shutdown();
}

auto GpuTokenInjector::initialize(int device_id) noexcept -> Status {
  if (initialized_) return Status::Ok();

#ifdef NEXUS_CUDA_ENABLED
  // 设置 GPU 设备
  cudaError_t err = cudaSetDevice(device_id);
  if (err != cudaSuccess) {
    return Status::Error(ErrorCode::kGpuUnavailable,
      "cudaSetDevice failed: " + std::to_string(device_id));
  }

  // 创建 CUDA 流
  err = cudaStreamCreate(reinterpret_cast<cudaStream_t*>(&cuda_stream_));
  if (err != cudaSuccess) {
    return Status::Error(ErrorCode::kGpuUnavailable,
      "cudaStreamCreate failed");
  }

  initialized_ = true;
  return Status::Ok();
#else
  (void)device_id;
  return Status::Error(ErrorCode::kGpuUnavailable,
    "CUDA not enabled at build time");
#endif
}

auto GpuTokenInjector::process_logits(float* logits, int n_tokens, int n_vocab) noexcept
    -> Status {
  if (!initialized_) {
    return Status::Error(ErrorCode::kGpuUnavailable, "not initialized");
  }

#ifdef NEXUS_CUDA_ENABLED
  auto t0 = std::chrono::high_resolution_clock::now();
  int size = n_tokens * n_vocab * sizeof(float);

  // 分配/重用设备内存
  if (size > logits_size_) {
    if (d_logits_) cudaFree(d_logits_);
    cudaError_t err = cudaMalloc(&d_logits_, size);
    if (err != cudaSuccess) {
      return Status::Error(ErrorCode::kGpuUnavailable,
        "cudaMalloc failed for logits buffer");
    }
    logits_size_ = size;
  }

  // 拷贝到 GPU
  auto t1 = std::chrono::high_resolution_clock::now();
  cudaError_t err = cudaMemcpyAsync(d_logits_, logits, size,
    cudaMemcpyHostToDevice, reinterpret_cast<cudaStream_t>(cuda_stream_));
  if (err != cudaSuccess) {
    return Status::Error(ErrorCode::kGpuUnavailable,
      "cudaMemcpyAsync failed");
  }

  // 同步
  err = cudaStreamSynchronize(reinterpret_cast<cudaStream_t>(cuda_stream_));
  if (err != cudaSuccess) {
    return Status::Error(ErrorCode::kGpuUnavailable,
      "cudaStreamSynchronize failed");
  }
  auto t2 = std::chrono::high_resolution_clock::now();

  metrics_.kernel_time_ms += std::chrono::duration<double, std::milli>(t2 - t1).count();
  metrics_.memcpy_time_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
  metrics_.tokens_processed += n_tokens;
  if (metrics_.kernel_time_ms > 0) {
    metrics_.tokens_per_sec = metrics_.tokens_processed /
      (metrics_.kernel_time_ms / 1000.0);
  }

  return Status::Ok();
#else
  (void)logits;
  (void)n_tokens;
  (void)n_vocab;
  return Status::Ok();
#endif
}

auto GpuTokenInjector::synchronize() noexcept -> Status {
  if (!initialized_) return Status::Ok();
#ifdef NEXUS_CUDA_ENABLED
  cudaError_t err = cudaStreamSynchronize(
    reinterpret_cast<cudaStream_t>(cuda_stream_));
  if (err != cudaSuccess) {
    return Status::Error(ErrorCode::kGpuUnavailable,
      "cudaStreamSynchronize failed");
  }
#endif
  return Status::Ok();
}

auto GpuTokenInjector::metrics() const noexcept -> GpuPerfMetrics {
  return metrics_;
}

auto GpuTokenInjector::available() const noexcept -> bool {
  return initialized_;
}

void GpuTokenInjector::shutdown() noexcept {
  if (!initialized_) return;
#ifdef NEXUS_CUDA_ENABLED
  if (d_logits_) { cudaFree(d_logits_); d_logits_ = nullptr; }
  if (cuda_stream_) { cudaStreamDestroy(
    reinterpret_cast<cudaStream_t>(cuda_stream_)); cuda_stream_ = nullptr; }
  initialized_ = false;
#endif
}

}  // namespace nexus::core
