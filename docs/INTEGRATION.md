# Nexus 安装与集成指南

## 下载与安装

```bash
# 1. 从 GitHub Releases 下载 nexus-v1.0.0.zip
#    https://github.com/hy7pc8gfmf-dotcom/nexus/releases

# 2. 解压到任意目录
unzip nexus-v1.0.0.zip -d D:/nexus
#   得到 9 个 EXE 文件

# 3. 运行环境检测
env_checker.exe --output .nexus/env.json --verbose
#   → GPU: RTX 3070 8GB | 29 个模型 | VRAM 预算 6128 MB
```

## 三种使用模式

### 模式 A：全系统启动（推荐）

```bash
coordinator.exe --start
# → 自动启动 6 个组件:
#   env_checker → core + algo + daemon → psyche + bridge
# → daemon 持续运行, 其他组件执行后退出

coordinator.exe --status
# → 查看各组件状态

coordinator.exe --stop
# → 优雅关闭
```

### 模式 B：按需调用单个组件

```bash
# 推理
core.exe --model "D:/hf_cache/VibeThinker-1.5B.Q4_K_M.gguf"
core.exe --skip-models  # 仅初始化 VRAM

# 算法
algo.exe --list       # 列出 12 个算法引擎
algo.exe --validate   # 运行验证

# 意识
psyche.exe --oneshot --steps 500

# 量子系统
quantum.exe --oneshot --steps 100
```

### 模式 C：嵌入智能体框架

```python
import subprocess, json, os

NEXUS_DIR = "D:/nexus"
os.chdir(NEXUS_DIR)

# 1. 启动神经系统守护
daemon = subprocess.Popen(
    ["daemon.exe", "--env", ".nexus/env.json", "--daemon"])

# 2. 调用推理
result = subprocess.run(
    ["core.exe", "--skip-models"],
    capture_output=True, text=True, cwd=NEXUS_DIR)

# 3. 调用算法引擎
result = subprocess.run(
    ["algo.exe", "--validate", "--list"],
    capture_output=True, text=True, cwd=NEXUS_DIR)

# 4. 读取状态
with open(".nexus/daemon.json") as f:
    state = json.load(f)
    print(f"守护运行: {state['status']} (PID {state['pid']})")
```

## 智能体框架集成

### 与 CherryStudio 集成

在 CherryStudio 的 MCP 配置中添加 Nexus 桥接：

```json
{
  "mcpServers": {
    "nexus": {
      "command": "cmd.exe",
      "args": ["/c", "D:/nexus/coordinator.exe", "--start"],
      "env": {}
    }
  }
}
```

### 与 Claude Code / Cursor 集成

使用 `coordinator.exe --status` 获取系统状态：

```bash
# 在智能体的 system prompt 中嵌入:
# "启动前先检查 Nexus 神经系统: coordinator.exe --status"
```

### Python SDK 封装

```python
"""nexus_client.py — Nexus 神经系统 Python 客户端"""

import subprocess, json, os, time
from pathlib import Path

class Nexus:
    def __init__(self, nexus_dir="D:/nexus"):
        self.dir = Path(nexus_dir)
        os.chdir(self.dir)

    def start(self):
        """启动全系统"""
        return subprocess.run(["coordinator.exe", "--start"], cwd=self.dir)

    def stop(self):
        """优雅关闭"""
        return subprocess.run(["coordinator.exe", "--stop"], cwd=self.dir)

    def status(self) -> dict:
        """获取各组件状态"""
        result = subprocess.run(
            ["coordinator.exe", "--status"],
            capture_output=True, text=True, cwd=self.dir)
        return self._parse_status(result.stdout)

    def infer(self, model_path: str) -> str:
        """加载模型并推理"""
        result = subprocess.run(
            ["core.exe", "--model", model_path],
            capture_output=True, text=True, cwd=self.dir, timeout=120)
        return result.stdout

    def detect_env(self) -> dict:
        """检测 GPU 和环境"""
        result = subprocess.run(
            ["env_checker.exe", "--output", str(self.dir/".nexus/env.json")],
            capture_output=True, text=True, cwd=self.dir)
        return result.stdout

    def list_engines(self) -> list:
        """列出算法引擎"""
        result = subprocess.run(
            ["algo.exe", "--list"], capture_output=True, text=True, cwd=self.dir)
        return result.stdout.split('\n')

    def psyche_step(self, steps=100):
        """运行意识导航"""
        return subprocess.run(
            ["psyche.exe", "--oneshot", "--steps", str(steps)],
            capture_output=True, text=True, cwd=self.dir)

    def daemon_running(self) -> bool:
        """检查守护是否运行"""
        state_file = self.dir/".nexus/daemon.json"
        if not state_file.exists(): return False
        with open(state_file) as f:
            return json.load(f).get("status") == "running"

    def _parse_status(self, output: str) -> dict:
        states = {}
        for line in output.split('\n'):
            parts = line.strip().split()
            if len(parts) >= 2:
                states[parts[0]] = parts[1]
        return states


# 使用示例
if __name__ == "__main__":
    nx = Nexus()
    env = nx.detect_env()
    print("GPU:", "可用" if "VRAM" in env else "不可用")
    nx.start()
    time.sleep(2)
    print(nx.status())
    nx.stop()
```
