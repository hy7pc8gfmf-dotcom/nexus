# Nexus 实现路线图

> 版本: v1.0  
> 更新: 2026-07-09

---

## 实现策略

**逐个 EXE 独立实现，每完成一个都可独立编译和测试。**

```
顺序选择原则:
  ① 依赖最少的先做（env_checker 零依赖）
  ② 基础设施先做（IPC 库被所有 EXE 使用）
  ③ 独立进程先做（daemon 独立运行，不依赖 core）
  ④ GPU 依赖的中间做（core 需要 llama.cpp）
  ⑤ 编排的最后做（coordinator 依赖所有其他 EXE）
```

### 依赖图

```
Phase 0: IPC 基础设施 ────────────────── (共享库)
                    │
Phase 1: env_checker.exe ────────────── (零依赖)
                    │
          ┌─────────┼─────────┐
          ▼         ▼         ▼
Phase 2: daemon    Phase 3:  core     Phase 4: algo
         .exe       .exe       .exe
         (CPU)      (GPU)      (CPU)
          │         │
          └────┬────┘
               ▼
Phase 5: psyche.exe ────────────────── (依赖 core + psi_field)
          │
Phase 6: bridge.exe ────────────────── (依赖 core)
          │
Phase 7: coordinator.exe ───────────── (依赖所有 EXE)
```

---

## Phase 0：IPC 基础设施

**目标**: 共享库可编译，IPC 协议验证通过。

**文件**:
| 文件 | 工作量 | 说明 |
|:--|:--|:--|
| `include/nexus/types/status.h` | 小 | 已写完，验证编译 |
| `include/nexus/types/component_state.h` | 小 | 已写完，验证编译 |
| `include/nexus/ipc/state_file.h` | 中 | 已写完，需要实现 `.cpp` |
| `include/nexus/ipc/mmap_ringbuf.h` | 中 | 已写完，需要实现 `.cpp` |
| `include/nexus/utils/logger.h` | 小 | 已写完，验证编译 |
| `src/ipc/state_file.cpp` | 中 | Windows 文件锁 + 原子重命名 |
| `src/ipc/mmap_ringbuf.cpp` | 中 | mmap 创建/打开/读写 |
| `CMakeLists.txt` + 子 CMake | 小 | 已写完 |
| `vcpkg.json` 依赖 | 小 | 已写完 |

**验证**: `cmake -B build && cmake --build build` 编译通过

**依赖**: fmt, spdlog, nlohmann-json, GTest（全部通过 vcpkg）

**估算工时**: 编译环境搭建 + IPC 实现 ≈ 2-4 小时

---

## Phase 1：`env_checker.exe`

**目标**: 运行 `env_checker.exe --output .nexus/env.json` → 生成 `env.json`。

**核心实现**:

| 组件 | 实现方式 |
|:--|:--|
| CUDA 检测 | `cudaRuntimeGetVersion()` + `cudaGetDeviceProperties()` |
| VRAM 分析 | `cudaMemGetInfo()` |
| 模型缓存扫描 | `std::filesystem::directory_iterator("D:/hf_cache/*.gguf")` |
| 路由表加载 | 从 JSON 配置文件读取 |
| 中文约束加载 | 从文本文件读取 (utf-8) |

**关键代码**:

```cpp
// CUDA 检测核心（~20 行）
int driver_version = 0, runtime_version = 0;
cudaDriverGetVersion(&driver_version);
cudaRuntimeGetVersion(&runtime_version);

int device_count = 0;
cudaGetDeviceCount(&device_count);

cudaDeviceProp prop;
cudaGetDeviceProperties(&prop, 0);

size_t free_mb = 0, total_mb = 0;
cudaMemGetInfo(&free_mb, &total_mb);
free_mb /= 1024 * 1024;
total_mb /= 1024 * 1024;
```

