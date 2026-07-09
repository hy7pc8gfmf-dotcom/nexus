@echo off
REM ──────────────────────────────────────────────────────────────
REM build.bat — Nexus 一键构建 (cmd.exe)
REM 用法: build.bat                Debug 构建
REM        build.bat release       Release 构建
REM ──────────────────────────────────────────────────────────────

setlocal enabledelayedexpansion
set CONFIG=Debug
if /i "%1"=="release" set CONFIG=Release

echo [Nexus] 配置: %CONFIG%

REM 让 CMake 自动选择生成器 (不指定 -G)
cmake -B build -S . -DNEXUS_CUDA_SUPPORT=OFF -DNEXUS_BUILD_TESTS=OFF -DNEXUS_USE_VCPKG=OFF
if errorlevel 1 (
    echo [错误] CMake 配置失败
    pause
    exit /b 1
)

cmake --build build --config %CONFIG% -j
if errorlevel 1 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

echo [OK] 编译成功
echo [OK] EXE: build\bin\%CONFIG%\
pause
