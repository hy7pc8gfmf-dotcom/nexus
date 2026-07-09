/**
 * @file main.cpp — algo.exe
 * @brief 算法引擎池 — 注册 + 初始化 + 审计
 *
 * 启动: algo.exe --env .nexus/env.json [--validate] [--list]
 *
 * Phase 4 实现:
 *   注册 10 个算法引擎 → 初始化 → 写状态文件
 *   引擎注册表使用插件架构, 可按需扩展。
 */

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "nexus/ipc/state_file.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"
#include "nexus/algo/engine.h"
#include "nexus/algo/audit.h"

// 算法引擎头文件
#include "engines/mcmc_engine.h"
#include "engines/dre_engine.h"
#include "engines/dual_pruning_engine.h"
#include "engines/infinitas_truth_engine.h"
#include "engines/dialectical_consensus_engine.h"
#include "engines/mtcs_engine.h"
#include "engines/multimodal_verifier_engine.h"
#include "engines/temporal_kg_engine.h"
#include "engines/ethical_evaluation_engine.h"
#include "engines/paper_generation_engine.h"

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════
// 命令行
// ═══════════════════════════════════════════════════════════════════

struct CliArgs {
  std::string env_path  = ".nexus/env.json";
  std::string data_dir  = ".nexus";
  bool validate = false;
  bool list     = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc)  args.env_path = argv[++i];
    else if (arg == "--validate")        args.validate = true;
    else if (arg == "--list")            args.list = true;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: algo.exe --env <path> [--validate] [--list]\n";
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════════
// 注册所有引擎
// ═══════════════════════════════════════════════════════════════════