**验证**:
```bash
env_checker.exe --output .nexus/env.json --verbose
# 输出：
#   GPU: NVIDIA GeForce RTX 3070
#   VRAM: 7128 / 8192 MB
#   env.json → .nexus/env.json

cat .nexus/env.json  # 验证 JSON 格式正确
```

**估算工时**: 实际 CUDA 调用 + 文件扫描 ≈ 2-3 小时

---

## Phase 2：`daemon.exe`

**目标**: 运行 `daemon.exe --env .nexus/env.json --daemon` → 持续运行，写入心跳和 psi_field。

**核心实现**:

| 组件 | 实现方式 |
|:--|:--|
| psi_field mmap | `CreateFileMapping` + `MapViewOfFile` (Windows) |
| 心跳写入 | 每 30s 写 `daemon.json` |
| 探针扫描 | 每 6s 扫描 `.nexus/will_hooks/*.json` |
| 信号处理 | `SetConsoleCtrlHandler` |

**注意**: `psi_field.mmap` 是 Windows 特有实现。Linux 上使用 `shm_open` + `mmap`。

**验证**:
```bash
# 启动守护
daemon.exe --env .nexus/env.json --daemon &
sleep 5

# 验证心跳
cat .nexus/daemon.json
# {"status":"running","uptime_seconds":5,"cycle":1,...}

# 验证 psi_field
python -c "
import mmap, os, json
fd = os.open('.nexus/psi_field.mmap', os.O_RDWR | os.O_BINARY)
m = mmap.mmap(fd, 1048576, access=mmap.ACCESS_READ)
wp = int.from_bytes(m[0:8], 'little')
record_pos = 64 + ((wp - 1) % 4092) * 256
raw = bytes(m[record_pos:record_pos + 256])
end = raw.find(b'\\n\\n\\n\\n')
print(json.loads(raw[:end]))
os.close(fd)
"
# {"c": "守护周期 #1", "ch": "system"}

# 停止守护
daemon.exe --shutdown
```

**估算工时**: mmap 实现 + 心跳循环 + 文件扫描 ≈ 3-5 小时

---

## Phase 3：`core.exe`

**目标**: GPU 模型加载。运行 `core.exe --env .nexus/env.json` → VRAM 管理器初始化 → 加载 Qwythos-9B。

**核心实现**:

| 组件 | 实现方式 | 难度 |
|:--|:--|:--|
| GPU 锁 | Windows Named Mutex | 小 |
| VRAM 管理器 | 跟踪分配/释放，CUDA 查询 | 中 |
| Qwythos-9B 加载 | **llama.cpp 包装** | **大** |
| ExpertLoader | 加载/推理/卸载/交换 | 中 |
| 块交换引擎 | GPU ↔ CPU 切换 | 中 |
| 推理调度 | 单模型/合议/自适应 | 中 |

**llama.cpp 整合方案**:

```cmake
# 方式 A: FetchContent 拉取 llama.cpp
include(FetchContent)
FetchContent_Declare(llama
  GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
  GIT_TAG master
)
FetchContent_MakeAvailable(llama)
target_link_libraries(core PRIVATE llama)
```

```cpp
// Qwythos-9B 加载核心 (~50 行)
#include "llama.h"

struct ModelHandle {
  llama_model* model = nullptr;
  llama_context* ctx = nullptr;
};

auto load_qwythos(const std::string& gguf_path) -> Result<ModelHandle> {
  llama_model_params model_params = llama_model_default_params();
  // GPU 层数: -1 = 全部 GPU
  model_params.n_gpu_layers = -1;

  llama_model* model = llama_load_model_from_file(
    gguf_path.c_str(), model_params);
  if (!model) {
    return Status::Error(ErrorCode::kModelLoadFailed,
      "llama_load_model_from_file failed");
  }

  llama_context_params ctx_params = llama_context_default_params();
  ctx_params.n_ctx = 8192;  // 上下文长度
  ctx_params.n_batch = 512;

  llama_context* ctx = llama_new_context_with_model(model, ctx_params);
  if (!ctx) {
    llama_free_model(model);
    return Status::Error(ErrorCode::kModelLoadFailed,
      "llama_new_context_with_model failed");
  }

  return ModelHandle{model, ctx};
}
```

