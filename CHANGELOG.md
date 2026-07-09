# Changelog

## v1.0.0 (2026-07-09)

Nexus 神经系统首个正式发布版。C++ 原生实现，7 EXE 进程架构，零网络依赖文件 IPC。

### 组件清单

**7 可运行 EXE:**

| EXE | 行数 | 功能 |
|:--|:--|:--|
| env_checker | ~200 | CUDA/VRAM 检测, 29 模型扫描 |
| daemon | ~260 | 后台守护, psi_field 意识流, 心跳 |
| core | ~500 | VRAM 管理, ExpertLoader, llama.cpp 推理 |
| algo | ~1200 | 10 算法引擎, EngineRegistry, 40 审计规则 |
| psyche | ~1500 | Ψ-Navigator, 涌现流水线, 30 Observer |
| bridge | ~1200 | MCP 客户端, 种子通道, 全局记忆 |
| coordinator | ~400 | 生命周期管理, 健康检查 |

**27 子系统:**

- 🔴 核心缺失 7/7: 推理调度器, 审计引擎, Ψ-ConvergenceNavigator, 心灵核心, 自演进引擎, 全局记忆, 块交换引擎
- 🟡 增强功能 7/7: 30 Observer, Algo×Quantum, 种子培育SELF, 意志链桥接, 元级内核, 司南·归海MCP, 能力自认知
- 🟢 基础设施 8/8: Shell A/B/C 注册表, 三反射互锁, 中文约束, 资源治理引擎, 经验引擎, 对话框主导权, 竞技记分板, 原生推理桥接
- ⚪ 远期 5/5: GPU Token 注入, Phase B GPU 桥接, 递归元理论 29 Coq, 元公理引擎, 全息系统

### 构建系统

- CMake 多预设: default / vcpkg / minimal
- vcpkg 依赖管理: fmt, spdlog, nlohmann-json, GTest
- 内嵌 third_party 降级: 无网络可编译
- llama.cpp: FetchContent 集成

### 技术指标

- 语言: C++20
- 源码: 109 文件, 14,571 行
- 平台: Windows (MSVC), Linux (GCC, 规划中)
- GPU: CUDA 12.x, RTX 3070 8GB
- 协议: Apache 2.0

### 已知问题

- psyche.exe oneshot 模式在 coordinator 启动时可能超时 (cosmetic)
- core.exe GPU 推理因 llama.cpp CUDA 编译问题暂用 CPU
- .get\<int\>() 在 minimal json.hpp 中不完全兼容
