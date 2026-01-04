// ============================================================================
// VectorDB - Image Encoder Implementation
// Uses CLIP ViT-B/32 for image embeddings (charts, visual data)
// ============================================================================

#include "vdb/embeddings/image.hpp"
#include "vdb/embeddings/stb_image.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <regex>

namespace vdb::embeddings {

// ============================================================================
// Image I/O Functions
// ============================================================================

Result<ImageData> load_image(const fs::path& path) {
    if (!fs::exists(path)) {
        return std::unexpected(Error{ErrorCode::IoError, "Image file not found: " + path.string()});
    }
    
    // Read file into memory
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open image: " + path.string()});
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to read image: " + path.string()});
    }
    
    return load_image_memory(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(buffer.data()), 
        buffer.size()
    ));
}

Result<ImageData> load_image_memory(std::span<const uint8_t> data) {
    int width, height, channels;
    
    unsigned char* pixels = stbi_load_from_memory(
        data.data(),
        static_cast<int>(data.size()),
        &width,
        &height,
        &channels,
        3  // Force RGB
    );
    
    if (pixels == nullptr) {
        return std::unexpected(Error{ErrorCode::InvalidData, 
                    std::string("Failed to decode image: ") + stbi_failure_reason()});
    }
    
    ImageData result;
    result.width = static_cast<size_t>(width);
    result.height = static_cast<size_t>(height);
    result.channels = 3;
    result.pixels.assign(pixels, pixels + result.size());
    
    stbi_image_free(pixels);
    
    return result;
}

Result<void> save_image(const fs::path& path, const ImageData& img) {
    // For now, only support raw PPM format (simple, no dependencies)
    // A full implementation would use stb_image_write
    
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to create image: " + path.string()});
    }
    
    // PPM header
    file << "P6\n" << img.width << " " << img.height << "\n255\n";
    file.write(reinterpret_cast<const char*>(img.pixels.data()), img.pixels.size());
    
    return {};
}

// ============================================================================
// ImageEncoder Implementation
// ============================================================================

ImageEncoder::~ImageEncoder() = default;

ImageEncoder::ImageEncoder(ImageEncoder&& other) noexcept
    : config_(std::move(other.config_))
    , session_(std::move(other.session_))
    , preprocessor_(std::move(other.preprocessor_))
    , ready_(other.ready_)
{
    other.ready_ = false;
}

ImageEncoder& ImageEncoder::operator=(ImageEncoder&& other) noexcept {
    if (this != &other) {
        config_ = std::move(other.config_);
        session_ = std::move(other.session_);
        preprocessor_ = std::move(other.preprocessor_);
        ready_ = other.ready_;
        other.ready_ = false;
    }
    return *this;
}

Result<void> ImageEncoder::init(const ImageEncoderConfig& config) {
    config_ = config;
    
    // Validate model path
    if (!fs::exists(config_.model_path)) {
        return std::unexpected(Error{ErrorCode::IoError,
                    "Image encoder model not found: " + config_.model_path.string()});
    }
    
    // Create preprocessor with CLIP normalization values
    preprocessor_ = std::make_unique<ImagePreprocessor>(
        Size{config_.input_size, config_.input_size},
        std::array<float, 3>{0.48145466f, 0.4578275f, 0.40821073f},
        std::array<float, 3>{0.26862954f, 0.26130258f, 0.27577711f}
    );
    
    // Load ONNX model
    try {
        session_ = std::make_unique<OnnxSession>(config_.model_path, config_.device);
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                    std::string("Failed to load ONNX model: ") + e.what()});
    }
    
    ready_ = true;
    return {};
}

Result<std::vector<float>> ImageEncoder::encode(const fs::path& image_path) {
    auto image_result = load_image(image_path);
    if (!image_result) {
        return std::unexpected(image_result.error());
    }
    
    return encode(*image_result);
}

Result<std::vector<float>> ImageEncoder::encode(const ImageData& image) {
    if (!ready_) {
        return std::unexpected(Error{ErrorCode::InvalidState, "ImageEncoder not initialized"});
    }
    
    if (!image.valid()) {
        return std::unexpected(Error{ErrorCode::InvalidData, "Invalid image data"});
    }
    
    // Preprocess image
    auto preprocessed = preprocess(image);
    
    try {
        // Create input tensor
        auto& mem_info = session_->memory_info();
        std::vector<int64_t> shape = {
            1, 
            3, 
            static_cast<int64_t>(config_.input_size), 
            static_cast<int64_t>(config_.input_size)
        };
        
        std::vector<Ort::Value> inputs;
        inputs.push_back(Ort::Value::CreateTensor<float>(
            mem_info,
            preprocessed.data(),
            preprocessed.size(),
            shape.data(),
            shape.size()
        ));
        
        // Run inference
        auto outputs = session_->run(inputs);
        
        if (outputs.empty()) {
            return std::unexpected(Error{ErrorCode::InvalidData, "Model returned no outputs"});
        }
        
        // Get output tensor
        auto& output = outputs[0];
        auto type_info = output.GetTensorTypeAndShapeInfo();
        auto output_shape = type_info.GetShape();
        
        // CLIP outputs [1, 512]
        size_t embed_dim = 1;
        for (auto dim : output_shape) {
            if (dim > 1) embed_dim = static_cast<size_t>(dim);
        }
        
        const float* output_data = output.GetTensorData<float>();
        std::vector<float> embedding(output_data, output_data + embed_dim);
        
        // Normalize if configured
        if (config_.normalize_embeddings) {
            normalize(embedding);
        }
        
        return embedding;
        
    } catch (const Ort::Exception& e) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                    std::string("ONNX inference failed: ") + e.what()});
    }
}

