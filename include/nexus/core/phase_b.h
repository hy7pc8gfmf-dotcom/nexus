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

#ifndef NEXUS_CORE_PHASE_B_H
#define NEXUS_CORE_PHASE_B_H

/**
 * @file phase_b.h
 * @brief Phase B GPU 桥接 — 多进程 GPU 资源共享
 *
 * 管理多个进程共享同一块 GPU 时的 VRAM 分配策略。
 * 通过 gpu.lock 文件 + TTL 租约机制协调。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

#include "nexus/types/status.h"

namespace nexus::core {

struct GpuLease {
  int     pid = 0;
  double  acquired_at = 0.0;
  int     ttl_seconds = 120;
  int     vram_reserved_mb = 0;

  auto to_json() const -> nlohmann::json;
  [[nodiscard]] auto is_expired() const noexcept -> bool;
};

class PhaseBBridge {
public:
  PhaseBBridge() noexcept = default;

  auto acquire_lease(int vram_mb, int ttl = 120) noexcept -> Status;
  auto release_lease() noexcept -> void;
  auto check_lease() noexcept -> Status;
  [[nodiscard]] auto current_lease() const noexcept -> const GpuLease&;

private:
  GpuLease lease_;
  static constexpr const char* kLockPath = ".nexus/gpu.lock";
};

}  // namespace nexus::core

#endif
