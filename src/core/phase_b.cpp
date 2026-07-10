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
 * @file phase_b.cpp
 * @brief Phase B GPU 桥接实现
 */

#include "nexus/core/phase_b.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace nexus::core {

namespace fs = std::filesystem;

auto GpuLease::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["pid"] = pid; j["acquired_at"] = acquired_at;
  j["ttl_seconds"] = ttl_seconds; j["vram_reserved_mb"] = vram_reserved_mb;
  return j;
}

auto GpuLease::is_expired() const noexcept -> bool {
  if (acquired_at == 0) return true;
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  return (now - acquired_at) > ttl_seconds;
}

auto PhaseBBridge::acquire_lease(int vram_mb, int ttl) noexcept -> Status {
  // 检查现有租约
  if (fs::exists(kLockPath)) {
    GpuLease existing;
    std::ifstream ifs(kLockPath);
    if (ifs.is_open()) {
      nlohmann::json j; ifs >> j;
      existing.pid = j.value("pid", 0);
      existing.acquired_at = j.value("acquired_at", 0.0);
      existing.ttl_seconds = j.value("ttl_seconds", 120);
    }
    if (!existing.is_expired() && existing.pid != GetCurrentProcessId()) {
      return Status::Error(ErrorCode::kIpcTimeout, "GPU locked by PID " + std::to_string(existing.pid));
    }
  }

  lease_.pid = static_cast<int>(GetCurrentProcessId());
  lease_.acquired_at = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  lease_.ttl_seconds = ttl;
  lease_.vram_reserved_mb = vram_mb;

  std::ofstream ofs(kLockPath);
  if (ofs.is_open()) ofs << lease_.to_json().dump(2);
  return Status::Ok();
}

void PhaseBBridge::release_lease() noexcept {
  lease_ = GpuLease{};
  std::error_code ec;
  fs::remove(kLockPath, ec);
}

auto PhaseBBridge::check_lease() noexcept -> Status {
  if (lease_.pid == 0) return Status::Error(ErrorCode::kIpcTimeout, "no active lease");
  if (lease_.is_expired()) {
    release_lease();
    return Status::Error(ErrorCode::kIpcTimeout, "lease expired");
  }
  return Status::Ok();
}

auto PhaseBBridge::current_lease() const noexcept -> const GpuLease& {
  return lease_;
}

}  // namespace nexus::core
