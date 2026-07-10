// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_DIALECTICAL_CONSENSUS_H
#define NEXUS_ALGO_ENGINES_DIALECTICAL_CONSENSUS_H

/* 辩证共识引擎 v2 — 观点层级 + 矛盾识别 + 加权共识 */

#include "nexus/algo/engine.h"
#include <cmath>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace nexus::algo::engines {

class DialecticalConsensusEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id = "dialectical_consensus", .name = "Dialectical Consensus v2",
      .version = "2.0.0",
      .description = "观点层级分析 + 矛盾识别 + 加权共识度",
      .tags = {"consensus", "dialectic", "agreement", "contradiction"},
    };
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    min_agreement_ = config.value("min_agreement", 0.4);
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    auto viewpoints = input.value("viewpoints", std::vector<std::string>());
    if (viewpoints.empty()) return Status::Error(ErrorCode::kInvalidConfig, "viewpoints required");

    // 矛盾知识库 (用于检测观点对立)
    const std::map<std::string, std::string> opposites = {
      {"是","非"},{"有","无"},{"生","死"},{"善","恶"},{"真","假"},
      {"正","反"},{"爱","恨"},{"进","退"},{"强","弱"},{"得","失"},
      {"yes","no"},{"true","false"},{"good","bad"},{"support","oppose"},
      {"positive","negative"},{"increase","decrease"},{"accept","reject"},
    };

    // 提取每段观点的关键词和立场
    struct Viewpoint {
      std::string text;
      std::vector<std::string> keywords;
      std::string stance;  // pro / con / neutral
      double weight = 1.0;
    };

    std::vector<Viewpoint> vps;
    for (const auto& v : viewpoints) {
      Viewpoint vp;
      vp.text = v;

      // 关键词提取
      std::string word;
      for (char c : v) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') { word += c; }
        else { if (word.size() > 1) vp.keywords.push_back(word); word.clear(); }
      }
      if (word.size() > 1) vp.keywords.push_back(word);

      // 立场检测: 统计对立词出现
      int pro = 0, con = 0;
      for (const auto& kw : vp.keywords) {
        for (const auto& [a, b] : opposites) {
          if (kw.find(a) != std::string::npos) pro++;
          if (kw.find(b) != std::string::npos) con++;
        }
      }
      if (pro > con) vp.stance = "pro";
      else if (con > pro) vp.stance = "con";
      else vp.stance = "neutral";

      // 观点越长权重越大
      vp.weight = std::min(2.0, 0.5 + v.size() / 200.0);
      vps.push_back(vp);
    }

    // 共识分析: 共享关键词比例 + 立场分布
    std::map<std::string, int> kw_freq;
    for (const auto& vp : vps) {
      std::set<std::string> seen;
      for (const auto& kw : vp.keywords)
        if (seen.insert(kw).second) kw_freq[kw]++;
    }

    // 全共享关键词
    auto shared = nlohmann::json::array();
    auto contested = nlohmann::json::array();
    int n = static_cast<int>(vps.size());
    for (const auto& [kw, count] : kw_freq) {
      if (count == n) shared.push_back(kw);
      else if (count >= 2) contested.push_back(kw);
    }

    // 共识度计算 (加权)
    double total_kw = static_cast<double>(kw_freq.size());
    double consensus = total_kw > 0
        ? static_cast<double>(shared.size()) / total_kw
        : 1.0;

    // 立场分散度
    int pro_n = 0, con_n = 0, neu_n = 0;
    for (const auto& vp : vps) {
      if (vp.stance == "pro") pro_n++;
      else if (vp.stance == "con") con_n++;
      else neu_n++;
    }
    double polarity = (n > 0)
        ? std::abs(static_cast<double>(pro_n - con_n)) / n
        : 1.0;

    // 综合共识: 共享度 + 立场集中度
    double combined = 0.6 * consensus + 0.4 * (1.0 - polarity);

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["viewpoint_count"] = n;
    result["consensus_score"] = std::round(combined * 1000) / 1000;
    result["keyword_consensus"] = std::round(consensus * 1000) / 1000;
    result["stance_polarity"] = std::round(polarity * 1000) / 1000;
    result["shared_terms"] = shared;
    result["contested_terms"] = contested;
    result["stance_distribution"] = {
      {"pro", pro_n}, {"con", con_n}, {"neutral", neu_n}
    };
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"] = initialized_;
    j["min_agreement"] = min_agreement_;
    return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
  double min_agreement_ = 0.4;
};

} // namespace
#endif
