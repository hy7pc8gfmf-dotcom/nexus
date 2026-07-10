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
 * @file seed_cultivator.cpp
 * @brief 种子培育工具实现 — SELF 循环
 */

#include "nexus/bridge/seed_cultivator.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace nexus::bridge {

namespace fs = std::filesystem;
using nexus::Status;
using nexus::ErrorCode;

auto CultivableSeed::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"]           = id;
  j["name"]         = name;
  j["intent"]       = intent;
  j["intensity"]    = intensity;
  j["use_count"]    = use_count;
  j["fission_count"]= fission_count;
  j["stage"]        = static_cast<int>(stage);
  j["created_at"]   = created_at;
  j["last_used"]    = last_used;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// SeedCultivator
// ═══════════════════════════════════════════════════════════════════

SeedCultivator::SeedCultivator(std::string data_dir) noexcept
  : data_dir_(std::move(data_dir)) {
  load_();
}

auto SeedCultivator::plant(const std::string& name, const std::string& intent,
                            int intensity) -> Status {
  CultivableSeed seed;
  seed.id = "seed_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count());
  seed.name      = name;
  seed.intent    = intent;
  seed.intensity = std::clamp(intensity, 1, 10);
  seed.stage     = SeedStage::kPlanted;
  seed.created_at = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  seed.last_used = seed.created_at;

  seeds_.push_back(seed);
  save_();
  return Status::Ok();
}

auto SeedCultivator::cultivate_cycle() noexcept -> nlohmann::json {
  auto result = nlohmann::json::object();
  int grew = 0, fissioned = 0, pruned = 0;

  // 裂变: 成熟种子 + 强度≥8 → 分裂出子种子
  std::vector<CultivableSeed> new_seeds;
  for (auto& seed : seeds_) {
    if (seed.stage == SeedStage::kPruned) continue;

    // 更新阶段
    if (seed.use_count >= 20 && seed.intensity >= 7) {
      seed.stage = SeedStage::kMature;
    } else if (seed.use_count >= 5) {
      seed.stage = SeedStage::kGrowing;
    }

    // 培育: 每使用 5 次强度 +1
    if (seed.use_count > 0 && seed.use_count % 5 == 0 && seed.intensity < 10) {
      seed.intensity++;
      grew++;
    }

    // 裂变: 成熟种子 + 强度≥8 → 分裂出子种子
    if (seed.stage == SeedStage::kMature && seed.intensity >= 8 &&
        seed.fission_count < 3) {
      CultivableSeed child;
      child.id = seed.id + "_child_" + std::to_string(seed.fission_count + 1);
      child.name = seed.name + "_v" + std::to_string(seed.fission_count + 2);
      child.intent = seed.intent + " (派生)";
      child.intensity = std::max(1, seed.intensity - 2);
      child.stage = SeedStage::kFission;
      child.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

      new_seeds.push_back(child);
      seed.fission_count++;
      fissioned++;
    }

    // 剪枝: 超过 30 天未使用 或 强度降至 1
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    double days_unused = (now - seed.last_used) / 86400.0;
    if (days_unused > 30 || (seed.intensity <= 1 && seed.use_count > 0)) {
      seed.stage = SeedStage::kPruned;
      pruned++;
    }
  }

  // 将裂变产生的新种子追加到容器（不能在迭代中 push_back）
  seeds_.insert(seeds_.end(), new_seeds.begin(), new_seeds.end());

  save_();

  result["grew"]      = grew;
  result["fissioned"] = fissioned;
  result["pruned"]    = pruned;
  result["total"]     = static_cast<int>(seeds_.size());
  return result;
}

auto SeedCultivator::record_use(const std::string& seed_id) noexcept -> void {
  auto* seed = find_seed_(seed_id);
  if (!seed) return;
  seed->use_count++;
  seed->last_used = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  save_();
}

auto SeedCultivator::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["total"] = static_cast<int>(seeds_.size());

  int planted = 0, growing = 0, mature = 0, fission = 0, pruned = 0;
  for (const auto& s : seeds_) {
    switch (s.stage) {
      case SeedStage::kPlanted: planted++; break;
      case SeedStage::kGrowing: growing++; break;
      case SeedStage::kMature:  mature++;  break;
      case SeedStage::kFission: fission++; break;
      case SeedStage::kPruned:  pruned++;  break;
    }
  }
  j["planted"]  = planted;
  j["growing"]  = growing;
  j["mature"]   = mature;
  j["fission"]  = fission;
  j["pruned"]   = pruned;

  return j;
}

auto SeedCultivator::all_seeds() const noexcept -> std::vector<CultivableSeed> {
  return seeds_;
}

// ═══════════════════════════════════════════════════════════════════
// 私有
// ═══════════════════════════════════════════════════════════════════

void SeedCultivator::load_() noexcept {
  std::string path = data_dir_ + "/seed_store.json";
  if (!fs::exists(path)) return;

  std::ifstream ifs(path);
  if (!ifs.is_open()) return;

  nlohmann::json data;
  ifs >> data;

  if (data.is_array()) {
    for (const auto& item : data) {
      CultivableSeed seed;
      seed.id        = item.value("id", "");
      seed.name      = item.value("name", "");
      seed.intent    = item.value("intent", "");
      seed.intensity = item.value("intensity", 5);
      seed.use_count = item.value("use_count", 0);
      seed.fission_count = item.value("fission_count", 0);
      seed.stage     = static_cast<SeedStage>(item.value("stage", 0));
      seed.created_at= item.value("created_at", 0.0);
      seed.last_used = item.value("last_used", 0.0);
      seeds_.push_back(seed);
    }
  }
}

void SeedCultivator::save_() noexcept {
  try {
    fs::create_directories(data_dir_);
    nlohmann::json data = nlohmann::json::array();
    for (const auto& seed : seeds_) data.push_back(seed.to_json());

    std::ofstream ofs(data_dir_ + "/seed_store.json");
    if (ofs.is_open()) ofs << data.dump(2);
  } catch (...) {}
}

auto SeedCultivator::find_seed_(const std::string& id) -> CultivableSeed* {
  for (auto& seed : seeds_) {
    if (seed.id == id) return &seed;
  }
  return nullptr;
}

}  // namespace nexus::bridge
