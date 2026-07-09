/**
 * @file interlock.cpp
 * @brief 三反射互锁实现
 */

#include "nexus/algo/interlock.h"
#include <sstream>

namespace nexus::algo {

auto InterlockResult::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["shell_a_approved"] = shell_a_approved;
  j["shell_b_approved"] = shell_b_approved;
  j["shell_c_approved"] = shell_c_approved;
  j["detail"] = detail;
  return j;
}

auto TripleInterlock::verify(const std::string& operation) noexcept -> InterlockResult {
  InterlockResult r;
  r.shell_a_approved = true;  // Shell A 始终可用
  r.shell_b_approved = true;  // Shell B 始终可用
  r.shell_c_approved = violation_count_ < kMaxViolations;
  r.detail = "operation=" + operation;
  if (!r.shell_c_approved) violation_count_++;
  return r;
}

void TripleInterlock::reset() noexcept { violation_count_ = 0; }

}  // namespace nexus::algo