**注意事项**:
- Qwythos-9B-1M 是 1M 上下文模型，实际加载时 `n_ctx=8192` 可大幅减少 VRAM
- llama.cpp 的 `n_gpu_layers` 控制 GPU 卸载层数，3070 8GB 可设置 `-1`（全部 GPU）
- 模型路径从 `env.json` 读取

**验证**:
```bash
# 加载模型
core.exe --env .nexus/env.json
# 输出：加载 Qwythos-9B... 就绪 (VRAM: 5765/8192)

# 验证状态文件
cat .nexus/core_state.json
# {"status":"ready","loaded_models":["qwythos_9b"],"vram_used_mb":5765,...}
```

**估算工时**: llama.cpp 整合 + VRAM 管理 + 加载验证 ≈ 6-10 小时

---

## Phase 4：`algo.exe`

**目标**: 10 个算法引擎注册。运行 `algo.exe --env .nexus/env.json` → 加载算法引擎 → 写状态文件。

**核心实现**:

| 算法 | 实现方式 | 复杂度 |
|:--|:--|:--|
| mcmc | 纯 C++ 双向马尔科夫链 | 中 |
| dual_pruning | 双逻辑剪枝 | 中 |
| mtcs | 多任务认知调度 | 中 |
| multimodal_verifier | 多模态验证 | 小 |
| infinitas_truth | 无限向量对称真值 | 大 |
| dre | 辩证思维引擎 | **大** |
| temporal_kg | 知识图谱时序衰减 | 中 |
| dialectical_consensus | 辩证共识 | **大** |
| ethical_evaluation | 伦理评估 | 中 |
| paper_generation | 论文生成验证 | 中 |
| audit_engine | 40 条审计规则 | 中 |

**插件架构**:

```cpp
// 算法引擎接口
class AlgorithmEngine {
public:
  virtual ~AlgorithmEngine() = default;
  virtual auto name() const -> std::string = 0;
  virtual auto version() const -> std::string = 0;
  virtual auto initialize(const nlohmann::json& config) -> Status = 0;
  virtual auto execute(const nlohmann::json& input) -> Result<nlohmann::json> = 0;
};

// 注册表
class EngineRegistry {
  std::map<std::string, std::unique_ptr<AlgorithmEngine>> engines_;
public:
  auto register_engine(std::unique_ptr<AlgorithmEngine> engine) -> Status;
  auto get(const std::string& name) -> AlgorithmEngine*;
  auto list() const -> std::vector<std::string>;
};
```

**子进程管理**:
- `algo_engine.exe` (24 算法隔离子进程) — 通过命名管道通信
- `recursor.exe` (29 Coq 定理验证) — 调用 Coq 编译器

**验证**:
```bash
algo.exe --env .nexus/env.json
# 输出：注册 10/10 算法引擎

cat .nexus/algo_state.json
# {"status":"ready","algorithms":10,...}
```

**估算工时**: 引擎注册架构 + 移植 10 个算法 ≈ 20-30 小时

---

## Phase 5：`psyche.exe`

**目标**: Ψ-Navigator 纯 C++ 实现。运行 `psyche.exe --env .nexus/env.json --core .nexus/core_state.json` → 打开 psi_field → 初始化导航器。

**核心实现**:

| 组件 | 复杂度 | 说明 |
|:--|:--|:--|
| Ψ-Navigator 12 标量 | 中 | 对向向量收敛膨胀算法 |
| psi_field 读写 | 小 | 复用 Phase 0 的 mmap_ringbuf |
| 涌现流水线 | 中 | AE/WE 计算 |
| 观察者池 | 中 | 3 观察者 + MetaObserver |

