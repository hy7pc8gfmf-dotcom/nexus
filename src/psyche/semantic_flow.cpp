// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/psyche/semantic_flow.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>
#include <sstream>
#include <unordered_set>

namespace nexus::psyche {

// ═══════════════════════════════════════════════════════════════════
// 句子模板 (50+, 文件作用域)
// ═══════════════════════════════════════════════════════════════════

struct SentenceTemplate {
  const char* pattern[12];  // 最大12个槽位, null终止
  const char* theme;
  const char* desc;
};

#define SPT(...) {__VA_ARGS__, nullptr}
#define TD(theme, desc) , theme, desc

static const SentenceTemplate kTemplates[] = {
  // 基本主谓宾 (5)
  {SPT("sub","verb","asp","obj") TD("基本主谓宾","人喝了水")},
  {SPT("mod","的","sub","verb","asp","obj") TD("基本主谓宾","红的花看着水")},
  {SPT("sub","verb","mod","的","obj") TD("基本主谓宾","人看美的河")},
  {SPT("sub","adv","verb","asp","obj") TD("基本主谓宾","人从看了河")},
  {SPT("mod","的","sub","adv","verb","asp","obj") TD("基本主谓宾","好的人从看了水")},
  // 把字句 (4)
  {SPT("sub","把","obj","verb","asp") TD("把字句","人把水喝了")},
  {SPT("mod","的","sub","把","obj","verb","asp") TD("把字句","红的人把水喝了")},
  {SPT("sub","把","obj","verb","给","sub2") TD("把字句","人把水给人")},
  {SPT("sub","把","obj","verb","了","和","verb","asp") TD("把字句","人把水喝了和倒了")},
  // 被字句 (3)
  {SPT("obj","被","sub","verb","asp") TD("被字句","水被人喝了")},
  {SPT("obj","给","sub","verb","asp") TD("被字句","水给人喝了")},
  {SPT("mod","的","obj","被","sub","verb","asp") TD("被字句","好的水被人喝了")},
  // 存现句 (6)
  {SPT("adv","有","obj") TD("存现句","前有水")},
  {SPT("place","有","sub") TD("存现句","山有人")},
  {SPT("adv","在","place","有","sub") TD("存现句","前在山有人")},
  {SPT("mod","的","place","有","mod","的","sub") TD("存现句","高的山有善的人")},
  {SPT("sub","无","obj") TD("存现句","人无水")},
  {SPT("adv","无","obj") TD("存现句","前无水")},
  // 是字句 (5)
  {SPT("sub","是","obj") TD("是字句","人是物")},
  {SPT("mod","的","sub","是","mod","的","obj") TD("是字句","仁的人是善的物")},
  {SPT("sub","非","obj") TD("是字句","人非物")},
  {SPT("sub","乃","obj") TD("是字句","人乃物")},
  {SPT("sub","即","obj") TD("是字句","人即物")},
  // 连动句 (5)
  {SPT("sub","verb","obj","verb","obj") TD("连动句","人取水看花")},
  {SPT("sub","verb","着","verb","asp","obj") TD("连动句","人站着看了水")},
  {SPT("sub","verb","asp","obj","verb","obj") TD("连动句","人喝了水看花")},
  {SPT("sub","verb","asp","obj","verb","了","obj") TD("连动句","人喝了水看了花")},
  {SPT("sub","verb","obj","又","verb","obj") TD("连动句","人喝水又看花")},
  // 兼语句 (4)
  {SPT("sub","verb","obj","verb") TD("兼语句","人命人行")},
  {SPT("sub","谓","obj","verb","asp","obj") TD("兼语句","人命人看了花")},
  {SPT("sub","使","obj","verb","asp","obj") TD("兼语句","人使人看了花")},
  {SPT("sub","令","obj","mod") TD("兼语句","人令人喜")},
  // 比较句 (5)
  {SPT("sub","比","obj","mod") TD("比较句","人比山高")},
  {SPT("mod","的","sub","比","obj","mod") TD("比较句","善的人比山仁")},
  {SPT("sub","如","obj","mod") TD("比较句","人如山高")},
  {SPT("sub","似","obj") TD("比较句","人似山")},
  {SPT("sub","不如","obj") TD("比较句","人不如山")},
  // 介词句 (6)
  {SPT("sub","在","place","verb","asp","obj") TD("介词句","人在山看了水")},
  {SPT("sub","从","adv","verb","asp","obj") TD("介词句","人从前看了水")},
  {SPT("sub","向","place","verb","asp","obj") TD("介词句","人向山看了水")},
  {SPT("sub","用","obj","verb","asp","obj2") TD("介词句","人用刀看了物")},
  {SPT("sub","为","obj","verb","asp") TD("介词句","人为水看了")},
  {SPT("sub","对","obj2","verb","asp") TD("介词句","人对物看了")},
  // 并列句 (4)
  {SPT("sub","verb","asp","和","sub2","verb","asp") TD("并列句","人走了和鸟飞了")},
  {SPT("sub","verb","obj","和","verb","obj") TD("并列句","人喝水且看花")},
  {SPT("sub","verb","但","adv","verb") TD("并列句","人走但前行")},
  {SPT("sub","verb","obj","而","verb","obj") TD("并列句","人喝水而看花")},
  // 转折/条件句 (4)
  {SPT("sub","虽","verb","但","verb","asp") TD("转折句","人虽行但看了")},
  {SPT("若","sub","verb","则","sub2","verb") TD("转折句","若人行则鸟飞")},
  {SPT("因","obj","sub","verb","asp") TD("转折句","因水人看了")},
  {SPT("sub","不","verb","而","verb","asp","obj") TD("转折句","人不走而看了水")},
  // 感叹/反问句 (3)
  {SPT("mod","的","sub","啊") TD("感叹句","善的人啊")},
  {SPT("sub","岂","verb","asp","obj") TD("感叹句","人岂喝了水")},
  {SPT("sub","何","verb","asp","obj") TD("感叹句","人何喝了水")},
  // 比况句 (3)
  {SPT("sub","如","obj") TD("比况句","人如山")},
  {SPT("sub","似","obj","mod") TD("比况句","人似山高")},
  {SPT("sub","若","obj") TD("比况句","人若山")},
  // 双宾语句 (2)
  {SPT("sub","给","sub2","obj") TD("双宾语句","人给人水")},
  {SPT("sub","予","sub2","obj") TD("双宾语句","人予人水")},
  // 工具句 (2)
  {SPT("sub","以","obj","verb","asp","obj2") TD("工具句","人以刀杀了物")},
  {SPT("sub","持","obj","verb","asp") TD("工具句","人持刀看了")},
};

static constexpr int kTemplateCount =
    sizeof(kTemplates) / sizeof(kTemplates[0]);

// ═══════════════════════════════════════════════════════════════════
// 模板主题
// ═══════════════════════════════════════════════════════════════════

struct ThemeEntry {
  const char* name;
  const char* clusters[3];
};

static const ThemeEntry kThemeList[] = {
  {"基本主谓宾", {"action", "nature", "person"}},
  {"把字句",     {"action", "war"}},
  {"被字句",     {"emotion", "abstract"}},
  {"存现句",     {"nature", "place", "time_space"}},
  {"是字句",     {"philosophy", "abstract"}},
  {"连动句",     {"action"}},
  {"兼语句",     {"politics", "person"}},
  {"比较句",     {"emotion", "philosophy"}},
  {"介词句",     {"action", "place"}},
  {"并列句",     {"person", "animal"}},
  {"转折句",     {"emotion", "abstract"}},
  {"感叹句",     {"emotion"}},
  {"比况句",     {"philosophy", "art"}},
  {"双宾语句",   {"person", "family"}},
  {"工具句",     {"action", "material"}},
};

static constexpr int kThemeCount =
    sizeof(kThemeList) / sizeof(kThemeList[0]);

// ═══════════════════════════════════════════════════════════════════
// 短语模板
// ═══════════════════════════════════════════════════════════════════

struct PhraseGroup {
  const char* cluster;
  const char* phrases[9];
};

static const PhraseGroup kPhraseGroups[] = {
  {"nature",     {"天地","日月","山河","风雨","花草","树木","江海","冰雪","云雨"}},
  {"body",       {"手足","耳目","口鼻","身心","骨肉","面目","头面"}},
  {"person",     {"君臣","父子","兄弟","姐妹","师生","亲友","人民","父母","子女"}},
  {"philosophy", {"道德","仁义","真理","善恶","美丑","是非","阴阳","有无","始终"}},
  {"emotion",    {"喜怒","哀乐","爱恨","善恶","福祸","吉凶","悲喜","恩仇"}},
  {"action",     {"行走","奔跑","飞跃","学习","思考","读写","听说","来去","开合"}},
  {"politics",   {"君臣","权力","法令","国家","天下","江山","社稷"}},
  {"art",        {"诗书","礼乐","书画","诗文","琴棋"}},
  {"war",        {"战争","胜负","攻守","生死","存亡"}},
  {"time_space", {"古今","中外","前后","上下","内外","春秋","冬夏"}},
  {"color",      {"红白","黑白","紫红","青绿"}},
  {"abstract",   {"善恶","是非","真假","虚实","始终","因果"}},
  {"food",       {"饮食","鱼肉","酒肉","茶饭"}},
  {"family",     {"父母","子女","兄弟","姐妹","夫妻","家人"}},
};

static constexpr int kPhraseCount =
    sizeof(kPhraseGroups) / sizeof(kPhraseGroups[0]);

// ═══════════════════════════════════════════════════════════════════
// 角色→团映射
// ═══════════════════════════════════════════════════════════════════

struct RoleClusterDef {
  const char* role;
  const char* clusters[6];
  const char* pos;
  const char* desc;
};

static const RoleClusterDef kRoleClusters[] = {
  {"sub",   {"person","animal"},                           "noun", "有生主语"},
  {"verb",  {"action"},                                     "verb", "动词"},
  {"obj",   {"food","material","abstract","art","nature","body"}, "noun", "无生宾语"},
  {"mod",   {"emotion","color","philosophy"},               "adj",  "定语"},
  {"adv",   {"time_space"},                                 "adv",  "状语"},
  {"place", {"place","nature"},                             "noun", "处所"},
  {"sub2",  {"animal","person"},                            "noun", "第二主语"},
  {"verb2", {"action"},                                     "verb", "第二动词"},
  {"obj2",  {"food","art","abstract"},                      "noun", "第二宾语"},
};

static constexpr int kRoleCount =
    sizeof(kRoleClusters) / sizeof(kRoleClusters[0]);

// ═══════════════════════════════════════════════════════════════════
// 时态标记 / 连接词 / 团→词性
// ═══════════════════════════════════════════════════════════════════

static const char* kAspectMarkers[] = {"了", "着", "过"};
static constexpr int kAspectCount = 3;

static bool is_connector_token(const char* token) {
  static const char* connectors[] = {
    "的","之","以","于","在","把","被","是","有","比",
    "从","和","谓","使","令","用","给","予","向","为",
    "对","持","非","乃","即","无","又","但","而","虽",
    "若","因","不","岂","何","啊","似","如","不如",
  };
  for (auto c : connectors) {
    if (std::strcmp(token, c) == 0) return true;
  }
  return false;
}

// ═══════════════════════════════════════════════════════════════════
// 类方法实现
// ═══════════════════════════════════════════════════════════════════

auto SemanticFlow::is_role_slot_(const std::string& token) noexcept -> bool {
  for (int i = 0; i < kRoleCount; ++i) {
    if (token == kRoleClusters[i].role) return true;
  }
  return false;
}

auto SemanticFlow::is_connector_(const std::string& token) noexcept -> bool {
  return is_connector_token(token.c_str());
}

// ═══════════════════════════════════════════════════════════════════
// 构造
// ═══════════════════════════════════════════════════════════════════

SemanticFlow::SemanticFlow(
    const std::vector<SemanticClusterDef>& clusters,
    const std::unordered_map<std::string, double>& pipe_thickness,
    const nlohmann::json& emergence_profile) noexcept
  : clusters_(clusters),
    pipe_thickness_(pipe_thickness),
    emergence_(emergence_profile) {
  build_member_index_();
}

void SemanticFlow::build_member_index_() noexcept {
  for (const auto& cl : clusters_) {
    cluster_members_[cl.name] = cl.members;
  }
}

// ═══════════════════════════════════════════════════════════════════
// 涌现加权选模板
// ═══════════════════════════════════════════════════════════════════

auto SemanticFlow::pick_template_(std::mt19937_64& rng) const
    -> std::vector<const char*> {
  std::vector<double> weights(kTemplateCount, 0.3);

  for (int i = 0; i < kTemplateCount; ++i) {
    const auto& tmpl = kTemplates[i];
    double max_e = 0.3;

    if (tmpl.theme) {
      for (int t = 0; t < kThemeCount; ++t) {
        if (std::strcmp(tmpl.theme, kThemeList[t].name) == 0) {
          for (int c = 0; c < 3 && kThemeList[t].clusters[c] != nullptr; ++c) {
            double e = 0.3;
            if (emergence_.is_object()) {
              try {
                auto& val = emergence_.at(kThemeList[t].clusters[c]);
                if (val.is_number()) {
                  e = val.get<double>();
                }
              } catch (...) {}
            }
            max_e = std::max(max_e, e);
          }
          break;
        }
      }
    }

    std::uniform_real_distribution<double> jitter(-0.15, 0.15);
    weights[i] = std::max(0.05, max_e + jitter(rng));
  }

  double total = std::accumulate(weights.begin(), weights.end(), 0.0);
  std::uniform_real_distribution<double> dist(0.0, total);
  double r = dist(rng);
  double cum = 0;
  for (int i = 0; i < kTemplateCount; ++i) {
    cum += weights[i];
    if (r <= cum) {
      std::vector<const char*> result;
      for (int j = 0; j < 12 && kTemplates[i].pattern[j] != nullptr; ++j) {
        result.push_back(kTemplates[i].pattern[j]);
      }
      return result;
    }
  }
  return {"sub", "verb", "asp", "obj"};
}

// ═══════════════════════════════════════════════════════════════════
// 展开时态标记
// ═══════════════════════════════════════════════════════════════════

auto SemanticFlow::expand_aspect_(const std::vector<const char*>& pattern,
                                   std::mt19937_64& rng)
    -> std::vector<const char*> {
  std::vector<const char*> result;
  std::uniform_real_distribution<double> prob(0.0, 1.0);
  std::uniform_int_distribution<int> pick(0, kAspectCount - 1);

  for (size_t i = 0; i < pattern.size(); ++i) {
    if (std::strcmp(pattern[i], "asp") == 0) {
      result.push_back(kAspectMarkers[pick(rng)]);
    } else {
      result.push_back(pattern[i]);
      if (is_role_slot_(pattern[i]) &&
          std::string(pattern[i]).find("verb") != std::string::npos &&
          prob(rng) < 0.3) {
        result.push_back(kAspectMarkers[pick(rng)]);
      }
    }
  }
  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 取词池
// ═══════════════════════════════════════════════════════════════════

auto SemanticFlow::role_pool_(const std::string& role,
                               const std::string* prev_word,
                               const std::unordered_set<std::string>& used_words,
                               std::mt19937_64& rng) const -> std::string {
  const RoleClusterDef* role_def = nullptr;
  for (int i = 0; i < kRoleCount; ++i) {
    if (role == kRoleClusters[i].role) {
      role_def = &kRoleClusters[i];
      break;
    }
  }
  if (!role_def) return "";

  std::vector<std::string> pool;
  for (int c = 0; c < 6 && role_def->clusters[c] != nullptr; ++c) {
    auto it = cluster_members_.find(role_def->clusters[c]);
    if (it != cluster_members_.end()) {
      for (const auto& m : it->second) {
        if (m.size() == 1 &&
            static_cast<unsigned char>(m[0]) >= 0xE4 &&
            static_cast<unsigned char>(m[0]) <= 0xE9) {
          pool.push_back(m);
        }
      }
    }

    for (int p = 0; p < kPhraseCount; ++p) {
      if (std::strcmp(role_def->clusters[c], kPhraseGroups[p].cluster) == 0) {
        for (int w = 0; w < 9 && kPhraseGroups[p].phrases[w] != nullptr; ++w) {
          if (used_words.find(kPhraseGroups[p].phrases[w]) == used_words.end()) {
            pool.push_back(kPhraseGroups[p].phrases[w]);
          }
        }
      }
    }
  }

  std::unordered_set<std::string> seen;
  std::vector<std::string> deduped;
  for (const auto& w : pool) {
    if (used_words.find(w) == used_words.end() && seen.find(w) == seen.end()) {
      deduped.push_back(w);
      seen.insert(w);
    }
  }
  pool = std::move(deduped);

  if (pool.empty()) return "";

  std::vector<double> weights;
  std::uniform_real_distribution<double> noise(-0.1, 0.1);

  for (const auto& w : pool) {
    double e = 0.3;

    double pipe_bonus = 1.0;
    if (prev_word) {
      std::string key = *prev_word + "|" + w;
      auto it = pipe_thickness_.find(key);
      if (it != pipe_thickness_.end()) {
        pipe_bonus = 1.0 + it->second * 2.0;
      } else {
        key = w + "|" + *prev_word;
        it = pipe_thickness_.find(key);
        if (it != pipe_thickness_.end()) {
          pipe_bonus = 1.0 + it->second * 2.0;
        }
      }
    }

    double phrase_bonus = (w.size() == 2) ? 1.3 : 1.0;
    weights.push_back(std::max(0.01, (e + noise(rng)) * pipe_bonus * phrase_bonus));
  }

  double total = std::accumulate(weights.begin(), weights.end(), 0.0);
  if (total <= 0) return pool.empty() ? "" : pool[0];

  std::uniform_real_distribution<double> dist(0.0, total);
  double r = dist(rng);
  double cum = 0;
  for (size_t i = 0; i < weights.size(); ++i) {
    cum += weights[i];
    if (r <= cum) return pool[i];
  }
  return pool.back();
}

// ═══════════════════════════════════════════════════════════════════
// 生成句子
// ═══════════════════════════════════════════════════════════════════

auto SemanticFlow::generate_sentence(int seed) -> std::string {
  std::mt19937_64 rng(static_cast<uint64_t>(seed));

  auto chosen = pick_template_(rng);
  auto pattern = expand_aspect_(chosen, rng);

  std::string result;
  std::unordered_set<std::string> used_words;
  std::string prev_word;

  for (const auto& slot : pattern) {
    if (is_role_slot_(slot)) {
      std::string word = role_pool_(slot, &prev_word, used_words, rng);
      if (!word.empty()) {
        result += word;
        used_words.insert(word);
        prev_word = word;
      }
    } else if (std::strcmp(slot, "asp") != 0) {
      result += slot;
      if (is_connector_token(slot)) {
        prev_word = slot;
      }
    }
  }

  return result;
}

// ═══════════════════════════════════════════════════════════════════
// 生成段落
// ═══════════════════════════════════════════════════════════════════

auto SemanticFlow::generate(int paragraphs, int sentences_per, int seed)
    -> std::string {
  std::mt19937_64 rng(static_cast<uint64_t>(seed));
  std::uniform_int_distribution<int> sentence_count(1, sentences_per);

  std::ostringstream oss;
  for (int p = 0; p < paragraphs; ++p) {
    int n = sentence_count(rng);
    for (int s = 0; s < n; ++s) {
      oss << generate_sentence(seed + p * 100 + s) << "\u3002";
    }
    if (p < paragraphs - 1) {
      oss << "\n\n";
    }
  }
  return oss.str();
}

// ═══════════════════════════════════════════════════════════════════
// 批量生成
// ═══════════════════════════════════════════════════════════════════

auto SemanticFlow::batch(int n, int paragraphs, int sentences_per)
    -> std::vector<std::string> {
  std::vector<std::string> results;
  results.reserve(n);
  for (int i = 0; i < n; ++i) {
    results.push_back(generate(paragraphs, sentences_per, 42 + i));
  }
  return results;
}

// ═══════════════════════════════════════════════════════════════════
// 统计
// ═══════════════════════════════════════════════════════════════════

auto SemanticFlow::stats() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["n_clusters"] = static_cast<int>(clusters_.size());
  j["n_templates"] = kTemplateCount;
  j["n_phrases"] = kPhraseCount;
  j["has_emergence"] = emergence_.is_object();
  return j;
}

}  // namespace nexus::psyche
