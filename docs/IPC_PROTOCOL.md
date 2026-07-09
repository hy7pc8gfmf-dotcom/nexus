# Nexus IPC 协议规范

> 版本: v1.0  
> 更新: 2026-07-09

---

## 1. 概述

Nexus 使用**文件 IPC** 作为进程间通信的唯⼀手段。没有 socket、没有命名管道、没有共享内存（mmap 除外）。所有通信通过对 `.nexus/` 目录中文件的写入和读取完成。

### 1.1 设计理由

```
网络 IPC 的问题              Nexus 文件 IPC 的方案
───────────────────────────── ─────────────────────────────
端口冲突、防火墙拦截         没有端口，只有文件
服务发现复杂（DNS/注册中心）  文件路径就是地址
序列化框架绑定               JSON 文本，任何语言可读
连接管理复杂                 无连接，无会话
调试困难                     cat 文件即可看到状态
```

---

## 2. 文件类型

### 2.1 状态文件（State File）

每个组件一个 JSON 文件，写入自己的运行时窗口。

```
路径: .nexus/{component}_state.json
示例: .nexus/core_state.json
格式: JSON
写入者: 组件自身
读取者: coordinator + 依赖组件
更新频率: 状态变化时 + 定期心跳
```

### 2.2 锁文件（Lock File）

```
路径: .nexus/gpu.lock
内容: JSON (pid, hostname, acquired_at, ttl_seconds)
锁机制: Windows Named Mutex + TTL 过期
```

### 2.3 mmap 文件

```
路径: .nexus/psi_field.mmap
大小: 1MB (固定)
访问: 内存映射文件 (mmap)
写入者: daemon.exe, psyche.exe
读取者: psyche.exe, daemon.exe
用途: 跨进程共享"意识流"(正在思考的内容)
```

### 2.4 日志文件

```
路径: .nexus/logs/{component}.log
格式: spdlog 格式
轮转: 每天轮转, 保留 7 天
```

### 2.5 追加文件（Append-only）

```
路径: .nexus/*.jsonl
格式: JSON Lines (每行一个 JSON 对象)
用途: 种子声明, 意志链, 认知观察
写入: 追加模式, 不覆盖
读取: 按行读取, 支持 tail
```

---

## 3. 状态文件读写协议

### 3.1 写入协议

```cpp
// 写入流程（使用 RAII FileLock）
Status write_state_file(const std::string& path, const nlohmann::json& data) {
  // 1. 获取文件锁 (最大等待 1000ms)
  FileLock lock;
  if (!lock.acquire(path + ".lock", 1000)) {
    return Status::Error(ErrorCode::kTimeout, "file lock acquisition timeout");
  }

  // 2. data 注入源信息
  data["updated_at"] = current_iso_timestamp();
  data["pid"] = getpid();

  // 3. 序列化到临时文件 (原子写入)
  std::string tmp_path = path + ".tmp";
  std::ofstream ofs(tmp_path);
  ofs << data.dump(2);
  ofs.close();

  // 4. 原子重命名 (替换原文件)
  if (rename(tmp_path.c_str(), path.c_str()) != 0) {
    return Status::Error(ErrorCode::kIoError, "atomic rename failed");
  }

  return Status::Ok();
}
```

### 3.2 读取协议

```cpp
// 读取流程
Result<nlohmann::json> read_state_file(const std::string& path, int retries = 1) {
  for (int i = 0; i <= retries; ++i) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
      if (i < retries) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }
      return Error{ErrorCode::kNotFound, "state file not found: " + path};
    }

    try {
      nlohmann::json data;
      ifs >> data;
      return data;
    } catch (const nlohmann::json::parse_error& e) {
      if (i < retries) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }
      return Error{ErrorCode::kInvalidArgument, "json parse error: " + std::string(e.what())};
    }
  }
  // unreachable
  return Error{ErrorCode::kInternal, "unreachable"};
}
```

### 3.3 状态字段约定

所有状态文件必须包含以下字段：

```json
{
  "$schema": "nexus-state-v1",
  "version": "1.0",
  "component": "core | algo | daemon | psyche | bridge | coordinator",
  "status": "starting | ready | degraded | error | stopped",
  "pid": 0,
  "started_at": "2026-07-09T18:00:00Z",
  "updated_at": "2026-07-09T18:05:00Z",
  "details": {}
}
```

---

## 4. mmap 环状缓冲区协议

`psi_field.mmap` 使用固定大小的环状缓冲区（ring buffer）。每个进程通过内存映射共享同一段物理内存。

### 4.1 布局

