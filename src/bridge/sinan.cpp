/**
 * @file sinan.cpp
 * @brief 司南·归海 MCP 引擎实现
 */

#include "nexus/bridge/sinan.h"

#include <algorithm>
#include <map>
#include <sstream>

namespace nexus::bridge {

auto RouteEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["task_type"] = task_type; j["server_name"] = server_name;
  j["tool_name"] = tool_name; j["priority"] = priority;
  return j;
}

SinanEngine::SinanEngine() noexcept {
  // 默认路由表
  add_routes({
    {"reasoning",    "agi_capability", "capability_query",     1},
    {"quick_local",  "agi_capability", "capability_status",    2},
    {"self_aware",   "agi_capability", "capability_summary",   1},
    {"capability",   "agi_capability", "capability_lookup",    1},
    {"suggest",      "agi_capability", "capability_suggest",   3},
  });
}

auto SinanEngine::add_route(const RouteEntry& route) noexcept -> void {
  routes_.push_back(route);
}

auto SinanEngine::add_routes(const std::vector<RouteEntry>& routes) noexcept -> void {
  for (const auto& r : routes) add_route(r);
}

auto SinanEngine::route_and_call(const std::string& task_type,
                                  const nlohmann::json& args) noexcept
    -> Result<nlohmann::json> {
  routed_count_++;

  auto entries = find_route(task_type);
  if (entries.empty()) {
    return Status::Error(ErrorCode::kFileNotFound,
      "no route for task: " + task_type);
  }

  // 选优先级最高的路由
  auto best = entries[0];
  for (const auto& e : entries) {
    if (e.priority < best.priority) best = e;
  }

  auto result = nlohmann::json::object();
  result["task_type"]    = task_type;
  result["server"]       = best.server_name;
  result["tool"]         = best.tool_name;
  result["routed_count"] = routed_count_;
  result["status"]       = "routed";

  return result;
}

auto SinanEngine::find_route(const std::string& task_type) const noexcept
    -> std::vector<RouteEntry> {
  std::vector<RouteEntry> result;
  for (const auto& r : routes_) {
    if (r.task_type == task_type) result.push_back(r);
  }
  return result;
}

auto SinanEngine::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_routes"] = static_cast<int>(routes_.size());
  j["routed_count"] = routed_count_;

  auto by_server = nlohmann::json::object();
  for (const auto& r : routes_) {
    int current = by_server[r.server_name].is_number() ? by_server[r.server_name].get<int>() : 0;
    by_server[r.server_name] = current + 1;
  }
  j["by_server"] = by_server;
  return j;
}

}  // namespace nexus::bridge
