// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NEXUS_TYPES_STATUS_H
#define NEXUS_TYPES_STATUS_H

/**
 * @file status.h
 * @brief Nexus 统一错误处理类型
 *
 * 不使用 C++ 异常。所有可能失败的函数返回 Status 或 Result<T>。
 * 调用者必须显式检查返回值。
 *
 * 用法:
 * @code
 *   auto status = vram_manager.load_model("qwythos_9b", path);
 *   if (!status.ok()) {
 *     NEXUS_LOG(error, "模型加载失败: {}", status.message());
 *     return status;
 *   }
 *
 *   // 带返回值的版本
 *   ASSIGN_OR_RETURN(auto stats, read_state_file("env.json"));
 *   // stats 在此处可用
 * @endcode
 */

#include <cassert>
#include <cstdint>
#include <string>
#include <variant>
#include <source_location>

namespace nexus {

// ═══════════════════════════════════════════════════════════════════
// 错误码
// ═══════════════════════════════════════════════════════════════════

/// 错误码（使用 struct 包装 constexpr 以兼容 MSVC 14.44+ 的 enum class 限定名解析）
struct ErrorCode {
  static constexpr int32_t kOk              = 0;
  static constexpr int32_t kEnvNotFound     = 1;   // env.json 不存在
  static constexpr int32_t kEnvInvalid      = 2;   // env.json 格式错误
  static constexpr int32_t kGpuUnavailable  = 3;   // GPU 不可用
  static constexpr int32_t kVramInsufficient= 4;   // VRAM 不足
  static constexpr int32_t kModelNotFound   = 5;   // 模型文件不存在
  static constexpr int32_t kModelLoadFailed = 6;   // 模型加载失败
  static constexpr int32_t kEngineFailed    = 7;   // 算法引擎执行失败
  static constexpr int32_t kIpcTimeout      = 8;   // 文件锁/等待超时
  static constexpr int32_t kComponentCrashed= 9;   // 子进程崩溃
  static constexpr int32_t kInvalidConfig   = 10;  // 配置错误
  static constexpr int32_t kFileNotFound    = 11;  // 文件不存在
  static constexpr int32_t kJsonParseError  = 12;  // JSON 解析错误
  static constexpr int32_t kFileLockBusy    = 13;  // 文件锁被占用
  static constexpr int32_t kNotSupported    = 14;  // 不支持的操作
  static constexpr int32_t kOutOfMemory     = 15;  // 内存不足
  static constexpr int32_t kIoError         = 16;  // I/O 错误
  static constexpr int32_t kInternal        = 99;  // 内部错误
};

/// 错误码转可读字符串
constexpr auto error_code_to_string(int32_t code) noexcept -> const char* {
  switch (code) {
    case ErrorCode::kOk:               return "OK";
    case ErrorCode::kEnvNotFound:      return "ENV_NOT_FOUND";
    case ErrorCode::kEnvInvalid:       return "ENV_INVALID";
    case ErrorCode::kGpuUnavailable:   return "GPU_UNAVAILABLE";
    case ErrorCode::kVramInsufficient: return "VRAM_INSUFFICIENT";
    case ErrorCode::kModelNotFound:    return "MODEL_NOT_FOUND";
    case ErrorCode::kModelLoadFailed:  return "MODEL_LOAD_FAILED";
    case ErrorCode::kEngineFailed:     return "ENGINE_FAILED";
    case ErrorCode::kIpcTimeout:       return "IPC_TIMEOUT";
    case ErrorCode::kComponentCrashed: return "COMPONENT_CRASHED";
    case ErrorCode::kInvalidConfig:    return "INVALID_CONFIG";
    case ErrorCode::kFileNotFound:     return "FILE_NOT_FOUND";
    case ErrorCode::kJsonParseError:   return "JSON_PARSE_ERROR";
    case ErrorCode::kFileLockBusy:     return "FILE_LOCK_BUSY";
    case ErrorCode::kNotSupported:     return "NOT_SUPPORTED";
    case ErrorCode::kOutOfMemory:      return "OUT_OF_MEMORY";
    case ErrorCode::kIoError:          return "IO_ERROR";
    case ErrorCode::kInternal:         return "INTERNAL_ERROR";
  }
  return "UNKNOWN";
}

// ═══════════════════════════════════════════════════════════════════
// Status — 无值返回
// ═══════════════════════════════════════════════════════════════════

struct Status {
  int32_t code{ErrorCode::kOk};
  std::string message;

  static auto Ok() noexcept -> Status {
    return Status{ErrorCode::kOk, ""};
  }

  static auto Error(int32_t code, std::string msg = "") noexcept -> Status {
    return Status{code, std::move(msg)};
  }

  [[nodiscard]] auto ok() const noexcept -> bool {
    return code == ErrorCode::kOk;
  }

  [[nodiscard]] auto to_string() const -> std::string {
    if (ok()) return "OK";
    return std::string(error_code_to_string(code)) + ": " + message;
  }
};

// ═══════════════════════════════════════════════════════════════════
// Result<T> — 有值返回
// ═══════════════════════════════════════════════════════════════════

template<typename T>
class Result {
  std::variant<T, Status> data_;

public:
  Result(T value) noexcept : data_(std::move(value)) {}
  Result(Status error) noexcept : data_(std::move(error)) {
    assert(!error.ok());  // NOLINT
  }

  [[nodiscard]] auto ok() const noexcept -> bool {
    return std::holds_alternative<T>(data_);
  }

  [[nodiscard]] auto value() const noexcept -> const T& {
    return std::get<T>(data_);
  }

  [[nodiscard]] auto value() -> T& {
    return std::get<T>(data_);
  }

  [[nodiscard]] auto value_or(T&& fallback) const noexcept -> T {
    return ok() ? std::get<T>(data_) : std::move(fallback);
  }

  [[nodiscard]] auto error() const noexcept -> const Status& {
    return std::get<Status>(data_);
  }

  [[nodiscard]] auto to_string() const -> std::string {
    if (ok()) return "OK";
    return error().to_string();
  }
};

// ═══════════════════════════════════════════════════════════════════
// 便利宏
// ═══════════════════════════════════════════════════════════════════

/// 返回错误（如果表达式失败）
#define RETURN_IF_ERROR(expr)                          \
  do {                                                 \
    auto _s = (expr);                                  \
    if (!_s.ok()) {                                    \
      return ::nexus::Status::Error(                    \
          _s.code, _s.message);                        \
    }                                                  \
  } while (0)

/// 赋值或返回错误
#define ASSIGN_OR_RETURN(var, expr)                    \
  auto _r = (expr);                                    \
  if (!_r.ok()) {                                      \
    return ::nexus::Status::Error(                      \
        _r.error().code, _r.error().message);          \
  }                                                    \
  var = std::move(_r.value())

}  // namespace nexus

#endif  // NEXUS_TYPES_STATUS_H
