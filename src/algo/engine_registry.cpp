/**
 * @file engine_registry.cpp
 * @brief 算法引擎注册表实现
 */

#include "nexus/algo/engine.h"

#include <algorithm>
#include <mutex>

namespace nexus::algo {

auto EngineRegistry::register_engine(
    std::unique_ptr<AlgorithmEngine> engine) noexcept -> Status {
  if (!engine) {
    return Status::Error(ErrorCode::kInvalidConfig,
      "null engine pointer");
  }

  auto id = engine->info().id;
  if (id.empty()) {
    return Status::Error(ErrorCode::kInvalidConfig,
      "engine id cannot be empty");
  }

  // 检查是否已注册同名引擎
  for (const auto& e : engines_) {
    if (e->info().id == id) {
      return Status::Error(ErrorCode::kInvalidConfig,
        "engine already registered: " + id);
    }
  }

  engines_.push_back(std::move(engine));
  return Status::Ok();
}

auto EngineRegistry::get(const std::string& id) noexcept -> AlgorithmEngine* {
  for (auto& e : engines_) {
    if (e->info().id == id) return e.get();
  }
  return nullptr;
}

auto EngineRegistry::get(const std::string& id) const noexcept
    -> const AlgorithmEngine* {
  for (const auto& e : engines_) {
    if (e->info().id == id) return e.get();
  }
  return nullptr;
}

auto EngineRegistry::list_ids() const noexcept -> std::vector<std::string> {
  std::vector<std::string> ids;
  ids.reserve(engines_.size());
  for (const auto& e : engines_) {
    ids.push_back(e->info().id);
  }
  return ids;
}

auto EngineRegistry::list_details() const noexcept -> std::vector<EngineInfo> {
  std::vector<EngineInfo> infos;
  infos.reserve(engines_.size());
  for (const auto& e : engines_) {
    infos.push_back(e->info());
  }
  return infos;
}

auto EngineRegistry::count() const noexcept -> size_t {
  return engines_.size();
}

auto EngineRegistry::initialize_all(const nlohmann::json& config) noexcept
    -> Status {
  for (auto& e : engines_) {
    auto s = e->initialize(config);
    if (!s.ok()) {
      return Status::Error(s.code,
        "failed to initialize engine '" + e->info().id + "': " + s.message);
    }
  }
  return Status::Ok();
}

auto EngineRegistry::instance() noexcept -> EngineRegistry& {
  static EngineRegistry reg;
  return reg;
}

}  // namespace nexus::algo
