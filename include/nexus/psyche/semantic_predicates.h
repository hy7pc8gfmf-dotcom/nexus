// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_PREDICATES_H
#define NEXUS_PSYCHE_SEMANTIC_PREDICATES_H

/**
 * @file semantic_predicates.h
 * @brief 谓词方向向量 — 从种子空间+标签系统推导
 *
 * 移植自 D:/synapse/semantic_predicates.py (264行)
 *
 * 谓词的本质是"语义变换"而非"语义点"。
 * 方向向量表示"从哪个标签域指向哪个标签域"的变换。
 */

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 谓词方向定义: {概念名, 源标签, 目标标签}
// ═══════════════════════════════════════════════════════════════════

struct PredicateDirEntry {
  const char* name;
  const char* src_tag;
  const char* dst_tag;
};

inline constexpr PredicateDirEntry kPredicateDirections[] = {
  // 位移动作
  {"走", "rel_self", "act_move"},
  {"行", "rel_self", "act_move"},
  {"跑", "act_move", "act_run"},
  {"跳", "act_move", "act_jump"},
  {"飞", "act_move", "act_fly"},
  {"游", "act_move", "act_swim"},
  {"来", "cat_place", "act_move"},
  {"去", "rel_self", "act_move"},
  {"出", "rel_self", "act_move"},
  {"入", "cat_place", "act_move"},
  {"上", "cat_place", "att_high"},
  {"下", "cat_place", "att_low"},
  {"进", "cat_direction", "act_move"},
  {"退", "act_move", "cat_direction"},
  // 身体动作
  {"坐", "act_move", "act_sit"},
  {"立", "act_move", "act_stand"},
  {"卧", "act_move", "act_lie"},
  {"看", "cat_body", "act_see"},
  {"听", "cat_body", "act_listen"},
  {"说", "act_think", "act_speak"},
  {"读", "act_see", "act_read"},
  {"写", "act_think", "act_write"},
  {"吃", "act_open", "act_eat"},
  {"喝", "act_open", "act_drink"},
  {"呼", "cat_body", "act_breathe"},
  // 变换
  {"变", "cat_nature", "act_transform"},
  {"化", "cat_nature", "act_transform"},
  {"生", "cat_time", "act_transform"},
  {"死", "cat_time", "act_destroy"},
  {"存", "cat_time", "cat_abstract"},
  {"开", "att_hard", "act_open"},
  {"关", "act_open", "act_close"},
  {"长", "cat_time", "att_long"},
  {"灭", "cat_time", "act_destroy"},
  // 情绪动作
  {"爱", "rel_self", "emo_love"},
  {"恨", "rel_self", "emo_hate"},
  {"思", "act_think", "emo_love"},
  {"想", "act_think", "cat_abstract"},
  {"念", "emo_love", "act_think"},
  {"望", "act_see", "emo_desire"},
  // 其他
  {"知", "act_think", "cat_science"},
  {"学", "act_read", "dom_education"},
  {"为", "rel_self", "act_create"},
  {"用", "cat_tool", "act_hold"},
  {"有", "cat_abstract", "act_hold"},
  {"在", "cat_place", "cat_time"},
  {"能", "cat_abstract", "act_create"},
  {"通", "cat_place", "act_communicate"},
};

inline constexpr int kPredicateCount =
    sizeof(kPredicateDirections) / sizeof(kPredicateDirections[0]);

// ═══════════════════════════════════════════════════════════════════
// JSON 导出
// ═══════════════════════════════════════════════════════════════════

/// 谓词方向表 → JSON 数组: [{name, src_tag, dst_tag}, ...]
auto predicate_directions_to_json() -> nlohmann::json;

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_PREDICATES_H
