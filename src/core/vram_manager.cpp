/**
 * @file vram_manager.cpp
 * @brief VRAM 管理器实现 — CUDA Runtime API
 */

#include "nexus/core/vram_manager.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// VRAM 管理器
// ═══════════════════════════════════════════════════════════════════

auto VramManager::initialize(int reserved_mb) noexcept -> Status {
  reserved_mb_ = reserved_mb;

#ifdef NEXUS_CUDA_ENABLED
  int device_count = 0;
  auto err = cudaGetDeviceCount(&device_count);
  if (err != cudaSuccess || device_count == 0) {
    return Status::Error(ErrorCode::kGpuUnavailable,
      "no CUDA device found");
  }

  // 使用 device 0
  device_id_ = 0;
  err = cudaSetDevice(device_id_);
  if (err != cudaSuccess) {
    return Status::Error(ErrorCode::kGpuUnavailable,
      "cudaSetDevice failed");
  }

  // 查询 VRAM
  size_t free_bytes = 0, total_bytes = 0;
  err = cudaMemGetInfo(&free_bytes, &total_bytes);
  if (err != cudaSuccess) {
    return Status::Error(ErrorCode::kGpuUnavailable,
      "cudaMemGetInfo failed");
  }

  total_bytes_ = static_cast<int64_t>(total_bytes);
  free_bytes_  = static_cast<int64_t>(free_bytes);
  used_bytes_  = total_bytes_ - free_bytes_;
  peak_bytes_  = used_bytes_;

  // 计算可用预算
  int free_mb = static_cast<int>(free_bytes / (1024 * 1024));
  budget_mb_ = free_mb - reserved_mb;
  if (budget_mb_ < 0) budget_mb_ = 0;

  return Status::Ok();
#else
  return Status::Error(ErrorCode::kGpuUnavailable, "CUDA disabled at build time");
#endif
}

auto VramManager::can_load(const ModelVramRequirement& req) const noexcept -> bool {
  if (budget_mb_ <= 0) return false;
  int used_mb = static_cast<int>(used_bytes_ / (1024 * 1024));
  return (used_mb + req.vram_mb) <= budget_mb_;
}

auto VramManager::record_load(const ModelVramRequirement& req) noexcept -> Status {
  if (!can_load(req)) {
    return Status::Error(ErrorCode::kVramInsufficient,
      "cannot load " + req.model_id + ": insufficient VRAM");
  }

  loaded_.push_back({req.model_id, req.vram_mb});
  used_bytes_ += req.vram_mb * 1024 * 1024;
  if (used_bytes_ > peak_bytes_) peak_bytes_ = used_bytes_;

  return Status::Ok();
}

auto VramManager::record_unload(const std::string& model_id) noexcept -> Status {
  for (auto it = loaded_.begin(); it != loaded_.end(); ++it) {
    if (it->model_id == model_id) {
      used_bytes_ -= it->vram_mb * 1024 * 1024;
      if (used_bytes_ < 0) used_bytes_ = 0;
      loaded_.erase(it);
      return Status::Ok();
    }
  }
  return Status::Error(ErrorCode::kFileNotFound,
    "model not loaded: " + model_id);
}

auto VramManager::status() const noexcept -> VramStatus {
#ifdef NEXUS_CUDA_ENABLED
  size_t free = 0, total = 0;
  cudaMemGetInfo(&free, &total);
  return VramStatus{
    .total_bytes  = static_cast<int64_t>(total),
    .free_bytes   = static_cast<int64_t>(free),
    .used_bytes   = used_bytes_,
    .peak_bytes   = peak_bytes_,
    .device_count = 1,
    .available    = true,
  };
#else
  return VramStatus{};
#endif
}

auto VramManager::loaded_models() const noexcept -> std::vector<std::string> {
  std::vector<std::string> ids;
  ids.reserve(loaded_.size());
  for (const auto& m : loaded_) ids.push_back(m.model_id);
  return ids;
}

// ═══════════════════════════════════════════════════════════════════
// GPU 锁
// ═══════════════════════════════════════════════════════════════════

auto GpuLock::acquire(int timeout_ms) noexcept -> Status {
  if (held_) return Status::Ok();

  mutex_ = CreateMutexA(nullptr, FALSE, "Global\\nexus_gpu_lock");
  if (!mutex_) {
    return Status::Error(ErrorCode::kInternal, "CreateMutex failed");
  }

  DWORD wait = WaitForSingleObject(mutex_, static_cast<DWORD>(timeout_ms));
  if (wait == WAIT_OBJECT_0) {
    held_ = true;
    return Status::Ok();
  }

  CloseHandle(mutex_);
  mutex_ = nullptr;
  return Status::Error(ErrorCode::kIpcTimeout, "GPU lock acquisition timeout");
}

void GpuLock::release() noexcept {
  if (mutex_ && held_) {
    ReleaseMutex(mutex_);
    CloseHandle(mutex_);
    mutex_ = nullptr;
    held_ = false;
  }
}

}  // namespace nexus::core
