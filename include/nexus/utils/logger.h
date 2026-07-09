#ifndef NEXUS_UTILS_LOGGER_H
#define NEXUS_UTILS_LOGGER_H

/**
 * @file logger.h
 * @brief 统一日志初始化
 *
 * 使用 spdlog，每个组件有独立 logger。
 * 日志文件按天轮转，保留 7 天。
 */

#include <memory>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace nexus::utils {

/// 初始化组件日志
/// @param component 组件名称（如 "core", "daemon"）
/// @param log_dir   日志目录（如 ".nexus/logs"）
/// @param level     日志级别（如 "info", "debug"）
inline auto init_logger(
    const std::string& component,
    const std::string& log_dir,
    const std::string& level = "info") noexcept
    -> std::shared_ptr<spdlog::logger> {

  // 控制台 sink
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_pattern(fmt::format(
    "[{{}}] [{}] [%^%l%$] %v",
    component));

  // 文件 sink (5MB 轮转, 保留 3 个)
  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    fmt::format("{}/{}.log", log_dir, component),
    5 * 1024 * 1024,  // 5MB
    3);

  file_sink->set_pattern(fmt::format(
    "[{{}}] [{}] [%l] %v",
    fmt::gmtime(std::chrono::system_clock::now())));

  // 创建 logger
  auto logger = std::make_shared<spdlog::logger>(
    "nexus." + component,
    spdlog::sinks_init_list{console_sink, file_sink});

  // 设置日志级别
  if (level == "debug")       logger->set_level(spdlog::level::debug);
  else if (level == "info")   logger->set_level(spdlog::level::info);
  else if (level == "warn")   logger->set_level(spdlog::level::warn);
  else if (level == "error")  logger->set_level(spdlog::level::err);
  else                        logger->set_level(spdlog::level::info);

  spdlog::register_logger(logger);
  return logger;
}

/// 快捷宏 — 替代 NEXUS_LOG(info, "message {}", arg) 形式
#define NEXUS_LOG(logger, level, ...) \
  (logger)->level(__VA_ARGS__)

}  // namespace nexus::utils

#endif  // NEXUS_UTILS_LOGGER_H
