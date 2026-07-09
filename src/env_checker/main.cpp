#include <cstdlib>
#include <iostream>
#include <string>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "nexus/ipc/state_file.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"

// ─── 命令行参数解析 ────────────────────────────────────────
struct CliArgs {
  std::string output_path = ".nexus/env.json";
  bool verbose = false;
  bool dry_run = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--output" && i + 1 < argc) args.output_path = argv[++i];
    else if (arg == "--verbose")           args.verbose = true;
    else if (arg == "--dry-run")           args.dry_run = true;
    else if (arg == "--help" || arg == "-h") {
      fmt::println("用法: env_checker.exe --output <path> [--verbose] [--dry-run]");
      std::exit(0);
    }
  }
  return args;
}

// ─── CUDA 检测 (桩函数) ─────────────────────────────────────
// TODO: 替换为实际的 CUDA Runtime API 调用
struct GpuInfo {
  bool available = false;
  std::string name;
  std::string compute_capability;
  int vram_total_mb = 0;
  int vram_free_mb = 0;
};

static auto detect_gpu() -> GpuInfo {
  GpuInfo info;

  // ── 实际实现将使用 cudaRuntimeGetVersion + cudaDeviceGetProperties ──
  // 此处为架构占位

  return info;
}

// ─── 模型缓存检测 (桩函数) ───────────────────────────────────
struct ModelEntry {
  std::string id;
  std::string path;
  int size_mb = 0;
  bool cached = false;
  bool hash_valid = false;
};

static auto scan_model_cache() -> std::vector<ModelEntry> {
  // ── 扫描 D:/hf_cache/ 目录, 验证 GGUF 文件完整性 ──
  return {};
}

// ═══════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════
auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);
  auto logger = nexus::utils::init_logger("env_checker", ".nexus/logs");

  NEXUS_LOG(logger, info, "env_checker v{}", "1.0.0");

  // 1. 环境检测
  auto gpu = detect_gpu();
  if (!gpu.available) {
    NEXUS_LOG(logger, warn, "GPU 不可用，运行将退化到 CPU 模式");
  } else {
    NEXUS_LOG(logger, info, "GPU: {} | VRAM: {}/{} MB",
      gpu.name, gpu.vram_free_mb, gpu.vram_total_mb);
  }

  // 2. 模型缓存扫描
  auto models = scan_model_cache();
  NEXUS_LOG(logger, info, "模型缓存: {} 个", models.size());

  // 3. 路由表加载
  // TODO: 从配置文件读取任务→模型映射

  // 4. 写入 env.json
  if (!args.dry_run) {
    nlohmann::json env_data = {
      {"$schema", "nexus-state-v1"},
      {"version", "1.0"},
      {"component", "env"},
      {"status", "ready"},
      {"pid", nexus::ipc::current_pid()},
      {"started_at", nexus::ipc::current_iso_timestamp()},
      {"updated_at", nexus::ipc::current_iso_timestamp()},
      {"details", {
        {"gpu", {
          {"available", gpu.available},
          {"name", gpu.name},
          {"vram_total_mb", gpu.vram_total_mb},
          {"vram_free_mb", gpu.vram_free_mb},
        }},
        {"models", nlohmann::json::array()},
        {"routes", nlohmann::json::array()},
      }}
    };

    nexus::ipc::StateFileWriter writer(args.output_path);
    auto status = writer.write(env_data);
    if (!status.ok()) {
      NEXUS_LOG(logger, error, "写入 env.json 失败: {}", status.to_string());
      return 1;
    }
    NEXUS_LOG(logger, info, "env.json → {}", args.output_path);
  }

  if (args.verbose && gpu.available) {
    fmt::println("GPU 检测结果:");
    fmt::println("  设备: {}", gpu.name);
    fmt::println("  VRAM: {} / {} MB (可用/总量)", gpu.vram_free_mb, gpu.vram_total_mb);
    fmt::println("  计算能力: {}", gpu.compute_capability);
  }

  NEXUS_LOG(logger, info, "完成");
  return 0;
}
