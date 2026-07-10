// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#include "nexus/core/thermal_governor.h"

#ifdef _MSC_VER
// MSVC /permissive- 模式下编译 thermal_governor 需要放宽
// Windows SDK sal.h 的严格检查。此文件使用 _popen 和 nvidia-smi,
// 不涉及 Windows API 的严格类型安全。
#pragma warning(disable : 4668)  // 未定义的预处理器宏
#pragma warning(disable : 4582)  // 构造函数未隐式调用
#pragma warning(disable : 4583)  // 析构函数未隐式调用
#pragma warning(disable : 4191)  // 不安全的类型转换
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

namespace nexus::core {