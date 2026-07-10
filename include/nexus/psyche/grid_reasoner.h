// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_GRID_REASONER_H
#define NEXUS_PSYCHE_GRID_REASONER_H

/**
 * @file grid_reasoner.h
 * @brief 网格推理引擎 — 多路并行收敛推理
 *
 * 移植自 D:/synapse/engines/psi_grid_reasoner.py + dark_horse_forest.py
 *
 * 管线:
 *   seed() → expand() [N路并行] → weave() [交叉编织]
 *   → converge() [路径汇合] → theorem() [最终定理]
 *
 * 用法:
 *   GridReasoner grid;
 *   grid.seed("原始问题", "math");
 *   grid.run(5);  // 5 层推理
 *   auto r = grid.report();
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 网格节点
// ═══════════════════════════════════════════════════════════════════

struct GridNode {
  int         id          = 0;
  int         parent_id   = 0;       // 0 = root
  int         path_id     = 0;       // 所属路径
  int         layer       = 0;       // 推理层数
  std::string content;               // 节点内容
  double      confidence  = 0.0;
  std::string source;                // "seed" | "expand" | "weave" | "theorem"
  bool        converged   = false;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 网格推理引擎
// ═══════════════════════════════════════════════════════════════════

class GridReasoner {
public:
  /// 扩展回调: 输入父节点内容 → 返回子节点列表 (内容, 置信度)
  using ExpandFn = std::function<std::vector<std::pair<std::string, double>>(
    const GridNode& parent, int path_id)>;

  /// 收敛回调: 输入一组待汇合节点 → 返回收敛结果
  using ConvergeFn = std::function<std::pair<std::string, double>(
    const std::vector<GridNode>& nodes)>;

  GridReasoner() noexcept = default;

  // ── 回调注册 ──

  void set_expand_fn(ExpandFn fn) noexcept { expand_fn_ = std::move(fn); }
  void set_converge_fn(ConvergeFn fn) noexcept { converge_fn_ = std::move(fn); }

  // ── 网格操作 ──

  /// 播种: 添加初始推理节点
  auto seed(const std::string& content, const std::string& path_name,
            double confidence = 0.5) noexcept -> int;

  /// 扩展: 对所有末梢节点执行一次扩展
  auto expand() noexcept -> Status;

  /// 交叉编织: 将不同路径的节点汇合
  auto weave() noexcept -> Status;

  /// 收敛: 将指定路径的末梢节点汇合为结论
  auto converge() noexcept -> Status;

  /// 全速运行 N 层
  auto run(int max_layers = 5) noexcept -> Status;

  // ── 查询 ──

  [[nodiscard]] auto nodes() const noexcept -> const std::vector<GridNode>&;
  [[nodiscard]] auto leaf_nodes() const noexcept -> std::vector<const GridNode*>;
  [[nodiscard]] auto report() const noexcept -> nlohmann::json;
  [[nodiscard]] auto total_nodes() const noexcept -> int;
  [[nodiscard]] auto converged() const noexcept -> bool;

private:
  std::vector<GridNode> nodes_;
  ExpandFn  expand_fn_;
  ConvergeFn converge_fn_;
  int next_id_     = 1;
  int next_path_   = 1;
  int current_layer_ = 0;
  bool converged_  = false;

  // 路径到 ID 的映射
  std::vector<int> path_leaves_;  // 每条路径的末梢节点 ID

  auto find_leaf_(int path_id) const noexcept -> const GridNode*;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_GRID_REASONER_H
