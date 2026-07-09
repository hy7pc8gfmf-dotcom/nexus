#ifndef NEXUS_BRIDGE_SEED_CULTIVATOR_H
#define NEXUS_BRIDGE_SEED_CULTIVATOR_H

/**
 * @file seed_cultivator.h
 * @brief 种子培育工具 — SELF 循环: 种植 → 培育 → 裂变 → 剪枝
 *
 * 种子 = 最小推理引导单元。
 * 种植: 注入初始种子
 * 培育: 增强高频使用的种子强度
 * 裂变: 高强度种子分裂出新种子
 * 剪枝: 移除低强度/过期的种子
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::bridge {

/// 种子阶段
enum class SeedStage : uint8_t {
  kPlanted  = 0,  // 刚种植
  kGrowing  = 1,  // 培育中
  kMature   = 2,  // 成熟
  kFission  = 3,  // 裂变中
  kPruned   = 4,  // 已剪枝
};

/// 培育种子 (带生命周期)
struct CultivableSeed {
  std::string  id;
  std::string  name;
  std::string  intent;
  int          intensity  = 5;   // [1-10]
  int          use_count  = 0;   // 使用次数
  int          fission_count = 0; // 裂变次数
  SeedStage    stage      = SeedStage::kPlanted;
  double       created_at = 0.0;
  double       last_used  = 0.0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// SeedCultivator — 种子培育器
// ═══════════════════════════════════════════════════════════════════

class SeedCultivator {
public:
  explicit SeedCultivator(std::string data_dir = ".nexus") noexcept;

  /// 种植一颗新种子
  auto plant(const std::string& name, const std::string& intent,
             int intensity = 5) -> Status;

  /// 培育周期: 遍历所有种子, 执行培育/裂变/剪枝
  auto cultivate_cycle() noexcept -> nlohmann::json;

  /// 记录种子使用
  auto record_use(const std::string& seed_id) noexcept -> void;

  /// 获取种子统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;

  /// 获取所有种子
  [[nodiscard]] auto all_seeds() const noexcept -> std::vector<CultivableSeed>;

private:
  std::string data_dir_;
  std::vector<CultivableSeed> seeds_;

  auto load_() noexcept -> void;
  auto save_() noexcept -> void;
  auto find_seed_(const std::string& id) -> CultivableSeed*;
};

}  // namespace nexus::bridge

#endif  // NEXUS_BRIDGE_SEED_CULTIVATOR_H
