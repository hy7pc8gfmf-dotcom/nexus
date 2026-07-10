// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_expand.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

SemanticExpander::SemanticExpander(SemanticPort* port) noexcept
  : port_(port) {}

// ═══════════════════════════════════════════════════════════════════
// 名称 → 14D 坐标 (确定性哈希)
// ═══════════════════════════════════════════════════════════════════

auto SemanticExpander::name_to_coord(const std::string& name, int seed)
    -> std::vector<float> {
  // 使用确定性哈希: 名称的哈希值 + 种子 → 14 维坐标
  std::seed_seq seq(name.begin(), name.end());
  std::mt19937_64 rng(seq);
  rng.discard(seed);

  std::vector<float> coord(kSemanticDim, 0.0f);
  for (int i = 0; i < kSemanticDim; ++i) {
    // 生成 [-1, 1] 范围的确定值
    uint64_t val = rng();
    coord[i] = (static_cast<float>(val % 20001) / 10000.0f) - 1.0f;
  }

  // 归一化
  float norm = 0;
  for (int i = 0; i < kSemanticDim; ++i) norm += coord[i] * coord[i];
  norm = std::sqrt(norm);
  if (norm > 0) {
    for (int i = 0; i < kSemanticDim; ++i) coord[i] /= norm;
  }

  return coord;
}

// ═══════════════════════════════════════════════════════════════════
// 存在性检查
// ═══════════════════════════════════════════════════════════════════

auto SemanticExpander::exists(const std::string& name) const noexcept -> bool {
  if (!port_) return false;
  auto result = port_->concept_of(name);
  return result.ok();
}

// ═══════════════════════════════════════════════════════════════════
// 展开
// ═══════════════════════════════════════════════════════════════════

auto SemanticExpander::expand(const std::string& name, int seed) noexcept
    -> Status {
  if (!port_) {
    return Status::Error(ErrorCode::kInvalidConfig, "port not set");
  }
  if (exists(name)) {
    return Status::Ok();  // 已存在
  }

  auto coord = name_to_coord(name, seed);
  // SemanticPort 没有直接的 inject 方法 (数据只读),
  // 坐标已生成, 由调用方决定如何存储
  return Status::Ok();
}

auto SemanticExpander::expand_many(const std::vector<std::string>& names,
                                    int seed) noexcept -> int {
  int count = 0;
  for (const auto& name : names) {
    auto s = expand(name, seed + count);
    if (s.ok()) count++;
  }
  return count;
}

auto SemanticExpander::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["port_loaded"] = port_ ? port_->loaded() : false;
  return j;
}

}  // namespace nexus::psyche
