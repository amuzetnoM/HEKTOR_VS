#pragma once
// ============================================================================
// VectorDB - Image Embedding Encoder (CLIP ViT-B/32)
// For encoding Gold Standard charts and visual data
// ============================================================================

#include "../core.hpp"
#include "onnx_runtime.hpp"
#include <filesystem>

namespace vdb::embeddings {

namespace fs = std::filesystem;

// ============================================================================
// Image Data Container
// ============================================================================

struct ImageData {
    std::vector<uint8_t> pixels;  // RGB, row-major (H x W x 3)
    size_t width = 0;
    size_t height = 0;
    size_t channels = 3;
    
    [[nodiscard]] bool valid() const {
        return !pixels.empty() && width > 0 && height > 0 &&
               pixels.size() == width * height * channels;
    }
    
    [[nodiscard]] size_t size() const {
        return width * height * channels;
    }
};

// ============================================================================
// Image I/O Functions
// ============================================================================

/// Load image from file (PNG, JPEG via stb_image)
[[nodiscard]] Result<ImageData> load_image(const fs::path& path);

/// Load image from memory buffer
[[nodiscard]] Result<ImageData> load_image_memory(std::span<const uint8_t> data);

/// Save image to file (PNG only)
[[nodiscard]] Result<void> save_image(const fs::path& path, const ImageData& img);

// ============================================================================
// Image Encoder Configuration
// ============================================================================

struct ImageEncoderConfig {
    fs::path model_path;        // Path to CLIP image encoder ONNX model
    Dim output_dim = 512;       // CLIP outputs 512-dim embeddings
    size_t input_size = 224;    // CLIP expects 224x224 images
    bool normalize_embeddings = true;
    Device device = Device::CPU;
};

// ============================================================================
// Image Encoder - CLIP ViT-B/32
// ============================================================================

class ImageEncoder {
public:
    ImageEncoder() = default;
    ~ImageEncoder();
    
    // Non-copyable, movable
    ImageEncoder(const ImageEncoder&) = delete;
    ImageEncoder& operator=(const ImageEncoder&) = delete;
    ImageEncoder(ImageEncoder&&) noexcept;
    ImageEncoder& operator=(ImageEncoder&&) noexcept;
    
    /// Initialize encoder with model
    [[nodiscard]] Result<void> init(const ImageEncoderConfig& config);
    
    /// Check if ready
    [[nodiscard]] bool is_ready() const { return ready_; }
    
    /// Encode image from file
    [[nodiscard]] Result<std::vector<float>> encode(const fs::path& image_path);
    
    /// Encode image from ImageData
    [[nodiscard]] Result<std::vector<float>> encode(const ImageData& image);
    
    /// Encode batch of images
    [[nodiscard]] Result<std::vector<std::vector<float>>> encode_batch(
        const std::vector<fs::path>& image_paths
    );
    
    /// Get embedding dimension
    [[nodiscard]] Dim dimension() const { return config_.output_dim; }
    
    /// Get device being used
    [[nodiscard]] Device device() const { return config_.device; }

private:
    /// Preprocess image for CLIP (resize, normalize, CHW format)
    [[nodiscard]] std::vector<float> preprocess(const ImageData& image);
    
    /// L2 normalize vector
    void normalize(std::vector<float>& vec);
    
    ImageEncoderConfig config_;
    std::unique_ptr<OnnxSession> session_;
    std::unique_ptr<ImagePreprocessor> preprocessor_;
    bool ready_ = false;
};

// ============================================================================
// Chart-Specific Utilities
// ============================================================================

/// Information extracted from chart filename
struct ChartInfo {
    std::string asset;      // GOLD, SILVER, DXY, etc.
    std::string date;       // YYYY-MM-DD from parent dir or filename
    std::string timeframe;  // daily, weekly, etc.
};

/// Parse chart filename to extract metadata
/// e.g., "GOLD.png" from "charts/2025-12-01/" -> {GOLD, 2025-12-01, daily}
[[nodiscard]] ChartInfo parse_chart_path(const fs::path& path);

/// Preprocess Gold Standard chart for better embedding
/// - Handles dark background charts
/// - Centers on the main chart area
[[nodiscard]] ImageData preprocess_chart(const ImageData& chart);

} // namespace vdb::embeddings
