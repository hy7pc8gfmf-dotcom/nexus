// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_TAGS_H
#define NEXUS_PSYCHE_SEMANTIC_TAGS_H

/**
 * @file semantic_tags.h
 * @brief 标签锚点系统 — 标签作为语义空间独立热点
 *
 * 移植自 D:/synapse/semantic_tags.py (358行)
 *
 * 标签不是概念属性，而是语义空间中的独立热点（锚点）。
 * 每个标签有自己的 14D 坐标，位于被标注概念的语义质心附近。
 */

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 标签分类表
// ═══════════════════════════════════════════════════════════════════

inline constexpr const char* kTagList[] = {
  // 范畴 (20)
  "cat_color", "cat_nature", "cat_animal", "cat_plant",
  "cat_body", "cat_food", "cat_tool", "cat_abstract",
  "cat_person", "cat_place", "cat_time", "cat_number",
  "cat_direction", "cat_material", "cat_building",
  "cat_art", "cat_music", "cat_science", "cat_religion",
  "cat_philosophy",
  // 领域 (8)
  "dom_politics", "dom_law", "dom_economy", "dom_war",
  "dom_education", "dom_medicine", "dom_agriculture",
  "dom_history",
  // 属性 (24)
  "att_hot", "att_cold", "att_wet", "att_dry",
  "att_big", "att_small", "att_high", "att_low",
  "att_long", "att_short", "att_deep", "att_shallow",
  "att_light", "att_dark",
  "att_red", "att_blue", "att_green", "att_white",
  "att_black", "att_yellow",
  "att_round", "att_sharp", "att_hard", "att_soft",
  // 关系 (12)
  "rel_family", "rel_friend", "rel_enemy",
  "rel_teacher", "rel_ruler", "rel_subject",
  "rel_parent", "rel_child", "rel_sibling", "rel_spouse",
  "rel_self", "rel_power",
  // 情绪 (8)
  "emo_joy", "emo_anger", "emo_sorrow", "emo_fear",
  "emo_love", "emo_hate", "emo_desire", "emo_shame",
  // 动作 (22)
  "act_move", "act_run", "act_jump", "act_fly", "act_swim",
  "act_sit", "act_stand", "act_lie",
  "act_hold", "act_push", "act_pull", "act_lift",
  "act_open", "act_close",
  "act_speak", "act_listen", "act_see", "act_eat", "act_drink",
  "act_breathe",
  "act_write", "act_read", "act_think",
  "act_create", "act_destroy",
  "act_give", "act_take", "act_transform", "act_communicate",
};

inline constexpr int kTagCount =
    sizeof(kTagList) / sizeof(kTagList[0]);

// ═══════════════════════════════════════════════════════════════════
// 概念 → 标签映射表
// ═══════════════════════════════════════════════════════════════════

struct ConceptTagEntry {
  const char* cname;  // 概念名
  const char* tags;   // 逗号分隔的标签名
};

