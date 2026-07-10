// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

/**
 * @file grid_test.cpp
 * @brief 网格推理引擎单元测试
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nexus/psyche/grid_reasoner.h"

using nexus::psyche::GridReasoner;
using nexus::psyche::GridNode;

static int g_expand_step = 0;

static auto expand_fn(const GridNode& parent, int) -> std::vector<std::pair<std::string, double>> {
  g_expand_step++;
  return {
    {parent.content + " -> 推理A", std::min(0.9, 0.3 + g_expand_step * 0.1)},
    {parent.content + " -> 推理B", std::min(0.85, 0.25 + g_expand_step * 0.1)},
  };
}

static auto converge_fn(const std::vector<GridNode>& nodes)
    -> std::pair<std::string, double> {
  return {"自定义收敛结论", 0.99};
}

TEST(GridTest, SeedAndCount) {
  g_expand_step = 0;
  GridReasoner grid;
  int id = grid.seed("初始问题", "path1", 0.5);
  EXPECT_GT(id, 0);
  EXPECT_EQ(grid.total_nodes(), 1);
}

TEST(GridTest, Expand) {
  GridReasoner grid;
  grid.set_expand_fn(expand_fn);
  grid.seed("测试", "p1", 0.5);

  auto s = grid.expand();
  ASSERT_TRUE(s.ok());

  // 1 seed + 2 expanded = 3 nodes
  EXPECT_EQ(grid.total_nodes(), 3);
}

TEST(GridTest, RunConverges) {
  GridReasoner grid;
  grid.set_expand_fn(expand_fn);
  grid.seed("问题", "p1", 0.5);

  auto s = grid.run(3);
  ASSERT_TRUE(s.ok());
  EXPECT_TRUE(grid.converged());
  EXPECT_GT(grid.total_nodes(), 3);
}

TEST(GridTest, ReportStructure) {
  GridReasoner grid;
  grid.set_expand_fn(expand_fn);
  grid.seed("测试问题", "p1", 0.5);
  grid.run(2);

  auto j = grid.report();
  EXPECT_TRUE(j["converged"].get<bool>());
  EXPECT_GT(j["total_nodes"].get<int>(), 0);
  EXPECT_TRUE(j["nodes"].is_array());
  EXPECT_FALSE(j["theorem"].is_null());
}

TEST(GridTest, MultiplePaths) {
  GridReasoner grid;
  grid.set_expand_fn(expand_fn);
  grid.seed("路径A", "math", 0.5);
  grid.seed("路径B", "logic", 0.6);

  // 不运行, 只检查播种
  EXPECT_EQ(grid.total_nodes(), 2);
}

TEST(GridTest, Weave) {
  GridReasoner grid;
  grid.set_expand_fn(expand_fn);
  grid.seed("A", "p1", 0.5);
  grid.expand();  // layer 1
  grid.expand();  // layer 2

  auto s = grid.weave();
  ASSERT_TRUE(s.ok());
  // 编织节点后应有 weave 源节点
  bool found_weave = false;
  for (const auto& n : grid.nodes()) {
    if (n.source == "weave") { found_weave = true; break; }
  }
  EXPECT_TRUE(found_weave);
}

TEST(GridTest, LeafNodes) {
  GridReasoner grid;
  grid.set_expand_fn(expand_fn);
  grid.seed("根", "p1", 0.5);
  grid.expand();

  auto leaves = grid.leaf_nodes();
  EXPECT_EQ(static_cast<int>(leaves.size()), 2);
}

TEST(GridTest, CustomConverge) {
  GridReasoner grid;
  grid.set_expand_fn(expand_fn);
  grid.set_converge_fn(converge_fn);
  grid.seed("测试", "p1", 0.5);
  grid.run(2);

  auto j = grid.report();
  EXPECT_NEAR(j["theorem_confidence"].get<double>(), 0.99, 0.001);
}

TEST(GridTest, NodeToJson) {
  GridNode node;
  node.id = 5;
  node.parent_id = 3;
  node.path_id = 1;
  node.layer = 2;
  node.content = "测试节点";
  node.confidence = 0.75;
  node.source = "expand";

  auto j = node.to_json();
  EXPECT_EQ(j["id"].get<int>(), 5);
  EXPECT_EQ(j["source"].get<std::string>(), "expand");
}

TEST(GridTest, ExpandWithoutFn) {
  GridReasoner grid;
  // 未设置 expand_fn 时调用 expand 应返回错误
  auto s = grid.expand();
  EXPECT_FALSE(s.ok());
}
