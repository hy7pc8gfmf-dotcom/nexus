// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_DUAL_PRUNING_H
#define NEXUS_ALGO_ENGINES_DUAL_PRUNING_H

/**
 * @file dual_pruning_engine.h
 * @brief 双逻辑剪枝引擎 v2 — Alpha/Beta 双通道 + 策略评分
 *
 * 移植自 D:/synapse/algorithms/dual_pruning_engine.py (46K)
 *
 * Alpha 通道: 前置约束剪枝 (保留高连贯性候选)
 * Beta  通道: 后置矛盾剪枝 (移除高相似度冗余)
 * 合并策略: 多样性 + 新颖度 + 连贯性 三维评分
 */

#include "nexus/algo/engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace nexus::algo::engines {

/// 策略评分
struct StrategyScore {
  double coherence  = 0.0;  // 连贯性: 与主题的语义一致性
  double novelty    = 0.0;  // 新颖度: 与现有候选的差异度
  double diversity  = 0.0;  // 多样性: 在候选集中的独特程度
  double total      = 0.0;  // 综合 = 0.4*coherence + 0.3*novelty + 0.3*diversity
};

// ═══════════════════════════════════════════════════════════════════
// 语义相似度 — 内置知识库
// ═══════════════════════════════════════════════════════════════════

/// 语义等价组: 同一组内概念相似度高
static constexpr const char* kSynonymGroups[][5] = {
  {"人","民","众","群","众"},
  {"山","岳","峰","岭"},
  {"水","江","河","川","海"},
  {"火","焰","焚","烧","燃"},
  {"大","巨","伟","宏","博"},
  {"小","微","细","纤","渺"},
  {"言","语","词","字","文"},
  {"思","想","念","虑","悟"},
  {"行","走","奔","驰","移"},
  {"喜","悦","欢","乐","欣"},
};

static constexpr int kSynonymGroupCount =
    sizeof(kSynonymGroups) / sizeof(kSynonymGroups[0]);

/// 内置语义距离 (0~1, 越大越相似)
/// 编辑距离 (Levenshtein)
static int edit_distance(const std::string& a, const std::string& b) {
  size_t n = a.size(), m = b.size();
  std::vector<int> prev(m + 1), curr(m + 1);
  for (size_t j = 0; j <= m; ++j) prev[j] = static_cast<int>(j);
  for (size_t i = 1; i <= n; ++i) {
    curr[0] = static_cast<int>(i);
    for (size_t j = 1; j <= m; ++j) {
      int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
      curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
    }
    std::swap(prev, curr);
  }
  return prev[m];
}

/// 内置语义距离 (0~1, 越大越相似)
static double semantic_similarity(const std::string& a, const std::string& b) {
  if (a == b) return 1.0;

  // 检查是否在同义组内
  for (int g = 0; g < kSynonymGroupCount; ++g) {
    bool in_a = false, in_b = false;
    for (int i = 0; i < 5 && kSynonymGroups[g][i] != nullptr; ++i) {
      if (a == kSynonymGroups[g][i]) in_a = true;
      if (b == kSynonymGroups[g][i]) in_b = true;
    }
    if (in_a && in_b) return 0.8;
  }

  // 首字相同 → 中等相似
  if (!a.empty() && !b.empty() && a[0] == b[0]) return 0.4;

  // 编辑距离兜底
  int dist = edit_distance(a, b);
  int max_len = std::max(static_cast<int>(a.size()), static_cast<int>(b.size()));
  return (max_len == 0) ? 1.0 : 1.0 - static_cast<double>(dist) / max_len;
}

// ═══════════════════════════════════════════════════════════════════
// 双逻辑剪枝引擎
// ═══════════════════════════════════════════════════════════════════

class DualPruningEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id          = "dual_pruning",
      .name        = "Dual Logic Pruning v2",
      .version     = "2.0.0",
      .description = "Alpha/Beta 双通道剪枝 + 策略评分",
      .tags        = {"pruning", "logic", "optimization", "dual_channel"},
    };
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    alpha_threshold_ = config.value("alpha_threshold", 0.3);  // 前置连贯性阈值
    beta_threshold_  = config.value("beta_threshold", 0.7);   // 后置相似度阈值
    min_diversity_   = config.value("min_diversity", 0.2);    // 最小多样性
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) {
      return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    }

    auto candidates = input.value("candidates", std::vector<std::string>());
    auto constraints = input.value("constraints", std::vector<std::string>());
    auto topic = input.value("topic", std::string(""));
    double alpha_th = input.value("alpha_threshold", alpha_threshold_);
    double beta_th  = input.value("beta_threshold", beta_threshold_);

    if (candidates.empty()) {
      return Status::Error(ErrorCode::kInvalidConfig, "candidates required");
    }

    nlohmann::json result;
    result["status"] = "ok";
    result["input_count"] = static_cast<int>(candidates.size());

    // ═════════════════════════════════════════════════════════════
    // Alpha 通道: 前置约束剪枝
    // ═════════════════════════════════════════════════════════════
    auto alpha_start = alpha_channel(candidates, topic, constraints, alpha_th);
    result["alpha"] = alpha_start["stats"];
    auto alpha_survivors = alpha_start["survivors"];

    // ═════════════════════════════════════════════════════════════
    // Beta 通道: 后置矛盾剪枝 (冗余移除)
    // ═════════════════════════════════════════════════════════════
    auto beta_start = beta_channel(alpha_survivors, beta_th);
    result["beta"] = beta_start["stats"];
    auto beta_survivors = beta_start["survivors"];

    // ═════════════════════════════════════════════════════════════
    // 三维评分: 连贯性 + 新颖度 + 多样性
    // ═════════════════════════════════════════════════════════════
    auto scored = score_candidates(beta_survivors, topic);
    result["scored"] = scored["scores"];
    result["output_count"] = scored["count"];
    result["results"] = scored["results"];
    result["n_coherence"] = scored["n_coherence"];
    result["n_novelty"]   = scored["n_novelty"];
    result["n_diversity"] = scored["n_diversity"];

    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"]      = initialized_;
    j["version"]          = "2.0.0";
    j["alpha_threshold"]  = alpha_threshold_;
    j["beta_threshold"]   = beta_threshold_;
    j["min_diversity"]    = min_diversity_;
    return j;
  }

  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
  double alpha_threshold_ = 0.3;
  double beta_threshold_  = 0.7;
  double min_diversity_   = 0.2;

  // ═══════════════════════════════════════════════════════════════
  // Alpha 通道: 前置约束剪枝
  // ═══════════════════════════════════════════════════════════════

  auto alpha_channel(const std::vector<std::string>& candidates,
                     const std::string& topic,
                     const std::vector<std::string>& constraints,
                     double threshold) const -> nlohmann::json {
    auto j = nlohmann::json::object();
    auto surv = nlohmann::json::array();

    int pre_total = static_cast<int>(candidates.size());
    int removed_minlen = 0;
    int removed_constraint = 0;
    int removed_coherence = 0;

    for (const auto& c : candidates) {
      // 规则 1: 最小长度 (单字概念除外)
      if (c.size() < 3 && c.size() != 1) {
        removed_minlen++;
        continue;
      }

      // 规则 2: 约束检查 (含违禁词则排除)
      bool violates = false;
      for (const auto& con : constraints) {
        if (!con.empty() && c.find(con) != std::string::npos) {
          violates = true;
          break;
        }
      }
      if (violates) { removed_constraint++; continue; }

      // 规则 3: 主题连贯性
      double coherence = 0.3;
      if (!topic.empty()) {
        coherence = compute_coherence(c, topic);
      }
      if (coherence < threshold) {
        removed_coherence++;
        continue;
      }

      surv.push_back(c);
    }

    auto stats = nlohmann::json::object();
    stats["input"] = pre_total;
    stats["min_length_excluded"] = removed_minlen;
    stats["constraint_excluded"] = removed_constraint;
    stats["coherence_excluded"]  = removed_coherence;
    stats["survived"] = static_cast<int>(surv.size());

    j["survivors"] = surv;
    j["stats"] = stats;
    return j;
  }

  // ═══════════════════════════════════════════════════════════════
  // Beta 通道: 后置矛盾剪枝
  // ═══════════════════════════════════════════════════════════════

  auto beta_channel(const nlohmann::json& survivors, double threshold) const
      -> nlohmann::json {
    auto j = nlohmann::json::object();

    // 转回 vector
    std::vector<std::string> items;
    for (const auto& s : survivors) items.push_back(s.get<std::string>());

    int before = static_cast<int>(items.size());

    // 贪心多样性选择: 从头遍历，保留与已选不相似的项目
    std::vector<std::string> selected;
    std::set<int> pruned_indices;

    for (size_t i = 0; i < items.size(); ++i) {
      if (pruned_indices.count(static_cast<int>(i))) continue;

      selected.push_back(items[i]);

      for (size_t j = i + 1; j < items.size(); ++j) {
        if (pruned_indices.count(static_cast<int>(j))) continue;
        double sim = semantic_similarity(items[i], items[j]);
        if (sim > threshold) {
          pruned_indices.insert(static_cast<int>(j));
        }
      }
    }

    auto result = nlohmann::json::array();
    for (const auto& s : selected) result.push_back(s);

    auto stats = nlohmann::json::object();
    stats["before_beta"]  = before;
    stats["after_beta"]   = static_cast<int>(selected.size());
    stats["pruned_beta"]  = before - static_cast<int>(selected.size());
    stats["threshold"]    = threshold;

    j["survivors"] = result;
    j["stats"] = stats;
    return j;
  }

  // ═══════════════════════════════════════════════════════════════
  // 三维评分
  // ═══════════════════════════════════════════════════════════════

  auto score_candidates(const nlohmann::json& candidates,
                        const std::string& topic) const -> nlohmann::json {
    // 转回 vector
    std::vector<std::string> items;
    for (const auto& c : candidates) items.push_back(c.get<std::string>());

    auto scores_arr = nlohmann::json::array();
    auto results_arr = nlohmann::json::array();

    int n_high_coherence = 0;
    int n_high_novelty   = 0;
    int n_high_diversity = 0;

    for (size_t i = 0; i < items.size(); ++i) {
      StrategyScore sc;

      // 连贯性: 与主题的语义相似度
      sc.coherence = compute_coherence(items[i], topic);

      // 新颖度: 与所有其他候选的平均差异度
      double total_diff = 0;
      for (size_t j = 0; j < items.size(); ++j) {
        if (i != j) {
          total_diff += (1.0 - semantic_similarity(items[i], items[j]));
        }
      }
      sc.novelty = (items.size() > 1)
          ? total_diff / (items.size() - 1) : 0.5;

      // 多样性: 在候选集中位置加权
      sc.diversity = compute_diversity(items, i);

      // 综合
      sc.total = 0.4 * sc.coherence + 0.3 * sc.novelty + 0.3 * sc.diversity;

      if (sc.coherence > 0.6) n_high_coherence++;
      if (sc.novelty > 0.5)   n_high_novelty++;
      if (sc.diversity > 0.4) n_high_diversity++;

      auto s = nlohmann::json::object();
      s["idx"]        = static_cast<int>(i);
      s["candidate"]  = items[i];
      s["coherence"]  = std::round(sc.coherence * 100) / 100;
      s["novelty"]    = std::round(sc.novelty * 100) / 100;
      s["diversity"]  = std::round(sc.diversity * 100) / 100;
      s["total"]      = std::round(sc.total * 100) / 100;
      scores_arr.push_back(s);
      results_arr.push_back(items[i]);
    }

    // 按综合分排序
    std::vector<std::pair<double, int>> order;
    for (int i = 0; i < static_cast<int>(scores_arr.size()); ++i) {
      order.emplace_back(scores_arr[i]["total"].get<double>(), i);
    }
    std::sort(order.begin(), order.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    auto sorted_scores = nlohmann::json::array();
    auto sorted_results = nlohmann::json::array();
    for (const auto& [score, idx] : order) {
      sorted_scores.push_back(scores_arr[idx]);
      sorted_results.push_back(results_arr[idx]);
    }

    auto j = nlohmann::json::object();
    j["scores"]      = sorted_scores;
    j["results"]     = sorted_results;
    j["count"]       = static_cast<int>(items.size());
    j["n_coherence"] = n_high_coherence;
    j["n_novelty"]   = n_high_novelty;
    j["n_diversity"] = n_high_diversity;
    return j;
  }

  // ═══════════════════════════════════════════════════════════════
  // 辅助函数
  // ═══════════════════════════════════════════════════════════════

  static double compute_coherence(const std::string& item,
                                   const std::string& topic) {
    if (topic.empty()) return 0.5;

    // 检查共享关键词
    double score = 0.3;
    for (size_t i = 0; i + 1 < topic.size() && i < item.size(); ++i) {
      if (topic[i] == item[i]) score += 0.1;
    }
    // 字符重叠
    for (char c : topic) {
      if (item.find(c) != std::string::npos) score += 0.05;
    }
    return std::min(1.0, score);
  }

  static double compute_diversity(const std::vector<std::string>& items,
                                   size_t idx) {
    // 位置加权: 在中段有更高多样性 (避免边界聚集)
    if (items.size() <= 2) return 0.5;
    double pos = static_cast<double>(idx) / (items.size() - 1);
    // 中间位置高分, 两端低分
    return 0.3 + 0.7 * (1.0 - std::abs(pos - 0.5) * 2.0);
  }
};

} // namespace nexus::algo::engines

#endif
