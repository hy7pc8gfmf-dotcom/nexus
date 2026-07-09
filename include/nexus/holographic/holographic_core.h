#ifndef NEXUS_HOLOGRAPHIC_CORE_H
#define NEXUS_HOLOGRAPHIC_CORE_H

/**
 * @file holographic_core.h
 * @brief 全息系统 — 语义场 + 概念路径 + 流桥接
 *
 * 语义场: 概念热点的分布式表示
 * 概念路径: 概念间确定性推理路径
 * 流桥接: 跨语言/跨模态语义映射
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>

#include "nexus/types/status.h"

namespace nexus::holographic {

/// 语义热点
struct Hotspot {
  std::string name;
  double      intensity    = 0.0;
  int         neighbors    = 0;
  std::string domain;

  auto to_json() const -> nlohmann::json;
};

/// 语义路径
struct Pathway {
  std::string from;
  std::string to;
  double      strength = 0.0;
  int         bridges  = 0;

  auto to_json() const -> nlohmann::json;
};

/// 生成语句
struct GeneratedSentence {
  std::string subject;
  std::string template_type;  // definition | causal | relation | example | contrast
  std::string sentence;
  std::string language;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 语义场
// ═══════════════════════════════════════════════════════════════════

class SemanticField {
public:
  SemanticField() noexcept;

  auto build_default() noexcept -> void;
  auto query(const std::string& name) noexcept -> nlohmann::json;
  auto neighbors(const std::string& name, int depth = 1) noexcept -> std::vector<Hotspot>;
  auto stats() const noexcept -> nlohmann::json;

private:
  std::unordered_map<std::string, Hotspot> hotspots_;
  std::vector<Pathway> pathways_;
};

// ═══════════════════════════════════════════════════════════════════
// 概念路径引擎
// ═══════════════════════════════════════════════════════════════════

class ConceptPathway {
public:
  ConceptPathway(SemanticField* field) noexcept;

  auto explore(const std::string& from, const std::string& to) noexcept
      -> std::vector<Pathway>;
  auto beam_search(const std::string& start, int width = 3, int depth = 3) noexcept
      -> std::vector<std::vector<Pathway>>;

private:
  SemanticField* field_;
};

// ═══════════════════════════════════════════════════════════════════
// 流桥接 (句子生成)
// ═══════════════════════════════════════════════════════════════════

class FlowBridge {
public:
  FlowBridge(SemanticField* field) noexcept;

  auto generate(const std::string& topic, const std::string& lang = "zh") noexcept
      -> std::vector<GeneratedSentence>;
  auto calibrate(const std::string& source, const std::string& target_lang) noexcept
      -> std::string;
  auto stats() const noexcept -> nlohmann::json;

private:
  SemanticField* field_;
};

}  // namespace nexus::holographic

#endif
