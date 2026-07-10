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
 * @file main.cpp — quantum.exe
 * @brief 量子系统 — 21 子系统涌现 + 共识 + 信念 + 感知 + 模拟 + 网络
 *
 * 启动: quantum.exe --env .nexus/env.json [--oneshot] [--steps N]
 *
 * 整合自: D:/quantum_solve_engine/ 全部模块
 */

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
#include "nexus/quantum/quantum_core.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

struct CliArgs {
  std::string env_path    = ".nexus/env.json";
  std::string output_path = ".nexus/quantum_state.json";
  std::string data_dir    = ".nexus";
  bool oneshot = false;
  int  n_steps = 50;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc)     args.env_path = argv[++i];
    else if (arg == "--steps" && i+1 < argc) args.n_steps = std::stoi(argv[++i]);
    else if (arg == "--oneshot")            args.oneshot = true;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: quantum.exe --env <path> [--oneshot] [--steps N]\n";
      std::exit(0);
    }
  }
  return args;
}

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  fs::create_directories(args.data_dir + "/logs");

  auto logger = nexus::utils::init_logger("quantum", args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "quantum v{} 启动", "1.0.0");

  // 1. 初始化所有子系统
  nexus::quantum::QuantumEmergence  emergence;
  nexus::quantum::ConsensusPipeline consensus;
  nexus::quantum::BeliefSystem      beliefs;
  nexus::quantum::SelfPerception    perception;
  nexus::quantum::SimulationChamber chamber;
  nexus::quantum::SynapseNetwork    network;
  nexus::quantum::AutonomyLayer     autonomy;
  nexus::quantum::MetaCognition     metacog;
  nexus::quantum::WorldModel        world;
  nexus::quantum::FissionVessel     fission;
  nexus::quantum::IntentDispatch    intent;
  nexus::quantum::TensorBridge      tensor;
  nexus::quantum::HybridReasoner    reasoner;
  nexus::quantum::SystemBridge      sysbridge;

  NEXUS_LOG(logger, info, "量子系统初始化: {} 子系统, Hilbert 维 2^{}",
    emergence.subsystems().size(), emergence.subsystems().size());

  // 2. 运行
  if (args.oneshot) {
    NEXUS_LOG(logger, info, "oneshot 模式 ({} 步)", args.n_steps);
    for (int i = 0; i < args.n_steps; ++i) {
      emergence.tick(0.01);
      beliefs.decay(0.001);
      if ((i + 1) % 10 == 0) {
        NEXUS_LOG(logger, debug, "step={}: ae={:.3f} we={:.3f}",
          i + 1, emergence.global_ae(), emergence.global_we());
      }
    }
  }

  // 3. 状态
  auto ae = emergence.global_ae();
  auto we = emergence.global_we();

  // 测试各子系统
  auto consensus_result = consensus.evaluate({ae, we, 0.5});
  auto decision = autonomy.decide({{"ae", ae}});
  auto meta = metacog.analyze({{"ae", ae}});
  world.update("quantum_system_initialized");

  NEXUS_LOG(logger, info, "完成: ae={:.3f} we={:.3f} consensus={:.2f} mode={}",
    ae, we, consensus_result["consensus"].get<double>(), decision);

  // 4. 写状态文件
  auto state = nlohmann::json::object();
  state["$schema"]    = "nexus-state-v1";
  state["version"]    = "1.0";
  state["component"]  = "quantum";
  state["status"]     = "ready";
  state["pid"]        = nexus::ipc::current_pid();
  state["started_at"] = nexus::ipc::current_iso_timestamp();
  state["updated_at"] = nexus::ipc::current_iso_timestamp();

  auto details = nlohmann::json::object();
  details["emergence"]        = emergence.to_json();
  details["consensus"]        = consensus_result;
  details["beliefs"]          = beliefs.to_json();
  details["synapse_network"]  = network.to_json();
  details["autonomy_level"]   = autonomy.autonomy_level();
  details["world_model"]      = world.to_json();
  details["intent_dispatch"]  = intent.to_json();
  state["details"] = details;

  nexus::ipc::StateFileWriter writer(args.output_path);
  writer.write(state);

  NEXUS_LOG(logger, info, "就绪");
  return 0;
}
