// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_pos.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "nexus/psyche/semantic_expand.h"

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 数词表 → JSON 导出
// ═══════════════════════════════════════════════════════════════════

auto numerals_to_json() -> nlohmann::json {
  auto arr = nlohmann::json::array();
  for (int i = 0; i < kNumeralCount; ++i) {
    auto j = nlohmann::json::object();
    j["name"]        = kNumerals[i].name;
    j["value"]       = kNumerals[i].value;
    j["usage"]       = kNumerals[i].usage;
    j["superscript"] = kNumerals[i].superscript;
    j["desc"]        = kNumerals[i].desc;
    // 生成 14D 坐标
    auto coord = SemanticExpander::name_to_coord(kNumerals[i].name, 0);
    // 抽象端偏高 (dim10-13 +0.35)
    for (int d = 10; d < 14 && d < static_cast<int>(coord.size()); ++d) {
      coord[d] = std::min(1.0f, coord[d] + 0.35f);
    }
    auto coord_arr = nlohmann::json::array();
    for (float v : coord) coord_arr.push_back(static_cast<double>(v));
    j["coord"] = coord_arr;
    arr.push_back(j);
  }
  return arr;
}

// ═══════════════════════════════════════════════════════════════════
// 量词表 → JSON 导出
// ═══════════════════════════════════════════════════════════════════

auto measures_to_json() -> nlohmann::json {
  auto arr = nlohmann::json::array();
  for (int i = 0; i < kMeasureCount; ++i) {
    auto j = nlohmann::json::object();
    j["name"]        = kMeasures[i].name;
    j["category"]    = kMeasures[i].category;
    j["applicable"]  = kMeasures[i].applicable;
    j["superscript"] = kMeasures[i].superscript;
    auto coord = SemanticExpander::name_to_coord(kMeasures[i].name, 1);
    auto coord_arr = nlohmann::json::array();
    for (float v : coord) coord_arr.push_back(static_cast<double>(v));
    j["coord"] = coord_arr;
    arr.push_back(j);
  }
  return arr;
}

// ═══════════════════════════════════════════════════════════════════
// 完整导出
// ═══════════════════════════════════════════════════════════════════

auto pos_tables_to_json() -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["numerals"]  = numerals_to_json();
  j["measures"]  = measures_to_json();
  j["n_numerals"] = kNumeralCount;
  j["n_measures"] = kMeasureCount;
  return j;
}

}  // namespace nexus::psyche
