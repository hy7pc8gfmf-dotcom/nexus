// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/psi_reasoner.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 工具
// ═══════════════════════════════════════════════════════════════════

auto step_type_string(StepType t) -> const char* {
  switch (t) {
    case StepType::kInitial:      return "initial";
    case StepType::kAdvance:      return "advance";
    case StepType::kRevision:     return "revision";
    case StepType::kBranch:       return "branch";
    case StepType::kHypothesis:   return "hypothesis";
    case StepType::kVerification: return "verification";
    case StepType::kConclusion:   return "conclusion";
  }
  return "unknown";
}

auto step_type_mark(StepType t) -> const char* {
  switch (t) {
    case StepType::kInitial:      return "\xe2\x97\x8f";     // ●
    case StepType::kAdvance:      return "\xe2\x86\x92";     // →
    case StepType::kRevision:     return "\xe2\x86\xa9";     // ↩
    case StepType::kBranch:       return "\xe2\x97\x87";     // ◇
    case StepType::kHypothesis:   return "?";
    case StepType::kVerification: return "\xe2\x9c\x93";     // ✓
    case StepType::kConclusion:   return "\xe2\x96\xa0";     // ■
  }
  return "\xc2\xb7";  // ·
}

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto ReasoningStep::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]          = id;
  j["parent_id"]   = parent_id;
  j["type"]        = step_type_string(type);
  j["content"]     = content;
  j["confidence"]  = std::round(confidence * 1000) / 1000;
  j["branch_id"]   = branch_id;
  j["revision_of"] = revision_of;
  j["quantum_ae"]  = std::round(quantum_ae * 1000) / 1000;
  j["verified"]    = verified;
  j["timestamp"]   = timestamp;
  return j;
}

auto ReasoningReport::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["problem"]         = problem;
  j["domain"]          = domain;
  j["converged"]       = converged;
  j["total_steps"]     = total_steps;
  j["total_branches"]  = total_branches;
  j["total_revisions"] = total_revisions;
  j["seeds_planted"]   = seeds_planted;
  j["seeds_harvested"] = seeds_harvested;
  j["final_confidence"] = std::round(final_confidence * 1000) / 1000;
  j["quantum_ae_avg"]  = std::round(quantum_ae_avg * 1000) / 1000;
  j["elapsed_seconds"] = std::round(elapsed_seconds * 10) / 10;

  auto steps_arr = nlohmann::json::array();
  for (const auto& s : steps) steps_arr.push_back(s.to_json());
  j["steps"]      = steps_arr;
  j["conclusion"] = conclusion;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// PsiReasoner
// ═══════════════════════════════════════════════════════════════════

PsiReasoner::PsiReasoner(Config config) noexcept
  : config_(std::move(config)) {}

void PsiReasoner::set_infer_callback(InferCallback cb) noexcept {
  infer_cb_ = std::move(cb);
}

void PsiReasoner::set_verify_callback(VerifyCallback cb) noexcept {
  verify_cb_ = std::move(cb);
}

void PsiReasoner::set_seed_callback(SeedCallback cb) noexcept {
  seed_cb_ = std::move(cb);
}

// ═══════════════════════════════════════════════════════════════════
// 主推理循环
// ═══════════════════════════════════════════════════════════════════

auto PsiReasoner::reason(const std::string& problem,
                          const std::string& domain) noexcept
    -> Result<ReasoningReport> {
  if (!infer_cb_) {
    return Status::Error(ErrorCode::kInvalidConfig,
      "PsiReasoner: no infer callback registered");
  }

  // 初始化状态
  problem_   = problem;
  domain_    = domain;
  steps_.clear();
  active_branches_.clear();
  next_step_id_   = 1;
  next_branch_id_ = 1;
  revision_count_ = 0;
  confidence_     = 0.0;
  running_        = true;
  converged_      = false;
  start_time_     = std::chrono::steady_clock::now();

  // 初始步
  std::string initial_content = "分析问题: " + problem;
  append_step_(StepType::kInitial, initial_content, 0.5, 0);
  active_branches_.push_back(0);  // 分支 0 = 主路径

  // 主推理循环
  while (running_ && next_step_id_ <= config_.max_steps) {
    auto result = step();
    if (!result.ok()) {
      running_ = false;
      return result.error();
    }

    auto& last = steps_.back();

    // 修正检测: 置信度过低 → 回溯
    if (last.type != StepType::kRevision &&
        last.confidence < config_.revision_threshold &&
        revision_count_ < config_.max_revisions &&
        steps_.size() > 1) {
      revise(last.id);
      continue;
    }

    // 分支检测: 不确定性高 → 分支
    if (last.type != StepType::kBranch &&
        last.confidence < config_.branch_threshold &&
        next_branch_id_ <= config_.max_branches) {
      branch(last.id);
    }

    // 收敛检测
    if (check_convergence()) {
      converged_ = true;
      // 生成结论
      std::string conclusion_text = "推理收敛。";
      if (!steps_.empty()) {
        int best_id = steps_.back().id;
        double best_conf = steps_.back().confidence;
        for (const auto& s : steps_) {
          if (s.confidence > best_conf) {
            best_conf = s.confidence;
            best_id = s.id;
          }
        }
        // 找到置信度最高的步的内容
        std::string best_content;
        for (const auto& s : steps_) {
          if (s.id == best_id) {
            best_content = s.content;
            break;
          }
        }
        conclusion_text = best_content.substr(0, 400);
      }
      append_step_(StepType::kConclusion, conclusion_text,
                   confidence_, 0);
      break;
    }
  }

  running_ = false;

  // 构建报告
  ReasoningReport rep;
  rep.problem         = problem_;
  rep.domain          = domain_;
  rep.converged       = converged_;
  rep.total_steps     = static_cast<int>(steps_.size());
  rep.total_branches  = next_branch_id_;
  rep.total_revisions = revision_count_;
  rep.final_confidence = confidence_;
  rep.quantum_ae_avg  = average_quantum_ae_();
  rep.steps           = steps_;
  rep.seeds_planted   = 0;

  if (!steps_.empty() && steps_.back().type == StepType::kConclusion) {
    rep.conclusion = steps_.back().content;
  }

  auto end_time = std::chrono::steady_clock::now();
  rep.elapsed_seconds = std::chrono::duration<double>(
    end_time - start_time_).count();

  return rep;
}

