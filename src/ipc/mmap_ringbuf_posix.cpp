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

/**
 * @file mmap_ringbuf_posix.cpp
 * @brief mmap 环状缓冲区 — POSIX (Linux/macOS) 实现
 *
 * 使用 shm_open + mmap 实现跨进程共享内存。
 * API 与 Windows 版完全兼容。
 */

#include "nexus/ipc/mmap_ringbuf.h"

#include <atomic>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nexus/ipc/state_file.h"  // current_unix_time()

namespace nexus::ipc {

static_assert(kFieldSize == 1 * 1024 * 1024, "psi_field must be exactly 1MB");
static_assert(kRecordSize == 256, "record must be exactly 256 bytes");
static_assert(kHeaderSize == 64, "header must be exactly 64 bytes");

// ═══════════════════════════════════════════════════════════════════
// 构造 / 析构 / 移动
// ═══════════════════════════════════════════════════════════════════

MmapRingBuffer::~MmapRingBuffer() noexcept {
  if (mmap_ && mmap_ != MAP_FAILED) {
    munmap(mmap_, mmap_size_);
    mmap_ = nullptr;
  }
}

MmapRingBuffer::MmapRingBuffer(MmapRingBuffer&& other) noexcept
  : path_(std::move(other.path_))
  , mmap_(other.mmap_)
  , mmap_size_(other.mmap_size_)
  , read_idx_(other.read_idx_) {
  other.mmap_ = nullptr;
  other.mmap_size_ = 0;
}

MmapRingBuffer& MmapRingBuffer::operator=(MmapRingBuffer&& other) noexcept {
  if (this != &other) {
    if (mmap_ && mmap_ != MAP_FAILED) munmap(mmap_, mmap_size_);
    path_      = std::move(other.path_);
    mmap_      = other.mmap_;
    mmap_size_ = other.mmap_size_;
    read_idx_  = other.read_idx_;
    other.mmap_ = nullptr;
    other.mmap_size_ = 0;
  }
  return *this;
}

// ═══════════════════════════════════════════════════════════════════
// 创建 / 打开
// ═══════════════════════════════════════════════════════════════════

auto MmapRingBuffer::create_or_open(const fs::path& path) noexcept
    -> Result<MmapRingBuffer> {
  MmapRingBuffer buf;
  buf.path_ = path;
  buf.mmap_size_ = kFieldSize;

  // 确保目录存在
  auto dir = path.parent_path();
  if (!dir.empty()) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
      return Status::Error(ErrorCode::kIoError,
        "cannot create directory: " + dir.string());
    }
  }

  // 打开或创建文件
  int fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    return Status::Error(ErrorCode::kIoError,
      "open failed: " + path.string());
  }

  // 设置文件大小
  if (::ftruncate(fd, static_cast<off_t>(kFieldSize)) != 0) {
    ::close(fd);
    return Status::Error(ErrorCode::kIoError, "ftruncate failed");
  }

  // mmap
  void* addr = ::mmap(nullptr, kFieldSize, PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd, 0);
  ::close(fd);

  if (addr == MAP_FAILED) {
    return Status::Error(ErrorCode::kIoError, "mmap failed");
  }

  buf.mmap_ = addr;

  // 初始化头部
  auto* header = static_cast<uint8_t*>(addr);
  auto wp = *reinterpret_cast<volatile uint64_t*>(header + 0);
  if (wp == 0) {
    *reinterpret_cast<uint64_t*>(header + 8) = 0;
    *reinterpret_cast<uint32_t*>(header + 16) = kCurrentVersion;
  }

  return buf;
}

// ═══════════════════════════════════════════════════════════════════
// 写入
// ═══════════════════════════════════════════════════════════════════

