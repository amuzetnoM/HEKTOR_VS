#pragma once
// ============================================================================
// VectorDB - Structured Quantization with Learned Codebooks
// Enhances Product Quantization with vector-level pattern learning
// ============================================================================

#include "../core.hpp"
#include "../distance.hpp"
#include "product_quantizer.hpp"
#include <vector>
#include <span>
#include <optional>

namespace vdb {
namespace quantization {

// ============================================================================
// Structured Quantizer Configuration
// ============================================================================

struct StructuredQuantizerConfig {
    Dim dimension = UNIFIED_DIM;
    
    // Codebook configuration
    uint32_t codebook_size = 65536;      // 16-bit codes (2^16)
    uint32_t num_iterations = 50;        // K-means iterations
    uint32_t num_threads = 0;            // 0 = auto-detect
    
    // Hierarchical codebook (optional)
    bool use_hierarchical = false;       // Two-level codebook
    uint32_t coarse_codebook_size = 256; // First level (8-bit)
    uint32_t fine_codebook_size = 256;   // Second level (8-bit)
    
    // Residual quantization (optional)
    bool use_residual = false;           // Quantize residuals
    uint32_t num_residual_stages = 2;    // Number of refinement stages
    
    // Distance metric
    DistanceMetric metric = DistanceMetric::L2;
    
    // Training
    uint64_t seed = 42;
    float convergence_threshold = 1e-4f;
};

// ============================================================================
// Structured Quantizer - Vector-level pattern learning
// ============================================================================

class StructuredQuantizer {
public:
    explicit StructuredQuantizer(const StructuredQuantizerConfig& config = {});
    ~StructuredQuantizer() = default;
    
    // Training
    [[nodiscard]] Result<void> train(std::span<const Vector> training_data);
    [[nodiscard]] bool is_trained() const { return trained_; }
    
    // Encoding (compression)
    [[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
    [[nodiscard]] Result<std::vector<std::vector<uint8_t>>> encode_batch(
        std::span<const Vector> vectors) const;
    
    // Decoding (reconstruction)
    [[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
    
    // Distance computation
    [[nodiscard]] Distance compute_distance(VectorView query, 
        std::span<const uint8_t> codes) const;
    
    // Stats
    [[nodiscard]] const StructuredQuantizerConfig& config() const { return config_; }
    [[nodiscard]] size_t code_size() const;
    [[nodiscard]] float compression_ratio() const;
    
    // Persistence
    [[nodiscard]] Result<void> save(std::string_view path) const;
    [[nodiscard]] static Result<StructuredQuantizer> load(std::string_view path);

private:
    StructuredQuantizerConfig config_;
    bool trained_;
    
    // Codebook storage
    std::vector<Vector> codebook_;              // Main codebook
    std::vector<Vector> coarse_codebook_;       // Hierarchical: coarse level
    std::vector<std::vector<Vector>> fine_codebooks_; // Hierarchical: fine level
    std::vector<std::vector<Vector>> residual_codebooks_; // Residual stages
    
    // Training helpers
    void train_flat_codebook(std::span<const Vector> data);
    void train_hierarchical_codebook(std::span<const Vector> data);
    void train_residual_codebook(std::span<const Vector> data);
    
    [[nodiscard]] uint32_t find_nearest_codeword(VectorView vector) const;
    [[nodiscard]] std::pair<uint32_t, uint32_t> find_nearest_hierarchical(
        VectorView vector) const;
    
    [[nodiscard]] Result<void> validate_config() const;
    [[nodiscard]] Result<void> validate_vector(VectorView vector) const;
};

// ============================================================================
// Cross-Channel Perceptual Coupling
// ============================================================================

enum class ColorSpace {
    RGB,    // Standard RGB
    LAB,    // Perceptual (CIE LAB)
    YCbCr,  // Luminance-Chrominance
};

struct PerceptualQuantizerConfig {
    Dim dimension = UNIFIED_DIM;
    ColorSpace color_space = ColorSpace::LAB;
    
    // Separate quantization for luminance vs chrominance
    bool separate_luma_chroma = true;
    uint32_t luma_bits = 10;        // Higher precision for luminance
    uint32_t chroma_bits = 6;       // Lower precision for chrominance
    
    // Underlying quantizer
    ProductQuantizerConfig pq_config;
};

class PerceptualQuantizer {
public:
    explicit PerceptualQuantizer(const PerceptualQuantizerConfig& config = {});
    
    // Training
    [[nodiscard]] Result<void> train(std::span<const Vector> training_data);
    [[nodiscard]] bool is_trained() const { return trained_; }
    
    // Encoding with perceptual transform
    [[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
    [[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
    
    // Distance in perceptual space
    [[nodiscard]] Distance compute_perceptual_distance(VectorView a, VectorView b) const;
    
    // Stats
    [[nodiscard]] size_t code_size() const;
    [[nodiscard]] float compression_ratio() const;

private:
    PerceptualQuantizerConfig config_;
    bool trained_;
    
    ProductQuantizer luma_quantizer_;
    ProductQuantizer chroma_quantizer_;
    
    // Color space transforms
    [[nodiscard]] Vector to_perceptual_space(VectorView rgb) const;
    [[nodiscard]] Vector from_perceptual_space(VectorView perceptual) const;
    
    [[nodiscard]] std::pair<Vector, Vector> separate_luma_chroma(VectorView perceptual) const;
    [[nodiscard]] Vector combine_luma_chroma(VectorView luma, VectorView chroma) const;
};

// ============================================================================
// Manifold-Aware Quantization
// ============================================================================

struct ManifoldQuantizerConfig {
    Dim dimension = UNIFIED_DIM;
    
    // PCA-based adaptation
    bool use_pca = true;
    float variance_threshold = 0.95f;  // Retain 95% variance
    
    // Adaptive bit allocation
    bool adaptive_precision = true;
    uint32_t min_bits_per_dim = 4;
    uint32_t max_bits_per_dim = 12;
    
    // Total bit budget
    uint32_t total_bits = 512 * 8;  // 512 bytes
};

class ManifoldQuantizer {
public:
    explicit ManifoldQuantizer(const ManifoldQuantizerConfig& config = {});
    
    // Training - learns manifold structure
    [[nodiscard]] Result<void> train(std::span<const Vector> training_data);
    [[nodiscard]] bool is_trained() const { return trained_; }
    
    // Encoding with manifold-aware precision
    [[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
    [[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
    
    // Stats
    [[nodiscard]] size_t code_size() const;
    [[nodiscard]] const std::vector<uint32_t>& bit_allocation() const { 
        return bits_per_dimension_; 
    }

private:
    ManifoldQuantizerConfig config_;
    bool trained_;
    
    // PCA components
    std::vector<Vector> principal_components_;
    std::vector<float> eigenvalues_;
    Vector mean_;
    
    // Adaptive bit allocation
    std::vector<uint32_t> bits_per_dimension_;
    std::vector<float> scales_;
    std::vector<float> offsets_;
    
    [[nodiscard]] Vector transform_to_pca_space(VectorView vector) const;
    [[nodiscard]] Vector transform_from_pca_space(VectorView pca_vector) const;
    
    void compute_bit_allocation();
};

}} // namespace vdb::quantization
