# Nexus — AGI 神经系统 C++ 原生实现

[![CI](https://github.com/hy7pc8gfmf-dotcom/nexus/actions/workflows/ci.yml/badge.svg)](https://github.com/hy7pc8gfmf-dotcom/nexus/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/hy7pc8gfmf-dotcom/nexus?include_prereleases&label=Release)](https://github.com/hy7pc8gfmf-dotcom/nexus/releases)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-20-00599C?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Windows_Linux-lightgrey)](https://github.com/hy7pc8gfmf-dotcom/nexus)
[![Code Size](https://img.shields.io/github/languages/code-size/hy7pc8gfmf-dotcom/nexus)]()

> **Nexus** 是 AGI 神经系统的 C++20 原生实现。  
> 零 Python 依赖 · 无网络 IPC · 纯文件通信 · Apache 2.0 开源

## 下载

最新稳定版: **[Nexus v1.1.1](https://github.com/hy7pc8gfmf-dotcom/nexus/releases/tag/v1.1.1)**

| 安装包 | 大小 | 含语义库 | 含证明库 |
|:-------|:-----|:---------|:---------|
| [Nexus-1.1.1-Setup.exe](https://github.com/hy7pc8gfmf-dotcom/nexus/releases/download/v1.1.1/Nexus-1.1.1-Setup.exe) | 10.9 MB | ✓ (63K concepts) | ✓ (66K theorems) |

Inno Setup 安装包, 一键安装 11 个核心模块 + 完整语义数据。

## 核心理念

```
┌─────────────────────────────────────────────────────────┐
│  一个子系统 = 一个独立 EXE = 一个 C++ 源码项目           │
│  所有 EXE 通过文件 IPC 通信，零网络依赖，零中心化服务    │
│  63147 语义概念 | 66K Coq 定理 | 11 引擎模块            │
└─────────────────────────────────────────────────────────┘
```

## 组件一览

| EXE | 角色 | 资源 | 行数 | 状态 |
|:--|:--|:--|:--|:--|
| `env_checker` | 环境验证 + GPU/VRAM 检测 | CPU | ~200 | ✅ 就绪 |
| `daemon` | 后台守护 + 意识流 + 心跳 | CPU | ~260 | ✅ 就绪 |
| `core` | 核心推理引擎 + VRAM 管理 | **GPU** | ~500 | ✅ 就绪 |
| `algo` | 10 算法引擎池 + 40 审计规则 | CPU | ~1200 | ✅ 就绪 |
| `algo_engine` | 隔离子进程 (占位) | CPU | ~50 | ✅ 就绪 |
| `psyche` | Ψ 意识层 + 12 标量涌现 | CPU | ~1500 | ✅ 就绪 |
| `bridge` | 语义桥接 + 种子通道 | CPU | ~1200 | ✅ 就绪 |
| `coordinator` | 生命周期管理 + 健康检查 | CPU | ~400 | ✅ 就绪 |
| `quantum` | 量子涌现引擎 | CPU | ~200 | ✅ 就绪 |
| `logic` | 逻辑求解引擎 | CPU | ~200 | ✅ 就绪 |
| `holographic` | 全息语义引擎 | CPU | ~200 | ✅ 就绪 |

## 核心能力

```
推理                    算法                     意识
─────                   ────                     ────
RuleEngine + ProofDAG   MCMC 采样               Ψ-Navigator 12 标量
通用推理框架 (295行)     辩证推理 DRE             涌现流水线 AE/WE
14D 语义空间             双剪枝/共识/伦理/论文     30 Observer 并行池
Lorentz/Doppler 规则集   真值建模/时序KG          自演进 Sense/Judge/Act
subset_trans 传递性      40 审计规则              Sophia Core WisdomDB

Coq 桥接                数据                     安装包
────────                ────                     ────
66K 形式化定理           63147 语义概念            Nexus-1.1.1-Setup.exe
12K 引理索引             21516 种子               LZMA2 压缩 (10.9 MB)
定理依赖图 1.3M 边       14D 语义场               Inno Setup 6 安装器
```

## 快速开始

```bash
# 安装后进入 Nexus 目录

# 1. 环境检测
env_checker.exe --output .nexus/env.json --verbose

# 2. 启动全系统
coordinator.exe --start

# 3. 查看状态
coordinator.exe --status

# 4. 查询语义空间 (Shell D)
echo '{"cmd":"solve","problem":"光子镜面反射多普勒效应"}' | data\.shell_d_semantic.mdb

# 5. 优雅关闭
coordinator.exe --stop
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

# 打包安装包 (需 Inno Setup 6)
ISCC.exe packaging/windows/setup.iss
```

### 编译产物 (11 EXE + 1 测试)

| EXE | 大小 | 启动时间 | 依赖 |
|:--|:--|:--|:--|
| `env_checker.exe` | 317 KB | 0.3s | CUDA (可选) |
| `daemon.exe` | 344 KB | 0.1s | — |
| `core.exe` | 3.6 MB | 1s + 模型加载 | CUDA + llama.cpp |
| `algo.exe` | 585 KB | 0.08s | — |
| `psyche.exe` | 436 KB | 0.08s | — |
| `bridge.exe` | 338 KB | 0.05s | — |
| `coordinator.exe` | 325 KB | 0.1s | 依赖所有组件 |
| `algo_engine.exe` | 10 KB | — | 隔离子进程 (占位) |
| `quantum.exe` | 318 KB | 0.05s | — |
| `logic.exe` | 310 KB | 0.05s | — |
| `holographic.exe` | 300 KB | 0.05s | — |
| `nexus_tests.exe` | 1.2 MB | — | 单元测试 (不纳入安装包) |

## 安装包内容 (v1.1.1)

```
Nexus-1.1.1-Setup.exe (10.9 MB)
├── bin/
│   ├── 11 个核心 EXE 模块       (~6 MB, LZMA2 压缩)
├── data/
│   ├── .shell_d_semantic.mdb    (26 MB, 63147 concepts 14D 空间)
│   ├── .shell_d_seeds.json      (9.8 MB, 种子库)
│   ├── .semantic_field.bin      (8.8 MB, 语义场)
│   ├── .proof_index.json        (23 MB, 证明索引)
│   ├── .theorem_deps.json       (13 MB, 66K 定理依赖图)
│   ├── .unified_seed_bank.json  (11 MB, 21516 种子)
│   ├── .engine_state.json       (20 MB, 引擎状态)
│   └── lemma_seed_index.json    (3.4 MB, 12K 引理)
├── README.md, LICENSE, NOTICE, CHANGELOG.md
```

## 推理架构

Nexus 包含两层推理管线:

### 1. Shell D — 语义推理层 (C++17, mmap 语义数据库)

- 14 维语义向量空间，63147 概念
- 短语提取 (最长 8 CJK 字符) → 语义聚类 → 矛盾检测 → 涌现洞察
- 基于 delta boost 的种子注入机制 (21516 种子)

### 2. RuleEngine + ProofDAG — 形式推理层 (C++20)

- 通用推理引擎: 同一代码处理物理/计算/AI 安全等所有领域
- 事实三元组 (Subject-Predicate-Object) + 规则变量捕获 ($X, $Y, $Z)
- ProofDAG: 每条推导边可追溯至 premises 来源
- 收敛检测: 同事实被多条路径推导出时标记
- 15 条内置规则 (逻辑传递性/物理定律/计算理论/AI 安全约束)

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
               包括 Shell D (C++17 语义引擎) 的完整移植
```
