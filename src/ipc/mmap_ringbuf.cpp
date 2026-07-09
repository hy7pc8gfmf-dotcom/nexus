/**
 * @file mmap_ringbuf.cpp
 * @brief mmap 环状缓冲区实现 — Windows 平台
 *
 * 使用 CreateFileMapping + MapViewOfFile 实现跨进程共享内存。
 * 用于 psi_field（意识流暂存器）的进程间通信。
 *
 * 布局:
 *   HEADER (64 bytes): write_pointer, read_pointer, version, flags
 *   RING (1MB - 64 bytes): ~4092 条记录, 每条 256 bytes
 *
 * 并发安全:
 *   写入者: 写指针单调递增, 写后 memory_order_release
 *   读取者: 读最新记录 (wp-1), 不需要读锁
 *   丢帧容忍: 多进程同时写入可能丢中间记录
 */

#include "nexus/ipc/mmap_ringbuf.h"

#include <atomic>
#include <cstring>
#include <iostream>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace nexus::ipc {

// ── 验证编译时常量 ──
static_assert(kFieldSize == 1 * 1024 * 1024, "psi_field must be exactly 1MB");
static_assert(kRecordSize == 256, "record must be exactly 256 bytes");
static_assert(kHeaderSize == 64, "header must be exactly 64 bytes");

// ═══════════════════════════════════════════════════════════════════
// 构造 / 析构 / 移动
// ═══════════════════════════════════════════════════════════════════

MmapRingBuffer::~MmapRingBuffer() noexcept {
  if (mmap_) {
    ::UnmapViewOfFile(mmap_);
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
    if (mmap_) ::UnmapViewOfFile(mmap_);
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

  // 1. 确保目录存在
  auto dir = path.parent_path();
  if (!dir.empty()) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
      return Status::Error(ErrorCode::kIoError,
        "cannot create directory: " + dir.string());
    }
  }

  // 2. 如果文件不存在，创建一个 1MB 的空文件
  if (!fs::exists(path)) {
    HANDLE hFile = ::CreateFileW(
      path.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      CREATE_NEW,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
      return Status::Error(ErrorCode::kIoError,
        "CreateFile failed: " + path.string());
    }

    // 设置文件大小
    LARGE_INTEGER size;
    size.QuadPart = static_cast<LONGLONG>(kFieldSize);
    if (!::SetFilePointerEx(hFile, size, nullptr, FILE_BEGIN)) {
      ::CloseHandle(hFile);
      return Status::Error(ErrorCode::kIoError, "SetFilePointerEx failed");
    }
    if (!::SetEndOfFile(hFile)) {
      ::CloseHandle(hFile);
      return Status::Error(ErrorCode::kIoError, "SetEndOfFile failed");
    }
    ::CloseHandle(hFile);
  }

  // 3. 打开文件映射 (FILE_MAP_ALL_ACCESS 允许读写)
  HANDLE hMapping = ::CreateFileMappingW(
    INVALID_HANDLE_VALUE,   // 使用系统分页文件（跨进程共享）
    nullptr,
    PAGE_READWRITE,
    0,
    static_cast<DWORD>(kFieldSize),
    (L"nexus_psi_field_" + path.filename().wstring()).c_str());

  if (!hMapping) {
    // 如果命名映射已存在，打开它
    hMapping = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE,
      (L"nexus_psi_field_" + path.filename().wstring()).c_str());
    if (!hMapping) {
      // Fallback: 使用文件映射（非命名）
      HANDLE hFile = ::CreateFileW(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
      if (hFile == INVALID_HANDLE_VALUE) {
        return Status::Error(ErrorCode::kIoError,
          "CreateFile (fallback) failed: " + path.string());
      }
      hMapping = ::CreateFileMappingW(hFile, nullptr, PAGE_READWRITE,
        0, static_cast<DWORD>(kFieldSize), nullptr);
      ::CloseHandle(hFile);
      if (!hMapping) {
        return Status::Error(ErrorCode::kIoError,
          "CreateFileMapping (fallback) failed");
      }
    }
  }

  // 4. 映射视图
  void* view = ::MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, kFieldSize);
  ::CloseHandle(hMapping);  // 映射后可以关闭句柄（视图保持引用）

  if (!view) {
    return Status::Error(ErrorCode::kIoError, "MapViewOfFile failed");
  }

  buf.mmap_ = view;

  // 5. 如果这是新创建的（写指针为 0），初始化头部的读指针
  auto* header = static_cast<uint8_t*>(view);
  auto wp = *reinterpret_cast<volatile uint64_t*>(header + 0);
  if (wp == 0) {
    // 初始化读指针
    *reinterpret_cast<uint64_t*>(header + 8) = 0;
    // 写版本
    *reinterpret_cast<uint32_t*>(header + 16) = kCurrentVersion;
  }

  return buf;
}