```
偏移      大小      内容
─────────────────────────────────────
0x0000    8 bytes   write_pointer (uint64, 环形写入计数)
0x0008    8 bytes   read_pointer  (uint64, 环形读取计数)
0x0010    4 bytes   version       (uint32, 当前=1)
0x0014    4 bytes   flags         (bitmask)
0x0018    40 bytes  保留 (填充至 64)
───────────────────────────────────── HEADER = 64 bytes
0x0040    256 bytes 记录 #0
0x0140    256 bytes 记录 #1
0x0240    256 bytes 记录 #2
...
0xFFC0    256 bytes 记录 #4091
───────────────────────────────────── 总记录数 = 4092
```

### 4.2 记录格式

每条记录 256 字节，固定格式：

```
字节 0-7  : JSON 开始标记 (0x7B)
字节 8-15 : timestamp (double, Unix epoch)
字节 16-31: channel (16 字节, 填充空格)
字节 32-247: JSON 内容 (216 字节)
字节 248-251: 分隔符 \n\n\n\n
字节 252-255: 保留
```

### 4.3 写入

```cpp
bool ringbuf_write(const void* mmap_base, const std::string& content,
                   const std::string& channel) {
  auto* header = static_cast<uint8_t*>(mmap_base);
  auto  wp = *reinterpret_cast<uint64_t*>(header + 0);
  auto  record_idx = wp % kMaxRecords;
  auto  record_pos = kHeaderSize + record_idx * kRecordSize;

  // 构造 JSON 记录
  auto data = nlohmann::json{
    {"ts", current_unix_time()},
    {"c", content},
    {"ch", channel}
  }.dump();

  if (data.size() > kContentSize) {
    data = data.substr(0, kContentSize);
  }

  // 写入记录
  auto* record = static_cast<uint8_t*>(mmap_base) + record_pos;
  std::memcpy(record + 0, data.data(), data.size());
  record[kRecordSize - 5] = '\n';
  record[kRecordSize - 4] = '\n';
  record[kRecordSize - 3] = '\n';
  record[kRecordSize - 2] = '\n';
  record[kRecordSize - 1] = '\n';

  // 内存屏障后更新写指针
  std::atomic_thread_fence(std::memory_order_release);
  *reinterpret_cast<uint64_t*>(header + 0) = wp + 1;

  return true;
}
```

### 4.4 读取

```cpp
std::optional<std::string> ringbuf_read_latest(const void* mmap_base) {
  auto* header = static_cast<const uint8_t*>(mmap_base);
  auto  wp = *reinterpret_cast<const uint64_t*>(header + 0);

  if (wp == 0) return std::nullopt;

  auto record_idx = (wp - 1) % kMaxRecords;
  auto record_pos = kHeaderSize + record_idx * kRecordSize;

  auto* record = static_cast<const uint8_t*>(mmap_base) + record_pos;
  // 查找 \n\n\n\n 分隔符
  auto end = std::search(record, record + kRecordSize,
                         "\n\n\n\n", "\n\n\n\n" + 4);
  if (end == record + kRecordSize) return std::nullopt;

  auto len = end - record;
  return std::string(reinterpret_cast<const char*>(record), len);
}
```

### 4.5 并发安全

- 写指针单调递增，不覆写未读记录（写入者在自己复写自己）
- 读取者总是读 `wp - 1`（最新一条），不需要读锁
- 内存屏障：写入后 `memory_order_release`，读取前 `memory_order_acquire`
- 没有线程锁——多进程同时写入时可能丢失中间记录，但神经网络的意识流允许丢帧

---

## 5. 文件锁协议

### 5.1 Windows 实现

```cpp
class FileLock {
  HANDLE mutex_ = nullptr;
  std::string name_;

public:
  bool acquire(const std::string& path, int timeout_ms = 1000) {
    // 使用文件路径的哈希作为命名互斥体名称
    name_ = "Global\\nexus_lock_" + std::to_string(std::hash<std::string>{}(path));
    mutex_ = CreateMutexA(nullptr, FALSE, name_.c_str());
    if (!mutex_) return false;
    auto wait = WaitForSingleObject(mutex_, timeout_ms);
    return wait == WAIT_OBJECT_0;
  }

  void release() {
    if (mutex_) {
      ReleaseMutex(mutex_);
      CloseHandle(mutex_);
      mutex_ = nullptr;
    }
  }

  ~FileLock() { release(); }
};
```

### 5.2 TTL 过期自动清理

```cpp
// GPU 锁带 TTL
struct GpuLock {
  int pid;
  std::string hostname;
  double acquired_at;  // unix timestamp
  int ttl_seconds = 120;

  [[nodiscard]] bool is_expired() const {
    return current_unix_time() - acquired_at > ttl_seconds;
  }
};
```

---

## 6. 进程管理协议

### 6.1 启动协议

组件启动时：

