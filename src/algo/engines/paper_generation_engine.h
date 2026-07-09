#ifndef NEXUS_ALGO_ENGINES_PAPER_GENERATION_H
#define NEXUS_ALGO_ENGINES_PAPER_GENERATION_H

#include "nexus/algo/engine.h"
#include <algorithm>
#include <sstream>

namespace nexus::algo::engines {

/// 论文生成与验证平台 — 结构化论文框架生成
class PaperGenerationEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return {"paper_generation", "Paper Generation Platform", "1.0.0",
            "结构化论文框架生成: 摘要/引言/方法/结论", {"paper", "generation", "academic"}};
  }

  auto initialize(const nlohmann::json& config) noexcept -> Status override {
    max_sections_ = config.value("max_sections", 6);
    style_ = config.value("style", std::string("academic"));
    initialized_ = true; return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");

    auto topics = input.value("topics", std::vector<std::string>());
    auto keywords = input.value("keywords", std::vector<std::string>());

    auto result = nlohmann::json::object();
    auto papers = nlohmann::json::array();

    for (const auto& topic : topics) {
      auto paper = nlohmann::json::object();
      paper["title"] = "On " + topic;
      paper["topic"] = topic;
      paper["style"] = style_;

      auto sections = nlohmann::json::array();
      std::vector<std::string> section_names = {
        "Abstract", "Introduction", "Background",
        "Methodology", "Results", "Conclusion"};

      int n_sec = std::min(max_sections_, static_cast<int>(section_names.size()));
      for (int i = 0; i < n_sec; ++i) {
        auto sec = nlohmann::json::object();
        sec["heading"] = section_names[i];
        sec["word_count"] = 100 + i * 50;
        sec["key_focus"] = topic.substr(0, std::min<size_t>(40, topic.size()));
        sections.push_back(sec);
      }

      paper["sections"] = sections;
      paper["total_sections"] = n_sec;
      paper["estimated_words"] = 100 * n_sec + 50 * n_sec * (n_sec - 1) / 2;

      // 关键词映射
      if (!keywords.empty()) {
        auto kws = nlohmann::json::array();
        for (const auto& kw : keywords) kws.push_back(kw);
        paper["keywords"] = kws;
      }

      papers.push_back(paper);
    }

    result["status"] = "ok";
    result["total_papers"] = static_cast<int>(topics.size());
    result["papers"] = papers;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object();
    j["initialized"] = initialized_; j["max_sections"] = max_sections_; j["style"] = style_; return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
  int max_sections_ = 6;
  std::string style_ = "academic";
};

} // namespace
#endif