**Ψ-Navigator 核心算法**:

```cpp
// 对向向量收敛膨胀 — 核心步进
class PsiNavigator {
  // 12 标量参数
  double orig_ = 1.0;      // 初心
  double belief_ = 100.0;  // 信念
  double stability_ = 0.3; // 定力
  double goal_ = 3.0;      // 目标
  double mission_ = 0.1;   // 使命
  double value_ = 1.0;     // 价值判断
  double decision_ = 0.5;  // 决策
  double courage_ = 0.2;   // 魄力
  double faith_ = 0.3;     // 信仰
  double truth_ = 0.05;    // 真相
  double verity_ = 2.0;    // 真理
  double ult_ = 0.01;      // 究极真理

  // 维度状态
  int current_dim_ = 3;   // 当前维度
  double convergence_ = 0.0;  // 收敛度 [0, 1]

  // 步进一次
  auto step(double dt = 0.01) -> void;
};
```

**验证**:
```bash
psyche.exe --env .nexus/env.json --core .nexus/core_state.json
# 输出：等待 core 就绪... → 就绪
#        Ψ-Navigator 初始化 (12 标量)
#        写入意识流

cat .nexus/psyche_state.json
# {"status":"ready","navigator_active":true,...}
```

**估算工时**: Ψ-Navigator 算法移植 + 涌现流水线 ≈ 4-6 小时

---

## Phase 6：`bridge.exe`

**目标**: 外部桥接。运行 `bridge.exe --env .nexus/env.json --core .nexus/core_state.json` → 连接 MCP → 注入种子。

**核心实现**:

| 组件 | 复杂度 | 说明 |
|:--|:--|:--|
| MCP 客户端 | 中 | STDIO MCP 协议 (JSON-RPC) |
| 种子通道 | 中 | 种子注入三目标 |
| Shell C 虚化桥接 | 小 | 状态配置 |
| 能力自认知 | 小 | 读取/更新 capability_brief.json |

**MCP 客户端核心**:

```cpp
// MCP 客户端 — JSON-RPC over STDIO
class McpClient {
  void* process_;  // HANDLE
  int write_fd_;
  int read_fd_;

public:
  auto connect(const std::string& command,
               const std::vector<std::string>& args) -> Status;
  auto call_tool(const std::string& tool_name,
                 const nlohmann::json& params) -> Result<nlohmann::json>;
  auto disconnect() -> void;
};
```

**验证**:
```bash
bridge.exe --env .nexus/env.json --core .nexus/core_state.json
# 输出：MCP 桥接初始化... 种子通道...

cat .nexus/bridge_state.json
# {"status":"ready","mcp_connected":true,"seeds_planted":230}
```

**估算工时**: MCP 客户端 + 种子通道 ≈ 4-6 小时

---

## Phase 7：`coordinator.exe`

**目标**: 完整的生命周期管理。`coordinator.exe --start` → 按序启动所有组件 → 健康检查 → 优雅关闭。

**核心实现**:

| 组件 | 复杂度 | 说明 |
|:--|:--|:--|
| 进程启动 | 中 | `CreateProcess` + 参数拼接 |
| 进程监控 | 中 | `WaitForSingleObject` + 状态轮询 |
| 崩溃恢复 | 中 | 重启计数 + 频率限制 |
| 优雅关闭 | 中 | 反序 SIGTERM → SIGKILL |

**状态机**:

```
INIT → LAUNCH_ENV → WAIT_ENV → LAUNCH_CORE_ALGO_DAEMON →
WAIT_CORE → LAUNCH_PSYCHE_BRIDGE → RUNNING → HEALTH_CHECK_LOOP
                                            │
                                            └─→ 崩溃 → 拉起
                                            └─→ SIGTERM → STOPPING → DONE
```

