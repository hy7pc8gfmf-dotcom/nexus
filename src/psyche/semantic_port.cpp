// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_port.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <numeric>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto SemanticConcept::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]  = name;
  j["lang"]  = lang;
  j["intensity"] = intensity;

  auto coord_arr = nlohmann::json::array();
  for (int i = 0; i < kSemanticDim; ++i) coord_arr.push_back(coord[i]);
  j["coord"] = coord_arr;

  auto neighbors_arr = nlohmann::json::array();
  j["n_neighbors"] = static_cast<int>(neighbor_indices.size());
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 加载
// ═══════════════════════════════════════════════════════════════════

auto SemanticPort::load(const std::string& path) noexcept -> Status {
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs.is_open()) {
    return Status::Error(ErrorCode::kFileNotFound,
      "semantic field not found: " + path + ". "
      "This file is part of the Nexus runtime components. "
      "See CLA/RLA.md for how to obtain it.");
  }

  // 读 HEADER
  uint32_t magic, version, n_concept, n_edge, dim;
  ifs.read(reinterpret_cast<char*>(&magic), 4);
  ifs.read(reinterpret_cast<char*>(&version), 4);
  ifs.read(reinterpret_cast<char*>(&n_concept), 4);
  ifs.read(reinterpret_cast<char*>(&n_edge), 4);
  ifs.read(reinterpret_cast<char*>(&dim), 4);

  if (magic != kSemanticMagic) {
    return Status::Error(ErrorCode::kJsonParseError,
      "invalid semantic field magic");
  }
  if (dim != kSemanticDim) {
    return Status::Error(ErrorCode::kJsonParseError,
      "expected dim=" + std::to_string(kSemanticDim) +
      " got " + std::to_string(dim));
  }

  concepts_.reserve(n_concept);
  name_to_idx_.reserve(n_concept);
  edges_.reserve(n_edge);

  // 读 CONCEPTS
  for (uint32_t i = 0; i < n_concept; ++i) {
    SemanticConcept c;
    c.index = static_cast<int>(i);

    uint16_t name_len;
    ifs.read(reinterpret_cast<char*>(&name_len), 2);

    std::string name_buf(name_len, '\0');
    ifs.read(name_buf.data(), name_len);
    c.name = name_buf;

    ifs.read(reinterpret_cast<char*>(c.coord), sizeof(float) * kSemanticDim);

    float intensity;
    ifs.read(reinterpret_cast<char*>(&intensity), 4);
    c.intensity = intensity;

    uint16_t lang_len;
    ifs.read(reinterpret_cast<char*>(&lang_len), 2);
    if (lang_len > 0) {
      std::string lang_buf(lang_len, '\0');
      ifs.read(lang_buf.data(), lang_len);
      c.lang = lang_buf;
    }

    // 2 字节填充 (对齐)
    char pad[2];
    ifs.read(pad, 2);

    concepts_.push_back(std::move(c));
    name_to_idx_[concepts_.back().name] = static_cast<int>(i);
  }

  // 读 EDGES (邻接表)
  for (uint32_t i = 0; i < n_concept; ++i) {
    uint32_t edge_count;
    ifs.read(reinterpret_cast<char*>(&edge_count), 4);

    concepts_[i].neighbor_indices.reserve(edge_count);
    for (uint32_t e = 0; e < edge_count; ++e) {
      int32_t neighbor_idx;
      ifs.read(reinterpret_cast<char*>(&neighbor_idx), 4);
      concepts_[i].neighbor_indices.push_back(neighbor_idx);
      edges_.emplace_back(static_cast<int>(i), neighbor_idx);
    }
  }

  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════════

auto SemanticPort::concept_of(const std::string& name) const noexcept
    -> Result<SemanticConcept> {
  auto it = name_to_idx_.find(name);
  if (it == name_to_idx_.end()) {
    // 模糊搜索: 找前缀匹配
    for (const auto& c : concepts_) {
      if (c.name.find(name) == 0) {
        return c;
      }
    }
    return Status::Error(ErrorCode::kFileNotFound,
      "concept not found: " + name);
  }
  return concepts_[it->second];
}

auto SemanticPort::neighbors(const std::string& name, int limit) const noexcept
    -> std::vector<std::string> {
  auto it = name_to_idx_.find(name);
  if (it == name_to_idx_.end()) return {};

  const auto& c = concepts_[it->second];
  std::vector<std::string> result;
  result.reserve(std::min(limit, static_cast<int>(c.neighbor_indices.size())));

  for (int idx : c.neighbor_indices) {
    if (idx >= 0 && idx < static_cast<int>(concepts_.size())) {
      result.push_back(concepts_[idx].name);
      if (static_cast<int>(result.size()) >= limit) break;
    }
  }
  return result;
}

auto SemanticPort::find(
    std::function<bool(const SemanticConcept&)> pred, int limit) const noexcept
    -> std::vector<SemanticConcept> {
  std::vector<SemanticConcept> result;
  for (const auto& c : concepts_) {
    if (pred(c)) {
      result.push_back(c);
      if (static_cast<int>(result.size()) >= limit) break;
    }
  }
  return result;
}

auto SemanticPort::distance(const std::string& a, const std::string& b) const noexcept
    -> Result<double> {
  auto it_a = name_to_idx_.find(a);
  auto it_b = name_to_idx_.find(b);
  if (it_a == name_to_idx_.end() || it_b == name_to_idx_.end()) {
    return Status::Error(ErrorCode::kFileNotFound, "concept not found");
  }

  float sim = cosine_sim_(coord_ptr_(it_a->second), coord_ptr_(it_b->second));
  return std::acos(std::clamp(static_cast<double>(sim), -1.0, 1.0));
}

// ═══════════════════════════════════════════════════════════════════
// 统计
// ═══════════════════════════════════════════════════════════════════

auto SemanticPort::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["concepts"]  = static_cast<int>(concepts_.size());
  j["edges"]     = static_cast<int>(edges_.size());
  j["dimension"] = kSemanticDim;
  j["loaded"]    = loaded();

  // 语言分布
  std::unordered_map<std::string, int> lang_counts;
  for (const auto& c : concepts_) {
    lang_counts[c.lang]++;
  }
  auto lang_json = nlohmann::json::object();
  for (const auto& [lang, count] : lang_counts) {
    lang_json[lang] = count;
  }
  j["languages"] = lang_json;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 内部
// ═══════════════════════════════════════════════════════════════════

auto SemanticPort::coord_ptr_(int idx) const noexcept -> const float* {
  if (idx < 0 || idx >= static_cast<int>(concepts_.size())) return nullptr;
  return concepts_[idx].coord;
}

float SemanticPort::cosine_sim_(const float* a, const float* b) {
  if (!a || !b) return 0.0f;
  float dot = 0, na = 0, nb = 0;
  for (int i = 0; i < kSemanticDim; ++i) {
    dot += a[i] * b[i];
    na  += a[i] * a[i];
    nb  += b[i] * b[i];
  }
  float denom = std::sqrt(na) * std::sqrt(nb);
  return denom > 0 ? dot / denom : 0.0f;
}

}  // namespace nexus::psyche
