#ifndef NEXUS_CORE_VRAM_MANAGER_H
#define NEXUS_CORE_VRAM_MANAGER_H

/**
 * @file vram_manager.h
 * @brief VRAM 管理器 — 基于 CUDA Runtime API
 *
 * 管理 GPU 显存的分配/释放/跟踪。
 * 单例模式，整个 core.exe 只有一个实例。
 */

#include <cstdint>
#include <string>
#include <vector>

#include "nexus/types/status.h"

#ifdef NEXUS_CUDA_ENABLED
#include <cuda_runtime.h>
#endif

namespace nexus::core {

/// VRAM 状态快照
struct VramStatus {
  int64_t total_bytes  = 0;
  int64_t free_bytes   = 0;
  int64_t used_bytes   = 0;
  int64_t peak_bytes   = 0;
  int     device_count = 0;
  bool    available    = false;

  auto total_mb() const -> int { return static_cast<int>(total_bytes / (1024 * 1024)); }
  auto free_mb()  const -> int { return static_cast<int>(free_bytes  / (1024 * 1024)); }
  auto used_mb()  const -> int { return static_cast<int>(used_bytes  / (1024 * 1024)); }
  auto peak_mb()  const -> int { return static_cast<int>(peak_bytes  / (1024 * 1024)); }
};

/// 模型 VRAM 需求
struct ModelVramRequirement {
  std::string model_id;
  int64_t     vram_mb = 0;       // 预期 VRAM 占用 (MB)
  int64_t     ctx_size = 8192;   // 上下文长度
  int         n_gpu_layers = -1; // GPU 层数 (-1 = 全部)
};

// ═══════════════════════════════════════════════════════════════════
// VRAM 管理器
// ═══════════════════════════════════════════════════════════════════

class VramManager {
public:
  /// 初始化：查询 GPU 状态并设定预算
  /// @param reserved_mb 为系统和后台进程预留的 VRAM (MB)
  auto initialize(int reserved_mb = 1000) noexcept -> Status;

  /// 检查某个模型是否可以加载
  auto can_load(const ModelVramRequirement& req) const noexcept -> bool;

  /// 记录模型加载（增加 used）
  auto record_load(const ModelVramRequirement& req) noexcept -> Status;

  /// 记录模型卸载（减少 used）
  auto record_unload(const std::string& model_id) noexcept -> Status;

  /// 获取当前状态
  [[nodiscard]] auto status() const noexcept -> VramStatus;

  /// 列出已加载的模型
  [[nodiscard]] auto loaded_models() const noexcept -> std::vector<std::string>;

  /// 获取预算 (MB)
  [[nodiscard]] auto budget_mb() const noexcept -> int { return budget_mb_; }

private:
  int64_t total_bytes_ = 0;
  int64_t free_bytes_  = 0;
  int64_t used_bytes_  = 0;
  int64_t peak_bytes_  = 0;
  int     device_id_   = 0;
  int     budget_mb_   = 0;
  int     reserved_mb_ = 1000;

  struct LoadedModel {
    std::string model_id;
    int64_t vram_mb;
  };
  std::vector<LoadedModel> loaded_;
};

// ═══════════════════════════════════════════════════════════════════
// GPU 锁 (Windows Named Mutex)
// ═══════════════════════════════════════════════════════════════════

class GpuLock {
public:
  /// 尝试获取 GPU 锁
  /// @param timeout_ms 等待超时 (ms), 0 = 不等待
  auto acquire(int timeout_ms = 5000) noexcept -> Status;

  /// 释放 GPU 锁
  void release() noexcept;

  /// 是否持有锁
  [[nodiscard]] auto is_held() const noexcept -> bool { return held_; }

  ~GpuLock() noexcept { release(); }
  GpuLock() noexcept = default;
  GpuLock(const GpuLock&) = delete;
  GpuLock& operator=(const GpuLock&) = delete;

private:
  void* mutex_ = nullptr;
  bool  held_  = false;
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_VRAM_MANAGER_H
