#ifndef NEXUS_CORE_BLOCK_SWAPPER_H
#define NEXUS_CORE_BLOCK_SWAPPER_H

/**
 * @file block_swapper.h
 * @brief 块交换引擎 — GPU/CPU 间 17 模型块的动态切换
 *
 * 每个块代表模型的一部分 (transformer layer block)。
 * 根据 VRAM 可用量自动在 GPU/CPU 间迁移块。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::core {

/// 块状态
enum class BlockLocation : uint8_t { kGpu = 0, kCpu = 1, kUnloaded = 2 };

/// 模型块
struct ModelBlock {
  int    id = 0;
  std::string name;
  int    vram_mb = 0;       // 在 GPU 上占用的 VRAM
  bool   is_gpu = false;    // 当前在 GPU 上

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// BlockSwapper
// ═══════════════════════════════════════════════════════════════════

class BlockSwapper {
public:
  BlockSwapper() noexcept = default;

  /// 注册 17 个模型块
  auto register_blocks(int total_layers = 28) noexcept -> void;

  /// 将指定块迁移到 GPU
  auto swap_to_gpu(int block_id) noexcept -> Status;

  /// 将指定块迁移到 CPU
  auto swap_to_cpu(int block_id) noexcept -> Status;

  /// 自动平衡: 根据 VRAM 预算迁移块
  auto auto_balance(int vram_budget_mb, int current_usage_mb) noexcept
      -> std::vector<int>;  // 返回迁移的块 ID

  /// 获取当前 GPU 块数
  [[nodiscard]] auto gpu_block_count() const noexcept -> int;

  /// 获取当前 CPU 块数
  [[nodiscard]] auto cpu_block_count() const noexcept -> int;

  /// 获取 GPU 总占用
  [[nodiscard]] auto gpu_vram_usage() const noexcept -> int;

  /// 获取状态
  [[nodiscard]] auto status() const noexcept -> nlohmann::json;

  /// 获取所有块
  [[nodiscard]] auto blocks() const noexcept -> const std::vector<ModelBlock>&;

private:
  std::vector<ModelBlock> blocks_;
  static constexpr int kBlockVramMb = 64;  // 每块 ~64MB
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_BLOCK_SWAPPER_H
