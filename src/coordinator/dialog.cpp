/**
 * @file dialog.cpp
 * @brief 对话框主导权接管实现
 */

#include "nexus/coordinator/dialog.h"

namespace nexus::coordinator {

auto DialogRoute::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["routed_count"] = routed_count; j["local_count"] = local_count;
  j["preferred"] = preferred;
  return j;
}

auto DialogRouter::route(const std::string& task_type) noexcept -> std::string {
  routed_++;
  if (task_type == "reasoning" || task_type == "dialectic") {
    local_++;
    return "local";  // 本地模型优先
  }
  return "cloud";
}

auto DialogRouter::stats() const noexcept -> DialogRoute {
  DialogRoute r; r.routed_count = routed_; r.local_count = local_; return r;
}

void DialogRouter::reset() noexcept { routed_ = 0; local_ = 0; }

}  // namespace nexus::coordinator
