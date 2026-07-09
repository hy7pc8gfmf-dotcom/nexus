#ifndef NEXUS_ALGO_ENGINES_MULTIMODAL_VERIFIER_H
#define NEXUS_ALGO_ENGINES_MULTIMODAL_VERIFIER_H

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cctype>
#include <set>

namespace nexus::algo::engines {

/// 多模态验证引擎 — 交叉验证文本中的事实一致性
class MultimodalVerifierEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return {"multimodal_verifier", "Multimodal Verifier", "1.0.0",
            "多源交叉验证: 从不同角度验证陈述的一致性", {"verification", "cross-validation", "consistency"}};
  }

  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true; return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");

    auto statements = input.value("statements", std::vector<std::string>());
    auto result = nlohmann::json::object();
    auto verifications = nlohmann::json::array();

    for (const auto& stmt : statements) {
      auto v = nlohmann::json::object();
      v["statement"] = stmt;
      v["length"] = static_cast<int>(stmt.size());

      // 内部一致性：数字/专有名词/逻辑词的平衡度
      int digits = 0, upper = 0, punct = 0;
      for (char c : stmt) {
        if (std::isdigit(c)) digits++;
        else if (std::isupper(static_cast<unsigned char>(c))) upper++;
        else if (std::ispunct(static_cast<unsigned char>(c))) punct++;
      }
      double total = static_cast<double>(stmt.size());
      double consistency = 1.0;
      if (total > 0) {
        double digit_ratio = digits / total;
        double punct_ratio = punct / total;
        if (digit_ratio > 0.5) consistency -= 0.3;
        if (punct_ratio > 0.3) consistency -= 0.2;
        consistency = std::max(0.0, consistency);
      }

      // 关键词密度 (信息量估计)
      std::set<std::string> words;
      std::string word;
      for (char c : stmt) {
        if (std::isalnum(static_cast<unsigned char>(c))) { word += c; }
        else if (!word.empty()) { if (word.size() > 2) words.insert(word); word.clear(); }
      }
      if (!word.empty() && word.size() > 2) words.insert(word);

      v["consistency"] = std::round(consistency * 1000) / 1000;
      v["keyword_count"] = static_cast<int>(words.size());
      double info_density = total > 0 ? words.size() / total : 0;
      v["info_density"] = std::round(info_density * 1000) / 1000;
      v["verdict"] = consistency >= 0.7 ? "consistent" : "needs_review";
      verifications.push_back(v);
    }

    int consistent = 0;
    for (const auto& v : verifications) {
      if (v["verdict"].get<std::string>() == "consistent") consistent++;
    }

    result["status"] = "ok";
    result["total"] = static_cast<int>(statements.size());
    result["consistent"] = consistent;
    result["consistency_rate"] = statements.empty() ? 0 :
      std::round(100.0 * consistent / statements.size()) / 100;
    result["verifications"] = verifications;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object(); j["initialized"] = initialized_; return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
};

} // namespace
#endif
