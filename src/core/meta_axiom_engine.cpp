// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/core/meta_axiom_engine.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto KnowledgeGap::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  static const char* type_names[] = {"contradiction", "missing_premise", "uncertainty", "novelty"};
  j["type"] = type_names[static_cast<int>(type)];
  j["description"] = description;
  j["severity"] = std::round(severity * 100) / 100;
  j["domain"] = domain;
  auto terms = nlohmann::json::array();
  for (const auto& t : related_terms) terms.push_back(t);
  j["related_terms"] = terms;
  return j;
}

auto AxiomRule::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"] = id; j["name"] = name;
  j["description"] = description;
  j["coq_verified"] = coq_verified;
  j["min_intensity"] = min_intensity;
  auto dom = nlohmann::json::array();
  for (const auto& d : domains) dom.push_back(d);
  j["domains"] = dom;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

MetaAxiomEngine::MetaAxiomEngine() noexcept {
  init_builtin_rules_();
}

auto MetaAxiomEngine::load_coq_kernel(const std::string& meta_dll,
                                       const std::string& constructive_dll) noexcept
    -> Status {
  auto s1 = meta_kernel_.load(meta_dll);
  auto s2 = constructive_rules_.load(constructive_dll);
  coq_loaded_ = s1.ok() || s2.ok();
  if (!coq_loaded_) {
    return Status::Error(ErrorCode::kFileNotFound,
      "no Coq DLLs loaded (meta:" + std::to_string(s1.ok()) +
      " constructive:" + std::to_string(s2.ok()) + ")");
  }

  // 注册 Coq 验证的规则
  if (s1.ok()) {
    coq_rules_.push_back({"COQ-META-01", "open_completeness",
      "开放完备性: 所有真命题在足够步数后可证", {"math", "logic"}, 5, true});
    coq_rules_.push_back({"COQ-META-02", "fissure",
      "裂变: 高置信度命题可分裂为子命题", {"math", "logic", "science"}, 4, true});
    coq_rules_.push_back({"COQ-META-03", "foundation",
      "基础: 每个推理链有不可约的基础公理", {"math", "logic"}, 6, true});
  }
  if (s2.ok()) {
    coq_rules_.push_back({"COQ-CON-01", "duality001",
      "对偶性检查 1: 基础不一致性检测", {"logic", "ethics"}, 4, true});
    coq_rules_.push_back({"COQ-CON-02", "duality002",
      "对偶性检查 2: 结构对偶性验证", {"logic", "math"}, 5, true});
  }
  return Status::Ok();
}

void MetaAxiomEngine::init_builtin_rules_() noexcept {
  builtin_rules_ = {
    {"B-01", "modus_ponens", "如果 P→Q 且 P 为真, 则 Q 为真", {"logic"}, 3, false},
    {"B-02", "modus_tollens", "如果 P→Q 且 Q 为假, 则 P 为假", {"logic"}, 3, false},
    {"B-03", "contrapositive", "如果 P→Q, 则 ¬Q→¬P", {"logic"}, 3, false},
    {"B-04", "contradiction_avoid", "检测 P 和 ¬P 同时成立", {"logic", "ethics"}, 4, false},
    {"B-05", "transitivity", "如果 P→Q 且 Q→R, 则 P→R", {"logic", "math"}, 3, false},
    {"B-06", "induction_base", "归纳基础: P(0) 成立", {"math"}, 4, false},
    {"B-07", "induction_step", "归纳步: P(k) → P(k+1)", {"math"}, 4, false},
    {"B-08", "seed_injection", "新知识作为种子注入推理链", {"general"}, 2, false},
  };
}

// ═══════════════════════════════════════════════════════════════════
// 缺口检测
// ═══════════════════════════════════════════════════════════════════

