@echo off
REM ──────────────────────────────────────────────────────────────
REM setup_vcpkg.bat — Nexus vcpkg 环境一键部署
REM ├── 1. 安装 vcpkg (如未安装)
REM ├── 2. 安装依赖 (fmt, spdlog, nlohmann-json, gtest)
REM └── 3. cmake 配置 (NEXUS_USE_VCPKG=ON)
REM ──────────────────────────────────────────────────────────────

setlocal enabledelayedexpansion

set VCPKG_DIR=D:\dev\vcpkg
set NEXUS_DIR=%~dp0

echo [Nexus] vcpkg 环境部署
echo ================================

REM 1. 检查/安装 vcpkg
if exist %VCPKG_DIR%\vcpkg.exe (
    echo [Nexus] vcpkg 已安装: %VCPKG_DIR%
) else (
    echo [Nexus] 正在下载 vcpkg...
    if not exist %VCPKG_DIR% mkdir %VCPKG_DIR%
    git clone --depth 1 https://github.com/microsoft/vcpkg.git %VCPKG_DIR%
    if errorlevel 1 (
        echo [Nexus] 错误: vcpkg 下载失败, 请检查网络
        exit /b 1
    )
    echo [Nexus] 正在引导 vcpkg...
    call %VCPKG_DIR%\bootstrap-vcpkg.bat
    if errorlevel 1 (
        echo [Nexus] 错误: vcpkg 引导失败
        exit /b 1
    )
)

REM 2. 安装依赖
echo [Nexus] 安装依赖 (首次编译耗时约 10-30 分钟)...
cd %VCPKG_DIR%

set DEPS=fmt spdlog nlohmann-json gtest

for %%d in (%DEPS%) do (
    echo [Nexus]   -> %%d
    vcpkg install %%d --triplet x64-windows
    if errorlevel 1 (
        echo [Nexus] 警告: %%d 安装失败
    )
)

REM 3. cmake 配置 (vcpkg 模式)
cd /d %NEXUS_DIR%

echo [Nexus] 配置 CMake (vcpkg 模式)...
cmake -B build -S . ^
    -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake ^
    -DNEXUS_USE_VCPKG=ON ^
    -DNEXUS_CUDA_SUPPORT=ON ^
    -DNEXUS_BUILD_TESTS=ON

if errorlevel 1 (
    echo [Nexus] CMake 配置失败
    exit /b 1
)

REM 4. 编译
echo [Nexus] 编译...
cmake --build build --config Release -j

if errorlevel 1 (
    echo [Nexus] 编译失败
    exit /b 1
)

REM 5. 运行测试
echo [Nexus] 运行测试...
ctest --test-dir build --output-on-failure -C Release

echo ================================
echo [Nexus] 部署完成!
echo [Nexus] EXE 位置: build\bin\Release\
endlocal
