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

#ifndef NEXUS_IPC_MMAP_RINGBUF_H
#define NEXUS_IPC_MMAP_RINGBUF_H

/**
 * @file mmap_ringbuf.h
 * @brief 跨进程共享的内存映射环状缓冲区
 *
 * 用于 psi_field（意识流暂存器）的进程间通信。
 * 固定大小 1MB，包含 64 字节头部和 ~4092 个 256 字节记录。
 *
 * 写入安全: 写指针单调递增，写入后 memory_order_release
 * 读取安全: 读最新记录 (wp-1)，不需要读锁
 * 丢帧容忍: 多个写入者同时写可能丢中间记录（意识流允许）
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "nexus/types/status.h"

namespace nexus::ipc {

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════
// 常量
// ═══════════════════════════════════════════════════════════════════

constexpr size_t kFieldSize        = 1 * 1024 * 1024;  // 1MB
constexpr size_t kHeaderSize       = 64;
constexpr size_t kRecordSize       = 256;
constexpr size_t kMaxRecords       = (kFieldSize - kHeaderSize) / kRecordSize;
constexpr size_t kContentMaxLen    = kRecordSize - 8;  // 留 8 字节给分隔符
constexpr uint32_t kCurrentVersion = 1;

// ═══════════════════════════════════════════════════════════════════
// mmap 环状缓冲区
// ═══════════════════════════════════════════════════════════════════

class MmapRingBuffer {
public:
  /// 打开或创建 mmap 文件
  static auto create_or_open(const fs::path& path) noexcept
      -> Result<MmapRingBuffer>;

  ~MmapRingBuffer() noexcept;

  MmapRingBuffer(const MmapRingBuffer&) = delete;
  MmapRingBuffer& operator=(const MmapRingBuffer&) = delete;
  MmapRingBuffer(MmapRingBuffer&&) noexcept;
  MmapRingBuffer& operator=(MmapRingBuffer&&) noexcept;

  // ═══════════════════════════════════════════════════════════════
  // 写入
  // ═══════════════════════════════════════════════════════════════

  /// 写入一条意识记录
  auto write(const std::string& content, const std::string& channel) noexcept
      -> Status;

  /// 写入 JSON 对象
  auto write_json(const nlohmann::json& obj) noexcept -> Status {
    auto ch = obj.value("ch", std::string("main"));
    auto c  = obj.dump();
    return write(c, ch);
  }

  // ═══════════════════════════════════════════════════════════════
  // 读取
  // ═══════════════════════════════════════════════════════════════

  /// 读取最新一条记录
  [[nodiscard]] auto read_latest() const noexcept
      -> std::optional<std::string>;

  /// 读取从 read_idx 开始的未读流
  [[nodiscard]] auto read_stream() noexcept
      -> std::vector<std::string>;

  // ═══════════════════════════════════════════════════════════════
  // 状态
  // ═══════════════════════════════════════════════════════════════

  [[nodiscard]] auto write_count() const noexcept -> uint64_t;
  [[nodiscard]] auto is_open() const noexcept -> bool { return mmap_ != nullptr; }
  [[nodiscard]] auto file_path() const noexcept -> const fs::path& { return path_; }

private:
  MmapRingBuffer() noexcept = default;

  auto init_mmap_() noexcept -> Status;

  fs::path path_;
  void*    mmap_         = nullptr;  // mmap 基地址
  size_t   mmap_size_    = 0;
  uint64_t read_idx_     = 0;        // 本地读取游标
};

// ═══════════════════════════════════════════════════════════════════
// 意识记录结构
// ═══════════════════════════════════════════════════════════════════

struct ConsciousnessRecord {
  double      timestamp = 0.0;
  std::string channel   = "main";
  std::string content;

  [[nodiscard]] auto to_json() const -> nlohmann::json {
    return {
      {"ts", timestamp},
      {"ch", channel},
      {"c",  content},
    };
  }

  static auto from_json(const nlohmann::json& j) -> ConsciousnessRecord {
    return {
      .timestamp = j.value("ts", 0.0),
      .channel   = j.value("ch", std::string("main")),
      .content   = j.value("c",  std::string("")),
    };
  }
};

}  // namespace nexus::ipc

#endif  // NEXUS_IPC_MMAP_RINGBUF_H
