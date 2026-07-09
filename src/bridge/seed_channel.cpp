/**
 * @file seed_channel.cpp
 * @brief 种子通道实现 — 种子注入与持久化
 */

#include "nexus/bridge/seed_channel.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace nexus::bridge {

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════
// Seed
// ═══════════════════════════════════════════════════════════════════

auto Seed::to_json() const -> nlohmann::json {
  const char* target_str = "model";
  switch (target) {
    case SeedTarget::kModel: target_str = "model"; break;
    case SeedTarget::kCore:  target_str = "core";  break;
    case SeedTarget::kSelf:  target_str = "self";  break;
  }

  auto j = nlohmann::json::object();
  j["name"]      = name;
  j["intent"]    = intent;
  j["intensity"] = intensity;
  j["domain"]    = domain;
  j["target"]    = target_str;
  j["created_at"]= created_at;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// SeedChannel
// ═══════════════════════════════════════════════════════════════════

SeedChannel::SeedChannel(std::string data_dir) noexcept
  : data_dir_(std::move(data_dir))
  , seed_path_(data_dir_ + "/seed_declarations.jsonl") {
  fs::create_directories(data_dir_);
}

auto SeedChannel::plant(const Seed& seed) noexcept -> Status {
  if (seed.name.empty()) {
    return Status::Error(ErrorCode::kInvalidConfig, "seed name required");
  }
  return persist_(seed) ? Status::Ok()
    : Status::Error(ErrorCode::kIoError, "seed persist failed");
}

auto SeedChannel::plant_defaults() noexcept -> Status {
  const std::vector<Seed> default_seeds = {
    {"持续认知驾驶",
     "模型推理过程中持续注入种子，引导方向，维持深度思考",
     9, "meta_cognition", SeedTarget::kModel, 0},
    {"收敛性思维锚点",
     "在推理偏离时提供收敛方向，防止发散",
     7, "meta_cognition", SeedTarget::kModel, 0},
    {"涌现触发",
     "定期触发心灵核心的涌现评估",
     6, "emergence", SeedTarget::kCore, 0},
    {"自我修正引导",
     "当检测到错误时引导自我修正方向",
     8, "self_correction", SeedTarget::kSelf, 0},
  };

  for (const auto& seed : default_seeds) {
    auto s = plant(seed);
    if (!s.ok()) return s;
  }

  return Status::Ok();
}

auto SeedChannel::count_by_target() const noexcept
    -> std::vector<std::pair<std::string, int>> {
  int model_count = 0, core_count = 0, self_count = 0;

  if (fs::exists(seed_path_)) {
    std::ifstream ifs(seed_path_);
    std::string line;
    while (std::getline(ifs, line)) {
      if (line.empty()) continue;
      try {
        auto j = nlohmann::json::parse(line);
        auto target = j.value("target", std::string("model"));
        if (target == "model") ++model_count;
        else if (target == "core") ++core_count;
        else if (target == "self") ++self_count;
      } catch (...) {}
    }
  }

  return {{"model", model_count}, {"core", core_count}, {"self", self_count}};
}

auto SeedChannel::total_seeds() const noexcept -> int {
  int count = 0;
  if (fs::exists(seed_path_)) {
    std::ifstream ifs(seed_path_);
    std::string line;
    while (std::getline(ifs, line)) {
      if (!line.empty()) ++count;
    }
  }
  return count;
}

auto SeedChannel::seed_path() const noexcept -> const std::string& {
  return seed_path_;
}

auto SeedChannel::persist_(const Seed& seed) noexcept -> bool {
  try {
    auto entry = seed.to_json();
    entry["created_at"] = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

    std::ofstream ofs(seed_path_, std::ios::app);
    if (!ofs.is_open()) return false;
    ofs << entry.dump() << "\n";
    return true;
  } catch (...) {
    return false;
  }
}

}  // namespace nexus::bridge