```
1. 读取 env.json 验证环境
2. 写状态文件 status=starting
3. 执行初始化
4. 写状态文件 status=ready
5. 进入主循环（或退出）
```

coordinator 检测到 `status=ready` 后认为组件已就绪。

### 6.2 健康检查

coordinator 每 10s 执行：

```
1. 读取组件的状态文件
2. 检查 status 是否 = ready
3. 检查 updated_at 是否 < 30s 前
4. 检查组件 PID 是否存活 (WaitForSingleObject, 0ms)
5. 以上任意失败 → 标记崩溃 → 尝试重启
```

### 6.3 优雅关闭

SIGTERM → 各组件：

```
1. coordinator 写 coordinator_state.json phase=stopping
2. coordinator 向 bridge 发 SIGTERM (先关无依赖的)
3. 等待 5s 检查状态文件
4. 向 psyche 发 SIGTERM
5. 等待 5s
6. 向 algo 发 SIGTERM
7. 等待 5s
8. 向 daemon 发 SIGTERM
9. 等待 5s
10. 向 core 发 SIGTERM (最后关 GPU)
11. 等待 5s
12. 对未退出进程发 SIGKILL (TerminateProcess)
13. 写 coordinator_state.json phase=done
```

---

## 7. JSON 状态文件格式参考

```jsonc
// .nexus/env.json
{
  "$schema": "nexus-state-v1",
  "version": "1.0",
  "component": "env",
  "status": "ready",           // env_checker 完成
  "pid": 1234,
  "started_at": "2026-07-09T18:00:00Z",
  "updated_at": "2026-07-09T18:00:01Z",
  "details": {
    "gpu": {
      "name": "NVIDIA GeForce RTX 3070",
      "vram_total_mb": 8192,
      "vram_free_mb": 7128,
      "budget_mb": 7192
    },
    "models": [
      { "id": "qwythos_9b", "path": "D:/hf_cache/qwythos-9b-1m.Q4_K_M.gguf", "size_mb": 5765, "cached": true }
    ]
  }
}
```

```jsonc
// .nexus/core_state.json
{
  "$schema": "nexus-state-v1",
  "version": "1.0",
  "component": "core",
  "status": "ready",
  "pid": 1235,
  "started_at": "2026-07-09T18:00:02Z",
  "updated_at": "2026-07-09T18:00:30Z",
  "details": {
    "vram_used_mb": 5765,
    "vram_free_mb": 2427,
    "loaded_models": ["qwythos_9b"],
    "gpu_lock_held": true
  }
}
```

```jsonc
// .nexus/daemon.json
{
  "$schema": "nexus-state-v1",
  "version": "1.0",
  "component": "daemon",
  "status": "running",
  "pid": 1237,
  "started_at": "2026-07-09T18:00:03Z",
  "updated_at": "2026-07-09T18:00:33Z",
  "details": {
    "uptime_seconds": 30,
    "cycle": 10,
    "psi_field_written": 5,
    "will_hooks_scanned": 12
  }
}
```

---

## 8. 错误处理约定

### 8.1 文件不存在

读取者遇到文件不存在时的行为：

| 文件 | 行为 |
|:--|:--|
| `env.json` | 退出（致命）——没有环境信息无法启动 |
| `core_state.json` | 等待（psyche/bridge 轮询直到文件出现） |
| `daemon.json` | 降级（psyche 继续运行但不读取意识流） |
| 其他 | 重试 3 次后标记 error |

### 8.2 JSON 解析错误

- 写入者确保原子写（tmp → rename）
- 读取者遇到解析错误 → 等待 100ms → 重试 1 次 → 标记 error
- 任何文件损坏都不应该导致进程崩溃（`try_parse` 而非 `parse`）

### 8.3 过期数据

- `updated_at` 超过 30s 未被更新 → 认为进程无响应
- coordinator 标记 crashed（不读数据本身，只检查时间戳）
- 锁文件 TTL 超过 120s → 自动释放

---

## 附录：通信总表

```
来源           目的           文件名              类型     频率
───────────────────────────────────────────────────────────────────
env_checker   所有组件       env.json             JSON     1x
core          coordinator   core_state.json      JSON     ~10s
algo          coordinator   algo_state.json      JSON     ~10s
daemon        coordinator   daemon.json          JSON     ~30s
psyche        coordinator   psyche_state.json    JSON     ~10s
bridge        coordinator   bridge_state.json    JSON     1x (初始化)
coordinator   用户          coordinator_state.json JSON    ~10s
daemon↔psyche —             psi_field.mmap       mmap     ~3s (每周期)
core          —             gpu.lock             锁       持有期间
daemon        —             will_hooks/*.json    文件      ~6s
daemon        —             *.jsonl              追加      ~6s
```