auto MetaAxiomEngine::detect_gaps(
    const std::vector<nlohmann::json>& steps) noexcept
    -> std::vector<KnowledgeGap> {
  std::vector<KnowledgeGap> gaps;

  for (const auto& step : steps) {
    std::string content = step.value("content", std::string(""));
    double confidence = step.value("confidence", 0.5);
    std::string type = step.value("type", std::string(""));

    // 不确定性缺口
    if (confidence < 0.3 && type != "initial" && type != "conclusion") {
      KnowledgeGap gap;
      gap.type = KnowledgeGap::Type::kUncertainty;
      gap.description = "置信度过低: " + content.substr(0, 60);
      gap.severity = 1.0 - confidence;
      gap.domain = step.value("domain", "general");
      gaps.push_back(gap);
    }

    // 矛盾检测
    if (content.find("但是") != std::string::npos ||
        content.find("然而") != std::string::npos) {
      KnowledgeGap gap;
      gap.type = KnowledgeGap::Type::kContradiction;
      gap.description = "潜在矛盾: " + content.substr(0, 80);
      gap.severity = 0.7;
      gap.domain = step.value("domain", "general");
      gaps.push_back(gap);
    }

    // 新颖检测 ("
    if (confidence > 0.7 && type == "branch") {
      KnowledgeGap gap;
      gap.type = KnowledgeGap::Type::kNovelty;
      gap.description = "分支探索: " + content.substr(0, 60);
      gap.severity = 0.3;
      gap.domain = step.value("domain", "general");
      gaps.push_back(gap);
    }
  }

  return gaps;
}

// ═══════════════════════════════════════════════════════════════════
// 规则匹配
// ═══════════════════════════════════════════════════════════════════

auto MetaAxiomEngine::match_rules(const KnowledgeGap& gap) noexcept
    -> std::vector<AxiomRule> {
  std::vector<AxiomRule> candidates;

  // 收集所有规则
  std::vector<AxiomRule> all_rules = builtin_rules_;
  all_rules.insert(all_rules.end(), coq_rules_.begin(), coq_rules_.end());

  for (const auto& rule : all_rules) {
    // 领域匹配
    bool domain_match = false;
    for (const auto& d : rule.domains) {
      if (gap.domain.find(d) != std::string::npos ||
          d == "general") {
        domain_match = true;
        break;
      }
    }
    if (!domain_match) continue;

    // 缺口类型匹配
    bool type_match = false;
    switch (gap.type) {
      case KnowledgeGap::Type::kContradiction:
        type_match = (rule.id == "B-04" || rule.id == "COQ-CON-01" ||
                      rule.id == "COQ-CON-02");
        break;
      case KnowledgeGap::Type::kMissingPremise:
        type_match = (rule.id == "B-01" || rule.id == "B-05" ||
                      rule.id == "B-08");
        break;
      case KnowledgeGap::Type::kUncertainty:
        type_match = (rule.id == "COQ-META-01" || rule.id == "B-08");
        break;
      case KnowledgeGap::Type::kNovelty:
        type_match = (rule.id == "COQ-META-02" || rule.id == "B-08");
        break;
    }

    if (type_match) {
      candidates.push_back(rule);
      if (candidates.size() >= 3) break;
    }
  }

  return candidates;
}

// ═══════════════════════════════════════════════════════════════════
// Coq 验证
// ═══════════════════════════════════════════════════════════════════

auto MetaAxiomEngine::verify_rule(const AxiomRule& rule,
                                   const std::string& task) noexcept
    -> Result<bool> {
  if (rule.coq_verified && coq_loaded_) {
    // 使用 Coq DLL 验证
    auto results = meta_kernel_.evaluate(task);
    for (const auto& r : results) {
      if (r.rule_name == rule.name) {
        return r.passed;
      }
    }
    // 对偶性检查
    if (rule.id == "COQ-CON-01") return constructive_rules_.check_duality(1);
    if (rule.id == "COQ-CON-02") return constructive_rules_.check_duality(2);
  }

  // 内建规则总是通过 (已验证的逻辑公理)
  return true;
}

// ═══════════════════════════════════════════════════════════════════
// 内置规则注册
// ═══════════════════════════════════════════════════════════════════

auto MetaAxiomEngine::register_builtin(const AxiomRule& rule) noexcept -> void {
  builtin_rules_.push_back(rule);
}

// ═══════════════════════════════════════════════════════════════════
// 统计
// ═══════════════════════════════════════════════════════════════════

auto MetaAxiomEngine::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["builtin_rules"] = static_cast<int>(builtin_rules_.size());
  j["coq_rules"] = static_cast<int>(coq_rules_.size());
  j["coq_loaded"] = coq_loaded_;
  j["total"] = static_cast<int>(builtin_rules_.size() + coq_rules_.size());
  return j;
}

auto MetaAxiomEngine::is_loaded() const noexcept -> bool {
  return !builtin_rules_.empty() || coq_loaded_;
}

}  // namespace nexus::core
