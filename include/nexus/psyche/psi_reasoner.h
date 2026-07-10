// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_PSI_REASONER_H
#define NEXUS_PSYCHE_PSI_REASONER_H

/**
 * @file psi_reasoner.h
 * @brief Ψ 推理引擎 — 纯 C++ 多步推理链
 *
 * 四阶段推理管线:
 *   Step  单步推理 → 当前思想步进
 *   Branch 分支 → 置信度低时拆分为多路并行
 *   Revise 修正 → 检测到矛盾时回溯重试
 *   Converge 收敛 → 稳定状态检测 + 结论合成
 *
 * 依赖 (通过函数注入, 无编译耦合):
 *   InferenceScheduler — 模型推理回调
 *   ObserverPool       — 验证回调 (可选)
 *   seed_bank          — 知识种子 (可选)
 */

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "nexus/types/status.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 步进类型
// ═══════════════════════════════════════════════════════════════════

enum class StepType : uint8_t {
  kInitial      = 0,  // ● 初始设定
  kAdvance      = 1,  // → 前向推进
  kRevision     = 2,  // ↩ 回溯修正
  kBranch       = 3,  // ◇ 分支探索
  kHypothesis   = 4,  // ? 假设生成
  kVerification = 5,  // ✓ 验证确认
  kConclusion   = 6,  // ■ 结论
};

auto step_type_string(StepType t) -> const char*;
auto step_type_mark(StepType t) -> const char*;

// ═══════════════════════════════════════════════════════════════════
// 推理步进
// ═══════════════════════════════════════════════════════════════════

struct ReasoningStep {
  int         id         = 0;
  int         parent_id  = 0;       // 上一步 ID (0 = root)
  StepType    type       = StepType::kInitial;
  std::string content;
  double      confidence = 0.0;     // [0, 1]
  int         branch_id  = 0;       // 所属分支
  int         revision_of = 0;      // 修正自哪一步 (0 = 不是修正)
  double      quantum_ae = 0.0;     // 量子激活能 (涌现偏置)
  bool        verified   = false;   // 是否被 Observer 验证
  double      timestamp  = 0.0;     // Unix 时间戳

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 推理报告
// ═══════════════════════════════════════════════════════════════════

struct ReasoningReport {
  std::string  problem;
  std::string  domain;
  bool         converged        = false;
  int          total_steps      = 0;
  int          total_branches   = 0;
  int          total_revisions  = 0;
  int          seeds_planted    = 0;
  int          seeds_harvested  = 0;
  double       final_confidence = 0.0;
  double       quantum_ae_avg   = 0.0;
  double       elapsed_seconds  = 0.0;
  std::vector<ReasoningStep> steps;
  std::string  conclusion;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 推理回调签名
// ═══════════════════════════════════════════════════════════════════

/// 单步推理回调: 输入 prompt → 返回推理文本 + 置信度
using InferCallback = std::function<Result<std::pair<std::string, double>>(
  const std::string& prompt, const std::string& context)>;

/// Observer 验证回调: 验证推理步的正确性 → 返回置信度
using VerifyCallback = std::function<double(const ReasoningStep& step)>;

/// 种子查询回调: 查询种子库 → 返回相关知识
using SeedCallback = std::function<std::vector<std::string>(
  const std::string& domain, const std::string& query)>;

// ═══════════════════════════════════════════════════════════════════
// Ψ 推理引擎
// ═══════════════════════════════════════════════════════════════════

class PsiReasoner {
public:
  /// 推理参数配置
  struct Config {
    int    max_steps         = 30;     // 最大步数
    int    max_branches      = 5;      // 最大并行分支数
    double branch_threshold  = 0.4;    // 低于此置信度触发分支
    double converge_threshold = 0.85;  // 收敛置信度阈值
    int    converge_window   = 5;      // 收敛检测滑动窗口
    double revision_threshold = 0.2;   // 低于此置信度触发修正
    int    max_revisions     = 3;      // 单步最大修正次数
    bool   enable_observer   = true;   // 启用 Observer 验证
    bool   enable_seeds      = true;   // 启用种子知识注入
  };

  explicit PsiReasoner(Config config = {}) noexcept;

  /// 注册推理回调 (必需)
  void set_infer_callback(InferCallback cb) noexcept;

  /// 注册验证回调 (可选)
  void set_verify_callback(VerifyCallback cb) noexcept;

  /// 注册种子查询回调 (可选)
  void set_seed_callback(SeedCallback cb) noexcept;

  // ── 推理执行 ──

  /// 执行完整推理链
  auto reason(const std::string& problem,
              const std::string& domain = "") noexcept
      -> Result<ReasoningReport>;

  /// 单步推理
  auto step() noexcept -> Result<ReasoningStep>;

  /// 分支探索: 从当前步分裂出多条路径
  auto branch(int from_step_id) noexcept -> std::vector<int>;

  /// 回溯修正: 回到上一步并标记为修正
  auto revise(int step_id) noexcept -> Result<ReasoningStep>;

  /// 收敛检测: 判断推理是否已稳定
  [[nodiscard]] auto check_convergence() const noexcept -> bool;

  // ── 状态查询 ──

  [[nodiscard]] auto current_step() const noexcept -> int;
  [[nodiscard]] auto total_steps() const noexcept -> int;
  [[nodiscard]] auto is_running() const noexcept -> bool;
  [[nodiscard]] auto current_confidence() const noexcept -> double;
  [[nodiscard]] auto report() const noexcept -> ReasoningReport;

private:
  Config config_;
  InferCallback  infer_cb_;
  VerifyCallback verify_cb_;
  SeedCallback   seed_cb_;

  std::vector<ReasoningStep> steps_;
  std::vector<int> active_branches_;    // 活跃分支的 step_id
  int next_step_id_    = 1;
  int next_branch_id_  = 1;
  int revision_count_  = 0;
  double confidence_   = 0.0;
  bool   running_      = false;
  bool   converged_    = false;

  std::chrono::steady_clock::time_point start_time_;
  std::string problem_;
  std::string domain_;

  // 工具方法
  auto build_context_(int from_step = 0) const -> std::string;
  auto append_step_(StepType type, std::string content,
                    double confidence, int branch_id,
                    int revision_of = 0) -> int;
  auto find_branch_leaf_(int branch_id) const -> int;
  auto trace_path_(int step_id) const -> std::vector<const ReasoningStep*>;
  double average_confidence_() const noexcept;
  double average_quantum_ae_() const noexcept;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_PSI_REASONER_H
