// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/core/runtime_manager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// 序列化
// ═══════════════════════════════════════════════════════════════════

auto RuntimeComponent::to_json() const -> nlohmann::json {
  auto j = nlohmann::json::object();
  j["name"]     = name;
  j["path"]     = path;
  j["present"]  = present;
  j["verified"] = verified;
  return j;
}

// ═══════════════════════════════════════════════════════════════════
// 进程执行
// ═══════════════════════════════════════════════════════════════════

auto CoqCompiler::run_process_(const std::string& cmd,
                                const std::vector<std::string>& args) noexcept
    -> Result<std::string> {
  std::string full_cmd = cmd;
  for (const auto& a : args) full_cmd += " \"" + a + "\"";
  full_cmd += " 2>&1";

  std::string result;
  std::array<char, 512> buffer{};
#ifdef _WIN32
  FILE* pipe = _popen(full_cmd.c_str(), "r");
#else
  FILE* pipe = popen(full_cmd.c_str(), "r");
#endif
  if (!pipe) {
    return Status::Error(ErrorCode::kEngineFailed, "process launch failed");
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
    result += buffer.data();
  }
#ifdef _WIN32
  int rc = _pclose(pipe);
#else
  int rc = pclose(pipe);
#endif

  if (rc != 0) {
    return Status::Error(ErrorCode::kEngineFailed,
      cmd + " exit code " + std::to_string(rc) + ": " + result.substr(0, 200));
  }
  return result;
}

// ═══════════════════════════════════════════════════════════════════
// CoqCompiler
// ═══════════════════════════════════════════════════════════════════

auto CoqCompiler::is_available() const noexcept -> bool {
  auto r = run_process_(coqc_path_, {"--version"});
  return r.ok();
}

auto CoqCompiler::compile(const std::string& v_file) noexcept
    -> Result<std::string> {
  return run_process_(coqc_path_, {v_file});
}

auto CoqCompiler::extract(const std::string& v_file,
                           const std::string& output_dir) noexcept
    -> Result<std::string> {
  // Coq Extraction: coqc -Q . Dir -w -extraction output_dir v_file
  std::vector<std::string> args = {
    "-R", output_dir, "Extraction",
    "-w", "-extraction",
    v_file
  };
  return run_process_(coqc_path_, args);
}

// ═══════════════════════════════════════════════════════════════════
// SHA256 计算 (简化版)
// ═══════════════════════════════════════════════════════════════════

std::string RuntimeManager::compute_sha256_(const std::string& path) {
  // 使用 certutil (Windows) 或 sha256sum (Linux)
#ifdef _WIN32
  std::string cmd = "certutil -hashfile \"" + path + "\" SHA256 2>nul";
#else
  std::string cmd = "sha256sum \"" + path + "\" 2>/dev/null";
#endif
  std::array<char, 256> buffer{};
  std::string result;
  FILE* pipe = nullptr;
#ifdef _WIN32
  pipe = _popen(cmd.c_str(), "r");
#else
  pipe = popen(cmd.c_str(), "r");
#endif
  if (!pipe) return "";
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
    result += buffer.data();
  }
#ifdef _WIN32
  _pclose(pipe);
#else
  pclose(pipe);
#endif
  // 提取 hash 值 (第一行或第一个 token)
  if (result.empty()) return "";
  std::string hash;
  for (char c : result) {
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
      hash += c;
    else if (!hash.empty()) break;
  }
  return hash;
}

// ═══════════════════════════════════════════════════════════════════
// RuntimeManager
// ═══════════════════════════════════════════════════════════════════

RuntimeManager::RuntimeManager() noexcept {
  register_component("meta_kernel_rules.dll",
    "D:/synapse/meta_kernel_rules.dll");
  register_component("constructive_rules.dll",
    "D:/synapse/constructive_rules.dll");
  register_component(".unified_seed_bank.json",
    "D:/synapse/logic_solve_engine/.unified_seed_bank.json");
  register_component(".semantic_field.bin",
    "D:/nexus/binary/.semantic_field.bin");
}

