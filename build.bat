@echo off
REM ──────────────────────────────────────────────────────────────
REM build.bat — Nexus 构建脚本 (Windows + VS2022)
REM 用法:
REM   build              Debug 构建
REM   build release      Release 构建
REM   build minimal      最小构建 (无 CUDA)
REM   build vcpkg        vcpkg 模式构建
REM   build clean        清理构建目录
REM ──────────────────────────────────────────────────────────────

setlocal enabledelayedexpansion

set BUILD_DIR=%CD%\build
set GENERATOR="Visual Studio 17 2022"
set ARCH=x64

if "%1"=="clean" (
  echo [Nexus] 清理构建目录...
  if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
  echo [Nexus] 已清理
  exit /b 0
)

REM 启动 VS2022 x64 开发者命令行
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul

if errorlevel 1 (
  echo [Nexus] 错误: VS2022 x64 开发者命令行未找到
  echo [Nexus] 确认路径: C:\Program Files\Microsoft Visual Studio\2022\Community\
  exit /b 1
)

echo [Nexus] VS2022 x64 开发者命令行已加载
echo.

set CONFIG=Debug
set PRESET=default
set CACHE_FLAGS=

if /i "%1"=="release"   set CONFIG=Release
if /i "%2"=="release"   set CONFIG=Release
if /i "%1"=="minimal"   set PRESET=minimal
if /i "%2"=="minimal"   set PRESET=minimal
if /i "%1"=="vcpkg"     set PRESET=vcpkg
if /i "%2"=="vcpkg"     set PRESET=vcpkg

echo [Nexus] 配置: preset=%PRESET% config=%CONFIG%
echo.

REM 配置
echo [Nexus] 配置 CMake...
cmake -B %BUILD_DIR% -S . --preset %PRESET% %CACHE_FLAGS%
if errorlevel 1 (
  echo [Nexus] 配置失败
  exit /b 1
)

echo.
echo [Nexus] 编译...
cmake --build %BUILD_DIR% --config %CONFIG% -j
if errorlevel 1 (
  echo [Nexus] 编译失败
  exit /b 1
)

echo.
echo [Nexus] 编译成功: %BUILD_DIR%\bin\%CONFIG%\
dir /b %BUILD_DIR%\bin\%CONFIG%\*.exe 2>nul

endlocal
