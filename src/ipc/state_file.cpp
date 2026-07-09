/**
 * @file state_file.cpp
 * @brief 状态文件读写实现 — Windows 平台
 *
 * 核心机制:
 *   写入: FileLock(1s timeout) → serialize JSON → .tmp → rename(原子替换)
 *   读取: open → 重试机制 → parse JSON
 */

#include "nexus/ipc/state_file.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <thread>

// ── Windows 头文件（谨慎包含，避免宏污染） ──
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace nexus::ipc {

// ═══════════════════════════════════════════════════════════════════
// FileLock — Windows Named Mutex
// ═══════════════════════════════════════════════════════════════════

FileLock::FileLock(FileLock&& other) noexcept
  : mutex_(other.mutex_)
  , name_(std::move(other.name_)) {
  other.mutex_ = nullptr;
}

FileLock& FileLock::operator=(FileLock&& other) noexcept {
  if (this != &other) {
    release();
    mutex_ = other.mutex_;
    name_ = std::move(other.name_);
    other.mutex_ = nullptr;
  }
  return *this;
}

FileLock::~FileLock() noexcept {
  release();
}

auto FileLock::acquire(const std::string& path, int timeout_ms) noexcept -> bool {
  if (mutex_) return true;  // 已经持有

  // 使用路径的哈希构建互斥体名称（唯一、可预测）
  auto hash = std::hash<std::string>{}(path);
  name_ = "Global\\nexus_lock_" + std::to_string(hash);

  HANDLE h = CreateMutexA(nullptr, FALSE, name_.c_str());
  if (!h) return false;

  DWORD wait = WaitForSingleObject(h, static_cast<DWORD>(timeout_ms));
  if (wait == WAIT_OBJECT_0) {
    mutex_ = h;
    return true;
  }

  // 超时或出错
  CloseHandle(h);
  mutex_ = nullptr;
  return false;
}

void FileLock::release() noexcept {
  if (mutex_) {
    ReleaseMutex(static_cast<HANDLE>(mutex_));
    CloseHandle(static_cast<HANDLE>(mutex_));
    mutex_ = nullptr;
  }
}

// ═══════════════════════════════════════════════════════════════════
// StateFileWriter
// ═══════════════════════════════════════════════════════════════════

auto StateFileWriter::write(const nlohmann::json& data) noexcept -> Status {
  std::lock_guard<std::mutex> lock(mutex_);

  // 1. 获取文件锁
  FileLock file_lock;
  if (!file_lock.acquire(path_ + ".lock", 1000)) {
    return Status::Error(ErrorCode::kFileLockBusy,
      "file lock timeout: " + path_);
  }

  try {
    // 2. 序列化
    std::string json_str = data.dump(2);

    // 3. 写入临时文件
    std::string tmp_path = path_ + ".tmp";
    {
      std::ofstream ofs(tmp_path, std::ios::binary);
      if (!ofs.is_open()) {
        return Status::Error(ErrorCode::kIoError,
          "cannot open temp file: " + tmp_path);
      }
      ofs.write(json_str.data(), static_cast<std::streamsize>(json_str.size()));
      ofs.flush();
      if (!ofs.good()) {
        return Status::Error(ErrorCode::kIoError,
          "write failed: " + tmp_path);
      }
    }

    // 4. 原子重命名（Windows: MoveFileEx 覆盖已存在）
    if (!MoveFileExA(tmp_path.c_str(), path_.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
      // 重命名失败，清理临时文件
      std::remove(tmp_path.c_str());
      return Status::Error(ErrorCode::kIoError,
        "atomic rename failed: " + path_);
    }

    return Status::Ok();
  } catch (const std::exception& e) {
    return Status::Error(ErrorCode::kInternal, e.what());
  }
}

// ═══════════════════════════════════════════════════════════════════
// StateFileReader
// ═══════════════════════════════════════════════════════════════════

auto StateFileReader::read(int retries, int retry_delay_ms) noexcept
    -> Result<nlohmann::json> {
  for (int i = 0; i <= retries; ++i) {
    // 检查文件是否存在
    if (!exists()) {
      if (i < retries) {
        std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
        continue;
      }
      return Status::Error(ErrorCode::kFileNotFound,
        "file not found: " + path_);
    }

    // 尝试解析
    try {
      std::ifstream ifs(path_, std::ios::binary);
      if (!ifs.is_open()) {
        if (i < retries) {
          std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
          continue;
        }
        return Status::Error(ErrorCode::kIoError,
          "cannot open file: " + path_);
      }

      nlohmann::json data;
      ifs >> data;
      return data;
    } catch (const nlohmann::json::parse_error& e) {
      if (i < retries) {
        std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
        continue;
      }
      return Status::Error(ErrorCode::kJsonParseError,
        std::string(e.what()) + " [" + path_ + "]");
    } catch (const std::exception& e) {
      return Status::Error(ErrorCode::kInternal, e.what());
    }
  }

  return Status::Error(ErrorCode::kInternal, "unreachable");
}

// ═══════════════════════════════════════════════════════════════════
// 工具函数
// ═══════════════════════════════════════════════════════════════════

auto current_pid() -> int {
  return static_cast<int>(GetCurrentProcessId());
}

}  // namespace nexus::ipc
