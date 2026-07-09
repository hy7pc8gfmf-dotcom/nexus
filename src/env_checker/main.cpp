/**
 * @file main.cpp — env_checker.exe
 * @brief 环境验证器：CUDA/GPU 检测 + VRAM 分析 + 模型缓存扫描
 *
 * 启动: env_checker.exe --output .nexus/env.json [--verbose] [--dry-run]
 * 输出: .nexus/env.json (供所有其他 EXE 读取)
 *
 * 零副作用，可安全反复执行。
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "nexus/ipc/state_file.h"
#include "nexus/types/component_state.h"
#include "nexus/utils/logger.h"

// ── CUDA Runtime API ──
#include <cuda_runtime.h>
#include <driver_types.h>

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════
// 命令行参数
// ═══════════════════════════════════════════════════════════════════

struct CliArgs {
  std::string output_path = ".nexus/env.json";
  std::string model_cache_path = "D:/hf_cache";
  std::string data_dir = ".nexus";
  bool verbose = false;
  bool dry_run = false;
};

static auto parse_args(int argc, char* argv[]) -> CliArgs {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--output" && i + 1 < argc)       args.output_path = argv[++i];
    else if (arg == "--model-cache" && i+1 < argc) args.model_cache_path = argv[++i];
    else if (arg == "--data-dir" && i+1 < argc)   args.data_dir = argv[++i];
    else if (arg == "--verbose")                  args.verbose = true;
    else if (arg == "--dry-run")                  args.dry_run = true;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "用法: env_checker.exe --output <path> [options]\n"
                << "  --output <path>     输出 env.json 路径\n"
                << "  --model-cache <path> 模型缓存目录 (默认 D:/hf_cache)\n"
                << "  --data-dir <path>    数据目录 (默认 .nexus)\n"
                << "  --verbose           详细信息\n"
                << "  --dry-run           干运行，仅打印不写入\n"
                << "  --help              -h  帮助\n";
      std::exit(0);
    }
  }
  return args;
}

// ═══════════════════════════════════════════════════════════════════
// GPU 信息
// ═══════════════════════════════════════════════════════════════════

struct GpuInfo {
  bool available = false;
  int  device_count = 0;
  std::string name;
  int  compute_capability_major = 0;
  int  compute_capability_minor = 0;
  int  driver_version = 0;
  int  runtime_version = 0;
  size_t vram_total = 0;  // bytes
  size_t vram_free  = 0;  // bytes

  auto vram_total_mb() const -> int {
    return static_cast<int>(vram_total / (1024 * 1024));
  }
  auto vram_free_mb() const -> int {
    return static_cast<int>(vram_free / (1024 * 1024));
  }
  auto compute_capability() const -> std::string {
    return std::to_string(compute_capability_major) + "." +
           std::to_string(compute_capability_minor);
  }
};

static auto detect_gpu() -> GpuInfo {
  GpuInfo info;

  // 获取驱动版本
  cudaError_t err = cudaDriverGetVersion(&info.driver_version);
  if (err != cudaSuccess) return info;  // CUDA 不可用

  err = cudaRuntimeGetVersion(&info.runtime_version);
  if (err != cudaSuccess) return info;

  err = cudaGetDeviceCount(&info.device_count);
  if (err != cudaSuccess || info.device_count == 0) return info;

  // 查询主 GPU (device 0)
  cudaDeviceProp prop;
  err = cudaGetDeviceProperties(&prop, 0);
  if (err != cudaSuccess) return info;

  info.available = true;
  info.name = prop.name;
  info.compute_capability_major = prop.major;
  info.compute_capability_minor = prop.minor;
  info.vram_total = prop.totalGlobalMem;

  // 查询当前可用 VRAM
  err = cudaMemGetInfo(&info.vram_free, &info.vram_total);
  if (err == cudaSuccess) {
    info.vram_total = info.vram_total;  // cudaMemGetInfo 也会返回总量
  } else {
    info.vram_free = info.vram_total;  // fallback: 假设全部可用
  }

  return info;
}

// ═══════════════════════════════════════════════════════════════════
// 模型缓存扫描
// ═══════════════════════════════════════════════════════════════════

struct ModelEntry {
  std::string id;
  std::string path;
  size_t size_bytes = 0;
  bool cached = false;

  auto size_mb() const -> int {
    return static_cast<int>(size_bytes / (1024 * 1024));
  }
};

static auto scan_model_cache(const std::string& cache_dir) -> std::vector<ModelEntry> {
  std::vector<ModelEntry> models;

  if (!fs::exists(cache_dir)) return models;

  try {
    for (const auto& entry : fs::directory_iterator(cache_dir)) {
      if (!entry.is_regular_file()) continue;
      auto path = entry.path();
      auto ext = path.extension().string();

      // 只关注 GGUF 文件
      if (ext != ".gguf") continue;

      auto filename = path.stem().string();
      auto size = entry.file_size();

      // 从文件名推测模型 ID
      std::string model_id;
      if (filename.find("qwythos") != std::string::npos ||
          filename.find("qwy") != std::string::npos) {
        model_id = "qwythos_9b";
      } else if (filename.find("grm") != std::string::npos) {
        model_id = "grm2";
      } else if (filename.find("vibethinker") != std::string::npos) {
        model_id = "vibethinker";
      } else if (filename.find("minicpm") != std::string::npos) {
        model_id = "minicpm_v";
      } else if (filename.find("hy") != std::string::npos &&
                 filename.find("mt") != std::string::npos) {
        model_id = "hy_mt2";
      } else {
        model_id = filename.substr(0, 32);  // 截断
      }

      models.push_back({model_id, path.string(), size, true});
    }
  } catch (const std::exception&) {
    // 扫描失败则返回空列表
  }

  return models;
}

// ═══════════════════════════════════════════════════════════════════
// VRAM 预算计算
// ═══════════════════════════════════════════════════════════════════

struct VramBudget {
  int total_mb = 0;
  int free_mb = 0;
  int reserved_mb = 1000;  // 预留用于系统 + 其他进程
  int available_mb = 0;    // 神经系统的可用预算 = free - reserved
  std::string recommendation;
};

static auto calculate_vram_budget(const GpuInfo& gpu) -> VramBudget {
  VramBudget budget;
  if (!gpu.available) {
    budget.recommendation = "no_gpu";
    return budget;
  }

  budget.total_mb = gpu.vram_total_mb();
  budget.free_mb  = gpu.vram_free_mb();

  // 预留一定量供系统使用
  if (budget.free_mb >= 2000) {
    budget.reserved_mb = 1000;
  } else if (budget.free_mb >= 1000) {
    budget.reserved_mb = 500;
  } else {
    budget.reserved_mb = 200;
  }

  budget.available_mb = budget.free_mb - budget.reserved_mb;
  if (budget.available_mb < 0) budget.available_mb = 0;

  // 推荐策略
  if (budget.available_mb >= 6000) {
    budget.recommendation = "full";
  } else if (budget.available_mb >= 4000) {
    budget.recommendation = "reduced";
  } else if (budget.available_mb >= 2000) {
    budget.recommendation = "minimal";
  } else {
    budget.recommendation = "cpu_only";
  }

  return budget;
}

// ═══════════════════════════════════════════════════════════════════
// 路由表（内嵌默认配置）
// ═══════════════════════════════════════════════════════════════════

static auto default_routes() -> nlohmann::json {
  return nlohmann::json::array({
    {{"task_type", "reasoning"},  {"model_id", "qwythos_9b"},     {"priority", 1}},
    {{"task_type", "quick_local"},{"model_id", "grm2"},          {"priority", 2}},
    {{"task_type", "translate"},  {"model_id", "hy_mt2"},        {"priority", 3}},
    {{"task_type", "vision"},     {"model_id", "minicpm_v"},     {"priority", 4}},
    {{"task_type", "dialectic"},  {"model_id", "qwythos_9b"},    {"priority", 1}},
    {{"task_type", "debug"},      {"model_id", "qwythos_9b"},    {"priority", 2}},
  });
}

// ═══════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════

auto main(int argc, char* argv[]) -> int {
  auto args = parse_args(argc, argv);

  // 确保数据目录存在
  fs::create_directories(args.data_dir);

  auto logger = nexus::utils::init_logger("env_checker",
    args.data_dir + "/logs");
  NEXUS_LOG(logger, info, "env_checker v{}", "1.0.0");

  // 1. GPU 检测
  auto gpu = detect_gpu();
  if (!gpu.available) {
    NEXUS_LOG(logger, warn, "GPU 不可用，将使用 CPU-only 模式");
  } else {
    NEXUS_LOG(logger, info,
      "GPU: {} | CUDA {}.{} | VRAM: {} MB 空闲 / {} MB 总量",
      gpu.name,
      gpu.compute_capability_major,
      gpu.compute_capability_minor,
      gpu.vram_free_mb(),
      gpu.vram_total_mb());
  }

  // 2. VRAM 预算
  auto budget = calculate_vram_budget(gpu);
  NEXUS_LOG(logger, info, "VRAM 预算: {} MB (策略: {})",
    budget.available_mb, budget.recommendation);

  // 3. 模型缓存扫描
  auto models = scan_model_cache(args.model_cache_path);
  NEXUS_LOG(logger, info, "模型缓存: {} 个 GGUF (路径: {})",
    models.size(), args.model_cache_path);
  if (args.verbose) {
    for (const auto& m : models) {
      NEXUS_LOG(logger, debug, "  模型: {} ({}) - {} MB",
        m.id, m.path, m.size_mb());
    }
  }

  // 4. 构建 env.json
  nlohmann::json model_list = nlohmann::json::array();
  for (const auto& m : models) {
    model_list.push_back({
      {"id",     m.id},
      {"path",   m.path},
      {"size_mb", m.size_mb()},
      {"cached",  m.cached},
    });
  }

  // 如果找不到 Qwythos-9B 但 GPU 可用，用默认路径兜底
  bool has_qwythos = false;
  for (const auto& m : models) {
    if (m.id == "qwythos_9b") { has_qwythos = true; break; }
  }
  if (!has_qwythos && gpu.available) {
    std::string default_path = args.model_cache_path + "/qwythos-9b-1m.Q4_K_M.gguf";
    model_list.push_back({
      {"id", "qwythos_9b"},
      {"path", default_path},
      {"size_mb", 5765},
      {"cached", false},
    });
    NEXUS_LOG(logger, warn, "Qwythos-9B 未在缓存中找到，使用默认路径");
  }

  // 组装状态
  std::string status_str = gpu.available ? "ready" : "degraded";

  nlohmann::json env_json = {
    {"$schema", "nexus-state-v1"},
    {"version", "1.0"},
    {"component", "env"},
    {"status", status_str},
    {"pid", nexus::ipc::current_pid()},
    {"started_at", nexus::ipc::current_iso_timestamp()},
    {"updated_at", nexus::ipc::current_iso_timestamp()},
    {"details", {
      {"platform", {
        {"os", "Windows"},
        {"cuda_version", std::to_string(gpu.runtime_version)},
      }},
      {"gpu", {
        {"available", gpu.available},
        {"name", gpu.name},
        {"compute_capability", gpu.compute_capability()},
        {"driver_version", gpu.driver_version},
        {"runtime_version", gpu.runtime_version},
        {"vram_total_mb", gpu.vram_total_mb()},
        {"vram_free_mb", gpu.vram_free_mb()},
        {"budget_mb", budget.available_mb},
        {"reserved_mb", budget.reserved_mb},
      }},
      {"models", model_list},
      {"routes", default_routes()},
      {"recommended_strategy", budget.recommendation},
    }},
  };

  // 5. 写入或打印
  if (args.dry_run) {
    std::cout << env_json.dump(2) << std::endl;
    NEXUS_LOG(logger, info, "dry-run 模式: 未写入文件");
  } else {
    nexus::ipc::StateFileWriter writer(args.output_path);
    auto status = writer.write(env_json);
    if (!status.ok()) {
      NEXUS_LOG(logger, error, "写入 {} 失败: {}",
        args.output_path, status.to_string());
      return 1;
    }
    NEXUS_LOG(logger, info, "→ {}", args.output_path);
  }

  // 6. 输出摘要
  if (args.verbose) {
    std::cout << "\n=== 环境摘要 ===\n";
    if (gpu.available) {
      std::cout << "CUDA 驱动: " << gpu.driver_version << "\n";
      std::cout << "CUDA 运行时: " << gpu.runtime_version << "\n";
      std::cout << "GPU: " << gpu.name
                << " (SM " << gpu.compute_capability() << ")\n";
      std::cout << "VRAM: " << gpu.vram_free_mb() << " / "
                << gpu.vram_total_mb() << " MB\n";
      std::cout << "预算: " << budget.available_mb << " MB (策略: "
                << budget.recommendation << ")\n";
    } else {
      std::cout << "GPU: 不可用 (CPU-only)\n";
    }
    std::cout << "模型: " << models.size() << " 个\n";
    for (const auto& m : models) {
      std::cout << "  " << m.id << ": " << m.size_mb() << " MB\n";
    }
  }

  NEXUS_LOG(logger, info, "完成");
  return 0;
}
