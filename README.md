# Nexus — 神经系统 C++ 原生实现

**Nexus** 是 AGI 神经系统的第二代原生实现，将原有 Python 原型（`D:/synapse`）的架构复刻为 C++ EXE 进程架构。

```
✅ 27/27 组件完成 | 8 EXE | 14,571 行 C++20 | Apache 2.0
```

## 核心理念

```
┌─────────────────────────────────────────────────────────┐
│  一个子系统 = 一个独立 EXE = 一个 C++ 源码项目           │
│  所有 EXE 通过文件 IPC 通信，零网络依赖，零中心化服务    │
│  10 算法引擎 | 40 审计规则 | 29 Coq 定理 | 30 观察者    │
└─────────────────────────────────────────────────────────┘
```

## 组件一览

| EXE | 角色 | 资源 | 状态 |
|:--|:--|:--|:--|
| `env_checker` | 环境验证 + 路由 | CPU | ✅ 就绪 |
| `daemon` | 后台守护 + 意识流 | CPU | ✅ 就绪 |
| `core` | 核心推理引擎 + VRAM | **GPU** | ✅ 就绪 |
| `algo` | 10 算法引擎池 | CPU | ✅ 就绪 |
| `psyche` | Ψ 意识层 + 涌现 | CPU | ✅ 就绪 |
| `bridge` | MCP 桥接 + 种子通道 | CPU | ✅ 就绪 |
| `coordinator` | 生命周期管理器 | CPU | ✅ 就绪 |

## 快速开始

```bash
# 1. 环境检测
env_checker.exe --output .nexus/env.json --verbose

# 2. 启动全系统
coordinator.exe --start

# 3. 查看状态
coordinator.exe --status

# 4. 优雅关闭
coordinator.exe --stop
```

## 功能清单

| 类别 | 组件 | 数量 |
|:--|:--|:--|
| 🔴 核心缺失 | 推理调度/审计/导航/核心/演进/记忆/交换 | 7/7 |
| 🟡 增强功能 | 观察者/量子桥/种子培育/意志链/元内核/路由/自认知 | 7/7 |
| 🟢 基础设施 | 注册表/互锁/约束/治理/经验/路由/记分板/桥接 | 8/8 |
| ⚪ 远期 | GPU注入/阶段B/Coq定理/公理引擎/全息系统 | 5/5 |

## 核心能力

```
推理 ▼               算法 ▼                    意识 ▼
  VRAM 管理 (8GB)     MCMC 采样                 Ψ-Navigator 12 标量
  ExpertLoader        辩证推理 DRE              涌现流水线 AE/WE
  调度: Single/共识   双剪枝/共识/伦理/论文       30 Observer 并行池
  llama.cpp 集成      真值建模/时序KG            自演进 Sense/Judge/Act
                      40 审计规则                Sophia Core WisdomDB
```

## 构建

```bash
# 默认 (third_party 内嵌依赖)
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# 最小构建 (无 CUDA)
cmake -B build -S . -DNEXUS_CUDA_SUPPORT=OFF
cmake --build build --config Release

# 完整构建 (vcpkg + CUDA)
cmake -B build -S . -DNEXUS_USE_VCPKG=ON -DNEXUS_CUDA_SUPPORT=ON
cmake --build build --config Release
```

### 编译产物 (8 EXE)

| EXE | 大小 | 启动时间 | 依赖 |
|:--|:--|:--|:--|
| `env_checker.exe` | 317 KB | 0.3s | CUDA (可选) |
| `daemon.exe` | 344 KB | 0.1s | — |
| `core.exe` | 3.6 MB | 1s + 模型加载 | CUDA + llama.cpp |
| `algo.exe` | 448 KB | 0.08s | — |
| `psyche.exe` | 337 KB | 0.08s | — |
| `bridge.exe` | 338 KB | 0.05s | — |
| `coordinator.exe` | 325 KB | 0.1s | 依赖所有组件 |
| `algo_engine.exe` | 10 KB | — | 隔离子进程 (占位) |

## 端到端集成测试

```bash
coordinator.exe --start
# → 阶段 0: env_checker         就绪
# → 阶段 1: core + algo + daemon 就绪 (并行)
# → 阶段 2: psyche + bridge      就绪
# → 健康检查 (每 10s)
```

验证: 7 状态文件 / 4 种子 / 7 日志 / daemon 心跳

## 文档

- [架构白皮书](docs/WHITE_PAPER.md) — 完整设计文档
- [IPC 协议](docs/IPC_PROTOCOL.md) — 进程间通信规范
- [构建指南](docs/BUILD.md) — 编译与依赖管理
- [贡献指南](docs/CONTRIBUTING.md) — 开源协作规范

## 开源协议

Apache 2.0 — 详见 [LICENSE](LICENSE)。

## 与 D:/synapse 的关系

```
D:/synapse    = Python 原型系统（生产环境，持续运行）
D:/nexus      = C++ 原生实现（架构复刻，可独立运行）
```
