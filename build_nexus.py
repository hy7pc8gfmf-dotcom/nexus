#!/usr/bin/env python3
"""
build_nexus.py — Nexus 编译脚本
在 VS2022 开发者命令行下运行 cmake 配置 + 编译。
"""

import subprocess
import sys
import os
import time

LOG = r"D:\nexus\_build_log.txt"
BAT = r"D:\nexus\_build_temp.bat"

# 删除可能存在的旧文件
for f in [LOG, BAT]:
    if os.path.exists(f):
        os.remove(f)

bat_content = f"""@echo off
set LOG={LOG}
echo === Nexus Build Log === > %LOG%
call "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >> %LOG% 2>&1
if errorlevel 1 echo VCVARS_FAILED >> %LOG%
cmake --version >> %LOG% 2>&1
echo --- CONFIG --- >> %LOG%
cmake -B D:\\nexus\\build -S D:\\nexus -G "Visual Studio 17 2022" -A x64 -DNEXUS_CUDA_SUPPORT=OFF -DNEXUS_BUILD_TESTS=OFF -DNEXUS_USE_VCPKG=OFF >> %LOG% 2>&1
echo CONFIG_DONE=%ERRORLEVEL% >> %LOG%
echo --- BUILD --- >> %LOG%
cmake --build D:\\nexus\\build --config Release -j >> %LOG% 2>&1
echo BUILD_DONE=%ERRORLEVEL% >> %LOG%
"""

with open(BAT, "w", newline="\r\n") as f:
    f.write(bat_content)

print("Running build script...", flush=True)
proc = subprocess.Popen(
    ["cmd.exe", "/c", BAT],
    stdout=subprocess.DEVNULL,
    stderr=subprocess.DEVNULL,
)
proc.wait(timeout=180)
print(f"Subprocess RC: {proc.returncode}", flush=True)
time.sleep(1)

if os.path.exists(LOG):
    with open(LOG, "rb") as f:
        raw = f.read()
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        text = raw.decode("gbk", errors="replace")
    print(text[-5000:])
else:
    print("LOG NOT FOUND")
    sys.exit(1)

# 检查生成的 EXE
exe_dir = r"D:\nexus\build\bin\Release"
if os.path.exists(exe_dir):
    print("\n=== EXEs ===")
    for f in os.listdir(exe_dir):
        if f.endswith(".exe"):
            sz = os.path.getsize(os.path.join(exe_dir, f))
            print(f"  {f:30s} {sz // 1024} KB")

# 清理
for f in [BAT]:
    if os.path.exists(f):
        os.remove(f)
