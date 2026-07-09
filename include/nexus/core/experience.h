#ifndef NEXUS_CORE_EXPERIENCE_H
#define NEXUS_CORE_EXPERIENCE_H

/**
 * @file experience.h
 * @brief 经验引擎 — 推理经验卡缓存与索引
 *
 * 每次推理后生成经验卡, 包含输入/输出/耗时/评分。
 * 后续推理可参考相似经验卡。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::core {

struct ExperienceCard {
  std::string id;
  std::string prompt_hash;
  std::string prompt_preview;
  std::string output_preview;
  double      duration_ms = 0;
  double      quality_score = 0.5;
  double      created_at = 0.0;

  auto to_json() const -> nlohmann::json;
};

class ExperienceEngine {
public:
  ExperienceEngine() noexcept = default;

  auto store(const std::string& prompt, const std::string& output,
             double duration_ms, double quality = 0.5) noexcept -> void;
  [[nodiscard]] auto lookup(const std::string& prompt, int limit = 3) const noexcept
      -> std::vector<ExperienceCard>;
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;
  [[nodiscard]] auto count() const noexcept -> int;

private:
  std::vector<ExperienceCard> cards_;
  static constexpr int kMaxCards = 100;
  static auto compute_hash_(const std::string& s) -> std::string;
};

}  // namespace nexus::core

#endif