void RuntimeManager::register_component(const std::string& name,
                                         const std::string& path,
                                         const std::string& sha256) noexcept {
  RuntimeComponent rc;
  rc.name   = name;
  rc.path   = path;
  rc.sha256 = sha256;
  components_.push_back(rc);
}

auto RuntimeManager::check_all() noexcept -> Status {
  for (auto& c : components_) {
    std::ifstream ifs(c.path, std::ios::binary);
    c.present = ifs.is_open();
    ifs.close();

    if (c.present && !c.sha256.empty()) {
      auto hash = compute_sha256_(c.path);
      c.verified = (hash == c.sha256);
    } else {
      c.verified = c.present;
    }
  }
  return Status::Ok();
}

auto RuntimeManager::status() const noexcept -> nlohmann::json {
  auto j = nlohmann::json::object();
  auto arr = nlohmann::json::array();
  for (const auto& c : components_) arr.push_back(c.to_json());
  j["components"] = arr;
  j["all_present"] = all_present();
  j["n_missing"] = static_cast<int>(missing_components().size());
  return j;
}

auto RuntimeManager::missing_components() const noexcept
    -> std::vector<RuntimeComponent> {
  std::vector<RuntimeComponent> missing;
  for (const auto& c : components_) {
    if (!c.present) missing.push_back(c);
  }
  return missing;
}

bool RuntimeManager::all_present() const noexcept {
  for (const auto& c : components_) {
    if (!c.present) return false;
  }
  return true;
}

auto RuntimeManager::load_coq_kernel(const std::string& meta_path,
                                      const std::string& constructive_path) noexcept
    -> Status {
  auto s1 = meta_kernel_.load(meta_path);
  auto s2 = constructive_rules_.load(constructive_path);
  if (!s1.ok() && !s2.ok()) {
    print_rla_notice();
    return Status::Error(ErrorCode::kFileNotFound,
      "Coq DLLs not loaded. " + s1.to_string());
  }
  return Status::Ok();
}

auto RuntimeManager::compile_and_inject(const std::string& v_file,
                                         const std::string& domain) noexcept
    -> Result<std::string> {
  if (!compiler_.is_available()) {
    return Status::Error(ErrorCode::kEngineFailed,
      "coqc not available. Install Coq from https://coq.inria.fr/");
  }

  // 编译 .v → .vo
  auto compile_result = compiler_.compile(v_file);
  if (!compile_result.ok()) {
    return compile_result.error();
  }

  // 返回编译输出
  return "compiled: " + v_file + "\n" + compile_result.value();
}

// ═══════════════════════════════════════════════════════════════════
// RLA 提示
// ═══════════════════════════════════════════════════════════════════

void RuntimeManager::print_rla_notice() noexcept {
  std::cout << "\n"
    << "╔══════════════════════════════════════════════════════╗\n"
    << "║  Nexus 运行时组件缺失                              ║\n"
    << "║  部分功能需要运行时组件才能正常工作。                 ║\n"
    << "║  这些组件受运行时许可协议 (RLA) 保护。               ║\n"
    << "║                                                    ║\n"
    << "║  获取方式:                                          ║\n"
    << "║  1. 签署 RLA: 见 CLA/RLA.md                        ║\n"
    << "║  2. 联系: nexus-legal@cherryclaw.dev               ║\n"
    << "║                                                    ║\n"
    << "║  运行时组件包括:                                    ║\n"
    << "║    - meta_kernel_rules.dll  (Coq 验证的元规则)       ║\n"
    << "║    - constructive_rules.dll (构造性逻辑规则)          ║\n"
    << "║    - .unified_seed_bank.json (32,629 颗种子)        ║\n"
    << "║    - .semantic_field.bin (50,945 概念语义场)        ║\n"
    << "╚══════════════════════════════════════════════════════╝\n"
    << std::endl;
}

}  // namespace nexus::core