// ═══════════════════════════════════════════════════════════════════
// 单步
// ═══════════════════════════════════════════════════════════════════

auto PsiReasoner::step() noexcept -> Result<ReasoningStep> {
  if (!infer_cb_) {
    return Status::Error(ErrorCode::kInvalidConfig, "no infer callback");
  }

  // 构建上下文 (从主路径最近的步开始)
  auto context = build_context_();

  // 调用模型推理
  auto result = infer_cb_(problem_, context);
  if (!result.ok()) {
    return result.error();
  }

  auto pair = result.value();
  std::string output = pair.first;
  double confidence = pair.second;

  // 确定当前分支
  int current_branch = 0;
  if (!active_branches_.empty()) {
    current_branch = active_branches_.back();
  }

  // 创建步进
  int step_id = append_step_(
    StepType::kAdvance, output, confidence, current_branch);

  // 更新全局置信度
  confidence_ = average_confidence_();

  return steps_.back();
}

// ═══════════════════════════════════════════════════════════════════
// 分支
// ═══════════════════════════════════════════════════════════════════

auto PsiReasoner::branch(int from_step_id) noexcept -> std::vector<int> {
  if (next_branch_id_ > config_.max_branches) return {};

  // 找到源步的置信度
  double from_confidence = 0.5;
  for (const auto& s : steps_) {
    if (s.id == from_step_id) { from_confidence = s.confidence; break; }
  }

  int new_branch_id = next_branch_id_++;
  int n_children = std::min(3, config_.max_branches - new_branch_id + 1);
  if (n_children <= 0) return {};

  std::vector<int> child_ids;
  for (int i = 0; i < n_children; ++i) {
    std::string branch_content = "分支 " + std::to_string(new_branch_id) +
      " (从步 " + std::to_string(from_step_id) + "): " +
      "探索替代路径 " + std::to_string(i + 1);

    int child_id = append_step_(
      StepType::kBranch, branch_content,
      from_confidence * 0.8, new_branch_id);
    child_ids.push_back(child_id);
  }

  active_branches_.push_back(new_branch_id);
  return child_ids;
}

// ═══════════════════════════════════════════════════════════════════
// 修正
// ═══════════════════════════════════════════════════════════════════

auto PsiReasoner::revise(int step_id) noexcept -> Result<ReasoningStep> {
  revision_count_++;

  // 回退: 移除 step_id 之后的所有步 (保留初始步)
  std::vector<ReasoningStep> kept;
  kept.reserve(steps_.size());
  for (auto& s : steps_) {
    if (s.id < step_id || s.type == StepType::kInitial) {
      kept.push_back(std::move(s));
    }
  }
  steps_ = std::move(kept);

  // 重新推理
  auto context = build_context_(step_id - 1);
  auto result = infer_cb_(problem_, context);

  std::string content;
  double confidence = 0.0;
  if (result.ok()) {
    content = "修正后: " + result.value().first;
    confidence = result.value().second;
  } else {
    content = "修正尝试 (重新推理)";
    confidence = 0.3;
  }

  // 找到被修正步的分支 ID
  int branch_id = 0;
  for (const auto& s : steps_) {
    if (s.id == step_id) { branch_id = s.branch_id; break; }
  }

  int new_id = append_step_(
    StepType::kRevision, content, confidence,
    branch_id, step_id);

  return steps_.back();
}

// ═══════════════════════════════════════════════════════════════════
// 收敛检测
// ═══════════════════════════════════════════════════════════════════

