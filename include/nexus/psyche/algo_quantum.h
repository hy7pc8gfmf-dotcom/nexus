#ifndef NEXUS_PSYCHE_ALGO_QUANTUM_H
#define NEXUS_PSYCHE_ALGO_QUANTUM_H

/**
 * @file algo_quantum.h
 * @brief Algo×Quantum 桥接 — 算法引擎与量子涌现的双向连接
 *
 * 正向: algo → psyche (算法结果注入涌现流水线)
 * 反向: psyche → algo (涌现状态驱动算法选择)
 *
 * 30 Observer 映射: 每个观察者对应一个量子子系统维度
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/types/status.h"

namespace nexus::psyche {

/// 桥接方向
enum class BridgeDirection : uint8_t {
  kAlgoToQuantum = 0,  // 算法 → 量子
  kQuantumToAlgo = 1,  // 量子 → 算法
  kBidirectional = 2,  // 双向
};

/// 桥接数据包
struct BridgePacket {
  std::string source;       // 来源组件
  std::string target;       // 目标组件
  std::string payload_type; // insight | state | command
  nlohmann::json payload;
  double timestamp = 0.0;
  BridgeDirection direction = BridgeDirection::kBidirectional;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// AlgoQuantumBridge
// ═══════════════════════════════════════════════════════════════════

class AlgoQuantumBridge {
public:
  AlgoQuantumBridge() noexcept = default;

  /// 算法 → 量子: 注入算法结果到涌现流水线
  auto feed_algo_result(const std::string& engine_id,
                        const nlohmann::json& result) noexcept -> Status;

  /// 量子 → 算法: 查询当前涌现状态
  [[nodiscard]] auto query_emergence() const noexcept -> nlohmann::json;

  /// 量子 → 算法: 根据涌现水平推荐算法
  [[nodiscard]] auto recommend_algo(double ae_level) const noexcept
      -> std::vector<std::string>;

  /// 获取桥接统计
  [[nodiscard]] auto stats() const noexcept -> nlohmann::json;

  /// 获取桥接历史
  [[nodiscard]] auto recent_packets(int n = 10) const noexcept
      -> std::vector<BridgePacket>;

private:
  std::vector<BridgePacket> history_;
  nlohmann::json last_quantum_state_;
  static constexpr int kMaxHistory = 50;

  // 30 Observer → 21 量子子系统映射 (略, 当前为简化映射)
  static constexpr int kQuantumSubsystems = 21;
};

}  // namespace nexus::psyche

#endif  // NEXUS_PSYCHE_ALGO_QUANTUM_H
