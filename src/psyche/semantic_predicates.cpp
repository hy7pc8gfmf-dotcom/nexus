// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_predicates.h"

#include <algorithm>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 谓词方向表 → JSON
// ═══════════════════════════════════════════════════════════════════

auto predicate_directions_to_json() -> nlohmann::json {
  auto arr = nlohmann::json::array();
  for (int i = 0; i < kPredicateCount; ++i) {
    auto j = nlohmann::json::object();
    j["name"]    = kPredicateDirections[i].name;
    j["src_tag"] = kPredicateDirections[i].src_tag;
    j["dst_tag"] = kPredicateDirections[i].dst_tag;
    arr.push_back(j);
  }
  return arr;
}

}  // namespace nexus::psyche
