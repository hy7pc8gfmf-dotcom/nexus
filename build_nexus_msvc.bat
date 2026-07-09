@echo off
REM ──────────────────────────────────────────────────────────────
REM build_nexus_msvc.bat — 手动设置 MSVC 环境后编译 Nexus
REM 绕过 vcvars64.bat (vswhere.exe 缺失的问题)
REM ──────────────────────────────────────────────────────────────

setlocal enabledelayedexpansion

set MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207
set SDK_ROOT=C:\Program Files (x86)\Windows Kits\10
set SDK_VER=10.0.22621.0

REM 手动设置环境变量
set "PATH=%MSVC_ROOT%\bin\Hostx64\x64;%PATH%"
set "INCLUDE=%MSVC_ROOT%\include;%SDK_ROOT%\Include\%SDK_VER%\ucrt;%SDK_ROOT%\Include\%SDK_VER%\shared;%SDK_ROOT%\Include\%SDK_VER%\um"
set "LIB=%MSVC_ROOT%\lib\x64;%SDK_ROOT%\Lib\%SDK_VER%\ucrt\x64;%SDK_ROOT%\Lib\%SDK_VER%\um\x64"

REM 验证
cl.exe 2>&1 | find "Microsoft" >nul
if errorlevel 1 (
    echo [错误] cl.exe 不可用，检查 MSVC 路径
    echo   MSVC_ROOT=%MSVC_ROOT%
    exit /b 1
)
echo [OK] MSVC cl.exe 已就绪

REM 配置 CMake
echo [Nexus] 配置 CMake...
cmake -B build -S . -G Ninja ^
    -DCMAKE_C_COMPILER=cl.exe ^
    -DCMAKE_CXX_COMPILER=cl.exe ^
    -DNEXUS_CUDA_SUPPORT=OFF ^
    -DNEXUS_BUILD_TESTS=OFF ^
    -DNEXUS_USE_VCPKG=OFF
if errorlevel 1 (
    echo [错误] CMake 配置失败
    exit /b 1
)

REM 编译
echo [Nexus] 编译...
cmake --build build -j

if errorlevel 1 (
    echo [错误] 编译失败
    exit /b 1
)

echo [OK] 编译成功
echo 产物: build\bin\
endlocal
