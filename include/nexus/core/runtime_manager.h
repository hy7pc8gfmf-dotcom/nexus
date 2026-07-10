// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_CORE_RUNTIME_MANAGER_H
#define NEXUS_CORE_RUNTIME_MANAGER_H

/**
 * @file runtime_manager.h
 * @brief 运行时管理器 — Coq 编译 + RLA 组件分发
 *
 * 功能:
 *   1. 检查运行时组件完整性 (Coq DLL + 种子 + 语义场)
 *   2. 在线编译 Coq 源码 → DLL (需 coqc 工具链)
 *   3. 加载已验证的 DLL
 *
 * RLA 组件列表:
 *   meta_kernel_rules.dll  — Coq 提取的 5 条元规则
 *   constructive_rules.dll — 构造性逻辑规则
 *   .unified_seed_bank.json  — 32,629 颗种子
 *   .semantic_field.bin      — 50,945 概念语义场
 */

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nexus/core/coq_kernel.h"
#include "nexus/types/status.h"

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// 组件状态
// ═══════════════════════════════════════════════════════════════════

struct RuntimeComponent {
  std::string name;
  std::string path;
  std::string sha256;       // 预期校验和 (空=不校验)
  bool present   = false;
  bool verified  = false;

  auto to_json() const -> nlohmann::json;
};

// ═══════════════════════════════════════════════════════════════════
// Coq 编译器 (调用 coqc 子进程)
// ═══════════════════════════════════════════════════════════════════

class CoqCompiler {
public:
  CoqCompiler() noexcept = default;

  /// 设置 coqc 路径
  void set_coqc_path(const std::string& path) noexcept { coqc_path_ = path; }

  /// 编译 .v 文件 → .vo
  auto compile(const std::string& v_file) noexcept -> Result<std::string>;

  /// 执行 Coq 提取命令 → .c/.h
  auto extract(const std::string& v_file,
               const std::string& output_dir) noexcept -> Result<std::string>;

  /// 检查 coqc 是否可用
  [[nodiscard]] auto is_available() const noexcept -> bool;

private:
  std::string coqc_path_ = "coqc";
  std::string coqtop_path_ = "coqtop";

  static auto run_process_(const std::string& cmd,
                            const std::vector<std::string>& args) noexcept
      -> Result<std::string>;
};

// ═══════════════════════════════════════════════════════════════════
// 运行时管理器
// ═══════════════════════════════════════════════════════════════════

class RuntimeManager {
public:
  RuntimeManager() noexcept;

  /// 注册一个运行时组件
  void register_component(const std::string& name, const std::string& path,
                           const std::string& sha256 = "") noexcept;

  /// 检查所有组件完整性
  auto check_all() noexcept -> Status;

  /// 获取状态
  auto status() const noexcept -> nlohmann::json;

  /// 缺失组件列表
  auto missing_components() const noexcept -> std::vector<RuntimeComponent>;

  /// 所有组件是否完整
  [[nodiscard]] auto all_present() const noexcept -> bool;

  /// 加载 Coq DLL (需要 RLA 授权)
  auto load_coq_kernel(const std::string& meta_path = "",
                        const std::string& constructive_path = "") noexcept
      -> Status;

  /// 在线编译 Coq .v 文件 → 注入种子
  auto compile_and_inject(const std::string& v_file,
                           const std::string& domain = "logic") noexcept
      -> Result<std::string>;

  /// 打印 RLA 提示
  static void print_rla_notice() noexcept;

private:
  std::vector<RuntimeComponent> components_;
  CoqCompiler compiler_;
  CoqMetaKernel meta_kernel_;
  ConstructiveRules constructive_rules_;

  static std::string compute_sha256_(const std::string& path);
};

}  // namespace nexus::core

#endif  // NEXUS_CORE_RUNTIME_MANAGER_H
