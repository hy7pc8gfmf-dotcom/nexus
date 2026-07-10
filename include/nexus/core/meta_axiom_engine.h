// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_CORE_META_AXIOM_ENGINE_H
#define NEXUS_CORE_META_AXIOM_ENGINE_H

/**
 * @file meta_axiom_engine.h
 * @brief 元公理引擎 — 知识缺口检测 + Coq 规则匹配 + 种子注入
 *
 * 架构:
 *   推理链缺口 → 缺口分类 → 规则匹配 → Coq DLL 验证 → 种子注入
 *
 * 不依赖 coqc 子进程: 所有 Coq 规则已预编译为 DLL,
 * 运行时仅做匹配 + 调用验证。
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/core/coq_kernel.h"
#include "nexus/types/status.h"

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// 知识缺口
// ═══════════════════════════════════════════════════════════════════

struct KnowledgeGap {
  enum class Type : uint8_t {
    kContradiction = 0,   // 矛盾: 前提 A 和 B 冲突
    kMissingPremise = 1,  // 缺前提: 结论缺少支持
    kUncertainty = 2,     // 不确定: 置信度不足
    kNovelty = 3,         // 新颖: 新领域无先验知识
  };

  Type        type;
  std::string description;     // 缺口描述
  double      severity = 0.0;  // [0,1]
  std::string domain;          // 所属领域
  std::vector<std::string> related_terms;  // 相关概念

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 公理规则
// ═══════════════════════════════════════════════════════════════════

struct AxiomRule {
  std::string id;            // 规则 ID
  std::string name;          // 规则名称
  std::string description;   // 规则描述
  std::vector<std::string> domains;     // 适用领域
  int         min_intensity = 3;        // 注入时最低强度
  bool        coq_verified = false;     // 是否经 Coq 验证

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 元公理引擎
// ═══════════════════════════════════════════════════════════════════

class MetaAxiomEngine {
public:
  MetaAxiomEngine() noexcept;

  /// 加载 Coq 规则 DLL
  auto load_coq_kernel(const std::string& meta_dll_path = "",
                        const std::string& constructive_dll_path = "") noexcept -> Status;

  /// 分析推理步骤 → 检测知识缺口
  auto detect_gaps(const std::vector<nlohmann::json>& reasoning_steps) noexcept
      -> std::vector<KnowledgeGap>;

  /// 匹配可用规则 → 填补缺口
  auto match_rules(const KnowledgeGap& gap) noexcept -> std::vector<AxiomRule>;

  /// 运行 Coq 验证并返回结果
  auto verify_rule(const AxiomRule& rule, const std::string& task) noexcept
      -> Result<bool>;

  /// 将规则注册为内建规则 (无需 DLL)
  auto register_builtin(const AxiomRule& rule) noexcept -> void;

  /// 获取统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;

  /// 是否已加载
  [[nodiscard]] auto is_loaded() const noexcept -> bool;

private:
  CoqMetaKernel meta_kernel_;
  ConstructiveRules constructive_rules_;
  std::vector<AxiomRule> builtin_rules_;
  std::vector<AxiomRule> coq_rules_;
  bool coq_loaded_ = false;

  void init_builtin_rules_() noexcept;
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_META_AXIOM_ENGINE_H
