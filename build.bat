@echo off
REM ──────────────────────────────────────────────────────────────
REM build.bat — Nexus 构建脚本 (Windows cmd.exe)
REM
REM 不依赖 Visual Studio 生成器检测。
REM 让 CMake 自动选本机可用的生成器 (Ninja/MinGW Makefiles)。
REM ──────────────────────────────────────────────────────────────

echo [Nexus] 配置中...
cmake -B build -S . -DNEXUS_CUDA_SUPPORT=OFF -DNEXUS_BUILD_TESTS=OFF -DNEXUS_USE_VCPKG=OFF

if errorlevel 1 (
    echo [错误] CMake 配置失败
    pause
    exit /b 1
)

echo [Nexus] 编译中...
cmake --build build --config Release -j

if errorlevel 1 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

echo [OK] 编译成功
dir /b build\bin\Release\*.exe 2>nul
pause
