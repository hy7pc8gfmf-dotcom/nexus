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
