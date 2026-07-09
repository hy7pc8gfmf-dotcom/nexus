# Nexus 构建指南

## 前提条件

| 依赖 | 版本要求 | 安装方式 |
|:--|:--|:--|
| Visual Studio | 2022 (v143) | Visual Studio Installer — 勾选"使用 C++ 的桌面开发" |
| CUDA Toolkit | 12.x | [NVIDIA 开发者网站](https://developer.nvidia.com/cuda-downloads) |
| CMake | ≥ 3.25 | `winget install CMake` 或 [cmake.org](https://cmake.org) |
| vcpkg | 最新 | 见下方说明 |
| Git | ≥ 2.40 | `winget install Git.Git` |

## 安装 vcpkg

```powershell
# 克隆 vcpkg（推荐放在 D:/dev/ 下）
cd D:/dev
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

# 集成到全局（可选）
vcpkg integrate install
```

## 配置和构建

```powershell
# 1. 克隆 Nexus 源码
git clone <nexus-repo-url> D:/nexus
cd D:/nexus

# 2. 配置 CMake
cmake -B build -S . `
  -DCMAKE_TOOLCHAIN_FILE="D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_BUILD_TYPE=Release `
  -DNEXUS_BUILD_TESTS=ON

# 3. 编译
cmake --build build --config Release -j

# 4. 运行测试
ctest --test-dir build --output-on-failure -C Release

# 5. 运行系统
.\build\bin\Release\coordinator.exe --start
```

## 常见问题

### vcpkg 下载依赖超时

```powershell
# 设置代理（需要先有 HTTP 代理）
set HTTPS_PROXY=http://127.0.0.1:7890
# 或使用二进制缓存
vcpkg binary-caching --provider=nuget --nuget-repository=D:/.vcpkg-cache
```

### CUDA 未找到

```powershell
# 确认 CUDA_PATH 环境变量已设置
echo $env:CUDA_PATH
# 如果未设置：
$env:CUDA_PATH = "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.8"
```

### MSVC 编译器未找到

在"Developer PowerShell for VS 2022"中运行 CMake，或在普通终端中运行：

```powershell
& "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/Tools/Launch-VsDevShell.ps1"
```

## 构建配置

| 配置 | 用途 |
|:--|:--|
| `Debug` | 开发调试，-O0 + 调试符号 |
| `Release` | 生产发布，-O2 + LTCG |
| `RelWithDebInfo` | 带符号的发布构建 |
| `MinSizeRel` | 最小体积发布构建 |

### 不同配置构建

```powershell
cmake -B build/debug -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --config Debug -j

cmake -B build/release -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --config Release -j
```
