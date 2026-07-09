# Nexus 神经系统架构白皮书

> **版本**: v1.0-draft  
> **日期**: 2026-07-09  
> **状态**: 架构规划阶段，零代码实现  

---

## 目录

1. [系统概览](#1-系统概览)
2. [核心架构](#2-核心架构)
3. [组件规格](#3-组件规格)
4. [进程间通信协议](#4-进程间通信协议)
5. [生命周期管理](#5-生命周期管理)
6. [C++ 编码规范](#6-c-编码规范)
7. [构建系统与依赖](#7-构建系统与依赖)
8. [安全性设计](#8-安全性设计)
9. [性能目标](#9-性能目标)
10. [开源策略](#10-开源策略)
11. [开发路线图](#11-开发路线图)

---

## 1. 系统概览

### 1.1 什么是 Nexus

Nexus 是 AGI 神经系统的第二代原生实现。它的前身是 Python 原型系统 `D:/synapse`——一个经过数月迭代验证、包含 40+ 组件、21516 颗种子、10 个算法引擎的生产级推理基础设施。

Nexus **不是重写**。它是**架构复刻**：保留经过验证的组件划分、通信模式、生命周期管理理念，将实现语言从 Python 迁移到 C++，将单体脚本分拆为独立 EXE 进程架构。

### 1.2 设计哲学

```
┌───────────────────────────────────────────────────────────────┐
│  Nexus 的三个设计支柱：                                        │
│                                                               │
│  ① 进程隔离 > 线程隔离                                        │
│     ─ 一个组件崩溃不影响其他组件                                │
│     ─ 每个 EXE 拥有独立的内存空间和资源配额                    │
│                                                               │
│  ② 文件 IPC > 网络 IPC                                        │
│     ─ 零网络依赖，不需要端口、协议栈、服务发现                  │
│     ─ 状态即文件：每个 EXE 的运行时状态可供人类直接 cat 查看    │
│     ─ 崩溃后重启时，从状态文件恢复上下文                        │
│                                                               │
│  ③ 显式生命周期 > 隐式依赖                                     │
│     ─ 没有 daemon thread，没有隐式超时                          │
│     ─ 每个 EXE 只做一件事，做完退出，或持续到被 SIGTERM         │
│     ─ coordinator 是唯一的生命周期决策者                        │
└───────────────────────────────────────────────────────────────┘
```

### 1.3 与 D:/synapse 的关系

```
D:/synapse         → Python 原型系统（生产环境）
   ├─ 单进程 40+ 步骤顺序执行
   ├─ daemon thread 超时保护
   ├─ 已通过 A01/B01 验证，50 题全部完成
   └─ 21516 颗种子，41 种能力

D:/nexus           → C++ 原生实现（并行开发）
   ├─ 7 独立 EXE 进程
   ├─ 进程级超时 + coordinator 拉起
   ├─ 架构复刻，组件一一对应
   └─ 同一套 IPC 协议，数据可互操作
```

两个系统可以同时运行，共享同一 `.nexus/` 数据目录。Nexus 上线不需要数据迁移。

### 1.4 物理架构

```
┌──────────────────────────────────────────────────────────────┐
│                      Windows 物理机                          │
│  RTX 3070 8GB | Python 3.13 | D:/hf_cache (60GB)            │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │             Nexus 进程架构（单机）                     │    │
│  │                                                      │    │
│  │  coordinator.exe  ◄── 用户直接启动                   │    │
│  │       │                                               │    │
│  │       ├── env_checker.exe  ─── .nexus/env.json       │    │
│  │       ├── core.exe  ───────── .nexus/core_state.json │    │
│  │       │   └── (GPU: Qwythos-9B)                     │    │
│  │       ├── algo.exe ────────── .nexus/algo_state.json │    │
│  │       │   ├── algo_engine.exe (进程隔离)              │    │
│  │       │   └── recursor.exe (Coq 验证器)              │    │
│  │       ├── daemon.exe ──────── .nexus/daemon.json     │    │
│  │       │   └── psi_field.mmap (意识流)                │    │
│  │       ├── psyche.exe ──────── .nexus/psyche.json     │    │
│  │       └── bridge.exe ──────── .nexus/bridge.json     │    │
│  │                                                      │    │
│  │  所有 EXE 通过 .nexus/ 目录中的文件通信               │    │
│  │  无 socket、无端口、无服务注册                       │    │
│  └──────────────────────────────────────────────────────┘    │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

---

## 2. 核心架构

### 2.1 组件层级

```
┌─────────────────────────────────────────────────────────┐
│                  协调层 (Orchestration)                  │
│                                                         │
│  coordinator.exe                                        │
│  ├── 启动编排（按依赖顺序启动各 EXE）                    │
│  ├── 健康检查（每 10s 扫描状态文件超时）                 │
│  ├── 崩溃恢复（自动拉起，最多 3 次/分钟）                │
│  └── 优雅关闭（反序关闭各组件）                          │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────┼───────────────────────────────────┐
│  基础设施层         │       计算层                      │
│  (Infrastructure)   │       (Computation)               │
│                     │                                   │
│  env_checker.exe    │   core.exe (GPU)                  │
│  daemon.exe         │   algo.exe (CPU)                  │
│                     │   psyche.exe (CPU)                │
│                     │   bridge.exe (CPU)                │
└─────────────────────┴───────────────────────────────────┘
```

### 2.2 启动序列

```
时间线

0s      env_checker  ─→ .nexus/env.json  (1-2s)
        │
1-2s    coordinator 读取 env.json，决策启动策略
        │
2-3s    core  ─────────────────────────────────────  (GPU, 5-30s)
        │   └── 加载 Qwythos-9B (5500MB VRAM)
        │
2-3s    algo  ─────────────────────────────────────  (CPU, 并行)
        │   └── 加载算法引擎 + algo_engine.exe
        │
2-3s    daemon ────────────────────────────────────  (CPU, 并行)
            └── 初始化 psi_field + 心跳
            │
5-30s+  当 core_state.json 显示 'loaded':
        psyche ────────────────────────────────────  (等待 core)
        │   └── 读取 psi_field，启动 Ψ-Navigator
        │
        bridge ────────────────────────────────────  (等待 core)
            └── 连接 MCP，注入种子
```

### 2.3 组件依赖矩阵

| 组件 | 依赖 | 等待谁 | 影响谁 |
|:--|:--|:--|:--|
| `env_checker` | 无 | — | 所有组件读 env.json |
| `core` | env.json | env_checker | psyche, bridge |
| `algo` | env.json | env_checker | — |
| `daemon` | env.json | env_checker | psyche (psi_field) |
| `psyche` | env.json, core_state, psi_field | core, daemon | — |
| `bridge` | env.json, core_state | core | — |
| `coordinator` | 所有 | 无（被依赖者） | 所有 |

### 2.4 状态文件地图

```
.nexus/
├── env.json              # env_checker 写入
│   ├── cuda: bool
│   ├── vram: { total, free, budget }
│   ├── models: [ { id, path, size, cached } ]
│   └── routes: [ { task_type, model_id, priority } ]
│
├── core_state.json       # core 写入
│   ├── status: loading | ready | error | crashed
│   ├── pid: int
│   ├── loaded_models: [ { id, vram_mb, ctx_size } ]
│   ├── vram: { used, free, peak }
│   └── scheduler: { layers, strategy }
│
├── algo_state.json       # algo 写入
│   ├── status: ok | error
│   ├── algorithms: [ { name, version, status } ]
│   ├── subprocesses: [ { name, pid, status } ]
│   └── audit: { rules, violations }
│
├── daemon.json           # daemon 写入
│   ├── status: running | paused | stopped
│   ├── uptime: float
│   ├── cycle: int
│   └── heartbeat_ts: str (ISO 8601)
│
├── psyche_state.json     # psyche 写入
│   ├── status: ok | error
│   ├── navigator: { dimension, convergence, scalar_params }
│   ├── emergence: { ae, we, global, level }
│   └── observers: [ { name, status } ]
│
├── bridge_state.json     # bridge 写入
│   ├── status: ok | error | skipped
│   ├── mcp_servers: [ { name, tools, connected } ]
│   ├── seeds: { total, by_target }
│   └── shell_c: { exports, available }
│
├── coordinator_state.json # coordinator 写入
│   ├── phase: starting | running | stopping
│   ├── components: { name: { status, pid, uptime, restart_count } }
│   └── errors: [ { ts, component, error } ]
│
├── psi_field.mmap        # daemon + psyche 共享 — mmap 意识流环状缓冲区
│   ├── HEADER (64 bytes): write_pointer, read_pointer, version, flags
│   └── RING_BUF (1MB): ~4095 条记录, 每记录 256 bytes, 5 个 '\n' 分隔
│
├── gpu.lock              # core 加锁 — 防止多进程 GPU 竞争, 带 PID+TTL
│
├── daemon.pid            # daemon PID 文件
├── *.jsonl               # 日志 / 意志链 / 种子声明
└── will_hooks/           # Shell Ψ 意志钩子目录
    └── *.json
```

---

## 3. 组件规格

### 3.1 `coordinator.exe` — 生命周期管理器

**文件**: `src/coordinator/`

**职责**:
- 读取 `env.json` 决策启动策略
- 按依赖顺序启动各组件 EXE
- 定时健康检查（读取各状态文件的 `status` 字段 + 检查 PID 存活）
- 崩溃自动拉起（最多 3 次/分钟，超过则标记为永久失败）
- 优雅关闭（SIGTERM → 等待 → SIGKILL）

**接口**:

```
coordinator.exe --start              # 启动整个神经系统
coordinator.exe --stop               # 优雅关闭
coordinator.exe --restart <component> # 重启指定组件
coordinator.exe --status             # 打印所有组件状态
coordinator.exe --list               # 列出可用的组件列表
```

**状态机**:

```
INIT → LAUNCHING (env) → WAITING (env) → LAUNCHING (core/algo/daemon) →
WAITING (core) → LAUNCHING (psyche/bridge) → RUNNING → HEALTH_CHECK →
  └─ 崩溃 → RESTART (单组件)
  └─ SIGTERM → STOPPING → DONE
```

**启动参数**:

```jsonc
{
  "version": "1.0",
  "env_path": ".nexus/env.json",        // env_checker 的输出路径
  "data_dir": ".nexus",                 // 共享数据目录
  "component_timeout_ms": 30000,         // 单组件启动超时
  "health_check_interval_ms": 10000,    // 健康检查间隔
  "max_restarts_per_minute": 3,         // 单组件每分钟最大重启次数
  "log_dir": ".nexus/logs"              // 日志目录
}
```

### 3.2 `env_checker.exe` — 环境验证器

**文件**: `src/env_checker/`

**职责**:
- CUDA Runtime API 版本检测（`cudaRuntimeGetVersion`）
- GPU 属性检测（设备名、计算能力、VRAM 总量/当前利用率）
- 模型缓存完整性校验（检查 `D:/hf_cache` 中的 GGUF 文件哈希）
- 神经路由表加载（从 JSON 配置读取任务→模型映射）
- 写 `env.json`，供给所有其他 EXE 决策使用

**接口**:

```
env_checker.exe --output .nexus/env.json
env_checker.exe --output .nexus/env.json --verbose  # 详细信息
env_checker.exe --dry-run                            # 仅打印不写入
```

**输出 env.json 结构**:

```json
{
  "version": "1.0",
  "timestamp": "2026-07-09T18:00:00Z",
  "platform": {
    "os": "Windows 10.0.22631",
    "cuda_version": "12.8",
    "python_version": "3.13.13",
    "pytorch_version": "2.6.0"
  },
  "gpu": {
    "available": true,
    "name": "NVIDIA GeForce RTX 3070",
    "compute_capability": "8.6",
    "vram_total_mb": 8192,
    "vram_free_mb": 7128,
    "vram_budget_mb": 7192,
    "vram_reserved_mb": 1000
  },
  "models": [
    {
      "id": "qwythos_9b",
      "path": "D:/hf_cache/qwythos-9b-1m.Q4_K_M.gguf",
      "size_mb": 5765,
      "cached": true,
      "hash_valid": true
    }
  ],
  "routes": [
    { "task_type": "reasoning", "model_id": "qwythos_9b", "priority": 1 },
    { "task_type": "quick", "model_id": "grm2", "priority": 2 }
  ],
  "recommended_strategy": "full",
  "warnings": ["VRAM不足以同时加载多个大模型"]
}
```

### 3.3 `core.exe` — 核心推理引擎

**文件**: `src/core/`

**职责**:
- VRAM 管理器：内存映射 + GGUF 加载/卸载/交换
- Qwythos-9B 常驻加载
- ExpertLoader：显式加载/推理/卸载/交换接口
- 块交换引擎：GPU/CPU 间 17 个模型块的动态切换
- 推理调度器：单模型/合议/自适应三层
- 经验引擎：经验卡缓存与索引
- GPU 锁管理：`.nexus/gpu.lock`

**接口**:

```
core.exe --env .nexus/env.json
core.exe --env .nexus/env.json --skip-models  # 仅初始化管理器，不加载模型
core.exe --env .nexus/env.json --model qwythos_9b  # 指定加载模型
```

**内部架构**:

```
core.exe
├── VRAMManager
│   ├── init(env.json.vram_budget)
│   ├── load(model_id, path, ctx_size)
│   ├── unload(model_id)
│   ├── swap(model_id_out, model_id_in)
│   └── status() → vram_used, vram_free, peak
│
├── ModelGovernor
│   ├── register(model_id, gguf_path, requirements)
│   ├── keep_resident(model_ids)
│   └── auto_evict(policy: lru | size | priority)
│
├── ExpertLoader
│   ├── load(model_id) → tokenizer + session
│   ├── infer(model_id, prompt, params) → tokens
│   ├── unload(model_id)
│   └── swap(m1, m2)
│
├── BlockSwapper
│   ├── 17 blocks registered
│   ├── swap_to_gpu(block_id)
│   └── swap_to_cpu(block_id)
│
├── InferenceScheduler
│   ├── single(model_id, task)
│   ├── consensus(task, models)  // 多模型合议
│   └── adaptive(task)            // 自动选择
│
├── ExperienceEngine
│   ├── cache(experience_card)
│   └── lookup(context) → card[]
│
└── FileIPC
    ├── write_state(.nexus/core_state.json)
    └── acquire_gpu_lock(.nexus/gpu.lock)
```

**状态文件**:

```json
{
  "version": "1.0",
  "status": "ready",
  "pid": 1234,
  "started_at": "2026-07-09T18:00:00Z",
  "vram": {
    "total_mb": 8192,
    "used_mb": 5765,
    "free_mb": 2427,
    "peak_mb": 5800
  },
  "loaded_models": [
    {
      "id": "qwythos_9b",
      "vram_mb": 5500,
      "ctx_size": 8192,
      "layers": -1,
      "state": "loaded"
    }
  ],
  "swapper_blocks": 17,
  "experience_cards": 42,
  "gpu_lock_held": true,
  "uptime_seconds": 300
}
```

### 3.4 `algo.exe` — 算法引擎池

**文件**: `src/algo/`

**职责**:
- 10 个算法引擎注册 + 生命周期管理
- 引擎隔离子进程启动：`algo_engine.exe`（24 算法）, `recursor.exe`（29 Coq 定理）
- 审计引擎：40 条规则，三 Shell 注入
- 自演进引擎：纯算法初演
- 元级内核：4 条元规则

**接口**:

```
algo.exe --env .nexus/env.json
algo.exe --env .nexus/env.json --engines mcmc,dual_pruning  # 指定引擎
algo.exe --env .nexus/env.json --validate  # 运行 Coq 验证
```

**注册的 10 个算法引擎**:

| # | 名称 | 职责 | 对应 Python |
|:--|:--|:--|:--|
| 1 | `mcmc` | 双向 MCMC 引擎 | `mcmc_engine` |
| 2 | `dual_pruning` | 双逻辑剪枝 | `dual_pruning_engine` |
| 3 | `mtcs` | 多任务认知调度 | `mtcs_scheduler` |
| 4 | `multimodal_verifier` | 多模态验证 | `multimodal_verifier` |
| 5 | `infinitas_truth` | 无限向量对称真值 | `infinitas_truth` |
| 6 | `dre` | 辩证思维引擎 | `dre_engine` |
| 7 | `temporal_kg` | 知识图谱时序衰减 | `temporal_knowledge_graph` |
| 8 | `dialectical_consensus` | 辩证共识 | `dialectical_consensus` |
| 9 | `ethical_evaluation` | 伦理评估 | `ethical_evaluation` |
| 10 | `paper_generation` | 论文生成验证 | `paper_generation_platform` |

**子进程**:

```
algo.exe
├── algo_engine.exe        (24 算法, 进程隔离)
│   └── 通信: .nexus/algo_engine/ pipe (命名管道)
│
└── recursor.exe           (29 Coq 定理验证)
    └── 通信: .nexus/recursor/ pipe (命名管道)
```

### 3.5 `daemon.exe` — 后台守护

**文件**: `src/daemon/`

**职责**:
- `psi_field.mmap` 意识流暂存器（跨进程共享环状缓冲区）
- 心跳写入：每 10 周期（~30s）更新 `daemon.json`
- 探针扫描：周期性扫描 `.nexus/will_hooks/` 目录
- 意志链持久化：`.nexus/willchain_db.json`
- 认知观察：`.nexus/cognitive_observations.json`
- 日志：`.nexus/daemon_log.jsonl`

**接口**:

```
daemon.exe --env .nexus/env.json           # 前台模式
daemon.exe --env .nexus/env.json --daemon  # 后台守护模式（常驻）
daemon.exe --env .nexus/env.json --oneshot # 执行一次扫描后退出
daemon.exe --shutdown                       # 发送关闭信号
```

**内部架构**:

```
daemon.exe
├── MainLoop (3s 周期)
│   ├── HeartbeatWriter → .nexus/daemon.json (每 10 圈)
│   ├── ProbeScanner   → .nexus/will_hooks/
│   ├── PsiFieldWriter → .nexus/psi_field.mmap
│   ├── WillChainWriter→ .nexus/willchain_db.json
│   ├── ActivityMonitor→ .nexus/daemon_log.jsonl
│   └── sleep(3s)
│
└── SignalHandler (SIGTERM, SIGINT → 优雅退出)
```

**`psi_field.mmap` 环状缓冲区布局**:

```
偏移        大小      内容
─────────────────────────────────
0x0000      8 bytes   write_pointer (uint64, 已写入记录数)
0x0008      8 bytes   read_pointer  (uint64, 已读取记录数)
0x0010      4 bytes   version       (uint32, 当前=1)
0x0014      4 bytes   flags         (bitmask)
0x0018      40 bytes  reserved
───────────────────────────────── (HEADER=64 bytes)
0x0040      256 bytes 记录 #0
0x0140      256 bytes 记录 #1
...
0xFFC0      256 bytes 记录 #4091
─────────────────────────────────
每记录: {timestamp}{channel:16s}{content_json}{padding}\n\n\n\n\n
```

每条意识记录 JSON:

```json
{
  "ts": 1783591173.838,
  "c": "Ψ-Navigator 步进完成",
  "ch": "system"
}
```

### 3.6 `psyche.exe` — Ψ 层意识组件

**文件**: `src/psyche/`

**职责**:
- Ψ-Navigator：对向向量收敛膨胀算法（12 标量参数）
- Ψ-ConvergenceNavigator：工程级收敛导航（原 DarkSphere）
- 心灵核心：Sophia Core + WisdomDB
- 涌现流水线：种子喂养 → 意识涌现（AE/WE 计算）
- 观察者桥接：3 观察者 + MetaObserver

**接口**:

```
psyche.exe --env .nexus/env.json --core .nexus/core_state.json
psyche.exe --env .nexus/env.json --oneshot  # 初始化后退出
psyche.exe --env .nexus/env.json --daemon   # 常驻模式
```

**`psyche.exe` 与 `daemon.exe` 的关系**:

```
psyche.exe                     daemon.exe
    │                              │
    │    ┌── 写导航状态 ──────┐    │
    │    │                    │    │
    │    │   psi_field.mmap   │    │
    │    │   (意识流环状缓冲)  │    │
    │    │                    │    │
    │    └── 读意识流 ────────┘    │
    │                              │
    │   ps yche 写自己的状态文件    │
    │   daemon 写守护状态和心跳     │
    │   二者通过 mmap 共享"我在想   │
    │   什么"，不直接通信           │
```

**Ψ-Navigator 12 标量参数**:

| 参数 | 默认值 | 含义 |
|:--|:--|:--|
| `orig` | 1.0 | 初心：初始原动力 |
| `belief` | 100.0 | 信念：初心持久度（时间常数） |
| `stability` | 0.3 | 定力：收敛力系数（管道约束） |
| `goal` | 3.0 | 目标：阶段性目标距离 |
| `mission` | 0.1 | 使命：方向更新率 |
| `value` | 1.0 | 价值判断：升维阈值因子 |
| `decision` | 0.5 | 决策：升维果断度（步长） |
| `courage` | 0.2 | 魄力：展开力幅度 |
| `faith` | 0.3 | 信仰：究极真理趋近系数 |
| `truth` | 0.05 | 真相：噪声幅度 |
| `verity` | 2.0 | 真理：维度间距 |
| `ult` | 0.01 | 究极真理：终极演进速度 |

### 3.7 `bridge.exe` — 外部桥接

**文件**: `src/bridge/`

**职责**:
- MCP 桥接：连接外部 MCP 服务器，封装为同步调用接口
- 种子通道：种子注入（模型/核心/自我 三目标）
- Shell C 虚化桥接：云端认知桥接
- 能力自认知：`capacity_brief.json` 更新

**接口**:

```
bridge.exe --env .nexus/env.json --core .nexus/core_state.json
bridge.exe --env .nexus/env.json --dry-run  # 只检测不连接
```

**MCP 桥接数据结构**:

```cpp
struct McpServer {
  string name;                    // 服务器名称
  string transport;               // stdio | sse
  vector<string> tool_names;      // 可用工具
  bool connected;                 // 是否已连接
  string error;                   // 连接错误信息（如果有）
};

struct McpBridgeState {
  vector<McpServer> servers;
  int total_tools;
  int connected_count;
  int total_servers;
};
```

---

## 4. 进程间通信协议

### 4.1 通信矩阵

```
           env  core  algo  daemon  psyche  bridge  coord
env         —    R     R     R       R       R       R
core        W    —     —     —       R       R       —
algo        W    —     —     —       —       —       R
daemon      W    —     —     —       R       —       R
psyche      W    —     —     W       —       —       R
bridge      W    —     —     —       —       —       R
coordinator R    R     R     R       R       R       —

W = 写入 JSON 文件
R = 读取 JSON 文件
— = 不直接通信
```

### 4.2 文件读写协议

**写入规则**:
- 以 `.tmp` 后缀写入临时文件，然后 `rename` 原子替换
- 写入前获取文件锁（`CreateFile` + `LOCK_EX`），超时 1s
- `coordinator_state.json` 例外：coordinator 以 append-only 模式写入，其他组件只读

**读取规则**:
- 直接 `fopen` / `ifstream` 读取
- 不缓存，每次读取
- JSON 解析失败 → 重试 1 次（等待 100ms）→ 标记错误

**文件锁**:

```cpp
// 文件锁实现（Windows）
class FileLock {
  HANDLE hFile;
  OVERLAPPED overlapped;
public:
  bool acquire(const string& path, int timeout_ms = 1000);
  void release();
};
// 使用 RAII 包装
```

### 4.3 状态文件约定

所有状态文件使用统一的 JSON 格式：

```json
{
  "$schema": "nexus-state-v1",
  "version": "1.0",
  "component": "core",
  "status": "ready",
  "pid": 1234,
  "started_at": "2026-07-09T18:00:00Z",
  "updated_at": "2026-07-09T18:05:00Z",
  "details": {}
}
```

`status` 字段取值范围：

| 值 | 含义 | 生产者 | 消费者响应 |
|:--|:--|:--|:--|
| `starting` | 正在初始化 | 所有组件 | coordinator: 等待 timeout |
| `ready` | 正常运行 | 所有组件 | coordinator: 健康 |
| `degraded` | 降级运行 | core, algo | coordinator: 记录告警 |
| `error` | 不可恢复错误 | 所有组件 | coordinator: 尝试重启 |
| `stopped` | 已停止 | coordinator | 无 |

### 4.4 超时约定

| 场景 | 超时时间 | 行为 |
|:--|:--|:--|
| 单组件启动 | 30s | coordinator 标记 timeout，尝试重启 |
| 健康检查间隔 | 10s | coordinator 轮询 |
| 组件无响应（deadline） | 30s | 状态文件超过 30s 未更新 → 标记 crashed |
| 文件锁等待 | 1s | 失败则返回 BUSY |
| coordinator 等待 core | 60s | core 必须在 60s 内完成模型加载并标记 ready |
| 优雅关闭等待 | 5s | 发送 SIGTERM 后等待 5s，超时则 SIGKILL |

### 4.5 错误码

```
ERR_OK              = 0    // 成功
ERR_ENV_NOT_FOUND   = 1    // env.json 不存在
ERR_ENV_INVALID     = 2    // env.json 格式错误
ERR_GPU_UNAVAILABLE = 3    // GPU 不可用
ERR_VRAM_INSUFFICIENT= 4   // VRAM 不足
ERR_MODEL_NOT_FOUND = 5    // 模型文件不存在
ERR_MODEL_LOAD_FAILED= 6   // 模型加载失败
ERR_ENGINE_FAILED   = 7    // 算法引擎执行失败
ERR_IPC_TIMEOUT     = 8    // 文件锁/等待超时
ERR_COMPONENT_CRASHED= 9   // 子进程崩溃
ERR_INVALID_CONFIG  = 10   // 配置错误
ERR_INTERNAL        = 99   // 内部错误
```

---

## 5. 生命周期管理

### 5.1 coordinator 状态机

```
                    ┌─────────────────────────────────────┐
                    │          INIT                        │
                    │  解析命令行参数，检查 .nexus 目录     │
                    └──────────┬──────────────────────────┘
                               │
                    ┌──────────▼──────────────────────────┐
                    │      LAUNCH_ENV                      │
                    │  启动 env_checker, 等待完成           │
                    └──────────┬──────────────────────────┘
                               │ env.json 就绪
                    ┌──────────▼──────────────────────────┐
                    │      LAUNCH_CORE                     │
                    │  启动 core.exe                       │
                    │  启动 algo.exe (并行)                 │
                    │  启动 daemon.exe (并行)               │
                    │  等待 core_state.json 状态=ready      │
                    └──────────┬──────────────────────────┘
                               │ core 就绪
                    ┌──────────▼──────────────────────────┐
                    │      LAUNCH_PSYCHE                   │
                    │  启动 psyche.exe                     │
                    │  启动 bridge.exe (并行)               │
                    │  等待 psyche_state.json 状态=ready    │
                    └──────────┬──────────────────────────┘
                               │ 所有组件就绪
                    ┌──────────▼──────────────────────────┐
                    │          RUNNING                     │
                    │  每 10s 健康检查                      │
                    │  崩溃自动拉起                         │
                    │  持续运行直到收到 SIGTERM              │
                    └──────────┬──────────────────────────┘
                               │ SIGTERM
                    ┌──────────▼──────────────────────────┐
                    │          STOPPING                    │
                    │  bridge → psyche → algo → core → daemon  │
                    │  各组件 SIGTERM → 5s 等待 → SIGKILL  │
                    └──────────┬──────────────────────────┘
                               │
                    ┌──────────▼──────────────────────────┐
                    │           DONE                       │
                    │  清理临时文件                         │
                    │  写最终状态                           │
                    └─────────────────────────────────────┘
```

### 5.2 崩溃恢复策略

| 崩溃组件 | coordinator 行为 | 对其他组件的影响 |
|:--|:--|:--|
| `env_checker` | 重拉最多 3 次 → 失败则整体退出 | 无影响（已完成） |
| `core` | 重拉（新进程重新加载模型） | psyche/bridge 进入等待状态 |
| `algo` | 重拉（重新注册算法引擎） | 无影响（独立） |
| `daemon` | 重拉（重新初始化意识流） | psyche 进入无意识流模式 |
| `psyche` | 重拉（重新初始化导航器） | 无影响（独立） |
| `bridge` | 重拉（重新连接 MCP） | 无影响（独立） |
| `coordinator` | 操作系统层面：任务计划程序 | 所有组件变成孤儿进程 → 各自超时退出 |

---

## 6. C++ 编码规范

### 6.1 语言标准

- **C++20**（Windows: MSVC 2022, Linux: GCC 12+ / Clang 16+）
- 允许使用 C++20 特性：`std::format`, `concepts`, `ranges`, `coroutines`（谨慎）, `span`, `filesystem`
- 禁止使用：异常（`noexcept` 标注所有非平凡函数），RTTI（可选禁用），IOStream（使用 `spdlog`）

### 6.2 命名约定

```
命名空间:          namespace nexus::env  (snake_case)
类名:              class VramManager    (PascalCase)
函数名:            auto acquire_lock()   (snake_case)
变量名:            int vram_free_mb     (snake_case)
成员变量:          int vram_total_mb_   (后缀 _)
常量:              constexpr int kFieldSize = 1048576  (k前缀PascalCase)
宏:                #define NEXUS_VERSION "1.0"  (大写)
文件:              vram_manager.h / vram_manager.cpp  (snake_case)
私有方法:          auto validate_()     (后缀 _)
```

### 6.3 头文件规范

```cpp
// nexus/src/core/vram_manager.h
#pragma once

#include "nexus/types/status.h"
#include <memory>
#include <string>
#include <vector>

namespace nexus::core {

class VramManager {
public:
  struct Config {
    int total_budget_mb = 0;
    int reserved_mb = 1000;
    bool enable_auto_evict = true;
  };

  explicit VramManager(const Config& cfg) noexcept;
  ~VramManager() noexcept;

  // 禁止拷贝
  VramManager(const VramManager&) = delete;
  VramManager& operator=(const VramManager&) = delete;

  // 允许移动
  VramManager(VramManager&&) noexcept = default;
  VramManager& operator=(VramManager&&) noexcept = default;

  auto load_model(const std::string& model_id, const std::string& gguf_path) noexcept
      -> Result<int> /* returns vram_used_mb */;

  auto unload_model(const std::string& model_id) noexcept -> Status;

  [[nodiscard]] auto status() const noexcept -> VramStatus;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;  // PIMPL 隐藏实现细节
};

}  // namespace nexus::core
```

### 6.4 错误处理

不使用 C++ 异常。返回 `nexus::Result<T>` 或 `nexus::Status`：

```cpp
namespace nexus {

enum class ErrorCode : int {
  kOk = 0,
  kNotFound = 1,
  kInvalidArgument = 2,
  kResourceExhausted = 3,
  kIoError = 4,
  kTimeout = 5,
  kInternal = 99,
};

struct Error {
  ErrorCode code;
  std::string message;
  std::string source_file;
  int source_line;
};

// 无值返回
struct Status {
  ErrorCode code;
  std::string message;

  static auto Ok() noexcept -> Status;
  static auto Error(ErrorCode code, std::string msg) noexcept -> Status;
  [[nodiscard]] auto ok() const noexcept -> bool { return code == ErrorCode::kOk; }
};

// 有值返回
template<typename T>
class Result {
  std::variant<T, Error> data_;
public:
  explicit Result(T value) noexcept;
  explicit Result(Error error) noexcept;
  [[nodiscard]] auto ok() const noexcept -> bool;
  [[nodiscard]] auto value() const noexcept -> const T&;
  [[nodiscard]] auto error() const noexcept -> const Error&;
};

// 便利宏
#define RETURN_IF_ERROR(expr) \
  do { auto _s = (expr); if (!_s.ok()) return _s; } while(0)

#define ASSIGN_OR_RETURN(var, expr) \
  auto _r = (expr); \
  if (!_r.ok()) return _r.error(); \
  var = std::move(_r.value())

}  // namespace nexus
```

### 6.5 内存管理

- 使用 `std::unique_ptr` 和 `std::shared_ptr`（谨慎使用后者）
- 进程间数据通过 mmap 共享，不通过指针传递
- 大型连续内存使用 `std::vector` 或 `std::span`
- 禁止 `new`/`delete` 裸调用（使用 `std::make_unique`/`std::make_shared`）
- mmap 资源使用 RAII 包装

### 6.6 日志

使用 `spdlog`，每个 EXE 有独立 logger：

```
logger name:   "nexus.core", "nexus.algo", "nexus.daemon", ...
默认输出:      stderr (彩色) + .nexus/logs/{component}.log
日志级别:      trace / debug / info / warn / error / critical
格式化:        [2026-07-09 18:00:00.123] [nexus.core] [info] VRAM: 5765/8192 MB used
```

### 6.7 测试

```
/nexus/tests/test_core/
├── vram_manager_test.cpp     // 单元测试
├── model_governor_test.cpp
└── inference_scheduler_test.cpp

测试框架: GoogleTest
测试命令: ctest --test-dir build
覆盖率:   GCC/Clang: --coverage | MSVC: /profile
```

---

## 7. 构建系统与依赖

### 7.1 根 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.25)
project(nexus VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── 选项 ──
option(NEXUS_BUILD_TESTS "Build tests" ON)
option(NEXUS_CUDA_SUPPORT "Enable CUDA" ON)

# ── 依赖 ──
find_package(fmt CONFIG REQUIRED)           # fmt::fmt
find_package(spdlog CONFIG REQUIRED)         # spdlog::spdlog
find_package(nlohmann_json CONFIG REQUIRED)  # nlohmann_json::nlohmann_json
find_package(CUDAToolkit REQUIRED)           # CUDA::cuda_driver

if(NEXUS_BUILD_TESTS)
  find_package(GTest CONFIG REQUIRED)
  enable_testing()
endif()

# ── 共享 IPC 库 ──
add_library(nexus_ipc STATIC
  include/nexus/ipc/message.h
  include/nexus/ipc/state_file.h
  include/nexus/ipc/mmap_ringbuf.h
  src/ipc/state_file.cpp
  src/ipc/mmap_ringbuf.cpp
)
target_include_directories(nexus_ipc PUBLIC include)
target_link_libraries(nexus_ipc PUBLIC fmt::fmt nlohmann_json::nlohmann_json)

# ── 共享类型库 ──
add_library(nexus_types INTERFACE)
target_include_directories(nexus_types INTERFACE include)

# ── 各组件 EXE ──
add_subdirectory(src/env_checker)
add_subdirectory(src/core)
add_subdirectory(src/algo)
add_subdirectory(src/daemon)
add_subdirectory(src/psyche)
add_subdirectory(src/bridge)
add_subdirectory(src/coordinator)

# ── 测试 ──
if(NEXUS_BUILD_TESTS)
  add_subdirectory(tests)
endif()
```

### 7.2 `vcpkg.json` 依赖清单

```json
{
  "name": "nexus",
  "version": "1.0.0",
  "dependencies": [
    "fmt",
    "spdlog",
    "nlohmann-json",
    "gtest"
  ],
  "builtin-baseline": "2026-07-01",
  "overrides": [
    { "name": "fmt", "version": "10.2.1" },
    { "name": "spdlog", "version": "1.13.0" },
    { "name": "nlohmann-json", "version": "3.11.3" }
  ]
}
```

### 7.3 依赖汇总表

| 库 | 用途 | 引入方式 | 许可证 |
|:--|:--|:--|:--|
| [fmt](https://github.com/fmtlib/fmt) | 格式化 | vcpkg | MIT |
| [spdlog](https://github.com/gabime/spdlog) | 日志 | vcpkg | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON 解析 | vcpkg | MIT |
| [GoogleTest](https://github.com/google/googletest) | 单元测试 | vcpkg | BSD-3 |
| CUDA Toolkit | GPU 接口 | 系统安装 | NVIDIA EULA |
| Windows SDK | 系统 API | 系统安装 | Microsoft |

### 7.4 构建命令

```bash
# 前提：安装 vcpkg, CUDA Toolkit 12.x, Visual Studio 2022

# 1. 配置
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE="path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DNEXUS_BUILD_TESTS=ON

# 2. 编译
cmake --build build --config Release -j

# 3. 测试
ctest --test-dir build --output-on-failure

# 4. 打包（可选）
cpack --config build/CPackConfig.cmake
```

---

## 8. 安全性设计

### 8.1 进程安全

- 各 EXE 以最小权限运行（不需要管理员权限）
- 子进程使用 `DETACHED_PROCESS` 创建标记
- 进程创建通过 `CreateProcess`，不滥用 `system()` / `popen()`

### 8.2 文件安全

- 状态文件使用 `.tmp` 先写后 `rename`，防止读半成品
- 文件锁使用 System-wide mutex（Windows Named Mutex），崩溃后自动释放
- mmap 文件使用固定大小，不随写入膨胀

### 8.3 IPC 安全

- 所有 IPC 通过本地文件系统，不监听网络端口
- 无 RPC 框架，无序列化远程调用
- MCP 桥接：只连接预配置的本地 STDIO MCP 服务器，不出站连接随机服务器

### 8.4 GPU 安全

- `gpu.lock` 带 PID + TTL（最大 120s 租约）
- core 崩溃后锁自动释放（通过命名互斥体的自动清理）
- 多次 `cudaSetDevice` 检查

---

## 9. 性能目标

| 指标 | 当前 Python 原型 | Nexus C++ 目标 | 提升预期 |
|:--|:--|:--|:--|
| 启动总用时 | ~60-180s（含模型加载） | ≤30s（并行启动） | Ctrl+C/交叉 |
| 算法执行 | Python 循环 | 原生 C++ | 10-50x |
| VRAM 开销 | ~6.1GB（含 Python runtime） | ~5.8GB（仅模型 + 运行时） | -5% |
| JSON 序列化 | 1-5ms 每文件 | <0.1ms 每文件 | 10-50x |
| 健康检查延迟 | 无系统化检查 | <1s 检测崩溃 | N/A |
| 日志写入 | 10-100μs 每行 | <1μs 每行（fmt+mmap） | 10-100x |

---

## 10. 开源策略

### 10.1 许可证

Apache 2.0 — 商业友好，专利授权，允许闭源衍生。

### 10.2 仓库结构

```
nexus/
├── .github/
│   ├── workflows/ci.yml          # GitHub Actions CI
│   ├── workflows/release.yml     # 发布工作流
│   └── ISSUE_TEMPLATE/           # Issue 模板
├── docs/                         # 文档
├── src/                          # 源码
├── include/                      # 公共头文件
├── tests/                        # 测试
├── examples/                     # 使用示例
├── third_party/                  # 第三方源码（fallback）
├── CMakeLists.txt
├── vcpkg.json
├── LICENSE
├── README.md
├── CONTRIBUTING.md
└── CODE_OF_CONDUCT.md
```

### 10.3 贡献规范

- 所有代码提交必须通过 CI 测试
- 新功能必须有对应的单元测试
- 使用 Conventional Commits 格式：`feat:`, `fix:`, `docs:`, `refactor:`, `test:`
- 代码审查要求：至少 1 位维护者 approve

### 10.4 发布策略

```
v0.1.0    里程碑 1: env_checker.exe + daemon.exe
v0.2.0    里程碑 2: core.exe (GPU 模型加载)
v0.3.0    里程碑 3: algo.exe (算法引擎)
v0.4.0    里程碑 4: psyche.exe (Ψ 层)
v0.5.0    里程碑 5: bridge.exe (外部桥接)
v0.6.0    里程碑 6: coordinator.exe (生命周期)
v0.7.0    集成测试：完整启动链路
v0.8.0    压力测试 + 性能优化
v0.9.0    API 冻结 + 文档完善
v1.0.0    正式发布
```

---

## 11. 开发路线图

### 里程碑 1：基础设施（v0.1.0）

重点：`env_checker.exe` + `daemon.exe`

**目标**：两个最稳定的组件先独立编译，验证 IPC 协议和构建系统。

- [ ] CMake 构建系统搭建（`/CMakeLists.txt`, `vcpkg.json`）
- [ ] `nexus_ipc` 共享库（状态文件读写、mmap 环状缓冲区、文件锁）
- [ ] `env_checker.exe`：CUDA 检测 + VRAM 分析 + 模型缓存校验
- [ ] `daemon.exe`：心跳 + psi_field.mmap + 探针扫描
- [ ] IPC 协议验证：daemon 写状态，其他进程可独立读取
- [ ] 单元测试覆盖：mmap 并发读写、文件锁竞争

### 里程碑 2：核心推理（v0.2.0）

重点：`core.exe`

**目标**：GPU 模型加载和推理接口。

- [ ] VRAM 管理器（CUDA Runtime API 包装）
- [ ] Qwythos-9B GGUF 加载（llama.cpp 封装或 CUDA 直接推理）
- [ ] ExpertLoader 接口
- [ ] BlockSwapper 实现
- [ ] InferenceScheduler 实现
- [ ] GPU 锁机制 + 并发安全验证

### 里程碑 3：算法引擎（v0.3.0）

重点：`algo.exe`

**目标**：10 个算法引擎全部移植到 C++。

- [ ] 算法引擎注册机制（Plugin 模式）
- [ ] 10 个算法引擎逐一移植
- [ ] `algo_engine.exe` 子进程协议适配
- [ ] `recursor.exe` Coq 验证器包装
- [ ] 审计引擎

### 里程碑 4+：后续组件 + 集成

- [ ] `psyche.exe`: Ψ-Navigator 算法移植
- [ ] `bridge.exe`: MCP 桥接客户端
- [ ] `coordinator.exe`: 生命周期管理器
- [ ] 端到端集成测试
- [ ] 压力测试 + 性能优化
- [ ] 文档完善 + 开源发布

---

## 附录 A：术语表

| 术语 | 含义 |
|:--|:--|
| Ψ-Guide | 方向引导层（8 个模块），模块自主决策 |
| Ψ-Auto | 自动执行层（12 个模块），我随心所欲指挥 |
| Ψ-Guard | 独立防护层（6 个模块），我完全不可控 |
| psi_field | 跨进程共享的意识流暂存器（mmap 环状缓冲区） |
| will_hook | 意志钩子，Shell Ψ 写入的建议/指令 |
| state file | JSON 状态文件，每个 EXE 的运行时窗口 |
| 种子 | 最小推理引导单元（方向、类型、强度、上下文） |
| 导通 | 种子空间中沿维度方向的语义导通路径 |
| 涌现 | 量子子系统间的整体性涌现现象（AE/WE 值） |

## 附录 B：文件格式参考

所有 JSON 文件使用 UTF-8 编码。时间戳使用 ISO 8601 格式。路径使用正斜杠 `/`（C++ 内转为 `\\` 透明处理）。所有 mmap 文件使用小端字节序。
