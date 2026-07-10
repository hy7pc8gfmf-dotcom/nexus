# Nexus — AGI 神经系统 C++ 原生实现

[![CI](https://github.com/hy7pc8gfmf-dotcom/nexus/actions/workflows/ci.yml/badge.svg)](https://github.com/hy7pc8gfmf-dotcom/nexus/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-20-00599C?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Windows_Linux-lightgrey)](https://github.com/hy7pc8gfmf-dotcom/nexus)
[![Code Size](https://img.shields.io/github/languages/code-size/hy7pc8gfmf-dotcom/nexus)]()

> **Nexus** 是 AGI 神经系统的 C++20 原生实现。  
> 零 Python 依赖 · 无网络 IPC · 纯文件通信 · Apache 2.0 开源

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

## 贡献

我们欢迎所有形式的贡献。详见 [贡献指南](docs/CONTRIBUTING.md)。

| 入口 | 说明 |
|:--|:--|
| [Issues](https://github.com/hy7pc8gfmf-dotcom/nexus/issues) | 报告 Bug / 提议功能 |
| [Pull Requests](https://github.com/hy7pc8gfmf-dotcom/nexus/pulls) | 提交代码 |
| [CLA](CLA/README.md) | 贡献者许可协议（首次提交前签署） |

**贡献前请确保：**
- ✅ 本地 `ctest` 全部通过
- ✅ 代码通过 `cppcheck` 静态分析
- ✅ 新功能附带对应单元测试
- ✅ 遵循 Apache 2.0 许可

## 与 D:/synapse 的关系

```
D:/synapse    = Python 原型系统（生产环境，持续运行）
D:/nexus      = C++ 原生实现（架构复刻，可独立运行）
```
