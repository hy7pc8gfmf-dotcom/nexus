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

#ifndef NEXUS_IPC_STATE_FILE_H
#define NEXUS_IPC_STATE_FILE_H

/**
 * @file state_file.h
 * @brief 状态文件读写封装
 *
 * 提供线程安全、崩溃安全的 JSON 状态文件读写。
 * 写入流程: serialize → .tmp → rename(原子替换)
 * 读取流程: 先检查 .lock 是否被持有 → 读文件 → 解析 JSON
 */

#include <chrono>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "nexus/types/status.h"

namespace nexus::ipc {

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════
// 文件锁 (Windows: Named Mutex / POSIX: flock)
// ═══════════════════════════════════════════════════════════════════

class FileLock {
public:
  FileLock() noexcept = default;
  ~FileLock() noexcept;

  FileLock(const FileLock&) = delete;
  FileLock& operator=(const FileLock&) = delete;
  FileLock(FileLock&&) noexcept;
  FileLock& operator=(FileLock&&) noexcept;

  /// 获取文件锁
  auto acquire(const std::string& path, int timeout_ms = 1000) noexcept -> bool;

  /// 释放锁
  void release() noexcept;

  /// 是否持有锁
  [[nodiscard]] auto is_held() const noexcept -> bool;

private:
#ifdef _WIN32
  void* mutex_ = nullptr;
  std::string name_;
#else
  int fd_ = -1;
  std::string path_;
#endif
};

// ═══════════════════════════════════════════════════════════════════
// 状态文件写入器
// ═══════════════════════════════════════════════════════════════════

class StateFileWriter {
public:
  explicit StateFileWriter(std::string path) noexcept
    : path_(std::move(path)) {}

  /// 写入 JSON 状态（含锁保护 + 原子重命名）
  auto write(const nlohmann::json& data) noexcept -> Status;

  /// 获取文件路径
  [[nodiscard]] auto path() const noexcept -> const std::string& { return path_; }

private:
  std::string path_;
  std::mutex mutex_;  // 本进程内互斥
};

// ═══════════════════════════════════════════════════════════════════
// 状态文件读取器
// ═══════════════════════════════════════════════════════════════════

class StateFileReader {
public:
  explicit StateFileReader(std::string path) noexcept
    : path_(std::move(path)) {}

  /// 读取状态文件，失败时重试
  auto read(int retries = 1, int retry_delay_ms = 100) noexcept
      -> Result<nlohmann::json>;

  /// 检查文件是否存在
  [[nodiscard]] auto exists() const noexcept -> bool {
    return fs::exists(path_);
  }

  /// 获取文件路径
  [[nodiscard]] auto path() const noexcept -> const std::string& { return path_; }

private:
  std::string path_;
};

// ═══════════════════════════════════════════════════════════════════
// 工具函数
// ═══════════════════════════════════════════════════════════════════

/// 获取当前 ISO 8601 时间戳
auto current_iso_timestamp() -> std::string;

/// 获取当前 Unix 时间戳（秒）
auto current_unix_time() -> double;

/// 获取当前进程 PID
auto current_pid() -> int;

// ═══════════════════════════════════════════════════════════════════
// 实现（内联）
// ═══════════════════════════════════════════════════════════════════

inline auto current_iso_timestamp() -> std::string {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto tt = system_clock::to_time_t(now);
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm, &tt);
#else
  gmtime_r(&tt, &tm);
#endif
  char buf[24];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buf);
}

inline auto current_unix_time() -> double {
  using namespace std::chrono;
  return duration_cast<duration<double>>(
    system_clock::now().time_since_epoch()).count();
}

}  // namespace nexus::ipc

#endif  // NEXUS_IPC_STATE_FILE_H
