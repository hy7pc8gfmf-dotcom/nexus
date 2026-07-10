# Nexus 贡献指南

感谢你考虑为 Nexus 做贡献！

## 贡献者许可协议 (CLA)

**所有贡献者在首个 Pull Request 合并前必须签署 CLA。**

- [个人贡献者协议](../CLA/ICLA.md) — 独立开发者
- [实体组织协议](../CLA/CCLA.md) — 公司/机构
- 签署后发送到 `nexus-cla@cherryclaw.dev`

## 开发环境

| 平台 | 编译器 | 最低版本 |
|------|--------|---------|
| Windows | MSVC | 19.44 (VS 2022 17.14) |
| Linux | GCC | 14.0 |

```bash
# 最小构建（含测试）
cmake -B build -S . -DNEXUS_CUDA_SUPPORT=OFF -DNEXUS_BUILD_TESTS=ON
cmake --build build -j

# 运行全部测试
ctest --test-dir build --output-on-failure

# 静态分析（Linux）
cppcheck --enable=all --suppress=missingIncludeSystem -I include/ -I third_party/ --std=c++20 src/
```

## CI 流水线

提交后自动运行以下质量门禁：

| 检查 | 工具 | 门槛 |
|------|------|------|
| 编译 | GCC 14 + MSVC 2022 | ✅ 必须通过 |
| 单元测试 | Google Test 1.15 | ✅ 必须通过 |
| 静态分析 | cppcheck | ⚠️ 参考（允许失败） |
| 线程安全 | ThreadSanitizer | ⚠️ 参考（允许失败） |
| 覆盖率 | gcovr | ⚠️ 参考（允许失败，逐步调优） |

## 如何贡献

### 报告 Bug

使用 GitHub Issues，标签为 `bug`。内容包括：
- 操作系统和版本
- GPU 型号和驱动版本
- 复现步骤
- 完整错误日志

### 提交功能请求

使用 GitHub Issues，标签为 `enhancement`。内容包括：
- 问题背景
- 建议方案
- 替代方案（如果有）

### 代码贡献

1. Fork 仓库
2. 创建分支: `git checkout -b feat/your-feature-name`
3. 提交改动: `git commit -m "feat: 你的改动描述"`
4. 推送到远程: `git push origin feat/your-feature-name`
5. 创建 Pull Request

## 编码规范

参见 [WHITE_PAPER.md §6](WHITE_PAPER.md#6-c-编码规范)。关键点：

### 提交信息格式

使用 Conventional Commits：

```
feat: 新功能
fix:  Bug 修复
docs: 文档变更
refactor: 重构（不修复 bug 也不增加功能）
test:   测试相关
chore:  构建/工具链变更
```

示例：
```
feat(core): 添加 VRAM 自动驱逐策略
fix(daemon): 修复 psi_field 时间戳溢出
docs(ipc): 更新文件锁协议说明
```

### 代码风格

- 统一使用 `clang-format` 格式化（`.clang-format` 在项目根目录）
- 所有头文件使用 `#pragma once`
- 所有公共 API 必须有 Doxygen 注释
- 不允许 `using namespace std;`

### 测试要求

- 新功能必须有单元测试
- Bug 修复必须有回归测试
- 所有测试通过后方可合并

## 开发工作流

```bash
# 克隆
git clone <your-fork>
cd nexus

# 创建特性分支
git checkout -b feat/my-feature

# 开发 → 测试 → 提交
# ...

# 同步上游
git fetch upstream
git rebase upstream/main

# 推送
git push origin feat/my-feature
```

## 架构决策记录（ADR）

重大架构变更需要写 ADR，放在 `docs/adr/` 目录：

```
docs/adr/
├── 0001-use-file-ipc.md
├── 0002-process-architecture.md
└── template.md
```

## 发布流程

参见 [WHITE_PAPER.md §10.4](WHITE_PAPER.md#104-发布策略)。

## 获取帮助

- Issues: GitHub Issues
- 讨论: GitHub Discussions
- 内部: 参见 white paper 文档
