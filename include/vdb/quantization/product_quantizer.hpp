#pragma once
// ============================================================================
// VectorDB - Product Quantization for Vector Compression
// Product Quantization achieves 8-32x compression with minimal accuracy loss
// ============================================================================

#include "../core.hpp"
#include "../distance.hpp"
#include <vector>
#include <array>
#include <span>
#include <memory>

namespace vdb {
namespace quantization {

// ============================================================================
// Product Quantization Configuration
// ============================================================================

struct ProductQuantizerConfig {
    Dim dimension = UNIFIED_DIM;          // Must match vector dimension  
    uint32_t num_subquantizers = 8;      // Number of sub-vectors (must divide dimension)
    uint32_t num_centroids = 256;        // Centroids per subquantizer (256 = 8-bit codes)
    uint32_t num_iterations = 25;        // K-means iterations
    uint32_t num_threads = 0;            // 0 = auto-detect
    DistanceMetric metric = DistanceMetric::L2;
    uint64_t seed = 42;
};

// ============================================================================
// Product Quantizer - 8-32x compression with asymmetric distance computation
// ============================================================================

class ProductQuantizer {
public:
    explicit ProductQuantizer(const ProductQuantizerConfig& config = {});
    ~ProductQuantizer();
    
    ProductQuantizer(const ProductQuantizer&) = delete;
    ProductQuantizer& operator=(const ProductQuantizer&) = delete;
    ProductQuantizer(ProductQuantizer&&) noexcept;
    ProductQuantizer& operator=(ProductQuantizer&&) noexcept;
    
    // Training
    [[nodiscard]] Result<void> train(std::span<const Vector> training_data);
    [[nodiscard]] bool is_trained() const { return trained_; }
    
    // Encoding (compression)
    [[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
    [[nodiscard]] Result<std::vector<std::vector<uint8_t>>> encode_batch(
        std::span<const Vector> vectors) const;
    
    // Decoding (reconstruction)
    [[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
    
    // Distance computation (ADC - Asymmetric Distance Computation)
    [[nodiscard]] Distance compute_distance(VectorView query, 
        std::span<const uint8_t> codes) const;
    [[nodiscard]] std::vector<Distance> precompute_distance_table(
        VectorView query) const;
    [[nodiscard]] Distance compute_distance_precomputed(
        std::span<const uint8_t> codes,
        std::span<const Distance> distance_table) const;
    
    // Stats
    [[nodiscard]] const ProductQuantizerConfig& config() const { return config_; }
    [[nodiscard]] Dim dimension() const { return config_.dimension; }
    [[nodiscard]] size_t code_size() const { return config_.num_subquantizers; }
    [[nodiscard]] float compression_ratio() const {
        return static_cast<float>(config_.dimension * sizeof(Scalar)) / code_size();
    }
    
    // Persistence
    [[nodiscard]] Result<void> save(std::string_view path) const;
    [[nodiscard]] static Result<ProductQuantizer> load(std::string_view path);

private:
    ProductQuantizerConfig config_;
    Dim subvector_dim_;
    bool trained_;
    std::vector<std::vector<Vector>> codebooks_;
    
    void train_subquantizer(uint32_t subq_idx, std::span<const Vector> subvectors);
    Vector extract_subvector(VectorView vector, uint32_t subq_idx) const;
    uint8_t find_nearest_centroid(const Vector& subvector, uint32_t subq_idx) const;
    [[nodiscard]] Result<void> validate_config() const;
    [[nodiscard]] Result<void> validate_vector(VectorView vector) const;
};

}} // namespace vdb::quantization
