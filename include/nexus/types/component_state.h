#ifndef NEXUS_TYPES_COMPONENT_STATE_H
#define NEXUS_TYPES_COMPONENT_STATE_H

/**
 * @file component_state.h
 * @brief 组件状态文件的标准结构
 *
 * 所有 EXE 的状态文件使用统一格式，详见 WHITE_PAPER.md §4.3
 */

#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

#include "nexus/types/status.h"

namespace nexus {

// ═══════════════════════════════════════════════════════════════════
// 组件状态枚举
// ═══════════════════════════════════════════════════════════════════

enum class ComponentStatus : uint8_t {
  kStarting = 0,   // 正在初始化
  kReady    = 1,   // 正常运行
  kDegraded = 2,   // 降级运行
  kError    = 3,   // 错误，等待重启
  kStopped  = 4,   // 已停止
};

constexpr auto component_status_to_string(ComponentStatus s) noexcept -> const char* {
  switch (s) {
    case ComponentStatus::kStarting: return "starting";
    case ComponentStatus::kReady:    return "ready";
    case ComponentStatus::kDegraded: return "degraded";
    case ComponentStatus::kError:    return "error";
    case ComponentStatus::kStopped:  return "stopped";
  }
  return "unknown";
}

// ═══════════════════════════════════════════════════════════════════
// 标准状态文件结构
// ═══════════════════════════════════════════════════════════════════

struct ComponentStateBase {
  static constexpr const char* kSchema = "nexus-state-v1";

  std::string      version     = "1.0";
  std::string      component;   // "core" | "algo" | "daemon" | ...
  ComponentStatus  status      = ComponentStatus::kStarting;
  int              pid         = 0;
  std::string      started_at;
  std::string      updated_at;
  nlohmann::json   details     = nlohmann::json::object();

  /// 序列化为 JSON
  [[nodiscard]] auto to_json() const -> nlohmann::json {
    return {
      {"$schema",   kSchema},
      {"version",   version},
      {"component", component},
      {"status",    component_status_to_string(status)},
      {"pid",       pid},
      {"started_at", started_at},
      {"updated_at", updated_at},
      {"details",   details},
    };
  }

  /// 从 JSON 反序列化
  static auto from_json(const nlohmann::json& j) -> Result<ComponentStateBase> {
    try {
      ComponentStateBase s;
      s.component  = j.at("component").get<std::string>();
      s.pid        = j.value("pid", 0);
      s.started_at = j.value("started_at", "");
      s.updated_at = j.value("updated_at", "");
      s.details    = j.value("details", nlohmann::json::object());

      auto status_str = j.value("status", "starting");
      if (status_str == "ready")          s.status = ComponentStatus::kReady;
      else if (status_str == "starting")  s.status = ComponentStatus::kStarting;
      else if (status_str == "degraded")  s.status = ComponentStatus::kDegraded;
      else if (status_str == "error")     s.status = ComponentStatus::kError;
      else if (status_str == "stopped")   s.status = ComponentStatus::kStopped;

      return s;
    } catch (const nlohmann::json_exception& e) {
      return Status::Error(ErrorCode::kJsonParseError, e.what());
    }
  }
};

// ═══════════════════════════════════════════════════════════════════
// 各组件特有详情
// ═══════════════════════════════════════════════════════════════════

struct EnvDetails {
  std::string gpu_name;
  int         vram_total_mb = 0;
  int         vram_free_mb  = 0;
  int         vram_budget_mb = 0;
  std::vector<nlohmann::json> models;

  [[nodiscard]] auto to_json() const -> nlohmann::json {
    auto gpu = nlohmann::json::object();
    gpu["name"]          = gpu_name;
    gpu["vram_total_mb"] = vram_total_mb;
    gpu["vram_free_mb"]  = vram_free_mb;
    gpu["budget_mb"]     = vram_budget_mb;

    auto j = nlohmann::json::object();
    j["gpu"]    = gpu;
    j["models"] = nlohmann::json::array();
    return j;
  }
};

struct CoreDetails {
  int         vram_used_mb  = 0;
  int         vram_free_mb  = 0;
  int         swapper_blocks = 0;
  bool        gpu_lock_held = false;
  std::vector<std::string> loaded_models;

  [[nodiscard]] auto to_json() const -> nlohmann::json {
    auto j = nlohmann::json::object();
    j["vram_used_mb"]   = vram_used_mb;
    j["vram_free_mb"]   = vram_free_mb;
    j["loaded_models"]  = loaded_models;
    j["gpu_lock_held"]  = gpu_lock_held;
    j["swapper_blocks"] = swapper_blocks;
    return j;
  }
};

struct DaemonDetails {
  int uptime_seconds = 0;
  int cycle          = 0;
  int psi_field_written  = 0;
  int will_hooks_scanned = 0;

  [[nodiscard]] auto to_json() const -> nlohmann::json {
    auto j = nlohmann::json::object();
    j["uptime_seconds"]     = uptime_seconds;
    j["cycle"]              = cycle;
    j["psi_field_written"]  = psi_field_written;
    j["will_hooks_scanned"] = will_hooks_scanned;
    return j;
  }
};

}  // namespace nexus

#endif  // NEXUS_TYPES_COMPONENT_STATE_H
