@echo off
REM ─────────────────────────────────────────────────────────────────────
REM Nexus Windows 一键打包脚本
REM ─────────────────────────────────────────────────────────────────────
REM 用法:
REM   build_package.bat             正常打包
REM   build_package.bat --skip-build  跳过构建，直接打包
REM ─────────────────────────────────────────────────────────────────────

setlocal enabledelayedexpansion

title Nexus Packaging

echo ═══════════════════════════════════════
echo  Nexus Windows 安装包生成
echo ═══════════════════════════════════════
echo.

REM ── 参数解析 ──
set SKIP_BUILD=0
if "%1"=="--skip-build" set SKIP_BUILD=1

REM ── 0. 检查前置 ──
echo [1/4] 检查工具链...

where ISCC.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] 未找到 ISCC.exe (Inno Setup 6)
    echo         请安装: https://jrsoftware.org/isinfo.php
    echo         或确保 ISCC.exe 在 PATH 中
    exit /b 1
)
echo   Inno Setup:     OK

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] 未找到 cmake
    exit /b 1
)
echo   CMake:         OK

where msbuild >nul 2>&1
if errorlevel 1 (
    echo [WARN] 未找到 msbuild (尝试 vsdevcmd)
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
            call "%%i\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
        )
    )
)
echo   MSBuild:       OK
echo.

REM ── 1. 构建 ──
if "%SKIP_BUILD%"=="0" (
    echo [2/4] 构建 Release...
    cmake --build build --config Release
    if errorlevel 1 (
        echo [ERROR] 构建失败
        exit /b 1
    )
    echo   构建完成
) else (
    echo [2/4] 跳过构建
)
echo.

REM ── 2. 验证产物 ──
echo [3/4] 验证构建产物...

set COUNT=0
for %%e in (neural daemon core algo algo_engine env_checker psyche bridge coordinator quantum logic holographic) do (
    if exist "build\bin\Release\%%e.exe" (
        set /a COUNT+=1
    ) else (
        echo   [WARN] 缺失: %%e.exe
    )
)
echo   已验证 %COUNT% 个可执行文件
echo.

REM ── 3. 打包 ──
echo [4/4] 生成安装包...

if not exist "dist" mkdir dist

ISCC.exe packaging\windows\setup.iss
if errorlevel 1 (
    echo [ERROR] 安装包生成失败
    exit /b 1
)

echo.
echo ═══════════════════════════════════════
echo  安装包生成完成!
echo.
for %%f in (dist\Nexus-*-Setup.exe) do (
    echo  输出: %%f
    for %%s in (%%f) do echo  大小: %%~zs 字节
)
echo ═══════════════════════════════════════

endlocal
