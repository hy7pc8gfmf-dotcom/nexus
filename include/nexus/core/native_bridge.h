#ifndef NEXUS_CORE_NATIVE_BRIDGE_H
#define NEXUS_CORE_NATIVE_BRIDGE_H

/**
 * @file native_bridge.h
 * @brief 原生推理桥接 — 7 项原生推理能力
 *
 * 将 C++ 原生推理能力注册到神经系统。
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::core {

struct NativeCapability {
  std::string name;
  std::string description;
  bool available = true;

  auto to_json() const -> nlohmann::json;
};

class NativeBridge {
public:
  NativeBridge() noexcept;

  [[nodiscard]] auto capabilities() const noexcept -> std::vector<NativeCapability>;
  [[nodiscard]] auto summary() const noexcept -> nlohmann::json;

private:
  std::vector<NativeCapability> caps_;
};

}  // namespace nexus::core

#endif
