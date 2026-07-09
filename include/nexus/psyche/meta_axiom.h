#ifndef NEXUS_PSYCHE_META_AXIOM_H
#define NEXUS_PSYCHE_META_AXIOM_H

/**
 * @file meta_axiom.h
 * @brief 元公理引擎 — 在线 Coq 风格公理推导
 *
 * 检测系统公理缺口, 使用 Coq 风格的形式化推理自动填补。
 * 产出新公理 → 注入种子库。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::psyche {

/// 公理缺口
struct AxiomGap {
  std::string id;
  std::string domain;
  std::string description;
  int severity = 5;

  auto to_json() const -> nlohmann::json;
};

/// 公理定义
struct Axiom {
  std::string id;
  std::string statement;
  std::string domain;
  double confidence = 1.0;
  bool from_coq = false;

  auto to_json() const -> nlohmann::json;
};

class MetaAxiomEngine {
public:
  MetaAxiomEngine() noexcept;

  auto run_cycle() noexcept -> std::vector<Axiom>;
  auto detect_gaps() noexcept -> std::vector<AxiomGap>;
  auto derive_axiom(const AxiomGap& gap) noexcept -> Axiom;
  [[nodiscard]] auto axioms() const noexcept -> const std::vector<Axiom>&;
  [[nodiscard]] auto summary() const noexcept -> nlohmann::json;

private:
  std::vector<Axiom> axioms_;
};

}  // namespace nexus::psyche

#endif
