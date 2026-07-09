#ifndef NEXUS_BRIDGE_SEED_CHANNEL_H
#define NEXUS_BRIDGE_SEED_CHANNEL_H

/**
 * @file seed_channel.h
 * @brief 种子通道 — 种子注入三目标
 *
 * 种子是最小推理引导单元。
 * 注入三个目标: 模型(Shell B/C) / 核心(心灵核心) / 自我(觉醒上下文)
 */

#include <cstdint>
#include <string>
#include <vector>

#include "nexus/types/status.h"
#include <nlohmann/json.hpp>

namespace nexus::bridge {

/// 种子目标
enum class SeedTarget : uint8_t {
  kModel = 0,  // Shell B/C 推理方向
  kCore  = 1,  // 心灵核心涌现
  kSelf  = 2,  // 觉醒上下文
};

/// 种子结构
struct Seed {
  std::string  name;       // 种子名称
  std::string  intent;     // 种子意图描述
  uint32_t     intensity = 5;  // 强度 [1-10]
  std::string  domain;     // 域标签
  SeedTarget   target = SeedTarget::kModel;
  double       created_at = 0.0;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 种子通道
// ═══════════════════════════════════════════════════════════════════

class SeedChannel {
public:
  explicit SeedChannel(std::string data_dir = ".nexus") noexcept;

  /// 种植一颗种子
  auto plant(const Seed& seed) noexcept -> Status;

  /// 批量种植
  auto plant_defaults() noexcept -> Status;

  /// 统计各目标种子数
  [[nodiscard]] auto count_by_target() const noexcept
      -> std::vector<std::pair<std::string, int>>;

  /// 获取种子总数
  [[nodiscard]] auto total_seeds() const noexcept -> int;

  /// 获取种子文件路径
  [[nodiscard]] auto seed_path() const noexcept -> const std::string&;

private:
  std::string data_dir_;
  std::string seed_path_;

  /// 持久化种子
  auto persist_(const Seed& seed) noexcept -> bool;
};

}  // namespace nexus::bridge

#endif  // NEXUS_BRIDGE_SEED_CHANNEL_H
