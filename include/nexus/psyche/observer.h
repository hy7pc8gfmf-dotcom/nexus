#ifndef NEXUS_PSYCHE_OBSERVER_H
#define NEXUS_PSYCHE_OBSERVER_H

/**
 * @file observer.h
 * @brief 30 Observer 并行观察者池
 *
 * 每个观察者从不同角度分析系统状态。
 * 特殊角色: auditor(审计) / tagger(标注) / critic(批评) + MetaObserver
 *
 * 21 个量子子系统映射到 30 个观察者, 形成多维感知网络。
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace nexus::psyche {

/// 观察者类型
enum class ObserverRole : uint8_t {
  kAuditor = 0,  // 审计者: 检查一致性
  kTagger  = 1,  // 标注者: 添加标签和分类
  kCritic  = 2,  // 批评者: 发现问题和矛盾
  kAnalyst = 3,  // 分析者: 深度分析
  kSynthesizer = 4,  // 综合者: 整合多源信息
};

/// 观察者定义
struct ObserverDef {
  int    id;
  std::string name;
  ObserverRole role;
  std::string focus;  // 关注领域

  auto to_json() const -> nlohmann::json;
};

/// 观察结果
struct Observation {
  int    observer_id;
  std::string observer_name;
  std::string content;
  double confidence;
  std::string type;  // "insight" | "warning" | "suggestion"

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// ObserverPool
// ═══════════════════════════════════════════════════════════════════

class ObserverPool {
public:
  ObserverPool() noexcept;

  /// 初始化 30 个观察者
  auto initialize() noexcept -> void;

  /// 运行所有观察者 (并行)
  auto observe_all(const nlohmann::json& context) noexcept
      -> std::vector<Observation>;

  /// 按角色运行观察者
  auto observe_by_role(ObserverRole role,
                       const nlohmann::json& context) noexcept
      -> std::vector<Observation>;

  /// 获取某个观察者
  [[nodiscard]] auto get_observer(int id) const noexcept -> const ObserverDef*;

  /// 获取统计
  [[nodiscard]] auto count_by_role() const noexcept -> nlohmann::json;
  [[nodiscard]] auto total_observers() const noexcept -> int;
  [[nodiscard]] auto recent_observations(int n = 10) const noexcept
      -> std::vector<Observation>;

private:
  std::vector<ObserverDef> observers_;
  std::vector<Observation> history_;
  static constexpr int kMaxHistory = 100;

  /// 根据角色运行单个观察者
  auto run_observer_(const ObserverDef& def,
                     const nlohmann::json& context) noexcept -> Observation;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_OBSERVER_H
