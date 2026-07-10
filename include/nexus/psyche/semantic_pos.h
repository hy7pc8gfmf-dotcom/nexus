// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_PSYCHE_SEMANTIC_POS_H
#define NEXUS_PSYCHE_SEMANTIC_POS_H

/**
 * @file semantic_pos.h
 * @brief 词性参数 — 数词/量词注册到语义场
 *
 * 移植自 D:/synapse/semantic_pos.py (258行)
 *
 * 给数词、量词添加词性参数并注册到语义场。
 * 每个概念附带 meta 参数（词性、数值、类别、角标等）。
 */

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/psyche/semantic_port.h"
#include "nexus/types/status.h"

namespace nexus::psyche {

/// 数词定义
struct NumeralDef {
  const char* name;
  double      value;
  const char* usage;       // "基数"|"序数"|"分数"|"疑问"|"约数"
  const char* superscript; // 角标
  const char* desc;        // 语义描述
};

/// 量词定义
struct MeasureDef {
  const char* name;
  const char* category;    // 类别
  const char* applicable;  // 适用于哪些概念
  const char* superscript;
};

// ═══════════════════════════════════════════════════════════════════
// 内置数词表 (23个)
// ═══════════════════════════════════════════════════════════════════

inline constexpr NumeralDef kNumerals[] = {
  {"零", 0,    "基数", "\u2070", "数量为零,空"},
  {"一", 1,    "基数", "\u00B9", "最小正整数,起始"},
  {"二", 2,    "基数", "\u00B2", "次正整数,对偶"},
  {"三", 3,    "基数", "\u00B3", "多数,完整"},
  {"四", 4,    "基数", "\u2074", "四方,稳定"},
  {"五", 5,    "基数", "\u2075", "中数,交叉"},
  {"六", 6,    "基数", "\u2076", "顺,流畅"},
  {"七", 7,    "基数", "\u2077", "变化,转折"},
  {"八", 8,    "基数", "\u2078", "发,繁荣"},
  {"九", 9,    "基数", "\u2079", "极数,最大"},
  {"十", 10,   "基数", "\u00B9\u2070", "完整,齐全"},
  {"百", 100,  "基数", "\u00B9\u2070\u2070", "百位"},
  {"千", 1000, "基数", "\u00B9\u2070\u2070\u2070", "千位"},
  {"万", 10000,"基数", "\u00B9\u2070\u2074", "万位,极多"},
  {"亿", 100000000,"基数","\u00B9\u2070\u2078", "亿位,极大量"},
  {"半", 0.5,  "分数", "\u00BD", "一半,中间"},
  {"几", 0,    "疑问", "?", "不确定数量"},
  {"多", 0,    "约数", "+", "多于,超出"},
  {"少", 0,    "约数", "-", "少于,不足"},
};

inline constexpr int kNumeralCount =
    sizeof(kNumerals) / sizeof(kNumerals[0]);

// ═══════════════════════════════════════════════════════════════════
// 内置量词表 (24个)
// ═══════════════════════════════════════════════════════════════════

inline constexpr MeasureDef kMeasures[] = {
  {"个", "通用个体量词", "人物问题地方", "\u00B9"},
  {"只", "动物/器物量词", "鸟狗猫船", "\u00B2"},
  {"条", "长条物量词", "鱼蛇路河", "\u00B3"},
  {"张", "平面物量词", "纸桌子票床", "\u2074"},
  {"块", "块状物量词", "石头蛋糕钱", "\u2075"},
  {"片", "片状物量词", "叶子面包药", "\u2076"},
  {"本", "册籍量词", "书杂志本子", "\u2077"},
  {"件", "事务量词", "衣服事件礼物", "\u2078"},
  {"双", "成对量词", "鞋袜子筷子手", "\u1D49"},
  {"对", "配对量词", "夫妻耳环翅膀", "\u1D50"},
  {"把", "握持量词", "刀椅子钥匙", "\u1D47"},
  {"根", "细长物量词", "绳子棍子头发", "\u02B3"},
  {"棵", "植物量词", "树草菜", "\u1D4F"},
  {"粒", "小颗粒量词", "米沙子种子药", "\u02E1"},
  {"滴", "液体量词", "水油血雨", "\u1D48"},
  {"颗", "小圆物量词", "星星珍珠心牙", "\u1D4D"},
  {"匹", "布/马量词", "布马", "\u1D50"},
  {"头", "大型动量词", "牛猪大象蒜", "\u1D57"},
  {"尾", "鱼量词", "鱼", "\u02B7"},
  {"斤", "重量单位", "重量", "\u02C8"},
  {"元", "货币单位", "金钱价格", "\u00A5"},
  {"角", "货币单位", "金钱", "\u1D5C"},
  {"分", "时间/货币单位", "时间金钱", "\u1D60"},
};

inline constexpr int kMeasureCount =
    sizeof(kMeasures) / sizeof(kMeasures[0]);

// ═══════════════════════════════════════════════════════════════════
// 安装函数
// ═══════════════════════════════════════════════════════════════════

/// 向语义场注册所有数词和量词的词性参数
/// 返回注册的概念数量
auto install_parts_of_speech(SemanticPort* port) noexcept -> int;

/// 数词表 → JSON 数组
auto numerals_to_json() -> nlohmann::json;

/// 量词表 → JSON 数组
auto measures_to_json() -> nlohmann::json;

/// 完整 POS 表 → JSON 对象: {numerals: [...], measures: [...], n_numerals: N, n_measures: M}
auto pos_tables_to_json() -> nlohmann::json;

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_SEMANTIC_POS_H
