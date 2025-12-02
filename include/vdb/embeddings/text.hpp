#pragma once
// ============================================================================
// VectorDB - Text Embedding Encoder
// Uses all-MiniLM-L6-v2 via ONNX Runtime for semantic text embeddings
// ============================================================================

#include "../core.hpp"
#include "onnx_runtime.hpp"
#include <filesystem>

namespace vdb::embeddings {

namespace fs = std::filesystem;

// ============================================================================
// Text Encoder Configuration
// ============================================================================

struct TextEncoderConfig {
    fs::path model_path;        // Path to text encoder ONNX model
    fs::path vocab_path;        // Path to vocab.txt
    Dim output_dim = 384;       // MiniLM outputs 384-dim embeddings
    size_t max_seq_length = 256;
    bool normalize_embeddings = true;
    Device device = Device::CPU;
};

// ============================================================================
// Text Encoder - Semantic text embedding
// ============================================================================

class TextEncoder {
public:
    TextEncoder() = default;
    ~TextEncoder();
    
    // Non-copyable, movable
    TextEncoder(const TextEncoder&) = delete;
    TextEncoder& operator=(const TextEncoder&) = delete;
    TextEncoder(TextEncoder&&) noexcept;
    TextEncoder& operator=(TextEncoder&&) noexcept;
    
    /// Initialize encoder with model and vocabulary
    [[nodiscard]] Result<void> init(const TextEncoderConfig& config);
    
    /// Check if ready
    [[nodiscard]] bool is_ready() const { return ready_; }
    
    /// Encode text to embedding vector
    [[nodiscard]] Result<std::vector<float>> encode(std::string_view text);
    
    /// Encode batch of texts
    [[nodiscard]] Result<std::vector<std::vector<float>>> encode_batch(
        const std::vector<std::string>& texts
    );
    
    /// Get embedding dimension
    [[nodiscard]] Dim dimension() const { return config_.output_dim; }
    
    /// Get device being used
    [[nodiscard]] Device device() const { return config_.device; }

private:
    /// Mean pooling over token embeddings with attention mask
    [[nodiscard]] std::vector<float> mean_pooling(
        const float* token_embeddings,
        const std::vector<int64_t>& attention_mask,
        size_t seq_length,
        Dim hidden_dim
    );
    
    /// L2 normalize vector
    void normalize(std::vector<float>& vec);
    
    TextEncoderConfig config_;
    std::unique_ptr<OnnxSession> session_;
    std::unique_ptr<Tokenizer> tokenizer_;
    bool ready_ = false;
};

} // namespace vdb::embeddings
