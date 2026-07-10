# Nexus 打包指南

## Windows 安装包

### 前置要求

1. **Inno Setup 6** — 下载: https://jrsoftware.org/isinfo.php
   - 安装时勾选 "Install Inno Setup Preprocessor" (ISPP)
2. **Nexus 已编译完成**

### 生成安装包

**方式 A — 手动编译（推荐）**
```batch
:: 先确保 Release 构建完成
cmake --build build --config Release

:: 编译安装包
ISCC.exe packaging\windows\setup.iss
```

输出位置: `dist\Nexus-1.0.0-Setup.exe`

**方式 B — 批处理一键打包**
```batch
build_package.bat
```

### 安装界面

安装程序为标准 Windows Wizard 风格：
1. 选择语言（中文/English）
2. 许可协议
3. 安装路径（默认 `C:\Program Files\Nexus`）
4. 选择组件（完整/精简/自定义）
5. 选项：添加到 PATH / CUDA 支持
6. 安装 → 完成

### 安装后

- 开始菜单 → Nexus → 各模块快捷方式
- 环境变量 `NEXUS_DATA_DIR` 自动注册
- 运行 `env_checker` 验证环境
- 运行 `daemon` 启动后台守护

### 静默安装（自动化部署）
```batch
Nexus-1.0.0-Setup.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
```

---

## Linux 安装包

### 方式 A — 安装脚本
```bash
chmod +x packaging/linux/install.sh
sudo ./packaging/linux/install.sh
```

### 方式 B — 用户目录安装（无 root）
```bash
./packaging/linux/install.sh --prefix=~/.local
source ~/.local/env.sh
```

### 方式 C — 通过 CMake CPack（.deb / .rpm）
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DNEXUS_CUDA_SUPPORT=OFF
cmake --build build --parallel $(nproc)
cpack --config build/CPackConfig.cmake -G DEB   # 或 RPM
```

### 卸载
```bash
sudo ./packaging/linux/install.sh --uninstall
```

---

## 输出结构

```
dist/
├── Nexus-1.0.0-Setup.exe          ← Windows 安装包
├── nexus-1.0.0-Linux.deb          ← Debian/Ubuntu 包
└── nexus-1.0.0-Linux.rpm          ← Fedora/RHEL 包
```

## 安装后的目录结构

```
{install_prefix}/
├── bin/
│   ├── neural.exe / neural          ← 神经路由核心
│   ├── daemon.exe / daemon          ← 后台守护进程
│   ├── core.exe / core              ← 核心推理引擎
│   ├── algo.exe / algo              ← 算法管理
│   ├── algo_engine.exe / algo_engine ← 算法工作器
│   ├── env_checker.exe / env_checker ← 环境检查
│   ├── psyche.exe / psyche          ← Ψ 心理引擎
│   ├── bridge.exe / bridge          ← 桥接层
│   ├── coordinator.exe / coordinator ← 对话协调器
│   ├── quantum.exe / quantum        ← 量子系统
│   ├── logic.exe / logic            ← 逻辑求解引擎
│   └── holographic.exe / holographic ← 全息系统
├── data/                            ← 持久化状态
├── logs/                            ← 运行日志
├── conf/                            ← 配置文件
├── examples/                        ← 示例代码
├── share/doc/
│   ├── README.md
│   ├── LICENSE
│   └── NOTICE
└── env.sh                           ← Linux 环境变量 (仅 Linux)
```
