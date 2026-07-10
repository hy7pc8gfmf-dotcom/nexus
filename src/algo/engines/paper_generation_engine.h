// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_PAPER_GENERATION_H
#define NEXUS_ALGO_ENGINES_PAPER_GENERATION_H

/* 论文生成平台 v2 — 多模板 + 结构评分 + 引用格式 */

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <vector>

namespace nexus::algo::engines {

class PaperGenerationEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id = "paper_generation", .name = "Paper Generation v2",
      .version = "2.0.0",
      .description = "多模板论文框架 + 结构评分 + 引用格式",
      .tags = {"paper", "academic", "template", "generation"},
    };
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    max_sections_ = config.value("max_sections", 6);
    template_ = config.value("template", std::string("academic"));
    citation_style_ = config.value("citation_style", std::string("IEEE"));
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    auto topics = input.value("topics", std::vector<std::string>());
    auto keywords = input.value("keywords", std::vector<std::string>());

    // 模板定义
    const std::map<std::string, std::vector<std::string>> templates = {
      {"academic",    {"Abstract","Introduction","Related Work","Methodology","Experiments","Conclusion","References"}},
      {"survey",      {"Abstract","Introduction","Taxonomy","Analysis","Discussion","Future Work","References"}},
      {"technical",   {"Abstract","Motivation","Design","Implementation","Evaluation","Discussion","References"}},
      {"position",    {"Abstract","Introduction","Argument","Counter-arguments","Synthesis","Conclusion","References"}},
    };

    auto tmpl_it = templates.find(template_);
    const auto& section_names = (tmpl_it != templates.end())
        ? tmpl_it->second : templates.at("academic");

    // 引用格式模板
    const std::map<std::string, std::string> citation_formats = {
      {"IEEE", "[%d] Author. \"Title,\" Journal, vol. x, no. y, pp. z, year."},
      {"ACM", "Author. Year. Title. ACM."},
      {"APA", "Author (Year). Title. Journal, volume(issue), pages."},
      {"Nature", "1. Author, A. Title. Journal vol, pages (year)."},
    };

    auto papers = nlohmann::json::array();
    for (const auto& topic : topics) {
      auto paper = nlohmann::json::object();
      paper["title"] = "On " + topic;
      paper["topic"] = topic;
      paper["template"] = template_;
      paper["citation_style"] = citation_style_;

      auto sections = nlohmann::json::array();
      for (int i = 0; i < std::min(max_sections_, static_cast<int>(section_names.size())); ++i) {
        auto sec = nlohmann::json::object();
        sec["heading"] = section_names[i];
        sec["order"] = i;

        // 各章节字数与内容结构
        sec["target_words"] = 150 + i * 80;
        sec["structure"] = (i == 0) ? "背景→问题→贡献" :
                           (i == 1) ? "现状→空白→目标" :
                           (i == 3) ? "框架→算法→理论" :
                           (i == 4) ? "设置→结果→分析" : "总结→局限→展望";

        sec["key_focus"] = topic.substr(0, std::min<size_t>(40, topic.size()));
        sections.push_back(sec);
      }
      paper["sections"] = sections;
      paper["total_sections"] = static_cast<int>(sections.size());
      paper["estimated_words"] = 150 * static_cast<int>(sections.size())
          + 80 * static_cast<int>(sections.size()) * (static_cast<int>(sections.size()) - 1) / 2;

      // 结构完整性评分
      double structure_score = std::min(1.0,
          static_cast<double>(sections.size()) / 7.0 * 0.7 + 0.3);
      paper["structure_score"] = std::round(structure_score * 1000) / 1000;

      // 引用生成
      if (!keywords.empty()) {
        auto refs = nlohmann::json::array();
        for (size_t r = 0; r < std::min<size_t>(3, keywords.size()); ++r) {
          auto ref = nlohmann::json::object();
          ref["index"] = static_cast<int>(r + 1);
          auto fmt = citation_formats.find(citation_style_);
          std::string template_str = (fmt != citation_formats.end())
              ? fmt->second : citation_formats.at("IEEE");
          char buf[256];
          std::snprintf(buf, sizeof(buf), template_str.c_str(), r + 1);
          ref["citation"] = buf;
          ref["topic"] = keywords[r];
          refs.push_back(ref);
        }
        paper["references"] = refs;
      }
      papers.push_back(paper);
    }

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["total_papers"] = static_cast<int>(topics.size());
    result["template"] = template_;
    result["citation_style"] = citation_style_;
    result["papers"] = papers;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"] = initialized_;
    j["template"] = template_;
    j["citation_style"] = citation_style_;
    return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
  int max_sections_ = 6;
  std::string template_ = "academic";
  std::string citation_style_ = "IEEE";
};

} // namespace
#endif
