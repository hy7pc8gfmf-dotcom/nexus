# Nexus 使用指南

## 快速命令速查

```bash
# ── 环境检测 ──
env_checker.exe --output .nexus/env.json               # 标准检测
env_checker.exe --output .nexus/env.json --verbose      # 详细信息

# ── 后台守护 ──
daemon.exe --env .nexus/env.json                        # 前台运行
daemon.exe --env .nexus/env.json --daemon               # 后台守护
daemon.exe --env .nexus/env.json --oneshot              # 单次扫描
daemon.exe --shutdown                                   # 停止守护

# ── 推理引擎 ──
core.exe --skip-models                                  # 仅VRAM管理
core.exe --model D:/hf_cache/VibeThinker-1.5B.Q4_K_M.gguf  # 加载推理

# ── 算法引擎 ──
algo.exe --list                                         # 列出引擎
algo.exe --validate                                     # 运行验证
algo.exe --validate --list                              # 全部信息

# ── 意识层 ──
psyche.exe --oneshot --steps 100                        # 快速验证
psyche.exe --daemon                                     # 常驻模式

# ── 桥接 ──
bridge.exe --dry-run                                    # 无副作用验证
bridge.exe                                              # 完整运行

# ── 全系统启动 ──
coordinator.exe --start                                 # 启动全部
coordinator.exe --status                                # 查看状态
coordinator.exe --stop                                  # 优雅关闭
coordinator.exe --restart daemon                        # 重启组件
coordinator.exe --list                                  # 组件清单
```

## 启动序列

```
coordinator --start

Phase 0 ─── env_checker ──→ .nexus/env_state.json
                │
Phase 1 ─── core ─────────→ .nexus/core_state.json    (并行)
            algo ─────────→ .nexus/algo_state.json    (并行)
            daemon ───────→ .nexus/daemon.json         (并行)
                │
Phase 2 ─── psyche ───────→ .nexus/psyche_state.json  (等core)
            bridge ───────→ .nexus/bridge_state.json  (等core)
                │
Running ─── health check ──→ 每 10s 扫描 daemon 存活
```

## 状态文件参照

| 文件 | 生产者 | 关键字段 |
|:--|:--|:--|
| `env_state.json` | env_checker | gpu.name, vram_free_mb, models[] |
| `core_state.json` | core | vram_used_mb, loaded_models[], gpu_lock_held |
| `algo_state.json` | algo | total_engines, audit_passed |
| `daemon.json` | daemon | uptime_seconds, cycle, psi_field_written |
| `psyche_state.json` | psyche | current_dim, ae, we, emergence_level |
| `bridge_state.json` | bridge | mcp_connected, seeds_total |
| `coordinator_state.json` | coordinator | phase, components{} |

## 数据目录结构 (.nexus/)

```
.nexus/
├── env_state.json          环境信息
├── core_state.json         VRAM + 模型状态
├── algo_state.json         算法引擎状态
├── daemon.json             守护进程心跳
├── psyche_state.json       Ψ 导航器状态
├── bridge_state.json       桥接状态
├── coordinator_state.json  协调器状态
├── psi_field.mmap          意识流 (1MB ring buffer)
├── seed_declarations.jsonl 种子声明 (JSONL)
├── seed_store.json         种子培育库
├── willchain_db.jsonl      意志链 (JSONL)
├── wisdom_db.json          Sophia Core 智慧库
├── agi_capability_brief.json 能力简报
├── memory/                 全局记忆
│   ├── INDEX.json
│   ├── keywords.json
│   ├── timeline.jsonl
│   └── entries/
└── logs/                   组件日志
    ├── env_checker.log
    ├── core.log
    ├── algo.log
    ├── daemon.log
    ├── psyche.log
    ├── bridge.log
    └── coordinator.log
```

## 常见场景

### 场景 1: 快速验证系统

```bash
env_checker.exe --output .nexus/env.json
algo.exe --env .nexus/env_state.json --validate
psyche.exe --oneshot --steps 200
```

### 场景 2: 全系统运行

```bash
coordinator.exe --start
# Ctrl+C 后自动优雅关闭
```

### 场景 3: 调试单个组件

```bash
# 只验证算法引擎
algo.exe --env .nexus/env_state.json --list --validate

# 只验证意识层
psyche.exe --oneshot --steps 500

# 只验证桥接
bridge.exe --dry-run
```
