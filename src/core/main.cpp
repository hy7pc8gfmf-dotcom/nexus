/**
 * @file main.cpp — core.exe
 * @brief 核心推理引擎 — VRAM 管理 + ExpertLoader + 推理调度
 *
 * 启动: core.exe --env .nexus/env.json [--model <gguf_path>] [--skip-models]
 *
 * 这是唯一持有 GPU 锁的进程。
 * 启动时读取 env.json 中的 VRAM 预算，加载 Qwythos-9B 模型。
 */

#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "nexus/ipc/state_file.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"
#include "nexus/core/vram_manager.h"
#include "nexus/core/expert_loader.h"

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════
// 命令行参数
// ═══════════════════════════════════════════════════════════════════

struct CliArgs {
  std::string env_path     = ".nexus/env.json";
  std::string data_dir     = ".nexus";
  std::string model_path;
  bool skip_models = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--env" && i + 1 < argc)       args.env_path   = argv[++i];
    else if (arg == "--model" && i+1 < argc)  args.model_path = argv[++i];
    else if (arg == "--skip-models")          args.skip_models = true;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: core.exe --env <path> [--model <gguf>] [--skip-models]\n";
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  fs::create_directories(args.data_dir + "/logs");

  auto logger = nexus::utils::init_logger("core", args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "core v{} 启动", "1.0.0");

  // 1. 读取 env.json
  std::string model_path = args.model_path;
  {
    nexus::ipc::StateFileReader env_reader(args.env_path);
    auto env = env_reader.read();
    if (env.ok()) {
      // 如果命令行未指定模型，从 env.json 读取 Qwythos 路径
      if (model_path.empty()) {
        auto models = env.value()["details"]["models"];
        if (models.is_array()) {
          for (const auto& m : models) {
            auto id = m.value("id", std::string(""));
            if (id == "qwythos_9b") {
              model_path = m.value("path", std::string(""));
              break;
            }
          }
        }
      }
      NEXUS_LOG(logger, info, "env.json 已读取");
    } else {
      NEXUS_LOG(logger, warn, "env.json 不可用 (降级运行)");
    }
  }

  // 2. 获取 GPU 锁
  nexus::core::GpuLock gpu_lock;
  auto lock_status = gpu_lock.acquire(5000);
  if (!lock_status.ok()) {
    NEXUS_LOG(logger, warn, "GPU 锁获取失败: {}", lock_status.to_string());
    NEXUS_LOG(logger, warn, "降级到 CPU-only 模式");
  } else {
    NEXUS_LOG(logger, info, "GPU 锁已获取");
  }

  // 3. 初始化 VRAM 管理器
  nexus::core::VramManager vram;
  auto vram_status = vram.initialize(1000);
  if (!vram_status.ok()) {
    NEXUS_LOG(logger, warn, "VRAM 初始化失败: {}", vram_status.to_string());
  } else {
    auto s = vram.status();
    NEXUS_LOG(logger, info, "VRAM: {} / {} MB (预算: {} MB)",
      s.free_mb(), s.total_mb(), vram.budget_mb());
  }

  // 4. 加载模型
  bool model_loaded = false;
  if (!args.skip_models && !model_path.empty()) {
    if (fs::exists(model_path)) {
      NEXUS_LOG(logger, info, "加载模型: {}", model_path);

      nexus::core::ExpertLoader loader;
      nexus::core::InferParams params;
      params.n_gpu_layers = -1;   // 全部 GPU
      params.n_ctx        = 8192;

      auto load_result = loader.load(model_path, params);
      if (load_result.ok()) {
        model_loaded = true;

        // 记录 VRAM 占用
        auto model_size = fs::file_size(model_path) / (1024 * 1024);
        nexus::core::ModelVramRequirement req;
        req.model_id = "qwythos_9b";
        req.vram_mb  = static_cast<int64_t>(model_size);
        vram.record_load(req);

        // 测试推理（元信息）
        auto test = loader.infer("describe the model");
        if (test.ok()) {
          NEXUS_LOG(logger, info, "模型信息: {}", test.value());
        }

        NEXUS_LOG(logger, info, "模型加载完成 ({} MB)", model_size);
      } else {
        NEXUS_LOG(logger, error, "模型加载失败: {}", load_result.to_string());
      }
    } else {
      NEXUS_LOG(logger, warn, "模型文件不存在: {}", model_path);
    }
  } else if (!args.skip_models && model_path.empty()) {
    NEXUS_LOG(logger, warn, "未指定模型路径 (使用 --model 或提供 env.json)");
  }

  // 5. 写状态文件
  auto vram_s = vram.status();
  nexus::ipc::StateFileWriter writer(args.data_dir + "/core_state.json");
  auto state = nlohmann::json::object();
  state["$schema"]     = "nexus-state-v1";
  state["version"]     = "1.0";
  state["component"]   = "core";
  state["status"]      = model_loaded ? "ready" : "degraded";
  state["pid"]         = nexus::ipc::current_pid();
  state["started_at"]  = nexus::ipc::current_iso_timestamp();
  state["updated_at"]  = nexus::ipc::current_iso_timestamp();

  auto details = nlohmann::json::object();
  details["vram_total_mb"]  = vram_s.total_mb();
  details["vram_free_mb"]   = vram_s.free_mb();
  details["vram_used_mb"]   = vram_s.used_mb();
  details["gpu_lock_held"]  = gpu_lock.is_held();
  details["model_loaded"]   = model_loaded;
  if (!model_path.empty()) {
    details["model_path"] = model_path;
  }
  state["details"] = details;

  auto ws = writer.write(state);
  if (!ws.ok()) {
    NEXUS_LOG(logger, error, "写入状态文件失败: {}", ws.to_string());
  }

  NEXUS_LOG(logger, info, "就绪");
  return 0;
}
