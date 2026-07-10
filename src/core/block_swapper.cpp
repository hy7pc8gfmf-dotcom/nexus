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
 * @file block_swapper.cpp
 * @brief 块交换引擎实现
 */

#include "nexus/core/block_swapper.h"

#include <algorithm>
#include <sstream>

namespace nexus::core {

auto ModelBlock::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]      = id;
  j["name"]    = name;
  j["vram_mb"] = vram_mb;
  j["is_gpu"]  = is_gpu;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 注册块
// ═══════════════════════════════════════════════════════════════════

void BlockSwapper::register_blocks(int total_layers) noexcept {
  blocks_.clear();

  int n_blocks = std::min(total_layers, 17);
  for (int i = 0; i < n_blocks; ++i) {
    ModelBlock block;
    block.id      = i;
    block.name    = "layer_" + std::to_string(i);
    block.vram_mb = kBlockVramMb;
    block.is_gpu  = (i < 5);  // 默认前 5 块在 GPU
    blocks_.push_back(block);
  }
}

// ═══════════════════════════════════════════════════════════════════
// 块迁移
// ═══════════════════════════════════════════════════════════════════

auto BlockSwapper::swap_to_gpu(int block_id) noexcept -> Status {
  for (auto& b : blocks_) {
    if (b.id == block_id) {
      if (b.is_gpu) return Status::Ok();  // 已在 GPU
      b.is_gpu = true;
      return Status::Ok();
    }
  }
  return Status::Error(ErrorCode::kFileNotFound,
    "block not found: " + std::to_string(block_id));
}

auto BlockSwapper::swap_to_cpu(int block_id) noexcept -> Status {
  for (auto& b : blocks_) {
    if (b.id == block_id) {
      if (!b.is_gpu) return Status::Ok();  // 已在 CPU
      b.is_gpu = false;
      return Status::Ok();
    }
  }
  return Status::Error(ErrorCode::kFileNotFound,
    "block not found: " + std::to_string(block_id));
}

// ═══════════════════════════════════════════════════════════════════
// 自动平衡
// ═══════════════════════════════════════════════════════════════════

auto BlockSwapper::auto_balance(int vram_budget_mb, int current_usage_mb) noexcept
    -> std::vector<int> {
  std::vector<int> migrated;

  int available_mb = vram_budget_mb - current_usage_mb;
  int max_gpu_blocks = available_mb / kBlockVramMb;

  // 统计当前 GPU 块数
  int current_gpu = gpu_block_count();
  int target_gpu = std::min(max_gpu_blocks, static_cast<int>(blocks_.size()));
  if (target_gpu < 1) target_gpu = 1;  // 至少保留 1 块在 GPU

  if (current_gpu < target_gpu) {
    // 需要迁移更多块到 GPU
    int to_move = target_gpu - current_gpu;
    for (auto& b : blocks_) {
      if (to_move <= 0) break;
      if (!b.is_gpu) {
        b.is_gpu = true;
        migrated.push_back(b.id);
        to_move--;
      }
    }
  } else if (current_gpu > target_gpu) {
    // 需要迁移一些块回 CPU
    int to_move = current_gpu - target_gpu;
    for (auto& b : blocks_) {
      if (to_move <= 0) break;
      if (b.is_gpu) {
        b.is_gpu = false;
        migrated.push_back(b.id);
        to_move--;
      }
    }
  }

  return migrated;
}

// ═══════════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════════

auto BlockSwapper::gpu_block_count() const noexcept -> int {
  int count = 0;
  for (const auto& b : blocks_) if (b.is_gpu) count++;
  return count;
}

auto BlockSwapper::cpu_block_count() const noexcept -> int {
  int count = 0;
  for (const auto& b : blocks_) if (!b.is_gpu) count++;
  return count;
}

auto BlockSwapper::gpu_vram_usage() const noexcept -> int {
  int usage = 0;
  for (const auto& b : blocks_) if (b.is_gpu) usage += b.vram_mb;
  return usage;
}

auto BlockSwapper::status() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_blocks"]   = static_cast<int>(blocks_.size());
  j["gpu_blocks"]     = gpu_block_count();
  j["cpu_blocks"]     = cpu_block_count();
  j["gpu_vram_mb"]    = gpu_vram_usage();
  j["block_vram_mb"]  = kBlockVramMb;

  auto blist = nlohmann::json::array();
  for (const auto& b : blocks_) blist.push_back(b.to_json());
  j["blocks"] = blist;
  return j;
}

auto BlockSwapper::blocks() const noexcept -> const std::vector<ModelBlock>& {
  return blocks_;
}

}  // namespace nexus::core
