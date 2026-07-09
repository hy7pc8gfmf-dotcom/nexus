# Nexus 构建指南

## 前提条件

| 依赖 | 版本要求 | 安装方式 |
|:--|:--|:--|
| Visual Studio | 2022 (v143) | Visual Studio Installer — 勾选"使用 C++ 的桌面开发" |
| CUDA Toolkit | 12.x | [NVIDIA 开发者网站](https://developer.nvidia.com/cuda-downloads) |
| CMake | ≥ 3.25 | `winget install CMake` 或 [cmake.org](https://cmake.org) |
| vcpkg | 最新 | 见下方说明 |
| Git | ≥ 2.40 | `winget install Git.Git` |

## 三种构建方式

### 方式 A: 一键部署 (推荐)

```powershell
# 自动安装 vcpkg + 依赖 + cmake 配置 + 编译
setup_vcpkg.bat
```

### 方式 B: vcpkg 手动

```powershell
# 1. 安装 vcpkg
cd D:/dev
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

# 2. 安装依赖
vcpkg install fmt spdlog nlohmann-json gtest --triplet x64-windows

# 3. 配置 (vcpkg 模式)
cd D:/nexus
cmake --preset vcpkg

# 4. 编译 + 测试
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure -C Release
```

### 方式 C: 无 vcpkg (内嵌 third_party)

```powershell
# 无网络可用时, 使用内嵌的 minimal json.hpp + std::format
cmake --preset default
cmake --build build --config Release -j
```

## CMake 预设

| 预设 | 说明 | vcpkg | CUDA | 测试 |
|:--|:--|:--|:--|:--|
| `default` | 默认 (内嵌依赖) | OFF | ON | OFF |
| `vcpkg` | vcpkg 完整模式 | ON | ON | ON |
| `minimal` | 最小构建 (无 CUDA) | OFF | OFF | OFF |

```powershell
# 使用预设
cmake --preset vcpkg
cmake --build build --config Release -j

# 或单行
cmake --preset default && cmake --build build --config Release -j
```

## 编译产物

| EXE | 大小 | 用途 |
|:--|:--|:--|
| `env_checker.exe` | 317 KB | 环境检测 |
| `daemon.exe` | 344 KB | 后台守护 |
| `core.exe` | 3.6 MB | 推理引擎 |
| `algo.exe` | 448 KB | 算法池 |
| `psyche.exe` | 337 KB | 意识层 |
| `bridge.exe` | 338 KB | 桥接层 |
| `coordinator.exe` | 325 KB | 生命周期 |
| `algo_engine.exe` | 10 KB | 隔离子进程 |

## 运行测试

```powershell
# 完整测试
ctest --test-dir build --output-on-failure -C Release

# 单独测试
build\bin\Release\env_checker.exe --output .nexus\env.json --verbose
build\bin\Release\algo.exe --env .nexus\env_state.json --validate
build\bin\Release\coordinator.exe --start
```

## 常见问题

### vcpkg 下载依赖超时

```powershell
# 设置代理
set HTTPS_PROXY=http://127.0.0.1:7890
# 或使用二进制缓存
vcpkg binary-caching --provider=nuget --nuget-repository=D:/.vcpkg-cache
```

### CUDA 未找到

```powershell
echo $env:CUDA_PATH
$env:CUDA_PATH = "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.4"
```

### MSVC 编译器未找到

```powershell
& "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/Tools/Launch-VsDevShell.ps1"
```
