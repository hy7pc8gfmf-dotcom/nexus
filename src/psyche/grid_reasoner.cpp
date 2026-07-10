// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/grid_reasoner.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto GridNode::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]         = id;
  j["parent_id"]  = parent_id;
  j["path_id"]    = path_id;
  j["layer"]      = layer;
  j["content"]    = content;
  j["confidence"] = std::round(confidence * 1000) / 1000;
  j["source"]     = source;
  j["converged"]  = converged;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 播种
// ═══════════════════════════════════════════════════════════════════

auto GridReasoner::seed(const std::string& content,
                         const std::string& /*path_name*/,
                         double confidence) noexcept -> int {
  GridNode node;
  node.id         = next_id_++;
  node.parent_id  = 0;
  node.path_id    = next_path_++;
  node.layer      = 0;
  node.content    = content;
  node.confidence = confidence;
  node.source     = "seed";
  nodes_.push_back(node);
  path_leaves_.push_back(node.id);
  return node.id;
}

// ═══════════════════════════════════════════════════════════════════
// 扩展
// ═══════════════════════════════════════════════════════════════════

auto GridReasoner::expand() noexcept -> Status {
  if (!expand_fn_) {
    return Status::Error(ErrorCode::kInvalidConfig, "no expand function");
  }

  current_layer_++;
  std::vector<int> new_leaves;

  for (int leaf_id : path_leaves_) {
    // 找到末梢节点
    const GridNode* leaf = nullptr;
    for (const auto& n : nodes_) {
      if (n.id == leaf_id) { leaf = &n; break; }
    }
    if (!leaf) continue;

    // 调用扩展回调
    auto children = expand_fn_(*leaf, leaf->path_id);
    for (const auto& [content, confidence] : children) {
      GridNode child;
      child.id         = next_id_++;
      child.parent_id  = leaf->id;
      child.path_id    = leaf->path_id;
      child.layer      = current_layer_;
      child.content    = content;
      child.confidence = confidence;
      child.source     = "expand";
      nodes_.push_back(child);
      new_leaves.push_back(child.id);
    }
  }

  path_leaves_ = std::move(new_leaves);
  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 交叉编织
// ═══════════════════════════════════════════════════════════════════

auto GridReasoner::weave() noexcept -> Status {
  if (path_leaves_.size() < 2) return Status::Ok();

  // 选取置信度最高的两个末梢节点进行编织
  std::vector<const GridNode*> leaves = leaf_nodes();
  std::sort(leaves.begin(), leaves.end(),
    [](const GridNode* a, const GridNode* b) {
      return a->confidence > b->confidence;
    });

  // 汇合前两个
  auto result = "编织(" + leaves[0]->content.substr(0, 30) +
                " + " + leaves[1]->content.substr(0, 30) + ")";
  double woven_conf = (leaves[0]->confidence + leaves[1]->confidence) / 2.0;

  GridNode node;
  node.id         = next_id_++;
  node.parent_id  = 0;
  node.path_id    = leaves[0]->path_id;  // 归入第一条路径
  node.layer      = current_layer_;
  node.content    = result;
  node.confidence = woven_conf;
  node.source     = "weave";
  nodes_.push_back(node);

  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 收敛
// ═══════════════════════════════════════════════════════════════════

auto GridReasoner::converge() noexcept -> Status {
  auto leaves = leaf_nodes();
  if (leaves.empty()) return Status::Ok();

  if (converge_fn_) {
    // 使用收敛回调
    std::vector<GridNode> leaf_copies;
    for (const auto* l : leaves) leaf_copies.push_back(*l);
    auto [content, confidence] = converge_fn_(leaf_copies);

    GridNode node;
    node.id         = next_id_++;
    node.parent_id  = 0;
    node.path_id    = 0;
    node.layer      = current_layer_;
    node.content    = content;
    node.confidence = confidence;
    node.source     = "theorem";
    node.converged  = true;
    nodes_.push_back(node);
    converged_ = true;
  } else {
    // 默认收敛: 取置信度最高的
    const GridNode* best = leaves[0];
    for (const auto* l : leaves) {
      if (l->confidence > best->confidence) best = l;
    }

    GridNode node;
    node.id         = next_id_++;
    node.parent_id  = best->id;
    node.path_id    = 0;
    node.layer      = current_layer_;
    node.content    = "收敛: " + best->content.substr(0, 200);
    node.confidence = best->confidence;
    node.source     = "theorem";
    node.converged  = true;
    nodes_.push_back(node);
    converged_ = true;
  }

  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 全速运行
// ═══════════════════════════════════════════════════════════════════

auto GridReasoner::run(int max_layers) noexcept -> Status {
  for (int layer = 0; layer < max_layers; ++layer) {
    // 扩展
    auto s = expand();
    if (!s.ok()) return s;

    // 编织 (从第 2 层开始)
    if (layer >= 1) {
      weave();
    }
  }

  // 最终收敛
  return converge();
}

// ═══════════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════════

auto GridReasoner::nodes() const noexcept -> const std::vector<GridNode>& {
  return nodes_;
}

auto GridReasoner::leaf_nodes() const noexcept
    -> std::vector<const GridNode*> {
  std::vector<const GridNode*> leaves;
  for (const auto& n : nodes_) {
    // 末梢: 没有子节点的节点
    bool has_child = false;
    for (const auto& child : nodes_) {
      if (child.parent_id == n.id) { has_child = true; break; }
    }
    if (!has_child && n.source != "theorem") {
      leaves.push_back(&n);
    }
  }
  return leaves;
}

auto GridReasoner::report() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_nodes"] = static_cast<int>(nodes_.size());
  j["layers"]      = current_layer_;
  j["paths"]       = next_path_ - 1;
  j["converged"]   = converged_;

  auto nodes_arr = nlohmann::json::array();
  for (const auto& n : nodes_) nodes_arr.push_back(n.to_json());
  j["nodes"] = nodes_arr;

  // 找到收敛节点
  for (const auto& n : nodes_) {
    if (n.source == "theorem") {
      j["theorem"] = n.content;
      j["theorem_confidence"] = std::round(n.confidence * 1000) / 1000;
      break;
    }
  }

  return j;
}

auto GridReasoner::total_nodes() const noexcept -> int {
  return static_cast<int>(nodes_.size());
}

auto GridReasoner::converged() const noexcept -> bool {
  return converged_;
}

}  // namespace nexus::psyche
