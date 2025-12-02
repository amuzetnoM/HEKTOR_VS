#pragma once
// ============================================================================
// VectorDB - ONNX Runtime Wrapper (CPU/CUDA/DirectML)
// Production-quality inference engine abstraction
// ============================================================================

#include "../core.hpp"
#include <onnxruntime_cxx_api.h>
#include <memory>
#include <filesystem>
#include <span>
#include <array>

namespace vdb::embeddings {

namespace fs = std::filesystem;

// ============================================================================
// Execution Device
// ============================================================================

enum class Device : uint8_t {
    CPU,        // Always available
    CUDA,       // NVIDIA GPU
    DirectML    // Windows GPU (AMD/Intel/NVIDIA)
};

/// Detect best available device
[[nodiscard]] Device detect_best_device();

/// Get human-readable device name
[[nodiscard]] std::string device_name(Device device);

// ============================================================================
// ONNX Session - Thin wrapper around Ort::Session
// ============================================================================

class OnnxSession {
public:
    /// Create session from model file
    explicit OnnxSession(const fs::path& model_path, Device device = Device::CPU);
    
    ~OnnxSession();
    
    // Non-copyable, movable
    OnnxSession(const OnnxSession&) = delete;
    OnnxSession& operator=(const OnnxSession&) = delete;
    OnnxSession(OnnxSession&&) noexcept;
    OnnxSession& operator=(OnnxSession&&) noexcept;
    
    /// Run inference
    [[nodiscard]] std::vector<Ort::Value> run(std::vector<Ort::Value>& inputs);
    
    /// Get device being used
    [[nodiscard]] Device device() const;
    
    /// Get input tensor names
    [[nodiscard]] std::vector<std::string> input_names() const;
    
    /// Get output tensor names
    [[nodiscard]] std::vector<std::string> output_names() const;
    
    /// Get input shape for specified input
    [[nodiscard]] std::vector<int64_t> input_shape(size_t idx = 0) const;
    
    /// Get output shape for specified output
    [[nodiscard]] std::vector<int64_t> output_shape(size_t idx = 0) const;
    
    /// Get memory info for tensor creation
    [[nodiscard]] Ort::MemoryInfo& memory_info();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// WordPiece Tokenizer - Full BERT-compatible implementation
// ============================================================================

class Tokenizer {
public:
    /// Create tokenizer and load vocabulary
    explicit Tokenizer(const fs::path& vocab_path);
    
    /// Tokenize text to token IDs with padding
    [[nodiscard]] std::vector<int64_t> encode(
        std::string_view text,
        size_t max_length = 256,
        bool add_special_tokens = true
    ) const;
    
    /// Decode token IDs back to text
    [[nodiscard]] std::string decode(std::span<const int64_t> token_ids) const;
    
    /// Get vocabulary size
    [[nodiscard]] size_t vocab_size() const { return vocab_.size(); }
    
    /// Check if token exists
    [[nodiscard]] bool has_token(std::string_view token) const {
        return vocab_.contains(std::string(token));
    }

private:
    void load_vocabulary(const fs::path& vocab_path);
    
    /// Basic tokenization (whitespace, punctuation, unicode)
    [[nodiscard]] std::vector<std::string> basic_tokenize(std::string_view text) const;
    
    /// WordPiece tokenization of a single word
    [[nodiscard]] std::vector<std::string> wordpiece_tokenize(const std::string& word) const;
    
    std::unordered_map<std::string, int32_t> vocab_;
    std::unordered_map<int32_t, std::string> id_to_token_;
};

// ============================================================================
// Image Preprocessor - CLIP-compatible preprocessing
// ============================================================================

struct Size {
    size_t width;
    size_t height;
};

class ImagePreprocessor {
public:
    /// Create preprocessor with target size and normalization params
    explicit ImagePreprocessor(
        Size target_size = {224, 224},
        std::array<float, 3> mean = {0.48145466f, 0.4578275f, 0.40821073f},  // CLIP
        std::array<float, 3> std = {0.26862954f, 0.26130258f, 0.27577711f}
    );
    
    /// Process raw RGB data, returns CHW float tensor
    [[nodiscard]] std::vector<float> process(
        const uint8_t* rgb_data,
        size_t width,
        size_t height
    ) const;
    
    /// Process from file (PNG/JPEG)
    [[nodiscard]] std::vector<float> process_file(const fs::path& path) const;
    
    /// Center crop and process (for square models like CLIP)
    [[nodiscard]] std::vector<float> center_crop_and_process(
        const uint8_t* rgb_data,
        size_t width,
        size_t height
    ) const;
    
    /// Get output dimensions
    [[nodiscard]] Size target_size() const { return target_size_; }
    [[nodiscard]] size_t output_size() const { 
        return 3 * target_size_.width * target_size_.height; 
    }

private:
    Size target_size_;
    std::array<float, 3> mean_;
    std::array<float, 3> std_;
};

} // namespace vdb::embeddings