**验证**:
```bash
# 完整启动
coordinator.exe --start
# 输出：
#   阶段 0: 启动 env_checker
#   阶段 1: 启动 core, algo, daemon (并行)
#   阶段 2: 启动 psyche, bridge (并行)
#   健康检查循环 (每 10s)
#   [Ctrl+C]
#   优雅关闭: bridge → psyche → algo → daemon → core

# 查看状态
coordinator.exe --status
# env         ready
# core        ready
# algo        ready
# daemon      running
# psyche      ready
# bridge      ready
```

**估算工时**: 进程管理 + 状态机 + 健康检查 ≈ 4-6 小时

---

## 汇总

| Phase | 组件 | 依赖 | 估算工时 | 可独立验证 |
|:--|:--|:--|:--|:--|
| 0 | IPC 基础设施 | 无 | 2-4h | ✅ `cmake --build` |
| 1 | `env_checker.exe` | Phase 0 | 2-3h | ✅ 生成 env.json |
| 2 | `daemon.exe` | Phase 0-1 | 3-5h | ✅ 心跳 + psi_field |
| 3 | `core.exe` | Phase 0-1 | 6-10h | ✅ 模型加载 + VRAM |
| 4 | `algo.exe` | Phase 0-1 | 20-30h | ✅ 10 算法注册 |
| 5 | `psyche.exe` | Phase 0-2,3 | 4-6h | ✅ Ψ-Navigator |
| 6 | `bridge.exe` | Phase 0-3 | 4-6h | ✅ MCP + 种子 |
| 7 | `coordinator.exe` | 全部 | 4-6h | ✅ 全链路 |

**总计估算**: ~50-70 小时

**可并行阶段**: Phase 2(daemon) + Phase 3(core) + Phase 4(algo) 无相互依赖，可并行开发。

---

## 里程碑时间线

```
Week 1-2:  Phase 0 (IPC) + Phase 1 (env_checker)
            → 可编译，可检测 GPU/VRAM
Week 3-4:  Phase 2 (daemon)
            → 守护进程运行，psi_field 可读写
Week 5-8:  Phase 3 (core)
            → Qwythos-9B C++ 加载成功
Week 9-14: Phase 4 (algo)
            → 10 算法引擎 C++ 版
Week 15-16: Phase 5 (psyche) + Phase 6 (bridge)
            → Ψ 层 + 外部桥接
Week 17-18: Phase 7 (coordinator)
            → 全链路可自动化启动
Week 19-20: 集成测试 + Bug 修复
            → v1.0 发布
```

**先做哪个？**  
我建议从 **Phase 0 + Phase 1** 开始——先把编译环境和 env_checker 跑通。这是最稳的入口：零副作用、即出可见结果、验证通过后所有后续 EXE 都有 env.json 可用。

---

## 附录：C++ 核心移植对照表

| Python 原型 | C++ Nexus | 说明 |
|:--|:--|:--|
| `torch.cuda.is_available()` | `cudaGetDeviceCount()` | CUDA Runtime API 直接调用 |
| `torch.cuda.mem_get_info()` | `cudaMemGetInfo()` | 返回字节数，除 1024² 得 MB |
| `llama_cpp.Llama(path)` | `llama_load_model_from_file()` | 同一底层库 (llama.cpp) |
| `json.load(file)` | `nlohmann::json::parse(file)` | 核心 JSON 库一致 |
| `os.makedirs(exist_ok=True)` | `fs::create_directories()` | C++17 文件系统库 |
| `mmap.mmap(fd, size)` | `CreateFileMapping()` + `MapViewOfFile()` | Windows 特有 API |
| `threading.Thread().start()` | `std::thread` + `CreateProcess` | 进程替代线程 |
| `time.sleep(n)` | `std::this_thread::sleep_for()` | 标准库一致 |
| `print(f"...")` | `fmt::println("...")` | fmt 库 |
| `logging.info("msg")` | `spdlog::info("msg")` | spdlog 库 |