auto PsiReasoner::check_convergence() const noexcept -> bool {
  if (steps_.size() < 2) return false;

  // 取最近 N 步
  int window = std::min(config_.converge_window,
    static_cast<int>(steps_.size()));
  double avg_conf = 0.0;
  double stable_count = 0;

  for (int i = static_cast<int>(steps_.size()) - window;
       i < static_cast<int>(steps_.size()); ++i) {
    avg_conf += steps_[i].confidence;
    if (steps_[i].confidence >= config_.converge_threshold) {
      stable_count++;
    }
  }
  avg_conf /= window;

  // 最近窗口平均置信度 ≥ 阈值
  if (avg_conf < config_.converge_threshold) return false;

  // 至少 60% 的步达到高置信度
  if (stable_count / window < 0.6) return false;

  return true;
}

// ═══════════════════════════════════════════════════════════════════
// 状态查询
// ═══════════════════════════════════════════════════════════════════

auto PsiReasoner::current_step() const noexcept -> int {
  return steps_.empty() ? 0 : steps_.back().id;
}

auto PsiReasoner::total_steps() const noexcept -> int {
  return static_cast<int>(steps_.size());
}

auto PsiReasoner::is_running() const noexcept -> bool {
  return running_;
}

auto PsiReasoner::current_confidence() const noexcept -> double {
  return confidence_;
}

auto PsiReasoner::report() const noexcept -> ReasoningReport {
  ReasoningReport rep;
  rep.problem    = problem_;
  rep.domain     = domain_;
  rep.converged  = converged_;
  rep.total_steps = static_cast<int>(steps_.size());
  rep.final_confidence = confidence_;
  rep.steps      = steps_;
  return rep;
}

// ═══════════════════════════════════════════════════════════════════
// 内部工具
// ═══════════════════════════════════════════════════════════════════

auto PsiReasoner::build_context_(int from_step) const -> std::string {
  std::ostringstream ctx;
  ctx << "问题: " << problem_ << "\n";
  if (!domain_.empty()) {
    ctx << "领域: " << domain_ << "\n";
  }
  ctx << "推理链:\n";

  int start = 0;
  if (from_step > 0) {
    // 从指定步开始
    for (const auto& s : steps_) {
      if (s.id >= from_step) break;
      start = s.id;
    }
  }

  // 最多携带最近 10 步
  int max_context = 10;
  int begin = std::max(0, static_cast<int>(steps_.size()) - max_context);
  for (int i = begin; i < static_cast<int>(steps_.size()); ++i) {
    const auto& s = steps_[i];
    ctx << "  " << step_type_mark(s.type)
        << " #" << s.id
        << " (置信: " << std::round(s.confidence * 100) << "%)"
        << ": " << s.content.substr(0, 200) << "\n";
  }

  ctx << "\n下一步:";
  return ctx.str();
}

auto PsiReasoner::append_step_(StepType type, std::string content,
                                double confidence, int branch_id,
                                int revision_of) -> int {
  ReasoningStep step;
  step.id          = next_step_id_++;
  step.parent_id   = steps_.empty() ? 0 : steps_.back().id;
  step.type        = type;
  step.content     = std::move(content);
  step.confidence  = std::clamp(confidence, 0.0, 1.0);
  step.branch_id   = branch_id;
  step.revision_of = revision_of;
  step.timestamp   = std::chrono::duration<double>(
    std::chrono::system_clock::now().time_since_epoch()).count();

  steps_.push_back(std::move(step));
  return steps_.back().id;
}

auto PsiReasoner::find_branch_leaf_(int branch_id) const -> int {
  int last_id = 0;
  for (const auto& s : steps_) {
    if (s.branch_id == branch_id && s.id > last_id) {
      last_id = s.id;
    }
  }
  return last_id;
}

auto PsiReasoner::trace_path_(int step_id) const
    -> std::vector<const ReasoningStep*> {
  std::vector<const ReasoningStep*> path;

  // 收集从 step_id 到根的路径
  int current = step_id;
  while (current > 0) {
    auto it = std::find_if(steps_.begin(), steps_.end(),
      [current](const ReasoningStep& s) { return s.id == current; });
    if (it == steps_.end()) break;
    path.push_back(&(*it));
    current = it->parent_id;
  }
  std::reverse(path.begin(), path.end());
  return path;
}

auto PsiReasoner::average_confidence_() const noexcept -> double {
  if (steps_.empty()) return 0.0;
  double sum = 0.0;
  for (const auto& s : steps_) sum += s.confidence;
  return sum / steps_.size();
}

auto PsiReasoner::average_quantum_ae_() const noexcept -> double {
  if (steps_.empty()) return 0.0;
  double sum = 0.0;
  for (const auto& s : steps_) sum += s.quantum_ae;
  return sum / steps_.size();
}

}  // namespace nexus::psyche
