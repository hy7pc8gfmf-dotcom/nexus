/**
 * @file state_file_posix.cpp
 * @brief 状态文件读写 — POSIX (Linux/macOS) 实现
 *
 * 使用 POSIX 文件锁 (flock) 替代 Windows Named Mutex。
 * 用法与 Windows 版完全兼容。
 */

#include "nexus/ipc/state_file.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <thread>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

namespace nexus::ipc {

// ═══════════════════════════════════════════════════════════════════
// FileLock — POSIX flock
// ═══════════════════════════════════════════════════════════════════

FileLock::FileLock(FileLock&& other) noexcept
  : fd_(other.fd_), path_(std::move(other.path_)) {
  other.fd_ = -1;
}

FileLock& FileLock::operator=(FileLock&& other) noexcept {
  if (this != &other) {
    release();
    fd_ = other.fd_;
    path_ = std::move(other.path_);
    other.fd_ = -1;
  }
  return *this;
}

FileLock::~FileLock() noexcept {
  release();
}

auto FileLock::acquire(const std::string& path, int timeout_ms) noexcept -> bool {
  if (fd_ >= 0) return true;

  std::string lock_path = path + ".lock";
  fd_ = ::open(lock_path.c_str(), O_CREAT | O_RDWR, 0644);
  if (fd_ < 0) return false;

  // 带超时的轮询锁
  auto deadline = std::chrono::steady_clock::now()
                + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    if (::flock(fd_, LOCK_EX | LOCK_NB) == 0) {
      path_ = path;
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ::close(fd_);
  fd_ = -1;
  return false;
}

void FileLock::release() noexcept {
  if (fd_ >= 0) {
    ::flock(fd_, LOCK_UN);
    ::close(fd_);
    fd_ = -1;
  }
}

auto FileLock::is_held() const noexcept -> bool {
  return fd_ >= 0;
}

// ═══════════════════════════════════════════════════════════════════
// StateFileWriter
// ═══════════════════════════════════════════════════════════════════

auto StateFileWriter::write(const nlohmann::json& data) noexcept -> Status {
  std::lock_guard<std::mutex> lock(mutex_);

  FileLock file_lock;
  if (!file_lock.acquire(path_ + ".lock", 1000)) {
    return Status::Error(ErrorCode::kFileLockBusy, "file lock timeout: " + path_);
  }

  try {
    std::string json_str = data.dump(2);
    std::string tmp_path = path_ + ".tmp";

    {
      std::ofstream ofs(tmp_path, std::ios::binary);
      if (!ofs.is_open()) {
        return Status::Error(ErrorCode::kIoError, "cannot open: " + tmp_path);
      }
      ofs.write(json_str.data(), static_cast<std::streamsize>(json_str.size()));
      ofs.flush();
      if (!ofs.good()) {
        return Status::Error(ErrorCode::kIoError, "write failed: " + tmp_path);
      }
    }

    if (std::rename(tmp_path.c_str(), path_.c_str()) != 0) {
      std::remove(tmp_path.c_str());
      return Status::Error(ErrorCode::kIoError, "rename failed: " + path_);
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
    if (!exists()) {
      if (i < retries) {
        std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
        continue;
      }
      return Status::Error(ErrorCode::kFileNotFound, "not found: " + path_);
    }

    try {
      std::ifstream ifs(path_, std::ios::binary);
      if (!ifs.is_open()) {
        if (i < retries) {
          std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
          continue;
        }
        return Status::Error(ErrorCode::kIoError, "cannot open: " + path_);
      }

      nlohmann::json data;
      ifs >> data;
      return data;
    } catch (const nlohmann::parse_error& e) {
      if (i < retries) {
        std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
        continue;
      }
      return Status::Error(ErrorCode::kJsonParseError, e.what());
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
  return static_cast<int>(::getpid());
}

}  // namespace nexus::ipc