Result<std::vector<std::vector<float>>> ImageEncoder::encode_batch(
    const std::vector<fs::path>& image_paths) {
    
    if (!ready_) {
        return std::unexpected(Error{ErrorCode::InvalidState, "ImageEncoder not initialized"});
    }
    
    if (image_paths.empty()) {
        return std::vector<std::vector<float>>{};
    }
    
    // Load and preprocess all images
    std::vector<std::vector<float>> preprocessed_images;
    preprocessed_images.reserve(image_paths.size());
    
    for (const auto& path : image_paths) {
        auto image_result = load_image(path);
        if (!image_result) {
            return std::unexpected(image_result.error());
        }
        
        if (!image_result->valid()) {
            return std::unexpected(Error{ErrorCode::InvalidData, 
                "Invalid image data: " + path.string()});
        }
        
        preprocessed_images.push_back(preprocess(*image_result));
    }
    
    // Create batch tensor [batch_size, 3, height, width]
    size_t batch_size = preprocessed_images.size();
    size_t single_image_size = config_.input_size * config_.input_size * 3;
    
    std::vector<float> batch_data;
    batch_data.reserve(batch_size * single_image_size);
    
    for (const auto& img : preprocessed_images) {
        batch_data.insert(batch_data.end(), img.begin(), img.end());
    }
    
    try {
        // Create batch input tensor
        auto& mem_info = session_->memory_info();
        std::vector<int64_t> shape = {
            static_cast<int64_t>(batch_size),
            3,
            static_cast<int64_t>(config_.input_size),
            static_cast<int64_t>(config_.input_size)
        };
        
        std::vector<Ort::Value> inputs;
        inputs.push_back(Ort::Value::CreateTensor<float>(
            mem_info,
            batch_data.data(),
            batch_data.size(),
            shape.data(),
            shape.size()
        ));
        
        // Run batch inference
        auto outputs = session_->run(inputs);
        
        if (outputs.empty()) {
            return std::unexpected(Error{ErrorCode::InvalidData, "Model returned no outputs"});
        }
        
        // Get output tensor
        auto& output = outputs[0];
        auto type_info = output.GetTensorTypeAndShapeInfo();
        auto output_shape = type_info.GetShape();
        
        // Output shape should be [batch_size, embed_dim]
        size_t output_batch_size = static_cast<size_t>(output_shape[0]);
        size_t embed_dim = (output_shape.size() >= 2) ? 
            static_cast<size_t>(output_shape[1]) : config_.output_dim;
        
        const float* output_data = output.GetTensorData<float>();
        
        // Extract embeddings for each image
        std::vector<std::vector<float>> results;
        results.reserve(output_batch_size);
        
        for (size_t b = 0; b < output_batch_size; ++b) {
            std::vector<float> embedding(
                output_data + (b * embed_dim),
                output_data + ((b + 1) * embed_dim)
            );
            
            if (config_.normalize_embeddings) {
                normalize(embedding);
            }
            
            results.push_back(std::move(embedding));
        }
        
        return results;
        
    } catch (const Ort::Exception& e) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                    std::string("ONNX batch inference failed: ") + e.what()});
    }
}

std::vector<float> ImageEncoder::preprocess(const ImageData& image) {
    // Use the preprocessor for resize + normalize
    return preprocessor_->center_crop_and_process(
        image.pixels.data(),
        image.width,
        image.height
    );
}

void ImageEncoder::normalize(std::vector<float>& vec) {
    float norm = 0.0f;
    for (float v : vec) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    
    if (norm > 1e-12f) {
        for (float& v : vec) {
            v /= norm;
        }
    }
}

// ============================================================================
// Chart Utilities
// ============================================================================

ChartInfo parse_chart_path(const fs::path& path) {
    ChartInfo info;
    
    // Extract asset from filename (e.g., "GOLD.png" -> "GOLD")
    std::string stem = path.stem().string();
    info.asset = stem;
    
    // Try to extract date from parent directory
    // Expected structure: output/charts/YYYY-MM-DD/ASSET.png
    // or: output/charts/ASSET.png (date in parent)
    fs::path parent = path.parent_path();
    std::string parent_name = parent.filename().string();
    
    // Check if parent matches date pattern YYYY-MM-DD
    std::regex date_pattern(R"(\d{4}-\d{2}-\d{2})");
    if (std::regex_match(parent_name, date_pattern)) {
        info.date = parent_name;
    } else {
        // Try to find date in grandparent
        fs::path grandparent = parent.parent_path();
        std::string gp_name = grandparent.filename().string();
        if (std::regex_match(gp_name, date_pattern)) {
            info.date = gp_name;
        }
    }
    
    // Default timeframe
    info.timeframe = "daily";
    
    return info;
}

ImageData preprocess_chart(const ImageData& chart) {
    // For Gold Standard charts, we want to focus on the main chart area
    // The charts have a dark background with candlesticks/lines
    
    // For now, return as-is
    // A more sophisticated version would:
    // 1. Detect chart boundaries
    // 2. Crop to main chart area
    // 3. Enhance contrast for better feature extraction
    
    return chart;
}

} // namespace vdb::embeddings
