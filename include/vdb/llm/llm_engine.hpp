// ============================================================================
// VectorDB - LLM Engine Header
// llama.cpp integration for local text generation
// ============================================================================

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

namespace vdb::llm {

// ============================================================================
// Configuration
// ============================================================================

struct LLMConfig {
    fs::path model_path;              // Path to GGUF model file
    int n_ctx = 4096;                 // Context window size
    int n_batch = 512;                // Batch size for prompt processing
    int n_threads = 0;                // 0 = auto-detect
    int n_gpu_layers = 0;             // Layers to offload to GPU (0 = CPU only)
    bool use_mmap = true;             // Memory-map model file
    bool use_mlock = false;           // Lock model in RAM
    float rope_freq_base = 0.0f;      // RoPE base frequency (0 = model default)
    float rope_freq_scale = 0.0f;     // RoPE frequency scale (0 = model default)
};

struct GenerationParams {
    int max_tokens = 1024;            // Maximum tokens to generate
    float temperature = 0.7f;         // Sampling temperature (0 = greedy)
    float top_p = 0.9f;               // Nucleus sampling threshold
    int top_k = 40;                   // Top-k sampling
    float repeat_penalty = 1.1f;      // Repetition penalty
    int repeat_last_n = 64;           // Tokens to consider for repetition penalty
    
    // Stop sequences
    std::vector<std::string> stop_sequences;
    
    // Streaming callback (token by token)
    std::function<bool(std::string_view token)> on_token;
};

// ============================================================================
// Message types for chat completion
// ============================================================================

enum class Role {
    System,
    User,
    Assistant
};

struct Message {
    Role role;
    std::string content;
};

struct ChatCompletionResult {
    std::string content;
    int tokens_generated;
    int tokens_prompt;
    double generation_time_ms;
    bool stopped_by_eos;
    std::string stop_reason;
};

// ============================================================================
// LLM Engine Interface
// ============================================================================

class LLMEngine {
public:
    virtual ~LLMEngine() = default;
    
    // Model loading
    virtual bool load(const LLMConfig& config) = 0;
    virtual bool is_loaded() const = 0;
    virtual void unload() = 0;
    
    // Model info
    virtual std::string model_name() const = 0;
    virtual int context_size() const = 0;
    virtual int vocab_size() const = 0;
    
    // Text generation
    virtual std::string generate(
        std::string_view prompt,
        const GenerationParams& params = {}
    ) = 0;
    
    // Chat completion (with message history)
    virtual ChatCompletionResult chat(
        const std::vector<Message>& messages,
        const GenerationParams& params = {}
    ) = 0;
    
    // Token counting
    virtual int count_tokens(std::string_view text) const = 0;
    
    // Embeddings (if model supports)
    virtual std::optional<std::vector<float>> embed(std::string_view text) const = 0;
};

// ============================================================================
// Factory function
// ============================================================================

std::unique_ptr<LLMEngine> create_llm_engine();

// ============================================================================
// Utility functions
// ============================================================================

// List available GGUF models in a directory
std::vector<fs::path> find_gguf_models(const fs::path& directory);

// Get model metadata from GGUF file
struct GGUFMetadata {
    std::string name;
    std::string architecture;
    int context_length;
    int embedding_length;
    int vocab_size;
    std::string quantization;
    size_t file_size;
};

std::optional<GGUFMetadata> read_gguf_metadata(const fs::path& model_path);

// ============================================================================
// Chat Templates
// ============================================================================

// Apply chat template to messages (Llama 2/3, Mistral, ChatML, etc.)
std::string apply_chat_template(
    const std::vector<Message>& messages,
    std::string_view template_name = "chatml"  // chatml, llama2, llama3, mistral
);

} // namespace vdb::llm
