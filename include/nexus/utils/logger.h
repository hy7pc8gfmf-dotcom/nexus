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

#ifndef NEXUS_UTILS_LOGGER_H
#define NEXUS_UTILS_LOGGER_H

/**
 * @file logger.h
 * @brief 统一日志 — 自包含实现，无外部依赖
 *
 * 输出到 stderr，格式: [timestamp] [component] [level] message
 * 日志文件: 同时写入 {log_dir}/{component}.log (追加模式)
 *
 * 当 vcpkg 可用时，可切换回 spdlog 以获得彩色输出、文件轮转等特性。
 * 当前实现为最小可行版本。
 */

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

namespace nexus::utils {

// ═══════════════════════════════════════════════════════════════════
// 日志级别
// ═══════════════════════════════════════════════════════════════════

enum class LogLevel {
  kDebug = 0,
  kInfo  = 1,
  kWarn  = 2,
  kError = 3,
};

constexpr auto log_level_to_string(LogLevel level) noexcept -> const char* {
  switch (level) {
    case LogLevel::kDebug: return "debug";
    case LogLevel::kInfo:  return "info";
    case LogLevel::kWarn:  return "warn";
    case LogLevel::kError: return "error";
  }
  return "unknown";
}

constexpr auto parse_log_level(const std::string& s) noexcept -> LogLevel {
  if (s == "debug") return LogLevel::kDebug;
  if (s == "info")  return LogLevel::kInfo;
  if (s == "warn")  return LogLevel::kWarn;
  if (s == "error") return LogLevel::kError;
  return LogLevel::kInfo;
}

// ═══════════════════════════════════════════════════════════════════
// Logger — 简单日志器
// ═══════════════════════════════════════════════════════════════════

class Logger {
public:
  Logger(std::string component, std::string log_dir = "",
         LogLevel level = LogLevel::kInfo) noexcept
    : component_(std::move(component))
    , level_(level) {

    // 尝试打开日志文件
    if (!log_dir.empty()) {
      std::error_code ec;
      std::filesystem::create_directories(log_dir, ec);
      if (!ec) {
        log_file_.open(
          log_dir + "/" + component_ + ".log",
          std::ios::app);
      }
    }
  }

  ~Logger() noexcept {
    if (log_file_.is_open()) log_file_.close();
  }

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
  Logger(Logger&&) noexcept = default;
  Logger& operator=(Logger&&) noexcept = default;

  // ── 核心日志方法 ──
  template<typename... Args>
  void debug(std::format_string<Args...> fmt, Args&&... args) {
    write(LogLevel::kDebug, std::format(fmt, std::forward<Args>(args)...));
  }

  template<typename... Args>
  void info(std::format_string<Args...> fmt, Args&&... args) {
    write(LogLevel::kInfo, std::format(fmt, std::forward<Args>(args)...));
  }

  template<typename... Args>
  void warn(std::format_string<Args...> fmt, Args&&... args) {
    write(LogLevel::kWarn, std::format(fmt, std::forward<Args>(args)...));
  }

  template<typename... Args>
  void error(std::format_string<Args...> fmt, Args&&... args) {
    write(LogLevel::kError, std::format(fmt, std::forward<Args>(args)...));
  }

  // ── 直接写入字符串（已格式化的） ──
  void write(LogLevel level, const std::string& message) noexcept {
    if (level < level_) return;

    auto timestamp = current_timestamp();
    auto level_str = log_level_to_string(level);

    auto line = std::format("[{}] [{}] [{}] {}\n",
      timestamp, component_, level_str, message);

    // stderr
    std::lock_guard<std::mutex> lock(mutex_);
    std::fputs(line.c_str(), stderr);

    // 日志文件
    if (log_file_.is_open()) {
      log_file_ << line;
      log_file_.flush();
    }
  }

private:
  std::string component_;
  LogLevel level_;
  std::mutex mutex_;
  std::ofstream log_file_;

  static auto current_timestamp() -> std::string {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto tt = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}",
      tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
      tm.tm_hour, tm.tm_min, tm.tm_sec,
      static_cast<int>(ms.count()));
  }
};

// ═══════════════════════════════════════════════════════════════════
// 工厂函数
// ═══════════════════════════════════════════════════════════════════

inline auto init_logger(
    const std::string& component,
    const std::string& log_dir = "",
    const std::string& level = "info") noexcept
    -> std::unique_ptr<Logger> {
  return std::make_unique<Logger>(
    component, log_dir, parse_log_level(level));
}

// ═══════════════════════════════════════════════════════════════════
// 便捷宏
// ═══════════════════════════════════════════════════════════════════

#define NEXUS_LOG(logger, level, ...)  (logger)->level(__VA_ARGS__)

}  // namespace nexus::utils

#endif  // NEXUS_UTILS_LOGGER_H
