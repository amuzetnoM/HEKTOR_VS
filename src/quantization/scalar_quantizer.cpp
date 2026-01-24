// ============================================================================
// VectorDB - Scalar Quantization Implementation
// ============================================================================

#include "vdb/quantization/scalar_quantizer.hpp"
#include <algorithm>
#include <fstream>
#include <cmath>

namespace vdb {
namespace quantization {

ScalarQuantizer::ScalarQuantizer(const ScalarQuantizerConfig& config)
    : config_(config), trained_(false) {}

Result<void> ScalarQuantizer::train(std::span<const Vector> training_data) {
    if (training_data.empty()) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Empty training data"});
    }
    
    min_values_.resize(config_.dimension, std::numeric_limits<Scalar>::max());
    max_values_.resize(config_.dimension, std::numeric_limits<Scalar>::lowest());
    
    for (const auto& vec : training_data) {
        if (vec.size() != config_.dimension) continue;
        
        for (size_t d = 0; d < config_.dimension; ++d) {
            min_values_[d] = std::min(min_values_[d], vec[d]);
            max_values_[d] = std::max(max_values_[d], vec[d]);
        }
    }
    
    scales_.resize(config_.dimension);
    offsets_.resize(config_.dimension);
    
    for (size_t d = 0; d < config_.dimension; ++d) {
        Scalar range = max_values_[d] - min_values_[d];
        scales_[d] = (range > 1e-6f) ? (255.0f / range) : 1.0f;
        offsets_[d] = min_values_[d];
    }
    
    trained_ = true;
    return {};
}

Result<std::vector<uint8_t>> ScalarQuantizer::encode(VectorView vector) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::InvalidState, "Not trained"});
    }
    
    std::vector<uint8_t> codes(config_.dimension);
    
    for (size_t d = 0; d < config_.dimension; ++d) {
        Scalar val = (vector.data()[d] - offsets_[d]) * scales_[d];
        codes[d] = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
    }
    
    return codes;
}

Result<Vector> ScalarQuantizer::decode(std::span<const uint8_t> codes) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::InvalidState, "Not trained"});
    }
    
    Vector result(config_.dimension);
    
    for (size_t d = 0; d < config_.dimension; ++d) {
        result[d] = (codes[d] / scales_[d]) + offsets_[d];
    }
    
    return result;
}

Distance ScalarQuantizer::compute_distance(VectorView query,
    std::span<const uint8_t> codes) const 
{
    if (!trained_) return std::numeric_limits<Distance>::max();
    
    Distance dist = 0.0f;
    
    for (size_t d = 0; d < config_.dimension; ++d) {
        Scalar decoded = (codes[d] / scales_[d]) + offsets_[d];
        Scalar diff = query.data()[d] - decoded;
        dist += diff * diff;
    }
    
    return std::sqrt(dist);
}

Result<void> ScalarQuantizer::save(std::string_view path) const {
    std::ofstream file(std::string(path), std::ios::binary);
    if (!file) return tl::unexpected(Error{ErrorCode::IoError, "Failed to open"});
    
    file.write(reinterpret_cast<const char*>(&config_.dimension), sizeof(Dim));
    file.write(reinterpret_cast<const char*>(&trained_), sizeof(bool));
    
    if (trained_) {
        file.write(reinterpret_cast<const char*>(min_values_.data()),
            min_values_.size() * sizeof(Scalar));
        file.write(reinterpret_cast<const char*>(max_values_.data()),
            max_values_.size() * sizeof(Scalar));
        file.write(reinterpret_cast<const char*>(scales_.data()),
            scales_.size() * sizeof(Scalar));
        file.write(reinterpret_cast<const char*>(offsets_.data()),
            offsets_.size() * sizeof(Scalar));
    }
    
    return {};
}

Result<ScalarQuantizer> ScalarQuantizer::load(std::string_view path) {
    std::ifstream file(std::string(path), std::ios::binary);
    if (!file) return tl::unexpected(Error{ErrorCode::IoError, "Failed to open"});
    
    ScalarQuantizerConfig config;
    bool trained;
    
    file.read(reinterpret_cast<char*>(&config.dimension), sizeof(Dim));
    file.read(reinterpret_cast<char*>(&trained), sizeof(bool));
    
    ScalarQuantizer sq(config);
    sq.trained_ = trained;
    
    if (trained) {
        sq.min_values_.resize(config.dimension);
        sq.max_values_.resize(config.dimension);
        sq.scales_.resize(config.dimension);
        sq.offsets_.resize(config.dimension);
        
        file.read(reinterpret_cast<char*>(sq.min_values_.data()),
            config.dimension * sizeof(Scalar));
        file.read(reinterpret_cast<char*>(sq.max_values_.data()),
            config.dimension * sizeof(Scalar));
        file.read(reinterpret_cast<char*>(sq.scales_.data()),
            config.dimension * sizeof(Scalar));
        file.read(reinterpret_cast<char*>(sq.offsets_.data()),
            config.dimension * sizeof(Scalar));
    }
    
    return sq;
}

}} // namespace vdb::quantization
