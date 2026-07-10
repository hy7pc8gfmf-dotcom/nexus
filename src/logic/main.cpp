/**
 * @file main.cpp — logic.exe
 * @brief 逻辑求解引擎 — 种子空间 + 证明搜索 + 逻辑推理
 *
 * 启动: logic.exe --seed-bank .nexus/seed_bank.jsonl [--oneshot]
 *
 * 使用 32,629 颗种子进行逻辑推理和证明搜索。
 */

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "nexus/ipc/state_file.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"
#include "nexus/logic/logic_core.h"

namespace fs = std::filesystem;

struct CliArgs {
  std::string seed_bank  = ".nexus/seed_bank.jsonl";
  std::string output_path = ".nexus/logic_state.json";
  std::string data_dir   = ".nexus";
  bool oneshot = false;
  bool import_seeds = false;
  std::string import_source;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--seed-bank" && i+1 < argc)  args.seed_bank = argv[++i];
    else if (arg == "--oneshot")            args.oneshot = true;
    else if (arg == "--import" && i+1 < argc) { args.import_seeds = true; args.import_source = argv[++i]; }
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: logic.exe [--seed-bank <path>] [--oneshot] [--import <source.json>]\n";
      std::exit(0);
    }
  }
  return args;
}

// ── 种子导入 (因 json 库限制, 种子数据需在首次部署时用外部工具生成) ──
// 参见 port_seed_bank.py (工具脚本, 不在构建链中)
static auto import_seed_bank(const std::string& source_path,
                              const std::string& output_dir,
                              nexus::utils::Logger* logger) -> bool {
  NEXUS_LOG(logger, error, "种子导入需使用 Python 工具: port_seed_bank.py");
  NEXUS_LOG(logger, info, "请运行: python port_seed_bank.py");
  return false;
}
      auto& seed = it.value();

      // 找域 — 使用 domain_tag 字段
      auto domain_tag = seed.value("domain_tag", std::string(""));

      nlohmann::json record;
      record["name"]        = name;
      record["intensity"]   = seed.value("intensity", 0);
      record["source"]      = seed.value("source", "");
      record["type"]        = seed.value("type", "");
      record["domain_tag"]  = seed.value("domain_tag", "");
      record["step_detail"] = seed.value("step_detail", "");

      ofs << record.dump() << "\n";
      written++;
    }

    // 写索引
    nlohmann::json idx;
    idx["version"]     = data.value("version", 2);
    idx["total_seeds"] = total;
    idx["dimension"]   = dimension;
    idx["domains"]     = nlohmann::json::array();

    std::ofstream idx_ofs(output_dir + "/seed_bank_index.json");
    idx_ofs << idx.dump(2);

    NEXUS_LOG(logger, info, "种子导入完成: {} 颗, {} 域, {}D", written,
      static_cast<int>(domain_map.size()), dimension);
    return true;

  } catch (const std::exception& e) {
    NEXUS_LOG(logger, error, "导入失败: {}", e.what());
    return false;
  }
}

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  fs::create_directories(args.data_dir + "/logs");

  auto logger = nexus::utils::init_logger("logic", args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "logic v{} 启动", "1.0.0");

  // 0. 种子导入模式 (替换 port_seed_bank.py)
  if (args.import_seeds) {
    fs::create_directories(args.data_dir);
    import_seed_bank(args.import_source, args.data_dir, logger.get());
    return 0;
  }

  // 1. 加载种子空间
  nexus::logic::SeedSpace space;
  auto load_status = space.load(args.seed_bank);
  if (!load_status.ok()) {
    NEXUS_LOG(logger, warn, "种子加载失败: {}", load_status.to_string());
  } else {
    auto s = space.stats();
    NEXUS_LOG(logger, info, "种子空间: {} 颗, {} 域",
      s["total_seeds"].get<int>(),
      static_cast<int>(s["domains"].size()));
  }

  // 2. 初始化证明搜索
  nexus::logic::ProofSearch searcher(&space);
  nexus::logic::LogicReasoner reasoner;

  // 3. 执行
  if (args.oneshot) {
    // 测试证明搜索
    auto proof = searcher.search("consciousness", 3);
    NEXUS_LOG(logger, info, "证明搜索: {} 步", proof.size());

    // 测试逻辑推理
    auto eval = reasoner.evaluate("consciousness → self_awareness");
    NEXUS_LOG(logger, info, "逻辑评估: {} 特征", static_cast<int>(eval.size()));
  }

  auto stats = space.stats();
  auto pstats = searcher.stats();

  // 4. 写状态
  auto state = nlohmann::json::object();
  state["$schema"]    = "nexus-state-v1";
  state["version"]    = "1.0";
  state["component"]  = "logic";
  state["status"]     = "ready";
  state["pid"]        = nexus::ipc::current_pid();
  state["started_at"] = nexus::ipc::current_iso_timestamp();
  state["updated_at"] = nexus::ipc::current_iso_timestamp();

  auto details = nlohmann::json::object();
  details["seed_space"] = stats;
  details["proof_search"] = pstats;
  state["details"] = details;

  nexus::ipc::StateFileWriter writer(args.output_path);
  writer.write(state);

  NEXUS_LOG(logger, info, "就绪 ({} 种子)", stats["total_seeds"].get<int>());
  return 0;
}
