#ifndef NEXUS_COORDINATOR_DIALOG_H
#define NEXUS_COORDINATOR_DIALOG_H

/**
 * @file dialog.h
 * @brief 对话框主导权接管 — 本地模型优先的推理路由
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace nexus::coordinator {

struct DialogRoute {
  int     routed_count = 0;
  int     local_count  = 0;
  std::string preferred = "local";
  auto to_json() const -> nlohmann::json;
};

class DialogRouter {
public:
  DialogRouter() noexcept = default;
  auto route(const std::string& task_type) noexcept -> std::string;
  auto stats() const noexcept -> DialogRoute;
  void reset() noexcept;

private:
  int routed_ = 0, local_ = 0;
};

}  // namespace nexus::coordinator

#endif
