// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/bridge/seed_buffer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_set>

namespace nexus::bridge {

// ═══════════════════════════════════════════════════════════════════
// SeedBufferEntry
// ═══════════════════════════════════════════════════════════════════

auto SeedBufferEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]      = name;
  j["intensity"] = intensity;
  j["source"]    = source;
  j["domain"]    = domain;
  j["detail"]    = detail;
  j["type"]      = seed_type;
  if (!center_14d.empty()) {
    auto arr = nlohmann::json::array();
    for (double v : center_14d) arr.push_back(v);
    j["center_14d"] = arr;
  }
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// SeedBuffer
// ═══════════════════════════════════════════════════════════════════

SeedBuffer::SeedBuffer(int auto_flush) noexcept
  : auto_flush_(auto_flush) {}

void SeedBuffer::add(const std::string& name, double intensity,
                      const std::string& source, const std::string& domain,
                      const std::string& detail,
                      const std::string& seed_type,
                      const std::vector<double>& center_14d) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  SeedBufferEntry entry;
  entry.name      = name;
  entry.intensity = intensity;
  entry.source    = source;
  entry.domain    = domain;
  entry.detail    = detail;
  entry.seed_type = seed_type;
  entry.center_14d = center_14d;
  pending_.push_back(std::move(entry));
}

auto SeedBuffer::flush(bridge::SeedBank* bank) noexcept -> int {
  std::lock_guard<std::mutex> lock(mutex_);
  if (pending_.empty() || !bank) return 0;

  int count = 0;
  for (const auto& p : pending_) {
    SeedEntry seed;
    seed.name        = p.name;
    seed.intensity   = static_cast<int>(p.intensity);
    seed.source      = p.source;
    seed.type        = p.seed_type;
    seed.domain_tag  = p.domain;
    seed.step_detail = p.detail;

    // 注入种子库
    bank->inject(seed, {p.domain});
    count++;
  }

  pending_.clear();
  return count;
}

auto SeedBuffer::size() const noexcept -> int {
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<int>(pending_.size());
}

void SeedBuffer::clear() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  pending_.clear();
}

// ═══════════════════════════════════════════════════════════════════
// SemanticSynthesizer — 域名称映射
// ═══════════════════════════════════════════════════════════════════

const std::vector<std::pair<std::string, std::string>>&
SemanticSynthesizer::domain_names_() {
  static const std::vector<std::pair<std::string, std::string>> names = {
    {"meta", "元认知"}, {"logic", "逻辑"}, {"physics", "物理"},
    {"metamathematics", "元数学"}, {"mathematics", "数学"},
    {"formal_language", "形式语言"}, {"game", "博弈"}, {"game_theory", "博弈论"},
    {"quantum", "量子"}, {"crypto", "密码"}, {"causal", "因果"},
    {"evolution", "进化"}, {"relativity", "相对论"}, {"rl", "强化学习"},
    {"cached", "标准库"}, {"proof_lib", "证明库"},
  };
  return names;
}

auto SemanticSynthesizer::domain_name(const std::string& tag) -> std::string {
  auto parts = tag;
  // 取 '+' 前的部分
  auto plus_pos = parts.find('+');
  if (plus_pos != std::string::npos) parts = parts.substr(0, plus_pos);

  for (const auto& [key, name] : domain_names_()) {
    if (parts == key) return name;
  }
  return parts;
}

// ═══════════════════════════════════════════════════════════════════
// SemanticSynthesizer — 融合模板
// ═══════════════════════════════════════════════════════════════════

const std::vector<std::pair<std::string, std::string>>&
SemanticSynthesizer::fusion_templates_() {
  static const std::vector<std::pair<std::string, std::string>> templates = {
    {"logic+physics", "逻辑形式系统与物理实在的对应结构"},
    {"logic+meta", "形式逻辑在元认知层面的自指应用"},
    {"physics+meta", "物理定律与认知约束的深层同构"},
    {"physics+quantum", "经典物理与量子理论的边界条件"},
    {"logic+mathematics", "逻辑系统在数学结构中的嵌入"},
    {"meta+mathematics", "元认知对数学真理的认知边界"},
    {"game_theory+meta", "博弈论中的元策略与认知层级"},
    {"game_theory+evolution", "博弈均衡与演化稳定策略的统一"},
    {"crypto+quantum", "量子计算对经典密码体制的冲击"},
    {"causal+meta", "因果推断与元认知的反事实推理联系"},
  };
  return templates;
}

