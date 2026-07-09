/**
 * @file expert_loader.cpp
 * @brief ExpertLoader 实现 — 封装 llama.cpp C API
 */

#include "nexus/core/expert_loader.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>

// llama.h 通过 FetchContent 引入
#include "llama.h"

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// ModelHandle (不透明实现)
// ═══════════════════════════════════════════════════════════════════

struct ModelHandle {
  llama_model*   model = nullptr;
  llama_context* ctx   = nullptr;
  bool           loaded = false;

  ~ModelHandle() {
    if (ctx)   { llama_free(ctx);   ctx   = nullptr; }
    if (model) { llama_model_free(model); model = nullptr; }
  }
};

// ═══════════════════════════════════════════════════════════════════
// 构造 / 析构 / 移动
// ═══════════════════════════════════════════════════════════════════

ExpertLoader::ExpertLoader() noexcept = default;
ExpertLoader::~ExpertLoader() noexcept {
  unload();
}

ExpertLoader::ExpertLoader(ExpertLoader&&) noexcept = default;
ExpertLoader& ExpertLoader::operator=(ExpertLoader&&) noexcept = default;

// ═══════════════════════════════════════════════════════════════════
// 加载模型
// ═══════════════════════════════════════════════════════════════════

auto ExpertLoader::load(const std::string& gguf_path,
                        const InferParams& params) noexcept -> Status {
  // 如有已加载模型，先卸载
  if (is_loaded()) {
    auto s = unload();
    if (!s.ok()) return s;
  }

  auto h = std::make_unique<ModelHandle>();

  // 模型参数
  auto model_params = llama_model_default_params();
  model_params.n_gpu_layers = params.n_gpu_layers;

  h->model = llama_model_load_from_file(gguf_path.c_str(), model_params);
  if (!h->model) {
    return Status::Error(ErrorCode::kModelLoadFailed,
      "llama_model_load_from_file failed: " + gguf_path);
  }

  // 上下文参数
  auto ctx_params = llama_context_default_params();
  ctx_params.n_ctx   = params.n_ctx;
  ctx_params.n_batch = params.n_batch;

  // 启用 GPU 加速
#ifdef GGML_USE_CUDA
  ctx_params.n_gpu_layers = params.n_gpu_layers;
#endif

  h->ctx = llama_init_from_model(h->model, ctx_params);
  if (!h->ctx) {
    llama_model_free(h->model);
    h->model = nullptr;
    return Status::Error(ErrorCode::kModelLoadFailed,
      "llama_new_context_with_model failed");
  }

  handle_     = std::move(h);
  model_path_ = gguf_path;
  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 卸载
// ═══════════════════════════════════════════════════════════════════

auto ExpertLoader::unload() noexcept -> Status {
  handle_.reset();
  model_path_.clear();
  return Status::Ok();
}

// ═══════════════════════════════════════════════════════════════════
// 推理
// ═══════════════════════════════════════════════════════════════════

auto ExpertLoader::infer(const std::string& prompt,
                         const InferParams& params,
                         TokenCallback on_token) noexcept -> Result<std::string> {
  if (!is_loaded()) {
    return Status::Error(ErrorCode::kModelNotFound,
      "no model loaded");
  }

  auto* ctx = handle_->ctx;
  auto* model = handle_->model;

  // 推理占位 — Phase 3 第二阶段实现完整推理管线
  // 当前返回模型元信息作为验证
  auto n_ctx = llama_n_ctx(ctx);
  char desc_buf[256] = {0};
  llama_model_desc(model, desc_buf, sizeof(desc_buf));

  std::ostringstream result;
  result << "{"
         << "\"model\": \"" << desc_buf << "\", "
         << "\"n_ctx\": " << n_ctx
         << "}";
  return result.str();
}

// ═══════════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════════

auto ExpertLoader::is_loaded() const noexcept -> bool {
  return handle_ != nullptr && handle_->loaded;
}

auto ExpertLoader::model_path() const noexcept -> const std::string& {
  return model_path_;
}

}  // namespace nexus::core
