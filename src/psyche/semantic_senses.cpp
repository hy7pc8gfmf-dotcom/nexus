// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_senses.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto SenseEntry::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["character"] = character;
  j["sense_id"]  = sense_id;
  j["gloss"]     = gloss;
  auto cls = nlohmann::json::array();
  for (const auto& c : clusters) cls.push_back(c);
  j["clusters"] = cls;
  auto off = nlohmann::json::array();
  for (int i = 0; i < kSemanticDim; ++i) off.push_back(offset[i]);
  j["offset"] = off;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

SenseRegistry::SenseRegistry() noexcept {
  register_builtin();
}

void SenseRegistry::add_sense_(const std::string& ch, const std::string& id,
                                const std::string& gloss,
                                const std::vector<std::string>& clusters,
                                const std::vector<float>& offset) {
  SenseEntry e;
  e.character = ch;
  e.sense_id  = id;
  e.gloss     = gloss;
  e.clusters  = clusters;
  for (size_t i = 0; i < kSemanticDim && i < offset.size(); ++i)
    e.offset[i] = offset[i];
  senses_[ch].push_back(e);
  total_++;
}

void SenseRegistry::register_builtin() noexcept {
  add_sense_("行", "xing_walk",    "行走/行动",   {"action"},     {0.55,0.50,0.45,0.35,0.40,0.45,0.50,0.40,0.45,0.35,0.15,0.10,0.15,0.10});
  add_sense_("行", "xing_ok",      "可以/同意",   {"connector"},  {0.50,0.55,0.35,0.40,0.50,0.45,0.50,0.50,0.35,0.40,0.10,0.05,0.10,0.05});
  add_sense_("行", "hang_prof",    "行业/职业",   {"politics"},   {0.40,0.45,0.50,0.35,0.35,0.55,0.50,0.45,0.50,0.45,0.15,0.10,0.15,0.10});
  add_sense_("行", "xing_able",    "能干/有本事", {"philosophy"},  {0.50,0.60,0.55,0.45,0.55,0.50,0.55,0.55,0.50,0.45,0.10,0.05,0.10,0.05});
  add_sense_("红", "hong_color",   "红色/颜色",   {"color"},      {0.68,0.55,0.40,0.55,0.45,0.50,0.55,0.50,0.40,0.35,0.10,0.05,0.10,0.05});
  add_sense_("红", "hong_love",    "爱/情",       {"emotion"},    {0.55,0.70,0.50,0.55,0.65,0.55,0.60,0.65,0.50,0.50,0.10,0.05,0.10,0.05});
  add_sense_("红", "hong_politics","革命/政治",   {"politics"},   {0.45,0.50,0.60,0.65,0.40,0.50,0.55,0.55,0.60,0.55,0.15,0.10,0.15,0.10});
  add_sense_("天", "tian_nature",  "天空/自然",   {"nature"},     {0.50,0.35,0.25,0.20,0.30,0.60,0.55,0.30,0.25,0.20,0.10,0.05,0.10,0.05});
  add_sense_("天", "tian_god",     "天神/命运",   {"religion"},   {0.35,0.70,0.50,0.40,0.75,0.65,0.70,0.60,0.50,0.45,0.20,0.15,0.20,0.10});
  add_sense_("金", "jin_metal",    "金属/黄金",   {"material"},   {0.55,0.40,0.40,0.35,0.35,0.70,0.55,0.45,0.40,0.30,0.10,0.05,0.10,0.05});
  add_sense_("金", "jin_money",    "金钱/财富",   {"politics"},   {0.45,0.55,0.65,0.50,0.50,0.55,0.50,0.65,0.60,0.55,0.15,0.10,0.15,0.10});
  add_sense_("黄", "huang_color",  "黄色/颜色",   {"color"},      {0.65,0.40,0.30,0.35,0.35,0.50,0.45,0.40,0.30,0.25,0.10,0.05,0.10,0.05});
  add_sense_("黄", "huang_porn",   "色情/低俗",   {"abstract"},   {0.40,0.45,0.45,0.50,0.40,0.30,0.35,0.55,0.45,0.40,0.15,0.10,0.15,0.10});
  add_sense_("白", "bai_color",    "白色/颜色",   {"color"},      {0.65,0.35,0.25,0.30,0.30,0.50,0.45,0.35,0.25,0.20,0.10,0.05,0.10,0.05});
  add_sense_("白", "bai_fear",     "恐惧/苍白",   {"emotion"},    {0.45,0.50,0.35,0.45,0.45,0.35,0.40,0.45,0.35,0.30,0.10,0.05,0.10,0.05});
  add_sense_("白", "bai_free",     "免费/空白",   {"abstract"},   {0.50,0.30,0.45,0.35,0.25,0.40,0.45,0.40,0.45,0.40,0.10,0.05,0.10,0.05});
  add_sense_("长", "chang_length", "长度/距离",   {"time_space"}, {0.45,0.30,0.35,0.25,0.30,0.55,0.50,0.35,0.35,0.25,0.10,0.05,0.10,0.05});
  add_sense_("长", "zhang_grow",   "生长/增长",   {"action"},     {0.55,0.40,0.55,0.45,0.35,0.45,0.50,0.40,0.55,0.40,0.15,0.10,0.15,0.10});
  add_sense_("水", "shui_nature",  "水流/自然",   {"nature"},     {0.60,0.50,0.30,0.25,0.35,0.50,0.40,0.30,0.25,0.20,0.10,0.05,0.10,0.05});
  add_sense_("水", "shui_food",    "饮用水",       {"food"},       {0.55,0.35,0.25,0.20,0.25,0.45,0.35,0.25,0.20,0.15,0.08,0.04,0.08,0.04});
}

// ═══════════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════════

auto SenseRegistry::get_senses(const std::string& character) const noexcept
    -> std::vector<SenseEntry> {
  auto it = senses_.find(character);
  if (it == senses_.end()) return {};
  return it->second;
}

auto SenseRegistry::get_entity(const std::string& sense_id) const noexcept
    -> std::string {
  for (const auto& [ch, entries] : senses_) {
    for (const auto& e : entries) {
      if (e.sense_id == sense_id)
        return ch + "/" + e.gloss;
    }
  }
  return "";
}

auto SenseRegistry::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total_senses"]   = total_;
  j["n_characters"]   = static_cast<int>(senses_.size());
  auto chars = nlohmann::json::array();
  for (const auto& [ch, _] : senses_) chars.push_back(ch);
  j["characters"] = chars;
  return j;
}

void SenseRegistry::register_sense(const SenseEntry& entry) noexcept {
  senses_[entry.character].push_back(entry);
  total_++;
}

}  // namespace nexus::psyche