// ═══════════════════════════════════════════════════════════════════
// SemanticSynthesizer — 关键术语提取
// ═══════════════════════════════════════════════════════════════════

auto SemanticSynthesizer::extract_key_terms(const std::string& text,
                                             int max_terms) -> std::vector<std::string> {
  if (text.empty()) return {};

  std::unordered_set<std::string> seen;
  std::vector<std::string> result;

  // 英文大写术语
  std::regex en_re(R"([A-Z][a-z]+(?:_[A-Z][a-z]+)*)");
  std::smatch m;
  std::string s = text;
  while (std::regex_search(s, m, en_re)) {
    std::string term = m[0];
    if (term.size() > 1 && seen.find(term) == seen.end()) {
      seen.insert(term);
      result.push_back(term);
    }
    s = m.suffix().str();
  }

  // 中文字词 (2-6 字) — 匹配多字节 UTF-8 序列
  std::regex cn_re(R"([\xC0-\xFF][\x80-\xBF]{1,5})");
  s = text;
  while (std::regex_search(s, m, cn_re)) {
    std::string term = m[0];
    if (term.size() >= 3 && seen.find(term) == seen.end()) {
      seen.insert(term);
      result.push_back(term);
    }
    s = m.suffix().str();
  }

  if (static_cast<int>(result.size()) > max_terms)
    result.resize(max_terms);
  return result;
}

// ═══════════════════════════════════════════════════════════════════
// SemanticSynthesizer — 合成
// ═══════════════════════════════════════════════════════════════════

auto SemanticSynthesizer::synthesize(
    const std::string& name_a, const std::string& detail_a,
    const std::string& domain_a,
    const std::string& name_b, const std::string& detail_b,
    const std::string& domain_b, double proximity) -> std::string {

  auto da_name = domain_name(domain_a);
  auto db_name = domain_name(domain_b);

  // 提取关键术语
  auto terms_a = extract_key_terms(detail_a);
  auto terms_b = extract_key_terms(detail_b);

  // 共同术语
  std::vector<std::string> common;
  for (const auto& ta : terms_a) {
    for (const auto& tb : terms_b) {
      if (ta == tb) { common.push_back(ta); break; }
    }
  }

  // 取种子名的关键部分
  auto conc_a = name_a;
  auto last_colon = conc_a.rfind(':');
  if (last_colon != std::string::npos) conc_a = conc_a.substr(last_colon + 1);
  auto last_slash = conc_a.rfind('/');
  if (last_slash != std::string::npos) conc_a = conc_a.substr(last_slash + 1);
  if (conc_a.size() > 25) conc_a = conc_a.substr(0, 25);

  auto conc_b = name_b;
  last_colon = conc_b.rfind(':');
  if (last_colon != std::string::npos) conc_b = conc_b.substr(last_colon + 1);
  last_slash = conc_b.rfind('/');
  if (last_slash != std::string::npos) conc_b = conc_b.substr(last_slash + 1);
  if (conc_b.size() > 25) conc_b = conc_b.substr(0, 25);

  // 尝试模板匹配
  std::vector<std::string> domains = {domain_a, domain_b};
  std::sort(domains.begin(), domains.end());
  std::string pair_key = domains[0] + "+" + domains[1];

  std::string lemma;
  bool found_template = false;
  for (const auto& [key, tmpl] : fusion_templates_()) {
    if (key == pair_key) { lemma = tmpl; found_template = true; break; }
  }

  if (!found_template) {
    if (!common.empty()) {
      lemma = da_name + " 与 " + db_name + " 的结构对应";
    } else {
      lemma = da_name + "(" + conc_a + ") 与 " + db_name + "(" + conc_b + ") 的结构关系";
    }
  }

  std::ostringstream os;
  os << "[引理] " << lemma;
  os << " | 来源: " << conc_a << " ✕ " << conc_b;
  os << " | 置信度:" << std::round(proximity * 100) / 100;

  return os.str();
}

}  // namespace nexus::bridge
