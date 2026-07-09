/**
 * @file expert_loader.cpp
 * @brief ExpertLoader 实现 — llama.cpp 推理管线
 */

#include "nexus/core/expert_loader.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <thread>
#include <vector>

#include "llama.h"

namespace nexus::core {

// ═══════════════════════════════════════════════════════════════════
// ModelHandle
// ═══════════════════════════════════════════════════════════════════

struct ModelHandle {
  llama_model*   model = nullptr;
  llama_context* ctx   = nullptr;
  const llama_vocab* vocab = nullptr;

  ~ModelHandle() {
    if (ctx)   { llama_free(ctx);   ctx = nullptr; }
    if (model) { llama_model_free(model); model = nullptr; }
  }
};

// ═══════════════════════════════════════════════════════════════════
// 构造 / 析构 / 移动
// ═══════════════════════════════════════════════════════════════════

ExpertLoader::ExpertLoader() noexcept = default;
ExpertLoader::~ExpertLoader() noexcept { unload(); }
ExpertLoader::ExpertLoader(ExpertLoader&&) noexcept = default;
ExpertLoader& ExpertLoader::operator=(ExpertLoader&&) noexcept = default;

// ═══════════════════════════════════════════════════════════════════
// 加载模型
// ═══════════════════════════════════════════════════════════════════

auto ExpertLoader::load(const std::string& gguf_path,
                        const InferParams& params) noexcept -> Status {
  if (is_loaded()) unload();

  auto h = std::make_unique<ModelHandle>();

  // 模型参数
  auto model_params = llama_model_default_params();
  model_params.n_gpu_layers = 0;  // CPU-only (llama.cpp CUDA 有编译问题)

  h->model = llama_model_load_from_file(gguf_path.c_str(), model_params);
  if (!h->model) {
    return Status::Error(ErrorCode::kModelLoadFailed,
      "llama_model_load_from_file failed: " + gguf_path);
  }

  // 上下文参数
  auto ctx_params = llama_context_default_params();
  ctx_params.n_ctx   = params.n_ctx;
  ctx_params.n_batch = params.n_batch;

  h->ctx = llama_init_from_model(h->model, ctx_params);
  if (!h->ctx) {
    llama_model_free(h->model); h->model = nullptr;
    return Status::Error(ErrorCode::kModelLoadFailed,
      "llama_init_from_model failed");
  }

  // 设置 CPU 线程数 (使用全部物理核)
  int n_threads = std::thread::hardware_concurrency();
  if (n_threads < 1) n_threads = 4;
  llama_set_n_threads(h->ctx, n_threads, n_threads);

  // 获取词汇表
  h->vocab = llama_model_get_vocab(h->model);
  if (!h->vocab) {
    return Status::Error(ErrorCode::kModelLoadFailed,
      "llama_model_get_vocab failed");
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
// 推理 (llama.cpp 管线)
// ═══════════════════════════════════════════════════════════════════

auto ExpertLoader::infer(const std::string& prompt,
                         const InferParams& params,
                         TokenCallback on_token) noexcept -> Result<std::string> {
  if (!is_loaded()) {
    return Status::Error(ErrorCode::kModelNotFound, "no model loaded");
  }

  auto* ctx    = handle_->ctx;
  auto* vocab  = handle_->vocab;
  auto* model  = handle_->model;

  // 1. Tokenize prompt (直接分配缓冲区)
  int max_tokens = static_cast<int>(prompt.size()) + 8;
  std::vector<llama_token> prompt_tokens(static_cast<size_t>(max_tokens));

  int n_tok = llama_tokenize(vocab, prompt.c_str(),
    static_cast<int>(prompt.size()),
    prompt_tokens.data(), max_tokens, false, false);

  if (n_tok < 0) {
    // 缓冲区不足, 用返回值的绝对值作为大小
    max_tokens = -n_tok;
    prompt_tokens.resize(static_cast<size_t>(max_tokens));
    n_tok = llama_tokenize(vocab, prompt.c_str(),
      static_cast<int>(prompt.size()),
      prompt_tokens.data(), max_tokens, false, false);
    if (n_tok < 0) {
      return Status::Error(ErrorCode::kEngineFailed,
        "tokenize failed: return=" + std::to_string(n_tok));
    }
  }

  prompt_tokens.resize(static_cast<size_t>(n_tok));

  if (n_tok < 0) {
    return Status::Error(ErrorCode::kEngineFailed, "tokenize failed (2)");
  }
  prompt_tokens.resize(n_tok);

  // 2. 创建采样器链 (greedy 解码)
  auto* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
  llama_sampler_chain_add(sampler, llama_sampler_init_greedy());

  // 3. 逐 token 生成
  std::ostringstream output;
  int n_predict = params.n_predict;
  int n_consumed = 0;

  // 初始化 batch
  auto batch = llama_batch_get_one(prompt_tokens.data(),
    static_cast<int32_t>(prompt_tokens.size()));

  while (n_consumed < n_predict) {
    // 解码
    if (llama_decode(ctx, batch) != 0) {
      llama_sampler_free(sampler);
      return Status::Error(ErrorCode::kEngineFailed, "llama_decode failed");
    }

    // 采样下一个 token
    llama_token new_token = llama_sampler_sample(sampler, ctx, -1);
    if (llama_vocab_is_eog(vocab, new_token)) break;  // EOS

    // Token → 文本
    char buf[16] = {0};
    int n = llama_token_to_piece(vocab, new_token, buf, sizeof(buf), 0, false);
    if (n > 0) {
      std::string piece(buf, static_cast<size_t>(n));
      output << piece;
      if (on_token) on_token(piece);
    }

    // 准备下一步
    n_consumed++;
    batch = llama_batch_get_one(&new_token, 1);
  }

  llama_sampler_free(sampler);
  return output.str();
}

// ═══════════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════════

auto ExpertLoader::is_loaded() const noexcept -> bool {
  return handle_ != nullptr && handle_->model != nullptr;
}

auto ExpertLoader::model_path() const noexcept -> const std::string& {
  return model_path_;
}

}  // namespace nexus::core
