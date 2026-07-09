#ifndef NEXUS_ALGO_RECURSOR_H
#define NEXUS_ALGO_RECURSOR_H

/**
 * @file recursor.h
 * @brief 递归元理论验证器 — 29 个 Coq 定理
 *
 * 五个 VO 模块:
 *   VO I:   否定之路 (Gödel/Löb/Tarski)        — 6 定理
 *   VO II:  肯定之路 (余归纳等价+非良基)        — 7 定理
 *   VO III: 桥接层 (行动性剩余+同构)            — 6 定理
 *   VO IV:  终极定理 (纯命题逻辑结论)            — 5 定理
 *   VO V:   哥德尔回响 (递归元论与哥德尔对话)    — 5 定理
 *
 * 每个定理包含: 陈述、前提、验证函数、证明状态
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::algo {

/// 定理模块
enum class VoModule : uint8_t {
  kVoI = 0,   // 否定之路
  kVoII = 1,  // 肯定之路
  kVoIII = 2, // 桥接层
  kVoIV = 3,  // 终极定理
  kVoV = 4,   // 哥德尔回响
};

/// 定理定义
struct CoqTheorem {
  int         id;
  VoModule    module;
  std::string name;
  std::string statement;
  std::vector<std::string> premises;
  bool        verified = false;

  auto to_json() const -> nlohmann::json;
};

/// 验证结果
struct ProofResult {
  int     theorem_id;
  bool    passed;
  double  confidence;  // [0, 1]
  std::string detail;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// RecursorEngine — 递归元理论验证器
// ═══════════════════════════════════════════════════════════════════

class RecursorEngine {
public:
  RecursorEngine() noexcept;

  /// 注册 29 个 Coq 定理
  auto register_theorems() noexcept -> void;

  /// 验证所有定理
  auto verify_all() noexcept -> std::vector<ProofResult>;

  /// 验证指定模块
  auto verify_module(VoModule module) noexcept -> std::vector<ProofResult>;

  /// 获取定理信息
  [[nodiscard]] auto theorems() const noexcept -> const std::vector<CoqTheorem>&;
  [[nodiscard]] auto get_theorem(int id) const noexcept -> const CoqTheorem*;

  /// 状态
  [[nodiscard]] auto summary() const noexcept -> nlohmann::json;

  /// 获取单例
  static auto instance() noexcept -> RecursorEngine&;

private:
  std::vector<CoqTheorem> theorems_;
  std::vector<ProofResult> last_results_;

  auto verify_theorem_(const CoqTheorem& t) noexcept -> ProofResult;
  auto register_vo_i_() noexcept -> void;   // 6 定理
  auto register_vo_ii_() noexcept -> void;  // 7 定理
  auto register_vo_iii_() noexcept -> void; // 6 定理
  auto register_vo_iv_() noexcept -> void;  // 5 定理
  auto register_vo_v_() noexcept -> void;   // 5 定理
};

}  // namespace nexus::algo

#endif  // NEXUS_ALGO_RECURSOR_H
