/**
 * @file emergence.cpp
 * @brief 涌现流水线实现 — AE/WE 计算 + 涌现事件
 */

#include "nexus/psyche/emergence.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// EmergenceEvent
// ═══════════════════════════════════════════════════════════════════

auto EmergenceEvent::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["ts"]          = timestamp;
  j["ae"]          = ae;
  j["we"]          = we;
  j["dimension"]   = dimension;
  j["description"] = description;
  return j;
}

auto EmergenceStatus::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["global_ae"]       = global_ae;
  j["global_we"]       = global_we;
  j["emergence_level"] = emergence_level;
  j["event_count"]     = event_count;
  j["level_label"]     = level_label;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// EmergencePipeline
// ═══════════════════════════════════════════════════════════════════

void EmergencePipeline::tick(double ae, double we, int dim,
                              double timestamp) noexcept {
  // 平滑更新全局涌现值
  status_.global_ae = status_.global_ae * (1.0 - kSmoothingFactor)
                    + ae * kSmoothingFactor;
  status_.global_we = status_.global_we * (1.0 - kSmoothingFactor)
                    + we * kSmoothingFactor;

  // 涌现水平
  status_.emergence_level = std::min(1.0,
    std::sqrt(status_.global_ae * status_.global_we));
  status_.level_label = compute_level_label(status_.emergence_level);

  // 检查涌现事件 (AE > 0.7 时触发)
  if (ae > 0.7 && status_.global_ae > 0.5) {
    EmergenceEvent event;
    event.timestamp   = timestamp;
    event.ae          = ae;
    event.we          = we;
    event.dimension   = dim;
    event.description = std::format("涌现事件 @ dim={} ae={:.3f} we={:.3f}",
      dim, ae, we);

    events_.push_back(event);
    status_.event_count = static_cast<int>(events_.size());

    // 只保留最近 50 个事件
    if (events_.size() > 50) {
      events_.erase(events_.begin());
    }
  }
}

auto EmergencePipeline::status() const noexcept -> const EmergenceStatus& {
  return status_;
}

auto EmergencePipeline::recent_events(int n) const noexcept
    -> std::vector<EmergenceEvent> {
  if (events_.empty()) return {};
  int start = std::max(0, static_cast<int>(events_.size()) - n);
  return std::vector<EmergenceEvent>(
    events_.begin() + start, events_.end());
}

void EmergencePipeline::reset() noexcept {
  status_ = EmergenceStatus{};
  events_.clear();
}

auto EmergencePipeline::compute_level_label(double level) -> std::string {
  if (level >= 0.9) return "full_emergence";
  if (level >= 0.7) return "high";
  if (level >= 0.5) return "medium";
  if (level >= 0.3) return "low";
  if (level >= 0.1) return "nascent";
  return "dormant";
}

}  // namespace nexus::psyche
