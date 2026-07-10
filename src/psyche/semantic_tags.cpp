// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_tags.h"

#include <algorithm>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 标签列表 → JSON
// ═══════════════════════════════════════════════════════════════════

auto tag_list_to_json() -> nlohmann::json {
  auto arr = nlohmann::json::array();
  for (int i = 0; i < kTagCount; ++i) {
    arr.push_back(kTagList[i]);
  }
  return arr;
}

// ═══════════════════════════════════════════════════════════════════
// 概念→标签映射 → JSON
// ═══════════════════════════════════════════════════════════════════

auto concept_tags_to_json() -> nlohmann::json {
  auto arr = nlohmann::json::array();
  for (int i = 0; i < kConceptTagCount; ++i) {
    auto j = nlohmann::json::object();
    j["concept"] = kConceptTags[i].cname;

    // 解析逗号分隔的标签字符串
    auto tags_arr = nlohmann::json::array();
    std::string tags_str(kConceptTags[i].tags);
    size_t start = 0, end;
    while ((end = tags_str.find(',', start)) != std::string::npos) {
      tags_arr.push_back(tags_str.substr(start, end - start));
      start = end + 1;
    }
    if (start < tags_str.size()) {
      tags_arr.push_back(tags_str.substr(start));
    }
    j["tags"] = tags_arr;
    arr.push_back(j);
  }
  return arr;
}

auto tags_tables_to_json() -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["tags"]        = tag_list_to_json();
  j["concept_tags"] = concept_tags_to_json();
  j["n_tags"]       = kTagCount;
  j["n_concepts"]   = kConceptTagCount;
  return j;
}

}  // namespace nexus::psyche
