#ifndef NEXUS_LOGIC_CORE_H
#define NEXUS_LOGIC_CORE_H

/**
 * @file logic_core.h
 * @brief 逻辑求解引擎 — 种子空间 + 证明搜索 + 逻辑推理
 *
 * 三核心能力:
 *   1. 种子空间: 32,629 种子, 184 域, 14D 坐标
 *   2. 证明搜索: 基于种子的路径探索
 *   3. 逻辑推理: 命题逻辑 + 谓词逻辑 + 构造性逻辑
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::logic {

// ═══════════════════════════════════════════════════════════════════
// 种子空间
// ═══════════════════════════════════════════════════════════════════

struct LogicSeed {
  std::string name;
  int         intensity  = 0;
  double      conductance = 0.0;  // 导通率 [0, 1]
  std::string domain;
  std::vector<double> coordinates; // 14D

  auto to_json() const -> nlohmann::json;
};

using SeedCallback = std::function<void(const LogicSeed&)>;

class SeedSpace {
public:
  SeedSpace() noexcept;

  auto load(const std::string& path) noexcept -> Status;
  auto query(const std::string& domain, int limit = 20) noexcept -> std::vector<LogicSeed>;
  auto search(const std::string& keyword, int limit = 20) noexcept -> std::vector<LogicSeed>;
  auto conduct(const std::string& from, const std::string& to) noexcept -> double;
  auto stats() const noexcept -> nlohmann::json;

private:
  std::vector<LogicSeed> seeds_;
  bool loaded_ = false;
};

// ═══════════════════════════════════════════════════════════════════
// 证明搜索
// ═══════════════════════════════════════════════════════════════════

struct ProofStep {
  std::string seed_name;
  std::string rule;
  double      confidence;
  auto to_json() const -> nlohmann::json;
};

class ProofSearch {
public:
  ProofSearch(SeedSpace* space) noexcept;

  auto search(const std::string& goal, int max_depth = 5) noexcept
      -> std::vector<ProofStep>;
  auto validate(const std::vector<ProofStep>& proof) noexcept -> bool;
  auto stats() const noexcept -> nlohmann::json;

private:
  SeedSpace* space_;
};

// ═══════════════════════════════════════════════════════════════════
// 逻辑推理器
// ═══════════════════════════════════════════════════════════════════

class LogicReasoner {
public:
  LogicReasoner() noexcept = default;

  auto evaluate(const std::string& proposition) noexcept -> nlohmann::json;
  auto entail(const std::string& premise, const std::string& conclusion) noexcept -> bool;
  auto verify(const std::string& formula) noexcept -> nlohmann::json;
};

}  // namespace nexus::logic

#endif
