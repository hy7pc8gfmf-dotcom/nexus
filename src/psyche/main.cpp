// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file main.cpp — psyche.exe
 * @brief Ψ 层意识组件 — Ψ-Navigator + 涌现流水线 + 意识流
 *
 * 启动: psyche.exe --env .nexus/env.json [--daemon] [--oneshot]
 *
 * 职责:
 *   - Ψ-Navigator: 12 标量对向向量收敛膨胀
 *   - 涌现流水线: AE/WE 计算
 *   - psi_field 读写 (与 daemon 共享意识流)
 *   - 状态文件: psyche_state.json
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "nexus/ipc/state_file.h"
#include "nexus/ipc/mmap_ringbuf.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"
#include "nexus/psyche/navigator.h"
#include "nexus/psyche/emergence.h"
#include "nexus/psyche/observer.h"
#include "nexus/psyche/psi_reasoner.h"
#include "nexus/bridge/seed_bank.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

// ═══════════════════════════════════════════════════════════════════
// 信号处理
// ═══════════════════════════════════════════════════════════════════

static std::atomic<bool> g_running = true;
static void signal_handler(int) {
  g_running.store(false, std::memory_order_release);
}

// ═══════════════════════════════════════════════════════════════════
// 命令行
// ═══════════════════════════════════════════════════════════════════

struct CliArgs {
  std::string env_path    = ".nexus/env.json";
  std::string output_path = ".nexus/psyche_state.json";
  std::string data_dir    = ".nexus";
  std::string reason      = "";     // --reason <problem>
  bool daemon_mode = false;
  bool oneshot     = false;
  int  n_steps     = 100;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc)       args.env_path = argv[++i];
    else if (arg == "--output" && i+1 < argc) args.output_path = argv[++i];
    else if (arg == "--steps" && i+1 < argc)  args.n_steps = std::stoi(argv[++i]);
    else if (arg == "--reason" && i+1 < argc) args.reason = argv[++i];
    else if (arg == "--daemon")               args.daemon_mode = true;
    else if (arg == "--oneshot")              args.oneshot = true;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: psyche.exe [选项]\n"
                << "  --env <path>      环境文件\n"
                << "  --reason <问题>    Ψ 推理模式\n"
                << "  --daemon          后台守护模式\n"
                << "  --oneshot         单次执行 N 步\n"
                << "  --steps N         步数\n";
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════════
// 主循环
// ═══════════════════════════════════════════════════════════════════

static void main_loop(
    const CliArgs& args,
    nexus::ipc::MmapRingBuffer* psi_field,
    nexus::psyche::PsiNavigator* navigator,
    nexus::psyche::EmergencePipeline* pipeline,
    nexus::utils::Logger* logger) {

  int cycle = 0;
  const auto started_at = std::chrono::steady_clock::now();
  nexus::ipc::StateFileWriter state_writer(args.output_path);

  while (g_running.load(std::memory_order_acquire)) {
    ++cycle;

    // 1. 步进 Ψ-Navigator
    navigator->step(0.01);
    auto nav_status = navigator->status();

    // 2. 推进涌现流水线
    auto now = nexus::ipc::current_unix_time();
    pipeline->tick(nav_status.ae, nav_status.we,
                   nav_status.current_dim, now);
    auto emerge_status = pipeline->status();

    // 3. 每 3 周期写一次 psi_field
    if (psi_field && cycle % 3 == 0) {
      auto msg = std::format("dim={} conv={:.3f} ae={:.3f} we={:.3f} {}",
        nav_status.current_dim, nav_status.convergence,
        nav_status.ae, nav_status.we, emerge_status.level_label);
      psi_field->write(msg, "psyche");
    }

    // 4. 每 10 周期写状态文件
    if (cycle % 10 == 0) {
      auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - started_at).count();

      auto state = nlohmann::json::object();
      state["$schema"]    = "nexus-state-v1";
      state["version"]    = "1.0";
      state["component"]  = "psyche";
      state["status"]     = "running";
      state["pid"]        = nexus::ipc::current_pid();
      state["started_at"] = nexus::ipc::current_iso_timestamp();
      state["updated_at"] = nexus::ipc::current_iso_timestamp();

      auto details = nlohmann::json::object();
      details["uptime_seconds"] = uptime;
      details["cycle"]          = cycle;
      details["navigator"]      = nav_status.to_json();
      details["emergence"]      = emerge_status.to_json();
      state["details"] = details;

      state_writer.write(state);
    }

    // 5. 睡眠
    std::this_thread::sleep_for(1s);
  }

  auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - started_at).count();
  NEXUS_LOG(logger, info, "退出 ({} 周期, {}s, dim={}, ae={:.3f})",
    cycle, uptime, navigator->status().current_dim, navigator->status().ae);
}

