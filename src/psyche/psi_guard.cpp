// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/psi_guard.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <regex>
#include <sstream>
#include <unordered_set>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 辅助
// ═══════════════════════════════════════════════════════════════════

auto AuditIssue::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["type"]      = type;
  j["detail"]    = detail;
  j["severity"]  = severity;
  if (!correction.empty()) j["correction"] = correction;
  return j;
}

auto AuditResult::errors() const noexcept -> std::vector<AuditIssue> {
  std::vector<AuditIssue> errs;
  for (const auto& i : issues) {
    if (i.severity == "error") errs.push_back(i);
  }
  return errs;
}

auto AuditResult::warnings() const noexcept -> std::vector<AuditIssue> {
  std::vector<AuditIssue> warns;
  for (const auto& i : issues) {
    if (i.severity == "warning") warns.push_back(i);
  }
  return warns;
}

auto AuditResult::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["passed"]  = passed;
  j["source"]  = source;
  j["domain"]  = domain;
  auto arr = nlohmann::json::array();
  for (const auto& i : issues) arr.push_back(i.to_json());
  j["issues"] = arr;
  auto corr = nlohmann::json::array();
  for (const auto& c : corrections) corr.push_back(c);
  j["corrections"] = corr;
  j["n_errors"]   = static_cast<int>(errors().size());
  j["n_warnings"] = static_cast<int>(warnings().size());
  return j;
}

bool PsiGuard::contains_word_(const std::string& text, const std::string& word) {
  return text.find(word) != std::string::npos;
}

