// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_BRIDGE_SEED_BUFFER_H
#define NEXUS_BRIDGE_SEED_BUFFER_H

/**
 * @file seed_buffer.h
 * @brief 种子缓冲区 + 语义合成器 — 批量写入 + 跨域种子内容合成
 *
 * 移植自 D:/synapse/engines/seed_buffer.py
 *
 * SeedBuffer: 内存缓冲, 达到阈值后批量写入 seed_bank
 * SemanticSynthesizer: 跨域种子内容合成, 生成引理级描述
 */

#include <chrono>
#include <cstdint>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/bridge/seed_bank.h"

namespace nexus::bridge {

// ═══════════════════════════════════════════════════════════════════
// 缓冲条目
// ═══════════════════════════════════════════════════════════════════

struct SeedBufferEntry {
  std::string name;
  double      intensity  = 0.0;
  std::string source;
  std::string domain;
  std::string detail;
  std::string seed_type = "formal_seed";
  std::vector<double> center_14d;  // 可选 14D 坐标

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// 种子缓冲区 — 线程安全批量写入
// ═══════════════════════════════════════════════════════════════════

class SeedBuffer {
public:
  explicit SeedBuffer(int auto_flush = 50) noexcept;

  /// 添加种子到缓冲区
  void add(const std::string& name, double intensity,
           const std::string& source, const std::string& domain,
           const std::string& detail,
           const std::string& seed_type = "formal_seed",
           const std::vector<double>& center_14d = {}) noexcept;

  /// 批量写入所有累积种子到 seed_bank
  auto flush(bridge::SeedBank* bank) noexcept -> int;

  /// 当前缓冲数量
  [[nodiscard]] auto size() const noexcept -> int;

  /// 清空缓冲
  void clear() noexcept;

private:
  int auto_flush_;
  std::vector<SeedBufferEntry> pending_;
  mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════
// 语义合成器 — 跨域种子内容合成
// ═══════════════════════════════════════════════════════════════════

class SemanticSynthesizer {
public:
  /// 提取关键术语
  static auto extract_key_terms(const std::string& text, int max_terms = 5)
      -> std::vector<std::string>;

  /// 域标签转可读名称
  static auto domain_name(const std::string& tag) -> std::string;

  /// 合成两个种子的汇合描述
  static auto synthesize(const std::string& name_a,
                          const std::string& detail_a,
                          const std::string& domain_a,
                          const std::string& name_b,
                          const std::string& detail_b,
                          const std::string& domain_b,
                          double proximity) -> std::string;

private:
  static const std::vector<std::pair<std::string, std::string>>&
      fusion_templates_();
  static const std::vector<std::pair<std::string, std::string>>&
      domain_names_();
};

}  // namespace nexus::bridge

#endif  // NEXUS_BRIDGE_SEED_BUFFER_H