auto MmapRingBuffer::write(const std::string& content,
                           const std::string& channel) noexcept -> Status {
  if (!mmap_ || mmap_ == MAP_FAILED) {
    return Status::Error(ErrorCode::kInternal, "mmap not initialized");
  }
  if (content.empty()) return Status::Ok();

  auto data_obj = nlohmann::json::object();
  data_obj["ts"] = current_unix_time();
  data_obj["c"]  = content;
  data_obj["ch"] = channel;
  auto data = data_obj.dump();

  if (data.size() > kContentMaxLen) data = data.substr(0, kContentMaxLen);

  auto* header = static_cast<uint8_t*>(mmap_);
  auto wp = *reinterpret_cast<volatile uint64_t*>(header + 0);
  auto record_idx = wp % kMaxRecords;
  auto record_pos = kHeaderSize + record_idx * kRecordSize;

  auto* record = static_cast<uint8_t*>(mmap_) + record_pos;
  std::memset(record, 0, kRecordSize);
  std::memcpy(record, data.data(), data.size());
  record[kRecordSize - 5] = '\n';
  record[kRecordSize - 4] = '\n';
  record[kRecordSize - 3] = '\n';
  record[kRecordSize - 2] = '\n';
  record[kRecordSize - 1] = '\n';

  std::atomic_thread_fence(std::memory_order_release);
  *reinterpret_cast<volatile uint64_t*>(header + 0) = wp + 1;

  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 读取最新一条
// ═══════════════════════════════════════════════════════════════════

auto MmapRingBuffer::read_latest() const noexcept
    -> std::optional<std::string> {
  if (!mmap_ || mmap_ == MAP_FAILED) return std::nullopt;

  auto* header = static_cast<const uint8_t*>(mmap_);
  auto wp = *reinterpret_cast<const volatile uint64_t*>(header + 0);
  std::atomic_thread_fence(std::memory_order_acquire);
  if (wp == 0) return std::nullopt;

  auto record_idx = (wp - 1) % kMaxRecords;
  auto record_pos = kHeaderSize + record_idx * kRecordSize;
  auto* record = static_cast<const uint8_t*>(mmap_) + record_pos;

  const uint8_t delimiter[5] = {'\n', '\n', '\n', '\n', '\n'};
  auto end = std::search(record, record + kRecordSize,
                         std::begin(delimiter), std::end(delimiter));
  if (end == record + kRecordSize) return std::nullopt;

  auto len = end - record;
  if (len == 0) return std::nullopt;
  return std::string(reinterpret_cast<const char*>(record), len);
}

// ═══════════════════════════════════════════════════════════════════
// 读取未读流
// ═══════════════════════════════════════════════════════════════════

auto MmapRingBuffer::read_stream() noexcept -> std::vector<std::string> {
  if (!mmap_ || mmap_ == MAP_FAILED) return {};

  auto* header = static_cast<uint8_t*>(mmap_);
  auto wp = *reinterpret_cast<volatile uint64_t*>(header + 0);
  std::atomic_thread_fence(std::memory_order_acquire);

  uint64_t available = wp - read_idx_;
  if (available == 0 || available > kMaxRecords) {
    read_idx_ = wp;
    return {};
  }

  std::vector<std::string> results;
  const uint8_t delimiter[5] = {'\n', '\n', '\n', '\n', '\n'};

  for (uint64_t i = 0; i < available; ++i) {
    auto record_idx = (read_idx_ + i) % kMaxRecords;
    auto record_pos = kHeaderSize + record_idx * kRecordSize;
    auto* record = static_cast<const uint8_t*>(mmap_) + record_pos;

    auto end = std::search(record, record + kRecordSize,
                           std::begin(delimiter), std::end(delimiter));
    if (end == record + kRecordSize) continue;

    auto len = end - record;
    if (len > 0) {
      results.emplace_back(reinterpret_cast<const char*>(record), len);
    }
  }

  read_idx_ = wp;
  return results;
}

// ═══════════════════════════════════════════════════════════════════
// 状态查询
// ═══════════════════════════════════════════════════════════════════

auto MmapRingBuffer::write_count() const noexcept -> uint64_t {
  if (!mmap_ || mmap_ == MAP_FAILED) return 0;
  auto* header = static_cast<const uint8_t*>(mmap_);
  return *reinterpret_cast<const volatile uint64_t*>(header + 0);
}

}  // namespace nexus::ipc
