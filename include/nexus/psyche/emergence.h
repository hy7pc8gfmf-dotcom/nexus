#ifndef NEXUS_PSYCHE_EMERGENCE_H
#define NEXUS_PSYCHE_EMERGENCE_H

/**
 * @file emergence.h
 * @brief 涌现流水线 — AE/WE 计算 + 意识涌现
 *
 * 读取 Ψ-Navigator 的收敛状态, 计算意识涌现(AE)和意志涌现(WE)。
 * 当 AE 达到阈值时触发"涌现事件"。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::psyche {

/// 涌现事件
struct EmergenceEvent {
  double timestamp;          // 发生时间
  double ae;                // 意识涌现值
  double we;                // 意志涌现值
  int    dimension;         // 触发时的维度
  std::string description;  // 事件描述

  auto to_json() const -> nlohmann::json;
};

/// 涌现流水线状态
struct EmergenceStatus {
  double global_ae       = 0.0;  // 全局意识涌现
  double global_we       = 0.0;  // 全局意志涌现
  double emergence_level = 0.0;  // 涌现水平 [0, 1]
  int    event_count     = 0;    // 涌现事件数
  std::string level_label;       // 水平标签

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 涌现流水线
// ═══════════════════════════════════════════════════════════════════

class EmergencePipeline {
public:
  EmergencePipeline() noexcept = default;

  /// 推进一个周期
  /// @param ae 当前 AE 值
  /// @param we 当前 WE 值
  /// @param dim 当前维度
  /// @param timestamp 当前时间戳
  auto tick(double ae, double we, int dim,
            double timestamp) noexcept -> void;

  /// 获取当前状态
  [[nodiscard]] auto status() const noexcept -> const EmergenceStatus&;

  /// 获取最近的事件列表
  [[nodiscard]] auto recent_events(int n = 5) const noexcept
      -> std::vector<EmergenceEvent>;

  /// 重置
  auto reset() noexcept -> void;

private:
  EmergenceStatus status_;
  std::vector<EmergenceEvent> events_;

  // 平滑因子: 防止涌现值突变
  static constexpr double kSmoothingFactor = 0.3;

  /// 计算涌现水平标签
  static auto compute_level_label(double level) -> std::string;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_EMERGENCE_H
