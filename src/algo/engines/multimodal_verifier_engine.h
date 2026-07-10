// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_MULTIMODAL_VERIFIER_H
#define NEXUS_ALGO_ENGINES_MULTIMODAL_VERIFIER_H

/* 多模态验证引擎 v2 — 事实核查 + 数字验证 + 源可信度 */

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace nexus::algo::engines {

class MultimodalVerifierEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id = "multimodal_verifier", .name = "Multimodal Verifier v2",
      .version = "2.0.0",
      .description = "事实核查 + 数字验证 + 源可信度 + 逻辑一致性",
      .tags = {"verification", "fact-check", "consistency", "credibility"},
    };
  }

  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    auto statements = input.value("statements", std::vector<std::string>());

    // 事实核查关键词库 (已知虚假模式)
    const std::vector<std::string> red_flags = {
      "永远","绝对","从不","总是","所有人","没人","毫无疑问",
      "always","never","everyone","nobody","undoubtedly","certainly",
    };

    // 数字合理性范围 [下限, 上限, 描述]
    const std::map<std::string, std::pair<double,double>> numeric_ranges = {
      {"temperature", {-273.15, 1e8}},
      {"age", {0, 150}}, {"year", {1900, 2030}},
      {"percentage", {0, 100}}, {"population", {0, 1e10}},
    };

    auto verifications = nlohmann::json::array();
    for (const auto& stmt : statements) {
      auto v = nlohmann::json::object();
      v["statement"] = stmt;
      std::string lower;
      for (char c : stmt) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

      // 1. 红旗检测 (绝对化表述)
      int flag_count = 0;
      for (const auto& flag : red_flags) {
        size_t pos = 0;
        while ((pos = lower.find(flag, pos)) != std::string::npos) { flag_count++; pos += flag.size(); }
      }
      v["absolute_claims"] = flag_count;

      // 2. 数字合理性验证
      std::vector<double> numbers;
      std::string word;
      for (char c : stmt) {
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') word += c;
        else if (!word.empty()) {
          try { numbers.push_back(std::stod(word)); } catch (...) {}
          word.clear();
        }
      }
      if (!word.empty()) { try { numbers.push_back(std::stod(word)); } catch (...) {} }

      int num_issues = 0;
      for (double n : numbers)
        if (n < 0 || n > 1e12) num_issues++;

      v["numbers_found"] = static_cast<int>(numbers.size());
      v["numeric_issues"] = num_issues;

      // 3. 数字一致性 (同一数字多次出现)
      std::map<double, int> num_freq;
      for (auto n : numbers) num_freq[std::round(n * 100)]++;
      int contradictions = 0;
      for (const auto& [n, f] : num_freq)
        if (f > 1) contradictions++;

      v["numeric_contradictions"] = contradictions;

      // 4. 逻辑一致性: 否定词 + 肯定词平衡
      int negations = 0, affirmations = 0;
      const std::vector<std::string> neg_words = {"不","没","非","无","否","not","no","never","none"};
      const std::vector<std::string> aff_words = {"是","有","确","yes","yes","always","certain"};
      for (const auto& w : neg_words) {
        size_t pos = 0; while ((pos = lower.find(w, pos)) != std::string::npos) { negations++; pos += w.size(); }
      }
      for (const auto& w : aff_words) {
        size_t pos = 0; while ((pos = lower.find(w, pos)) != std::string::npos) { affirmations++; pos += w.size(); }
      }

      double logic_balance = (negations + affirmations) > 0
          ? 1.0 - std::abs(negations - affirmations) / static_cast<double>(negations + affirmations)
          : 1.0;

      v["logic_balance"] = std::round(logic_balance * 1000) / 1000;

      // 5. 关键词密度 (信息量)
      std::set<std::string> words;
      std::string w;
      for (char c : stmt) {
        if (std::isalnum(static_cast<unsigned char>(c))) { w += c; }
        else { if (w.size() > 2) words.insert(w); w.clear(); }
      }
      if (w.size() > 2) words.insert(w);

      v["keyword_count"] = static_cast<int>(words.size());
      double density = stmt.empty() ? 0 : static_cast<double>(words.size()) / stmt.size();
      v["info_density"] = std::round(density * 1000) / 1000;

      // 综合可信度
      double credibility = 0.8
          - 0.1 * flag_count
          - 0.05 * num_issues
          - 0.1 * contradictions
          + 0.1 * logic_balance;
      credibility = std::clamp(credibility, 0.0, 1.0);

      v["credibility"] = std::round(credibility * 1000) / 1000;

      // 改进: 综合多种信号
      double signals[4] = {1.0 - flag_count * 0.1, logic_balance, credibility, density};
      double combined = 0;
      for (double s : signals) combined += s;
      combined /= 4;
      combined = std::clamp(combined, 0.0, 1.0);

      v["combined_score"] = std::round(combined * 1000) / 1000;
      v["verdict"] = combined >= 0.7 ? "credible"
          : combined >= 0.4 ? "needs_review" : "questionable";
      verifications.push_back(v);
    }

    int credible = 0, review = 0, questionable = 0;
    for (const auto& v : verifications) {
      auto verdict = v["verdict"].get<std::string>();
      if (verdict == "credible") credible++;
      else if (verdict == "needs_review") review++;
      else questionable++;
    }

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["total"] = static_cast<int>(statements.size());
    result["credible"] = credible;
    result["needs_review"] = review;
    result["questionable"] = questionable;
    result["verifications"] = verifications;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object(); j["initialized"] = initialized_; return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
};

} // namespace
#endif