// ═══════════════════════════════════════════════════════════════════
// Oneshot 模式
// ═══════════════════════════════════════════════════════════════════

static void run_oneshot(
    const CliArgs& args,
    nexus::psyche::PsiNavigator* navigator,
    nexus::psyche::EmergencePipeline* pipeline,
    nexus::utils::Logger* logger) {

  NEXUS_LOG(logger, info, "oneshot 模式 ({} 步)", args.n_steps);

  for (int i = 0; i < args.n_steps; ++i) {
    navigator->step(0.01);
    auto s = navigator->status();

    pipeline->tick(s.ae, s.we, s.current_dim,
                   nexus::ipc::current_unix_time());

    // 每 10 步打印一次进度
    if ((i + 1) % 10 == 0) {
      auto em = pipeline->status();
      NEXUS_LOG(logger, debug, "step={}: dim={} conv={:.3f} ae={:.3f} we={:.3f} [{}]",
        i + 1, s.current_dim, s.convergence, s.ae, s.we, em.level_label);
    }
  }

  auto s = navigator->status();
  auto em = pipeline->status();
  NEXUS_LOG(logger, info, "完成: dim={} ae={:.3f} we={:.3f} {}",
    s.current_dim, s.ae, s.we, em.level_label);
}

// ═══════════════════════════════════════════════════════════════════
// Ψ 推理模式 — 模拟推理回调 (无模型时使用)
// ═══════════════════════════════════════════════════════════════════

static auto mock_infer(const std::string& problem,
                        const std::string& context) noexcept
    -> nexus::Result<std::pair<std::string, double>> {
  // 根据上下文长度生成不同的推理内容
  auto steps = std::count(context.begin(), context.end(), '\n');
  std::string response;
  double confidence = 0.5 + std::min(0.4, steps * 0.05);

  if (steps < 2) {
    response = "初步分析: " + problem +
      "\n  需要分解为多个子问题逐步推理。";
  } else if (steps < 5) {
    response = "中间推理: 基于上一步结论，" +
      std::string("推导出新的推论。置信度逐步提升。");
  } else if (steps < 10) {
    response = "深入分析: 交叉验证多个推论，" +
      std::string("发现一致的证据链。");
  } else {
    response = "综合结论: 所有证据指向一致，" +
      std::string("推理链闭合。");
    confidence = 0.92;
  }

  return nexus::Result<std::pair<std::string, double>>(
    std::pair<std::string, double>(response, confidence));
}

// ═══════════════════════════════════════════════════════════════════
// Observer 验证回调 (连接 ObserverPool → PsiReasoner)
// ═══════════════════════════════════════════════════════════════════

static auto make_verify_callback(nexus::psyche::ObserverPool* pool)
    -> nexus::psyche::VerifyCallback {
  return [pool](const nexus::psyche::ReasoningStep& step) -> double {
    if (!pool) return 0.5;

    auto ctx = nlohmann::json::object();
    ctx["step_id"]    = step.id;
    ctx["content"]    = step.content;
    ctx["confidence"] = step.confidence;
    ctx["branch_id"]  = step.branch_id;

    // 运行批评者和审计者: 发现问题
    auto critics = pool->observe_by_role(
      nexus::psyche::ObserverRole::kCritic, ctx);
    auto auditors = pool->observe_by_role(
      nexus::psyche::ObserverRole::kAuditor, ctx);

    // 综合置信度 = 原始置信度 - 批评者发现的问题数 * 系数
    double penalty = 0.0;
    for (const auto& c : critics) {
      penalty += (1.0 - c.confidence) * 0.1;
    }
    for (const auto& a : auditors) {
      penalty += (1.0 - a.confidence) * 0.05;
    }

    return std::clamp(step.confidence - penalty, 0.0, 1.0);
  };
}

