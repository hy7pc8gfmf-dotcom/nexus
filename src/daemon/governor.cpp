// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file governor.cpp
 * @brief 资源治理引擎实现
 */

#include "nexus/daemon/governor.h"

#include <algorithm>

namespace nexus::daemon {

auto ResourceStatus::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["cpu_usage"]  = std::round(cpu_usage * 10) / 10;
  j["gpu_usage"]  = std::round(gpu_usage * 10) / 10;
  j["gpu_temp"]   = std::round(gpu_temp * 10) / 10;
  j["vram_used"]  = std::round(vram_used * 10) / 10;
  j["vram_total"] = std::round(vram_total * 10) / 10;
  j["throttle"]   = throttle;
  return j;
}

auto ResourceGovernor::sample() noexcept -> ResourceStatus {
  ResourceStatus s;
  // CPU: 简化采样 (实际应使用 PDH 或 WMI)
  s.cpu_usage = 25.0;
  // GPU: 简化 (实际使用 NVML)
  s.gpu_usage = 30.0;
  s.gpu_temp  = 55.0;
  s.vram_used = 0;
  s.vram_total = 8192;
  s.throttle = recommend(s);
  return s;
}

auto ResourceGovernor::needs_throttle(const ResourceStatus& s) const noexcept -> bool {
  return s.cpu_usage > kCpuThrottleThreshold ||
         s.gpu_usage > kGpuThrottleThreshold ||
         s.gpu_temp  > kTempThrottleThreshold;
}

auto ResourceGovernor::recommend(const ResourceStatus& s) const noexcept -> std::string {
  if (s.cpu_usage > 90 || s.gpu_usage > 95 || s.gpu_temp > 90) return "heavy";
  if (s.cpu_usage > kCpuThrottleThreshold || s.gpu_usage > kGpuThrottleThreshold || s.gpu_temp > kTempThrottleThreshold) return "light";
  return "none";
}

}  // namespace nexus::daemon