static auto register_all_engines(nexus::utils::Logger* logger,
                                  bool run_validate) -> nexus::Status {
  auto& registry = nexus::algo::EngineRegistry::instance();

  // ── 已实现的引擎 ──
  registry.register_engine(
    std::make_unique<nexus::algo::engines::McmcEngine>());
  NEXUS_LOG(logger, info, "注册引擎: mcmc");

  registry.register_engine(
    std::make_unique<nexus::algo::engines::DreEngine>());
  NEXUS_LOG(logger, info, "注册引擎: dre");

  registry.register_engine(
    std::make_unique<nexus::algo::engines::DualPruningEngine>());
  NEXUS_LOG(logger, info, "注册引擎: dual_pruning");

  registry.register_engine(
    std::make_unique<nexus::algo::engines::MtcsEngine>());
  NEXUS_LOG(logger, info, "注册引擎: mtcs");

  registry.register_engine(
    std::make_unique<nexus::algo::engines::InfinitasTruthEngine>());
  NEXUS_LOG(logger, info, "注册引擎: infinitas_truth");

  registry.register_engine(
    std::make_unique<nexus::algo::engines::DialecticalConsensusEngine>());
  NEXUS_LOG(logger, info, "注册引擎: dialectical_consensus");

  registry.register_engine(
    std::make_unique<nexus::algo::engines::MultimodalVerifierEngine>());
  NEXUS_LOG(logger, info, "注册引擎: multimodal_verifier");

  registry.register_engine(
    std::make_unique<nexus::algo::engines::TemporalKgEngine>());
  NEXUS_LOG(logger, info, "注册引擎: temporal_kg");

  registry.register_engine(
    std::make_unique<nexus::algo::engines::EthicalEvaluationEngine>());
  NEXUS_LOG(logger, info, "注册引擎: ethical_evaluation");

  registry.register_engine(
    std::make_unique<nexus::algo::engines::PaperGenerationEngine>());
  NEXUS_LOG(logger, info, "注册引擎: paper_generation");

  // ── 初始化所有引擎 ──
  auto init_status = registry.initialize_all();
  if (!init_status.ok()) {
    return init_status;
  }

  NEXUS_LOG(logger, info, "引擎注册完成: {} 个", registry.count());

  // 运行审计
  auto& audit = nexus::algo::AuditEngine::instance();
  auto audit_report = audit.run_all();
  NEXUS_LOG(logger, info, "审计: {}/{} 通过 ({} 失败)",
    audit_report.passed, audit_report.total, audit_report.failed);

  // ── 验证模式 ──
  if (run_validate) {
    // 测试 MCMC 引擎
    auto* mcmc = registry.get("mcmc");
    if (mcmc) {
      auto test_input = nlohmann::json::object();
      test_input["mean"]     = 0.0;
      test_input["std"]      = 1.0;
      test_input["n_samples"] = 100;
      test_input["n_chains"]  = 2;
      auto result = mcmc->execute(test_input);
      if (result.ok()) {
        auto chains = result.value()["chains"];
        NEXUS_LOG(logger, info, "MCMC 验证: {} chains, 每链 {} samples",
          static_cast<int>(chains.size()),
          static_cast<int>(chains[0]["samples"].size()));
      }
    }

    // 测试 DRE 引擎
    auto* dre = registry.get("dre");
    if (dre) {
      auto test_input = nlohmann::json::object();
      test_input["thesis"] = "人工智能应该被严格监管";
      test_input["iterations"] = 2;
      auto result = dre->execute(test_input);
      if (result.ok()) {
        NEXUS_LOG(logger, info, "DRE 验证: {}",
          result.value()["final_synthesis"].get<std::string>());
      }
    }

    // 测试双剪枝引擎
    auto* dp = registry.get("dual_pruning");
    if (dp) {
      auto test_input = nlohmann::json::object();
      auto cand = nlohmann::json::array();
      cand.push_back("人工智能发展迅速");
      cand.push_back("AI发展很快");
      cand.push_back("今天天气很好");
      test_input["candidates"] = cand;
      test_input["threshold"] = 0.6;
      auto result = dp->execute(test_input);
      if (result.ok()) {
        NEXUS_LOG(logger, info, "双剪枝验证: {} → {} (剪除 {})",
          result.value()["input_count"].get<int>(),
          result.value()["output_count"].get<int>(),
          result.value()["pruned_count"].get<int>());
      }
    }

    // 测试辩证共识引擎
    auto* dc = registry.get("dialectical_consensus");
    if (dc) {
      auto test_input = nlohmann::json::object();
      auto vps = nlohmann::json::array();
      vps.push_back("AI should be regulated for safety");
      vps.push_back("AI regulation is needed for safety");
      test_input["viewpoints"] = vps;
      auto result = dc->execute(test_input);
      if (result.ok()) {
        NEXUS_LOG(logger, info, "共识验证: score={}",
          result.value()["consensus_score"].get<double>());
      }
    }
  }

  return nexus::Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  fs::create_directories(args.data_dir + "/logs");

  auto logger = nexus::utils::init_logger("algo", args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "algo v{} 启动", "1.0.0");

  // 1. 读取 env.json
  {
    nexus::ipc::StateFileReader env_reader(args.env_path);
    auto env = env_reader.read();
    if (env.ok()) {
      NEXUS_LOG(logger, info, "env.json 已读取");
    } else {
      NEXUS_LOG(logger, warn, "env.json 不可用 (降级运行)");
    }
  }

  // 2. 注册和初始化引擎
  auto reg_status = register_all_engines(logger.get(), args.validate);
  if (!reg_status.ok()) {
    NEXUS_LOG(logger, error, "引擎注册失败: {}", reg_status.to_string());
  }

  // 3. 列出引擎
  if (args.list) {
    auto& registry = nexus::algo::EngineRegistry::instance();
    std::cout << "\n=== 算法引擎列表 ===\n";
    for (const auto& info : registry.list_details()) {
      std::cout << "  " << info.id << ": " << info.name
                << " (v" << info.version << ")\n";
    }
    std::cout << "  总计: " << registry.count() << " 个引擎\n\n";
  }

  // 4. 运行审计
  auto audit_report = nexus::algo::AuditEngine::instance().run_all();
  NEXUS_LOG(logger, info, "审计: {}/{} 通过", audit_report.passed, audit_report.total);

  // 5. 写状态文件
  auto& registry = nexus::algo::EngineRegistry::instance();
  auto engine_list = nlohmann::json::array();
  for (const auto& info : registry.list_details()) {
    auto entry = nlohmann::json::object();
    entry["id"]      = info.id;
    entry["name"]    = info.name;
    entry["version"] = info.version;
    engine_list.push_back(entry);
  }

  nexus::ipc::StateFileWriter writer(args.data_dir + "/algo_state.json");
  auto state = nlohmann::json::object();
  state["$schema"]    = "nexus-state-v1";
  state["version"]    = "1.0";
  state["component"]  = "algo";
  state["status"]     = "ready";
  state["pid"]        = nexus::ipc::current_pid();
  state["started_at"] = nexus::ipc::current_iso_timestamp();
  state["updated_at"] = nexus::ipc::current_iso_timestamp();

  auto details = nlohmann::json::object();
  details["total_engines"]    = registry.count();
  details["implemented"]      = 10;
  details["engines"]          = engine_list;
  state["details"] = details;

  auto ws = writer.write(state);
  if (!ws.ok()) {
    NEXUS_LOG(logger, error, "写入状态文件失败: {}", ws.to_string());
  }

  NEXUS_LOG(logger, info, "完成 ({} 引擎)", registry.count());
  return 0;
}
