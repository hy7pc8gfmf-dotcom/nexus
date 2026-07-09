#ifndef NEXUS_BRIDGE_SINAN_H
#define NEXUS_BRIDGE_SINAN_H

/**
 * @file sinan.h
 * @brief 司南·归海 MCP 引擎 — 任务路由 + 工具分发
 *
 * 司南(指南): 分析任务类型 → 路由到合适 MCP 服务器
 * 归海(汇聚): 多服务器结果 → 综合返回
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::bridge {

struct RouteEntry {
  std::string task_type;     // "reasoning" | "file_read" | "search" | ...
  std::string server_name;   // 目标 MCP 服务器
  std::string tool_name;
  int         priority = 5;

  auto to_json() const -> nlohmann::json;
};

class SinanEngine {
public:
  SinanEngine() noexcept;

  /// 注册路由
  auto add_route(const RouteEntry& route) noexcept -> void;
  auto add_routes(const std::vector<RouteEntry>& routes) noexcept -> void;

  /// 路由并执行
  auto route_and_call(const std::string& task_type,
                      const nlohmann::json& args) noexcept
      -> Result<nlohmann::json>;

  /// 路由查询
  [[nodiscard]] auto find_route(const std::string& task_type) const noexcept
      -> std::vector<RouteEntry>;

  /// 统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;

private:
  std::vector<RouteEntry> routes_;
  int routed_count_ = 0;
};

}  // namespace nexus::bridge

#endif
