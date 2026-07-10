// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/flow_bridge.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 辅助
// ═══════════════════════════════════════════════════════════════════

std::string FlowBridge::detokenize_(const std::string& name) {
  std::string r = name;
  for (auto& c : r) { if (c == '_' || c == '/') c = ' '; }
  return r;
}

std::string FlowBridge::translate_if_zh_(const std::string& name,
                                           const std::string& lang) {
  if (lang != "zh") return detokenize_(name);
  // 简单英文→中文映射
  static const std::map<std::string, std::string> dict = {
    {"entropy", "熵"}, {"energy", "能量"}, {"system", "系统"},
    {"emergence", "涌现"}, {"recursion", "递归"}, {"symmetry", "对称"},
    {"information", "信息"}, {"causality", "因果"}, {"duality", "对偶"},
    {"coherence", "相干"}, {"complexity", "复杂度"}, {"network", "网络"},
    {"water", "水"}, {"fire", "火"}, {"consciousness", "意识"},
    {"time", "时间"}, {"space", "空间"}, {"truth", "真"},
    {"belief", "信念"}, {"knowledge", "知识"},
  };
  auto it = dict.find(name);
  if (it != dict.end()) return it->second;
  return detokenize_(name);
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

FlowBridge::FlowBridge(SemanticPort* port) noexcept : port_(port) {}

// ═══════════════════════════════════════════════════════════════════
// 句型模板
// ═══════════════════════════════════════════════════════════════════

auto FlowBridge::define(const std::string& c) const noexcept -> std::string {
  auto name = translate_if_zh_(c, "zh");
  return name + " 是语义场中的一个重要概念。";
}

auto FlowBridge::causal(const std::string& a, const std::string& b) const noexcept
    -> std::string {
  auto na = translate_if_zh_(a, "zh");
  auto nb = translate_if_zh_(b, "zh");
  return na + " 影响 " + nb + ", 二者存在因果关系。";
}

auto FlowBridge::relate(const std::string& a, const std::string& b) const noexcept
    -> std::string {
  auto na = translate_if_zh_(a, "zh");
  auto nb = translate_if_zh_(b, "zh");
  return na + " 与 " + nb + " 在语义上紧密关联。";
}

auto FlowBridge::example(const std::string& c) const noexcept -> std::string {
  auto name = translate_if_zh_(c, "zh");
  return "例如, " + name + " 在系统中表现为自组织现象。";
}

auto FlowBridge::contrast(const std::string& a, const std::string& b) const noexcept
    -> std::string {
  auto na = translate_if_zh_(a, "zh");
  auto nb = translate_if_zh_(b, "zh");
  return na + " 与 " + nb + " 虽然相关, 但分属不同的语义维度。";
}

// ═══════════════════════════════════════════════════════════════════
// 路径→多句
// ═══════════════════════════════════════════════════════════════════

auto FlowBridge::path_to_sentences(const std::vector<std::string>& path,
                                     const std::string& lang) const noexcept
    -> std::vector<std::string> {
  std::vector<std::string> result;
  if (path.empty()) return result;

  // 起始概念: 定义句
  result.push_back(define(path[0]));

  // 中间路径: 关系句 / 因果句
  for (size_t i = 1; i < path.size(); ++i) {
    result.push_back(relate(path[i-1], path[i]));
  }

  // 末尾: 示例句
  result.push_back(example(path.back()));

  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 生成
// ═══════════════════════════════════════════════════════════════════

auto FlowBridge::generate(const std::string& topic, int n_sentences) const noexcept
    -> std::vector<std::string> {
  std::vector<std::string> result;

  if (!port_ || !port_->loaded()) {
    result.push_back(topic + " 是一个语义概念。");
    return result;
  }

  // 获取邻居
  auto neighbors = port_->neighbors(topic, n_sentences);

  // 定义句
  result.push_back(define(topic));

  // 关系句 / 对比句交替
  for (size_t i = 0; i < neighbors.size() && static_cast<int>(result.size()) < n_sentences; ++i) {
    if (i % 2 == 0) {
      result.push_back(relate(topic, neighbors[i]));
    } else {
      result.push_back(contrast(topic, neighbors[i]));
    }
  }

  // 示例句收尾
  if (static_cast<int>(result.size()) < n_sentences + 1) {
    result.push_back(example(topic));
  }

  return result;
}

}  // namespace nexus::psyche
