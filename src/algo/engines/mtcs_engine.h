#ifndef NEXUS_ALGO_ENGINES_MTCS_H
#define NEXUS_ALGO_ENGINES_MTCS_H

#include "nexus/algo/engine.h"
#include <cmath>
#include <numeric>
#include <queue>

namespace nexus::algo::engines {

/// 多任务认知调度 — 基于优先级+依赖的DAG调度
class MtcsEngine : public AlgorithmEngine {
public:
  auto info() const noexcept -> EngineInfo override {
    return {"mtcs", "Multi-Task Cognitive Scheduler", "1.0.0",
            "DAG 任务调度: 优先级队列 + 依赖解析", {"scheduling", "dag", "optimization"}};
  }

  auto initialize(const nlohmann::json&) noexcept -> Status override {
    initialized_ = true; return Status::Ok();
  }

  auto execute(const nlohmann::json& input) noexcept
      -> Result<nlohmann::json> override {
    if (!initialized_) return Status::Error(ErrorCode::kEngineFailed, "not initialized");
    auto tasks_raw = input.value("tasks", std::vector<nlohmann::json>());
    int max_concurrent = input.value("max_concurrent", 2);

    struct Task {
      int id; int priority; int duration; std::vector<int> deps; int start_time = 0;
    };
    std::vector<Task> tasks;
    for (const auto& t : tasks_raw) {
      Task task;
      task.id = t.value("id", 0); task.priority = t.value("priority", 5);
      task.duration = t.value("duration", 1);
      auto deps = t.value("dependencies", std::vector<int>());
      task.deps = deps; tasks.push_back(task);
    }

    // 拓扑排序 + 优先级队列
    std::vector<int> in_degree(tasks.size(), 0);
    for (const auto& t : tasks) for (int d : t.deps) in_degree[t.id]++;
    auto cmp = [](const Task& a, const Task& b) { return a.priority < b.priority; };
    std::priority_queue<Task, std::vector<Task>, decltype(cmp)> pq(cmp);

    for (const auto& t : tasks) if (in_degree[t.id] == 0) pq.push(t);

    std::vector<Task> schedule; int current_time = 0;
    while (!pq.empty()) {
      int batch = std::min(static_cast<int>(pq.size()), max_concurrent);
      std::vector<Task> batch_tasks;
      for (int i = 0; i < batch && !pq.empty(); ++i) {
        batch_tasks.push_back(pq.top()); pq.pop();
      }
      for (auto& t : batch_tasks) { t.start_time = current_time; schedule.push_back(t); }
      current_time += batch_tasks.empty() ? 1 : batch_tasks[0].duration;
      for (auto& t : batch_tasks) {
        for (auto& task : tasks) {
          auto it = std::find(task.deps.begin(), task.deps.end(), t.id);
          if (it != task.deps.end()) { task.deps.erase(it); in_degree[t.id]--; }
        }
      }
      for (const auto& t : tasks) if (in_degree[t.id] == 0) pq.push(t);
    }

    auto result = nlohmann::json::object();
    result["status"] = "ok";
    result["total_time"] = current_time;
    auto sched = nlohmann::json::array();
    for (const auto& t : schedule) {
      auto j = nlohmann::json::object();
      j["id"] = t.id; j["start"] = t.start_time; j["priority"] = t.priority;
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