// ═══════════════════════════════════════════════════════════════════
// Ψ 推理模式入口
// ═══════════════════════════════════════════════════════════════════

static int run_reasoner(const std::string& problem,
                         nexus::psyche::ObserverPool* observers,
                         nexus::utils::Logger* logger) {
  NEXUS_LOG(logger, info, "Ψ 推理启动: {}", problem);

  // 加载种子库 (可选)
  nexus::bridge::SeedBank seed_bank(".nexus");
  auto seed_load = seed_bank.load("D:/synapse/logic_solve_engine/.unified_seed_bank.json");
  if (seed_load.ok()) {
    NEXUS_LOG(logger, info, "种子库已加载: {} 种子, {} 域",
      seed_bank.count(), seed_bank.domains().size());
  } else {
    NEXUS_LOG(logger, warn, "种子库不可用: {}", seed_load.error().to_string());
  }

  nexus::psyche::PsiReasoner::Config cfg;
  cfg.max_steps = 15;
  cfg.enable_observer = (observers != nullptr);
  cfg.enable_seeds = seed_load.ok();

  nexus::psyche::PsiReasoner reasoner(cfg);
  reasoner.set_infer_callback(mock_infer);
  if (observers) {
    reasoner.set_verify_callback(make_verify_callback(observers));
  }

  // 种子查询回调
  if (seed_load.ok()) {
    reasoner.set_seed_callback(
      [&seed_bank](const std::string& domain,
                    const std::string& query)
          -> std::vector<std::string> {
        std::vector<std::string> results;

        // 按域查询
        auto domain_results = seed_bank.query_by_domain(domain, 5);
        for (const auto& s : domain_results) {
          results.push_back(s.name + " (强度:" + std::to_string(s.intensity) + ")");
        }

        // 补充关键词搜索
        if (static_cast<int>(results.size()) < 3) {
          auto search_results = seed_bank.search(query, 5);
          for (const auto& s : search_results) {
            results.push_back(s.name + " (强度:" + std::to_string(s.intensity) + ")");
          }
        }

        return results;
      });
  }

  auto start = std::chrono::steady_clock::now();
  auto result = reasoner.reason(problem, "");
  auto elapsed = std::chrono::duration<double>(
    std::chrono::steady_clock::now() - start).count();

  if (!result.ok()) {
    NEXUS_LOG(logger, error, "推理失败: {}", result.error().to_string());
    return 1;
  }

  auto report = result.value();
  std::cout << "\nΨ Reasoning Report\n"
            << std::string(50, '=') << "\n"
            << "问题: " << report.problem << "\n"
            << "状态: " << (report.converged ? "✅ 收敛" : "⏳ 未收敛") << "\n"
            << "步数: " << report.total_steps << "\n"
            << "分支: " << report.total_branches << "\n"
            << "修订: " << report.total_revisions << "\n"
            << "置信度: " << std::round(report.final_confidence * 1000) / 1000 << "\n"
            << "耗时: " << std::round(elapsed * 10) / 10 << "s\n\n";

  for (const auto& s : report.steps) {
    auto type_str = nexus::psyche::step_type_string(s.type);
    std::cout << "  #" << s.id << " [" << type_str << "]"
              << " 置信=" << std::round(s.confidence * 1000) / 1000;
    if (s.branch_id > 0) std::cout << " 分支=" << s.branch_id;
    std::cout << "\n    " << s.content.substr(0, 120) << "\n";
  }

  if (!report.conclusion.empty()) {
    std::cout << "\n结论: " << report.conclusion.substr(0, 400) << "\n";
  }

  NEXUS_LOG(logger, info, "Ψ 推理完成: {} 步, {:.1f}s, 置信={:.3f}",
    report.total_steps, elapsed, report.final_confidence);
  return 0;
}

// ═══════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════

