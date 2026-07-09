/**
 * @file main.cpp — holographic.exe
 * @brief 全息系统 — 语义场 + 概念路径 + 流桥接
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
#include "nexus/holographic/holographic_core.h"

namespace fs = std::filesystem;

struct CliArgs {
  std::string output_path = ".nexus/holographic_state.json";
  std::string data_dir   = ".nexus";
  std::string topic;
  bool oneshot = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--topic" && i+1 < argc)   args.topic = argv[++i];
    else if (arg == "--oneshot")          args.oneshot = true;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: holographic.exe [--oneshot] [--topic <name>]\n";
      std::exit(0);
    }
  }
  return args;
}

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  fs::create_directories(args.data_dir + "/logs");

  auto logger = nexus::utils::init_logger("holographic", args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "holographic v{} 启动", "1.0.0");

  nexus::holographic::SemanticField field;
  NEXUS_LOG(logger, info, "语义场: {} 热点, {} 路径",
    field.stats()["hotspots"].get<int>(),
    field.stats()["pathways"].get<int>());

  nexus::holographic::ConceptPathway pathway(&field);
  nexus::holographic::FlowBridge bridge(&field);

  if (args.oneshot && !args.topic.empty()) {
    auto q = field.query(args.topic);
    NEXUS_LOG(logger, info, "查询 '{}': {}", args.topic,
      q["found"].get<bool>() ? "找到" : "未找到");

    auto sentences = bridge.generate(args.topic);
    for (const auto& s : sentences) {
      NEXUS_LOG(logger, info, "  [{}] {}", s.template_type, s.sentence);
    }
  }

  auto state = nlohmann::json::object();
  state["$schema"]    = "nexus-state-v1";
  state["version"]    = "1.0";
  state["component"]  = "holographic";
  state["status"]     = "ready";
  state["pid"]        = nexus::ipc::current_pid();
  state["started_at"] = nexus::ipc::current_iso_timestamp();
  state["updated_at"] = nexus::ipc::current_iso_timestamp();

  auto details = nlohmann::json::object();
  details["semantic_field"] = field.stats();
  details["flow_bridge"] = bridge.stats();
  state["details"] = details;

  nexus::ipc::StateFileWriter writer(args.output_path);
  writer.write(state);

  NEXUS_LOG(logger, info, "就绪 ({} 热点)",
    field.stats()["hotspots"].get<int>());
  return 0;
}