std::vector<std::string> PsiGuard::extract_symbols_(const std::string& text) {
  std::vector<std::string> symbols;
  std::string cur;
  for (size_t i = 0; i < text.size(); ++i) {
    if (std::isalpha(static_cast<unsigned char>(text[i]))) {
      cur += text[i];
    } else {
      if (cur.size() > 1 && std::isupper(static_cast<unsigned char>(cur[0]))) {
        symbols.push_back(cur);
      }
      cur.clear();
    }
  }
  if (cur.size() > 1 && std::isupper(static_cast<unsigned char>(cur[0]))) {
    symbols.push_back(cur);
  }
  return symbols;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

PsiGuard::PsiGuard() noexcept {
  // 内置已知术语
  std::vector<const char*> builtin = {
    "ZFC","PA","PRA","CoC","CIC","λC",
    "Gödel","Löb","Tarski","Feferman","Schütte","Buchholz",
    "Shannon","Boltzmann","Hilbert","Landauer",
    "PAC","VCdim","RL","MDP","POMDP","CNN","MLP","GNN",
    "softmax","ReLU","MoE","TPR",
    "Lyapunov","Hamiltonian","Lagrangian","Jacobian",
    "Condorcet","Ising","Hopfield","Solomonoff",
    "argmax","argmin","sup","inf","lim","log","exp",
  };
  for (auto* t : builtin) known_terms_.push_back(t);
  terms_loaded_ = true;
}

void PsiGuard::load_known_terms(bridge::SeedBank* bank) noexcept {
  terms_loaded_ = true;
  if (!bank) return;

  // 从种子库加载种子名作为已知术语
  for (const auto& s : bank->query_by_intensity(1, 500)) {
    known_terms_.push_back(s.name);
    // 提取下划线分隔的片段
    std::string part;
    for (char c : s.name) {
      if (c == '_' || c == ':') {
        if (part.size() > 1) known_terms_.push_back(part);
        part.clear();
      } else {
        part += c;
      }
    }
    if (part.size() > 1) known_terms_.push_back(part);
  }
}

// ═══════════════════════════════════════════════════════════════════
// Phase 1: 形式验证
// ═══════════════════════════════════════════════════════════════════

auto PsiGuard::phase1_formal_(const std::string& claim) noexcept
    -> std::vector<AuditIssue> {
  std::vector<AuditIssue> issues;

  // 1. 未定义符号检测
  auto symbols = extract_symbols_(claim);
  for (const auto& s : symbols) {
    bool found = false;
    for (const auto& t : known_terms_) {
      if (t == s) { found = true; break; }
    }
    if (!found) {
      issues.push_back({"undefined_term",
        "符号 '" + s + "' 未在已知术语库中找到", "warning", ""});
    }
  }

  // 2. 未证明断言检测
  struct Pattern { std::string pattern; std::string msg; };
  std::vector<Pattern> patterns = {
    {"something huge", "非学术表述: 'something huge' -> 需替换为精确序数分析"},
    {"known inconsistent", "需验证: '已知不一致' -> 引用原始文献确认"},
    {"trivial", "'trivial' -> 应给出简要证明或引用"},
    {"obviously", "'obviously' -> 应明确推导步骤"},
    {"clearly", "'clearly' -> 应明确推导步骤"},
    {"it can be shown", "'it can be shown' -> 需实际展示"},
    {"one can prove", "'one can prove' -> 需实际证明"},
  };
  for (const auto& p : patterns) {
    if (contains_word_(claim, p.pattern)) {
      issues.push_back({"unproven_assertion", p.msg, "error", ""});
    }
  }

  // 3. CoC 一致性声明检查
  if (contains_word_(claim, "CoC") &&
      (contains_word_(claim, "不一致") || contains_word_(claim, "inconsistent"))) {
    issues.push_back({
      "consistency_misstatement",
      "CoC(λC)是一致系统(Coquand-Huet 1988). 不一致的是Type:Type, 非CoC.",
      "error",
      "λC(Coquand-Huet 1988)是一致的. 误将Type:Type的不一致归因于CoC."
    });
  }

  // 4. 收敛条件完整性检查
  if (contains_word_(claim, "收敛") || contains_word_(claim, "converge") ||
      contains_word_(claim, "convergence")) {
    if (!contains_word_(claim, "当") && !contains_word_(claim, "if") &&
        !contains_word_(claim, "假设") && !contains_word_(claim, "assume")) {
      issues.push_back({
        "missing_condition",
        "收敛断言缺少边界条件 (如 N->oo? 有限样本? 概率意义?)",
        "warning", ""
      });
    }
  }

  // 5. 量纲检查: exp() 参数必须无量纲
  std::regex exp_re(R"(exp\(([^)]+)\))");
  std::smatch m;
  std::string s = claim;
  while (std::regex_search(s, m, exp_re)) {
    std::string arg = m[1];
    bool has_div = (arg.find('/') != std::string::npos);
    bool has_tau = (arg.find("tau") != std::string::npos || arg.find("τ") != std::string::npos);
    if (has_tau && !has_div) {
      issues.push_back({
        "dimensional_error",
        "exp(" + arg + ") 中包含时间参数tau但无除法消去量纲",
        "error",
        "tau是时间量纲, 必须与其他时间量纲变量相除才能消去"
      });
    }
    s = m.suffix().str();
  }

  return issues;
}

// ═══════════════════════════════════════════════════════════════════
// Phase 2: 跨题一致性
// ═══════════════════════════════════════════════════════════════════

auto PsiGuard::phase2_cross_question_(const std::string& claim,
                                       const std::string& source) noexcept
    -> std::vector<AuditIssue> {
  std::vector<AuditIssue> issues;

  for (const auto& h : claim_history_) {
    // 检查一致性反转
    if ((contains_word_(claim, "inconsistent") || contains_word_(claim, "不一致")) &&
        (contains_word_(h.claim, "consistent") || contains_word_(h.claim, "一致"))) {
      issues.push_back({
        "consistency_contradiction",
        "与本系统之前的断言(" + h.source + ")矛盾",
        "error", ""
      });
    }

    // 检查模糊 vs 精确
    if (contains_word_(claim, "huge") && contains_word_(h.claim, "ordinal")) {
      issues.push_back({
        "imprecise_vs_precise",
        "模糊表述(huge)与已存的精确序数分析(" + h.source + ")冲突",
        "error", ""
      });
    }
  }

  // 记录当前断言
  ClaimRecord rec;
  rec.claim = claim.substr(0, 100);
  rec.source = source;
  rec.timestamp = std::time(nullptr);
  claim_history_.push_back(rec);

  return issues;
}

// ═══════════════════════════════════════════════════════════════════
// Phase 3: 种子库一致性
// ═══════════════════════════════════════════════════════════════════

auto PsiGuard::phase3_seed_consistency_(const std::string& claim,
                                         const std::string& domain,
                                         bridge::SeedBank* bank) noexcept
    -> std::vector<AuditIssue> {
  std::vector<AuditIssue> issues;
  if (!bank) return issues;

  // 按域查询种子
  auto domain_seeds = bank->query_by_domain(domain, 20);
  for (const auto& s : domain_seeds) {
    if (s.intensity <= 1) {
      issues.push_back({
        "deprecated_seed_ref",
        "种子 '" + s.name + "' 强度极低(" + std::to_string(s.intensity) + "), 可能已过时",
        "info", ""
      });
    }
  }

  // 检查跨域引用
  auto all_seeds = bank->query_by_intensity(5, 50);
  for (const auto& s : all_seeds) {
    if (s.domain_tag != domain && contains_word_(claim, s.name)) {
      issues.push_back({
        "cross_domain_seed",
        "使用了域'" + s.domain_tag + "'的种子'" + s.name + "'",
        "info", ""
      });
    }
  }

  return issues;
}

// ═══════════════════════════════════════════════════════════════════
// 主审计
// ═══════════════════════════════════════════════════════════════════

auto PsiGuard::audit(const std::string& claim,
                      const std::string& source,
                      const std::string& domain) noexcept -> AuditResult {
  AuditResult result;
  result.source = source;
  result.domain = domain;

  // Phase 1
  auto p1 = phase1_formal_(claim);
  result.issues.insert(result.issues.end(), p1.begin(), p1.end());

  // Phase 2
  auto p2 = phase2_cross_question_(claim, source);
  result.issues.insert(result.issues.end(), p2.begin(), p2.end());

  // 判断是否通过
  result.passed = result.errors().empty();

  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 自动修正
// ═══════════════════════════════════════════════════════════════════

auto PsiGuard::auto_correct(const std::string& claim,
                             const AuditResult& result) noexcept
    -> std::pair<std::string, std::vector<std::string>> {
  std::string corrected = claim;
  std::vector<std::string> applied;

  for (const auto& issue : result.issues) {
    // 替换 "something huge"
    if (issue.type == "unproven_assertion" &&
        contains_word_(corrected, "something huge")) {
      auto pos = corrected.find("something huge");
      if (pos != std::string::npos) {
        corrected.replace(pos, 14,
          "proof-theoretically galactic (beyond current ordinal analysis)");
        applied.push_back("替换 'something huge' 为精确表述");
      }
    }

    // 修正 CoC 一致性
    if (issue.type == "consistency_misstatement") {
      auto pos = corrected.find("known inconsistent");
      if (pos != std::string::npos) {
        corrected.replace(pos, 18, "consistent (Coquand-Huet 1988)");
        applied.push_back("修正 CoC 一致性声明");
      }
      pos = corrected.find("已知不一致");
      if (pos != std::string::npos) {
        corrected.replace(pos, 5, "是一致系统(Coquand-Huet 1988)");
        applied.push_back("修正 CoC 一致性声明");
      }
    }

    // 添加收敛条件注释
    if (issue.type == "missing_condition") {
      corrected += " [注: 收敛范围需明确边界条件]";
      applied.push_back("添加收敛条件注释");
    }
  }

  return {corrected, applied};
}

}  // namespace nexus::psyche
