/**
 * @file chinese_constraint.cpp
 * @brief 中文硬约束实现
 */

#include "nexus/env/chinese_constraint.h"

#include <algorithm>

namespace nexus::env {

auto ChineseConstraint::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["id"] = id; j["description"] = description; j["enforced"] = enforced;
  return j;
}

ChineseConstraintLoader::ChineseConstraintLoader() noexcept {
  init_defaults_();
}

auto ChineseConstraintLoader::load() noexcept -> std::vector<ChineseConstraint> {
  return constraints_;
}

auto ChineseConstraintLoader::inject() noexcept -> Status {
  // Shell B 注入标记
  return Status::Ok();
}

auto ChineseConstraintLoader::constraints() const noexcept
    -> const std::vector<ChineseConstraint>& {
  return constraints_;
}

void ChineseConstraintLoader::init_defaults_() noexcept {
  constraints_ = {
    {"CC-01", "所有推理过程必须使用中文, 零容忍英文推理", true},
    {"CC-02", "技术术语首次出现时需标注中文译名", true},
    {"CC-03", "代码注释和文档使用中文", true},
    {"CC-04", "跨语言概念需提供中文等价表述", false},
    {"CC-05", "Shell B 模型响应语言锁定为中文", true},
  };
}

}  // namespace nexus::env
