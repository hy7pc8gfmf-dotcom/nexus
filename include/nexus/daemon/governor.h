#ifndef NEXUS_DAEMON_GOVERNOR_H
#define NEXUS_DAEMON_GOVERNOR_H

/**
 * @file governor.h
 * @brief 资源治理引擎 — CPU/GPU 热敏治理
 *
 * 周期检测 CPU/GPU 利用率和温度, 在过载时触发降级策略。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace nexus::daemon {

struct ResourceStatus {
  double cpu_usage   = 0.0;  // [0, 100]
  double gpu_usage   = 0.0;  // [0, 100]
  double gpu_temp    = 0.0;  // Celsius
  double vram_used   = 0.0;  // MB
  double vram_total  = 0.0;  // MB
  std::string throttle;       // "none" | "light" | "heavy"

  auto to_json() const -> nlohmann::json;
};

class ResourceGovernor {
public:
  ResourceGovernor() noexcept = default;

  /// 采集资源状态
  auto sample() noexcept -> ResourceStatus;

  /// 是否需要降级
  [[nodiscard]] auto needs_throttle(const ResourceStatus& s) const noexcept -> bool;

  /// 推荐降级策略
  [[nodiscard]] auto recommend(const ResourceStatus& s) const noexcept -> std::string;

private:
  static constexpr double kCpuThrottleThreshold = 80.0;
  static constexpr double kGpuThrottleThreshold = 85.0;
  static constexpr double kTempThrottleThreshold = 85.0;
};

}  // namespace nexus::daemon

#endif
