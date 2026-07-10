// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 CherryClaw & Contributors

#ifndef NEXUS_ALGO_ENGINES_MTCS_H
#define NEXUS_ALGO_ENGINES_MTCS_H

/* MTCS v2 — 多任务认知调度 + 优先级继承 + 截止期感知 */

#include "nexus/algo/engine.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <vector>

namespace nexus::algo::engines {

class MtcsEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return EngineInfo{
      .id = "mtcs", .name = "Multi-Task Cognitive Scheduler v2",
      .version = "2.0.0",
      .description = "DAG 调度 + 优先级继承 + 截止期感知 + 依赖追踪",
      .tags = {"scheduling", "dag", "optimization", "deadline"},
    };
  }

  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true;
    return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    auto tasks_raw = input.value("tasks", std::vector<nlohmann::json>());
    int max_concurrent = input.value("max_concurrent", 2);
    bool deadline_aware = input.value("deadline_aware", true);

    struct Task {
      int id = 0; int priority = 5; int duration = 1;
      std::vector<int> deps;
      int deadline = 0; double progress = 0;
      int start_time = 0; int end_time = 0;
      std::string category = "general";
    };
    std::vector<Task> tasks;
    for (const auto& t : tasks_raw) {
      Task task;
      task.id = t.value("id", 0);
      task.priority = t.value("priority", 5);
      task.duration = t.value("duration", 1);
      task.deadline = t.value("deadline", 0);
      task.category = t.value("category", std::string("general"));
      auto deps = t.value("dependencies", std::vector<int>());
      task.deps = deps;
      tasks.push_back(task);
    }

    if (tasks.empty()) {
      auto r = nlohmann::json::object();
      r["status"] = "ok"; r["total_time"] = 0; r["schedule"] = nlohmann::json::array();
      return r;
    }

    // 优先级继承: 如果任务 A 依赖任务 B, B 继承 A 的最高优先级
    auto tasks_work = tasks;
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto& t : tasks_work) {
        for (auto dep_id : t.deps) {
          if (dep_id >= 0 && dep_id < static_cast<int>(tasks_work.size())) {
            if (tasks_work[dep_id].priority < t.priority) {
              tasks_work[dep_id].priority = t.priority;
              changed = true;
            }
          }
        }
      }
    }

    // 入度计算
    std::vector<int> in_degree(tasks_work.size(), 0);
    for (const auto& t : tasks_work)
      for (int d : t.deps)
        if (d >= 0 && d < static_cast<int>(in_degree.size()))
          in_degree[d]++;

    // 优先级队列 (复合排序: 截止期优先, 同截止期优先级高优先)
    auto cmp = [&](const Task& a, const Task& b) {
      if (deadline_aware && a.deadline > 0 && b.deadline > 0) {
        if (a.deadline != b.deadline) return a.deadline > b.deadline;
      }
      return a.priority < b.priority;
    };
    std::priority_queue<Task, std::vector<Task>, decltype(cmp)> pq(cmp);

    // 初始可执行任务
    for (const auto& t : tasks_work)
      if (t.deps.empty() || std::all_of(t.deps.begin(), t.deps.end(),
          [&](int d) { return in_degree[d] == 0; }))
        pq.push(t);

    // DAG 调度
    std::vector<Task> schedule;
    int current_time = 0;
    std::set<int> scheduled;

    while (!pq.empty()) {
      int batch = std::min(static_cast<int>(pq.size()), max_concurrent);
      std::vector<Task> batch_tasks;
      for (int i = 0; i < batch && !pq.empty(); ++i) {
        batch_tasks.push_back(pq.top());
        pq.pop();
      }

      for (auto& t : batch_tasks) {
        t.start_time = current_time;
        t.end_time = current_time + t.duration;
        t.progress = 1.0;
        scheduled.insert(t.id);
        schedule.push_back(t);
      }

      current_time += batch_tasks.empty() ? 1 : batch_tasks[0].duration;

      // 更新依赖图: 已调度任务解锁依赖
      for (auto& t : tasks_work) {
        if (scheduled.count(t.id)) continue;
        bool all_deps_done = true;
        for (int d : t.deps)
          if (!scheduled.count(d)) { all_deps_done = false; break; }
        if (all_deps_done) pq.push(t);
      }
    }

    // 调度质量指标
    int missed_deadlines = 0;
    for (const auto& t : schedule)
      if (t.deadline > 0 && t.end_time > t.deadline) missed_deadlines++;

    std::map<std::string, int> cat_counts;
    for (const auto& t : tasks_work) cat_counts[t.category]++;

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["total_tasks"] = static_cast<int>(tasks_work.size());
    result["scheduled"] = static_cast<int>(schedule.size());
    result["total_time"] = current_time;
    result["missed_deadlines"] = missed_deadlines;
    result["max_concurrent"] = max_concurrent;
    result["used_priority_inheritance"] = true;

    auto sched = nlohmann::json::array();
    for (const auto& t : schedule) {
      auto j = nlohmann::json::object();
      j["id"] = t.id; j["start"] = t.start_time;
      j["duration"] = t.duration; j["priority"] = t.priority;
      j["category"] = t.category;
      j["on_time"] = (t.deadline <= 0 || t.end_time <= t.deadline);
      sched.push_back(j);
    }
    result["schedule"] = sched;
    return result;
  }

  auto status() const noexcept -> nlohmann::json override {
    auto j = nlohmann::json::object(); j["initialized"] = initialized_; return j;
  }
  auto is_initialized() const noexcept -> bool override { return initialized_; }

private:
  bool initialized_ = false;
};

} // namespace
#endif
