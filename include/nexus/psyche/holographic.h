#ifndef NEXUS_PSYCHE_HOLOGRAPHIC_H
#define NEXUS_PSYCHE_HOLOGRAPHIC_H

/**
 * @file holographic.h
 * @brief 全息系统初始化 — 语义场 + 路径引擎 + 概念网络
 *
 * 全息系统是"整体包含局部"的语义表示层。
 * 初始化: 语义场(HotspotMap) → 路径引擎(ConceptPathway) → 桥接(FlowBridge)
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::psyche {

/// 语义热点
struct SemanticHotspot {
  std::string name;
  double      intensity = 0.0;
  int         neighbor_count = 0;

  auto to_json() const -> nlohmann::json;
};

/// 语义路径
struct SemanticPath {
  std::string from;
  std::string to;
  double      strength = 0.0;
  int         bridges = 0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// HolographicSystem
// ═══════════════════════════════════════════════════════════════════

class HolographicSystem {
public:
  HolographicSystem() noexcept;

  /// 初始化全息系统
  auto initialize() noexcept -> Status;

  /// 查询概念在语义场中的位置
  auto query_concept(const std::string& name) noexcept -> nlohmann::json;

  /// 探索概念路径
  auto explore_path(const std::string& from, const std::string& to) noexcept
      -> std::vector<SemanticPath>;

  /// 获取统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;

private:
  bool initialized_ = false;
  std::vector<SemanticHotspot> hotspots_;
  std::vector<SemanticPath> paths_;

  auto build_default_hotspots_() noexcept -> void;
  auto build_default_paths_() noexcept -> void;
};

}  // namespace nexus::psyche

#endif
