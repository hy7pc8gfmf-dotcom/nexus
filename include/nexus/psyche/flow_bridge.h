// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_FLOW_BRIDGE_H
#define NEXUS_PSYCHE_FLOW_BRIDGE_H

/**
 * @file flow_bridge.h
 * @brief 语义流桥 — 路径→多模板多句生成
 *
 * 移植自 D:/synapse/semantic_flow_bridge.py (802行)
 *
 * 将 ConceptPathway 的路径转换为多模板自然文本。
 * 支持: 定义/因果/关系/举例/对比 五种句型
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/psyche/semantic_port.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 语义流桥
// ═══════════════════════════════════════════════════════════════════

class FlowBridge {
public:
  explicit FlowBridge(SemanticPort* port = nullptr) noexcept;

  void set_port(SemanticPort* port) noexcept { port_ = port; }

  /// 路径 → 多句生成
  auto path_to_sentences(const std::vector<std::string>& path,
                          const std::string& lang = "zh") const noexcept
      -> std::vector<std::string>;

  /// 概念 → 定义句
  auto define(const std::string& c) const noexcept -> std::string;

  /// 概念A → 概念B 因果句
  auto causal(const std::string& a, const std::string& b) const noexcept
      -> std::string;

  /// 概念A ↔ 概念B 关系句
  auto relate(const std::string& a, const std::string& b) const noexcept
      -> std::string;

  /// 概念 → 示例句
  auto example(const std::string& c) const noexcept -> std::string;

  /// 概念A vs 概念B 对比句
  auto contrast(const std::string& a, const std::string& b) const noexcept
      -> std::string;

  /// 生成完整报告
  auto generate(const std::string& topic, int n_sentences = 5) const noexcept
      -> std::vector<std::string>;

private:
  SemanticPort* port_ = nullptr;

  static std::string translate_if_zh_(const std::string& name,
                                       const std::string& lang);
  static std::string detokenize_(const std::string& name);
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_FLOW_BRIDGE_H