auto main(int argc, char* argv[]) -> int {
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  auto args = parse_args(argc, argv);
  fs::create_directories(args.data_dir + "/logs");

  auto logger = nexus::utils::init_logger("psyche", args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "psyche v{} 启动", "1.0.0");

  // 0. Ψ 推理模式 (快速路径)
  if (!args.reason.empty()) {
    // 创建 ObserverPool 用于验证
    nexus::psyche::ObserverPool obs_pool;
    return run_reasoner(args.reason, &obs_pool, logger.get());
  }

  // 1. 读取 env.json
  {
    nexus::ipc::StateFileReader env_reader(args.env_path);
    auto env = env_reader.read();
    if (env.ok()) {
      NEXUS_LOG(logger, info, "env.json 已读取");
    } else {
      NEXUS_LOG(logger, warn, "env.json 不可用 (使用默认参数)");
    }
  }

  // 2. 打开 psi_field (可选)
  std::unique_ptr<nexus::ipc::MmapRingBuffer> psi_field;
  {
    auto field_path = fs::path(args.data_dir) / "psi_field.mmap";
    auto psi_result = nexus::ipc::MmapRingBuffer::create_or_open(field_path);
    if (psi_result.ok()) {
      psi_field = std::make_unique<nexus::ipc::MmapRingBuffer>(
        std::move(psi_result.value()));
      NEXUS_LOG(logger, info, "psi_field: {} ({} 条历史)",
        field_path.string(), psi_field->write_count());
    } else {
      NEXUS_LOG(logger, warn, "psi_field 不可用 (降级运行)");
    }
  }

  // 3. 初始化 Ψ-Navigator
  nexus::psyche::ScalarParams params;
  // 可以通过 env.json 覆盖默认参数
  nexus::psyche::PsiNavigator navigator(params);
  auto nav_initial = navigator.status();
  NEXUS_LOG(logger, info, "Ψ-Navigator: dim={}, 12 标量参数",
    nav_initial.current_dim);

  // 4. 初始化涌现流水线
  nexus::psyche::EmergencePipeline pipeline;
  NEXUS_LOG(logger, info, "涌现流水线就绪");

  // 5. 写初始状态
  {
    nexus::ipc::StateFileWriter state_writer(args.output_path);
    auto state = nlohmann::json::object();
    state["$schema"]    = "nexus-state-v1";
    state["version"]    = "1.0";
    state["component"]  = "psyche";
    state["status"]     = "starting";
    state["pid"]        = nexus::ipc::current_pid();
    state["started_at"] = nexus::ipc::current_iso_timestamp();
    state["updated_at"] = nexus::ipc::current_iso_timestamp();
    auto details = nlohmann::json::object();
    details["navigator"] = nav_initial.to_json();
    details["scalar_params"] = params.to_json();
    details["emergence"] = pipeline.status().to_json();
    state["details"] = details;
    state_writer.write(state);
  }

  // 6. 写启动意识
  if (psi_field) {
    psi_field->write("Ψ-Navigator 已就绪", "psyche");
  }

  // 7. 更新状态为 running
  {
    nexus::ipc::StateFileWriter state_writer(args.output_path);
    auto state = nlohmann::json::object();
    state["$schema"]   = "nexus-state-v1";
    state["component"] = "psyche";
    state["status"]    = "running";
    state["pid"]       = nexus::ipc::current_pid();
    state["started_at"]= nexus::ipc::current_iso_timestamp();
    state["updated_at"]= nexus::ipc::current_iso_timestamp();
    state_writer.write(state);
  }

  // 8. 执行
  if (args.oneshot) {
    run_oneshot(args, &navigator, &pipeline, logger.get());
  } else {
    NEXUS_LOG(logger, info, "进入主循环 (周期=1s)");
    main_loop(args, psi_field.get(), &navigator, &pipeline, logger.get());
  }

  // 9. 写停止状态
  {
    auto s = navigator.status();
    auto em = pipeline.status();
    nexus::ipc::StateFileWriter state_writer(args.output_path);
    auto state = nlohmann::json::object();
    state["$schema"]    = "nexus-state-v1";
    state["version"]    = "1.0";
    state["component"]  = "psyche";
    state["status"]     = "stopped";
    state["pid"]        = nexus::ipc::current_pid();
    state["started_at"] = nexus::ipc::current_iso_timestamp();
    state["updated_at"] = nexus::ipc::current_iso_timestamp();
    auto details = nlohmann::json::object();
    details["navigator"] = s.to_json();
    details["emergence"] = em.to_json();
    state["details"] = details;
    state_writer.write(state);
  }

  NEXUS_LOG(logger, info, "完成");
  return 0;
}