inline constexpr ConceptTagEntry kConceptTags[] = {
  {"天", "cat_nature,cat_place,dom_religion,att_high"},
  {"地", "cat_nature,cat_place,att_low"},
  {"山", "cat_nature,cat_place,att_high"},
  {"水", "cat_nature,cat_food,att_wet,act_swim"},
  {"火", "cat_nature,att_hot,att_red,act_destroy"},
  {"风", "cat_nature,act_move"},
  {"云", "cat_nature,att_white,att_light"},
  {"雨", "cat_nature,att_wet,att_cold"},
  {"雪", "cat_nature,att_cold,att_white"},
  {"雷", "cat_nature,att_hot"},
  {"日", "cat_nature,cat_time,att_hot,att_light"},
  {"月", "cat_nature,cat_time,att_cold,att_light"},
  {"星", "cat_nature,att_light,att_high"},
  {"海", "cat_nature,cat_place,att_wet,att_deep"},
  {"江", "cat_nature,att_wet,att_long"},
  {"河", "cat_nature,att_wet"},
  {"木", "cat_nature,cat_plant,cat_material"},
  {"花", "cat_nature,cat_plant,emo_joy,cat_art"},
  {"草", "cat_nature,cat_plant"},
  {"石", "cat_nature,cat_material,att_hard"},
  {"金", "cat_nature,cat_material,att_hard,dom_economy"},
  {"冰", "cat_nature,att_cold,att_hard"},
  {"春", "cat_time,cat_plant"},
  {"夏", "cat_time,att_hot"},
  {"秋", "cat_time,att_cold,emo_sorrow"},
  {"冬", "cat_time,att_cold"},
  {"东", "cat_place,cat_direction"},
  {"西", "cat_place,cat_direction"},
  {"南", "cat_place,cat_direction"},
  {"北", "cat_place,cat_direction"},
  {"前", "cat_direction,cat_time"},
  {"后", "cat_direction,cat_time"},
  {"上", "cat_direction,att_high"},
  {"下", "cat_direction,att_low"},
  {"内", "cat_place,rel_self"},
  {"外", "cat_place"},
  {"人", "cat_person,cat_animal,rel_self"},
  {"民", "cat_person,dom_politics"},
  {"君", "cat_person,rel_ruler,rel_teacher"},
  {"臣", "cat_person,rel_subject"},
  {"父", "cat_person,rel_parent,rel_family"},
  {"母", "cat_person,rel_parent,rel_family"},
  {"子", "cat_person,rel_child,rel_family"},
  {"兄", "cat_person,rel_sibling,rel_family"},
  {"弟", "cat_person,rel_sibling,rel_family"},
  {"妹", "cat_person,rel_family"},
  {"朋", "cat_person,rel_friend"},
  {"友", "cat_person,rel_friend"},
  {"师", "cat_person,rel_teacher,dom_education"},
  {"王", "cat_person,rel_ruler,dom_politics"},
  {"兵", "cat_person,dom_war"},
  {"农", "cat_person,dom_agriculture"},
  {"士", "cat_person,dom_education"},
  {"医", "cat_person,dom_medicine"},
  {"道", "cat_philosophy,cat_abstract,act_move"},
  {"德", "cat_philosophy,cat_abstract"},
  {"仁", "cat_philosophy,emo_love,cat_abstract"},
  {"义", "cat_philosophy,cat_abstract"},
  {"礼", "cat_philosophy,cat_art"},
  {"智", "cat_philosophy,cat_abstract,act_think"},
  {"信", "cat_philosophy,rel_friend"},
  {"忠", "cat_philosophy,rel_ruler,rel_family"},
  {"耻", "cat_philosophy,emo_shame"},
  {"勇", "cat_philosophy,act_move"},
  {"理", "cat_philosophy,cat_science,act_think"},
  {"性", "cat_philosophy,cat_abstract"},
  {"命", "cat_philosophy,cat_time,cat_abstract"},
  {"真", "cat_philosophy,cat_abstract,cat_science"},
  {"善", "cat_philosophy,emo_love,cat_abstract"},
  {"美", "cat_philosophy,cat_art,emo_joy,cat_abstract"},
  {"无", "cat_philosophy,cat_abstract"},
  {"自", "cat_philosophy,rel_self,cat_abstract"},
  {"爱", "emo_love,rel_family,rel_friend,cat_abstract"},
  {"恨", "emo_hate,rel_enemy"},
  {"喜", "emo_joy"},
  {"怒", "emo_anger"},
  {"哀", "emo_sorrow"},
  {"惧", "emo_fear"},
  {"欲", "emo_desire"},
  {"情", "emo_love,cat_abstract"},
  {"仇", "emo_hate,rel_enemy"},
  {"思", "act_think,emo_love,cat_abstract"},
  {"想", "act_think,cat_abstract"},
  {"意", "act_think,cat_abstract"},
  {"念", "act_think,emo_love,cat_abstract"},
  {"志", "act_think,cat_philosophy,cat_abstract"},
  {"望", "act_think,emo_desire"},
  {"政", "dom_politics,cat_abstract"},
  {"法", "dom_law,dom_politics"},
  {"国", "cat_place,dom_politics,rel_family"},
  {"家", "cat_place,rel_family,cat_building"},
  {"世", "cat_place,cat_time,cat_abstract"},
  {"界", "cat_place,cat_abstract"},
  {"战", "dom_war,act_destroy"},
  {"争", "dom_war,act_take"},
  {"和", "dom_politics,emo_joy,rel_friend"},
  {"名", "cat_abstract,rel_self"},
  {"权", "dom_politics,rel_power,cat_abstract"},
  {"势", "dom_politics,rel_power,cat_abstract"},
  {"财", "dom_economy,emo_desire"},
  {"福", "emo_joy,dom_religion"},
  {"祸", "emo_fear,emo_sorrow"},
  {"吉", "emo_joy,dom_religion"},
  {"凶", "emo_fear,dom_religion"},
  {"神", "cat_religion,cat_abstract,att_high"},
  {"佛", "cat_religion,cat_philosophy"},
  {"仙", "cat_religion,act_fly"},
  {"鬼", "cat_religion,emo_fear"},
  {"圣", "cat_religion,cat_philosophy,att_high"},
  {"龙", "cat_religion,cat_animal,act_fly"},
  {"红", "cat_color,att_red,emo_love,emo_anger,dom_politics"},
  {"黄", "cat_color,att_yellow,dom_politics"},
  {"蓝", "cat_color,att_blue,emo_sorrow"},
  {"绿", "cat_color,att_green,cat_nature"},
  {"白", "cat_color,att_white,emo_fear"},
  {"黑", "cat_color,att_black,emo_fear,emo_hate"},
  {"紫", "cat_color,cat_religion"},
  {"身", "cat_body,rel_self"},
  {"心", "cat_body,emo_love,act_think,cat_abstract"},
  {"头", "cat_body,act_think"},
  {"目", "cat_body,act_see"},
  {"耳", "cat_body,act_listen"},
  {"口", "cat_body,act_speak,act_eat,act_drink"},
  {"鼻", "cat_body,act_breathe"},
  {"舌", "cat_body,act_eat"},
  {"手", "cat_body,act_hold,act_write"},
  {"足", "cat_body,act_move,act_run"},
  {"骨", "cat_body,att_hard"},
  {"血", "cat_body,rel_family"},
  {"肉", "cat_body,cat_food,att_soft"},
  {"皮", "cat_body,att_soft"},
  {"面", "cat_body,rel_self"},
  {"走", "act_move,act_run"},
  {"行", "act_move,act_communicate"},
  {"跑", "act_run,act_move"},
  {"跳", "act_jump,act_move"},
  {"飞", "act_fly,act_move"},
  {"游", "act_swim,act_move"},
  {"坐", "act_sit"},
  {"立", "act_stand"},
  {"卧", "act_lie"},
  {"来", "act_move,cat_time"},
  {"去", "act_move,cat_time"},
  {"开", "act_open,act_create"},
  {"说", "act_speak,act_communicate"},
  {"读", "act_read,act_think"},
  {"写", "act_write,act_create"},
  {"知", "act_think,cat_science"},
  {"学", "act_read,act_think,dom_education"},
  {"变", "act_transform,cat_abstract"},
  {"化", "act_transform,cat_abstract"},
  {"牛", "cat_animal,dom_agriculture"},
  {"羊", "cat_animal,dom_agriculture"},
  {"马", "cat_animal,act_move"},
  {"虎", "cat_animal,emo_fear"},
  {"狼", "cat_animal,emo_hate,emo_fear"},
  {"犬", "cat_animal,rel_family"},
  {"鱼", "cat_animal,cat_food,act_swim"},
  {"鸟", "cat_animal,act_fly"},
  {"松", "cat_plant,att_long"},
  {"竹", "cat_plant,att_hard,cat_tool"},
  {"梅", "cat_plant,emo_joy,cat_art"},
  {"兰", "cat_plant,cat_art,emo_joy"},
  {"菊", "cat_plant,cat_art,emo_sorrow"},
  {"荷", "cat_plant,cat_art,att_wet"},
  {"桃", "cat_plant,cat_food"},
  {"柳", "cat_plant,att_soft,emo_sorrow"},
  {"书", "cat_tool,dom_education,act_read"},
  {"诗", "cat_art,cat_abstract,emo_joy"},
  {"文", "cat_art,dom_education,act_write"},
  {"乐", "cat_art,cat_music,emo_joy"},
  {"易", "cat_philosophy,cat_abstract,act_transform"},
  {"阴", "cat_philosophy,att_dark,att_cold"},
  {"阳", "cat_philosophy,att_light,att_hot"},
};

inline constexpr int kConceptTagCount =
    sizeof(kConceptTags) / sizeof(kConceptTags[0]);

// ═══════════════════════════════════════════════════════════════════
// JSON 导出
// ═══════════════════════════════════════════════════════════════════

/// 标签列表 → JSON 数组
auto tag_list_to_json() -> nlohmann::json;

/// 概念→标签映射 → JSON 数组: [{concept, tags: [tag1, ...]}, ...]
auto concept_tags_to_json() -> nlohmann::json;

/// 完整标签表 → JSON 对象
auto tags_tables_to_json() -> nlohmann::json;

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_TAGS_H