// ═══════════════════════════════════════════════════════════════════
// 写入
// ═══════════════════════════════════════════════════════════════════

auto MmapRingBuffer::write(const std::string& content,
                           const std::string& channel) noexcept -> Status {
  if (!mmap_) {
    return Status::Error(ErrorCode::kInternal, "mmap not initialized");
  }
  if (content.empty()) {
    return Status::Ok();  // 空内容跳过
  }

  // 1. 构造 JSON 记录
  auto data = nlohmann::json{
    {"ts", current_unix_time()},
    {"c",  content},
    {"ch", channel},
  }.dump();

  // 截断到最大内容长度
  if (data.size() > kContentMaxLen) {
    data = data.substr(0, kContentMaxLen);
  }

  // 2. 当前写入位置
  auto* header = static_cast<uint8_t*>(mmap_);
  auto wp = *reinterpret_cast<volatile uint64_t*>(header + 0);
  auto record_idx = wp % kMaxRecords;
  auto record_pos = kHeaderSize + record_idx * kRecordSize;

  // 3. 写入记录
  auto* record = static_cast<uint8_t*>(mmap_) + record_pos;

  // 清空记录（先写分隔符，再写内容）
  std::memset(record, 0, kRecordSize);
  std::memcpy(record, data.data(), data.size());

  // 最后 5 字节作为记录分隔符标记 (5个换行符)
  record[kRecordSize - 5] = '\n';
  record[kRecordSize - 4] = '\n';
  record[kRecordSize - 3] = '\n';
  record[kRecordSize - 2] = '\n';
  record[kRecordSize - 1] = '\n';

  // 4. 内存屏障后更新写指针（release 语义）
  std::atomic_thread_fence(std::memory_order_release);
  *reinterpret_cast<volatile uint64_t*>(header + 0) = wp + 1;

  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 读取最新一条
// ═══════════════════════════════════════════════════════════════════

auto MmapRingBuffer::read_latest() const noexcept
    -> std::optional<std::string> {
  if (!mmap_) return std::nullopt;

  auto* header = static_cast<const uint8_t*>(mmap_);

  // acquire 语义读取写指针
  auto wp = *reinterpret_cast<const volatile uint64_t*>(header + 0);
  std::atomic_thread_fence(std::memory_order_acquire);

  if (wp == 0) return std::nullopt;

  auto record_idx = (wp - 1) % kMaxRecords;
  auto record_pos = kHeaderSize + record_idx * kRecordSize;
  auto* record = static_cast<const uint8_t*>(mmap_) + record_pos;

  // 查找分隔符 \n\n\n\n\n
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
  if (!mmap_) return {};

  auto* header = static_cast<uint8_t*>(mmap_);
  auto wp = *reinterpret_cast<volatile uint64_t*>(header + 0);
  std::atomic_thread_fence(std::memory_order_acquire);

  // 计算可读取的未读条目
  // read_idx_ 是本地的读取游标，不写入共享内存
  uint64_t available = wp - read_idx_;
  if (available == 0 || available > kMaxRecords) {
    read_idx_ = wp;  // 如果超出范围，跳到当前
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
  if (!mmap_) return 0;
  auto* header = static_cast<const uint8_t*>(mmap_);
  return *reinterpret_cast<const volatile uint64_t*>(header + 0);
}

}  // namespace nexus::ipc
