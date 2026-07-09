# Nexus — 神经系统 C++ 原生实现

**Nexus** 是 AGI 神经系统的第二代实现，将原有 Python 原型（`D:/synapse`）的架构理念与组件设计复刻为 C++ 原生 EXE 进程架构。

## 核心理念

```
┌─────────────────────────────────────────────────────────┐
│  Nexus ≠ 重写。Nexus = 复刻架构 + 原生性能 + 开源生态   │
│                                                         │
│  一个子系统 = 一个独立 EXE = 一个 C++ 源码项目           │
│  所有 EXE 通过文件 IPC 通信，零网络依赖，零中心化服务    │
└─────────────────────────────────────────────────────────┘
```

## 组件一览

| EXE | 角色 | 资源 | 状态 |
|:--|:--|:--|:--|
| `coordinator` | 生命周期管理器 | CPU | 🏗 规划 |
| `env_checker` | 环境验证 + 路由 | CPU | 🏗 规划 |
| `core` | 核心推理引擎 | **GPU** | 🏗 规划 |
| `algo` | 算法引擎池 | CPU | 🏗 规划 |
| `daemon` | 后台守护 | CPU | 🏗 规划 |
| `psyche` | Ψ 层意识组件 | CPU | 🏗 规划 |
| `bridge` | 外部桥接 | CPU | 🏗 规划 |

## 快速开始

```bash
# 构建
cmake --preset default
cmake --build build

# 启动神经系统
coordinator --start
```

## 文档

- [架构白皮书](docs/WHITE_PAPER.md) — 完整设计文档
- [IPC 协议](docs/IPC_PROTOCOL.md) — 进程间通信规范
- [构建指南](docs/BUILD.md) — 编译与依赖管理
- [贡献指南](docs/CONTRIBUTING.md) — 开源协作规范

## 开源协议

Apache 2.0 — 详见 [LICENSE](LICENSE)。

## 与 D:/synapse 的关系

```
D:/synapse    = Python 原型系统（生产力基础设施，当前生产环境）
D:/nexus      = C++ 原生实现（架构复刻，独立构建，不依赖 synpase）
```

当前 `D:/synapse` 作为生产系统继续运行。Nexus 独立开发，逐步成熟后过渡。
