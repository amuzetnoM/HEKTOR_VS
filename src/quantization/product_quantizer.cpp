// ============================================================================
// VectorDB - Product Quantization Implementation
// Achieves 8-32x compression with asymmetric distance computation
// ============================================================================

#include "vdb/quantization/product_quantizer.hpp"
#include "vdb/distance.hpp"
#include <algorithm>
#include <random>
#include <cmath>
#include <fstream>
#include <cstring>

namespace vdb {
namespace quantization {

// ============================================================================
// Constructor / Destructor
// ============================================================================

ProductQuantizer::ProductQuantizer(const ProductQuantizerConfig& config)
    : config_(config)
    , subvector_dim_(0)
    , trained_(false)
{
    if (config_.dimension % config_.num_subquantizers != 0) {
        config_.num_subquantizers = 8; // Fallback
    }
    subvector_dim_ = config_.dimension / config_.num_subquantizers;
    codebooks_.resize(config_.num_subquantizers);
}

ProductQuantizer::~ProductQuantizer() = default;

ProductQuantizer::ProductQuantizer(ProductQuantizer&&) noexcept = default;
ProductQuantizer& ProductQuantizer::operator=(ProductQuantizer&&) noexcept = default;

// ============================================================================
// Training
// ============================================================================

Result<void> ProductQuantizer::train(std::span<const Vector> training_data) {
    if (training_data.empty()) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Empty training data"});
    }
    
    auto validation = validate_config();
    if (!validation) return validation;
    
    // Extract subvectors for each subquantizer
    std::vector<std::vector<Vector>> subvectors_per_sq(config_.num_subquantizers);
    
    for (const auto& vec : training_data) {
        if (vec.size() != config_.dimension) {
            return tl::unexpected(Error{ErrorCode::InvalidDimension, "Vector dimension mismatch"});
        }
        
        for (uint32_t sq = 0; sq < config_.num_subquantizers; ++sq) {
            subvectors_per_sq[sq].push_back(extract_subvector(vec, sq));
        }
    }
    
    // Train each subquantizer independently
    for (uint32_t sq = 0; sq < config_.num_subquantizers; ++sq) {
        train_subquantizer(sq, subvectors_per_sq[sq]);
    }
    
    trained_ = true;
    return {};
}

void ProductQuantizer::train_subquantizer(uint32_t subq_idx, 
    std::span<const Vector> subvectors) 
{
    const size_t n = subvectors.size();
    const uint32_t k = config_.num_centroids;
    const size_t dim = subvector_dim_;
    
    // Initialize centroids using k-means++
    std::vector<Vector> centroids(k);
    std::mt19937_64 rng(config_.seed + subq_idx);
    
    // First centroid: random
    std::uniform_int_distribution<size_t> dist(0, n - 1);
    centroids[0] = subvectors[dist(rng)];
    
    // Remaining centroids: k-means++
    std::vector<float> min_distances(n, std::numeric_limits<float>::max());
    
    for (uint32_t c = 1; c < k; ++c) {
        // Update minimum distances
        for (size_t i = 0; i < n; ++i) {
            float d = squared_euclidean(subvectors[i].data(), 
                centroids[c-1].data(), dim);
            min_distances[i] = std::min(min_distances[i], d);
        }
        
        // Sample proportional to squared distance
        std::discrete_distribution<size_t> weighted(
            min_distances.begin(), min_distances.end());
        centroids[c] = subvectors[weighted(rng)];
    }
    
    // K-means iterations
    std::vector<uint32_t> assignments(n);
    std::vector<size_t> counts(k);
    
    for (uint32_t iter = 0; iter < config_.num_iterations; ++iter) {
        // Assignment step
        std::fill(counts.begin(), counts.end(), 0);
        
        for (size_t i = 0; i < n; ++i) {
            float min_dist = std::numeric_limits<float>::max();
            uint32_t best_c = 0;
            
            for (uint32_t c = 0; c < k; ++c) {
                float d = squared_euclidean(subvectors[i].data(),
                    centroids[c].data(), dim);
                if (d < min_dist) {
                    min_dist = d;
                    best_c = c;
                }
            }
            
            assignments[i] = best_c;
            counts[best_c]++;
        }
        
        // Update step
        for (uint32_t c = 0; c < k; ++c) {
            if (counts[c] == 0) continue;
            
            std::fill(centroids[c].begin(), centroids[c].end(), 0.0f);
            
            for (size_t i = 0; i < n; ++i) {
                if (assignments[i] == c) {
                    for (size_t d = 0; d < dim; ++d) {
                        centroids[c][d] += subvectors[i][d];
                    }
                }
            }
            
            for (size_t d = 0; d < dim; ++d) {
                centroids[c][d] /= counts[c];
            }
        }
    }
    
    codebooks_[subq_idx] = std::move(centroids);
}

// ============================================================================
// Encoding
// ============================================================================

Result<std::vector<uint8_t>> ProductQuantizer::encode(VectorView vector) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::InvalidState, "Quantizer not trained"});
    }
    
    auto validation = validate_vector(vector);
    if (!validation) return tl::unexpected(validation.error());
    
    std::vector<uint8_t> codes(config_.num_subquantizers);
    
    for (uint32_t sq = 0; sq < config_.num_subquantizers; ++sq) {
        Vector subvec = extract_subvector(vector, sq);
        codes[sq] = find_nearest_centroid(subvec, sq);
    }
    
    return codes;
}

Result<std::vector<std::vector<uint8_t>>> ProductQuantizer::encode_batch(
    std::span<const Vector> vectors) const 
{
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::InvalidState, "Quantizer not trained"});
    }
    
    std::vector<std::vector<uint8_t>> codes_batch;
    codes_batch.reserve(vectors.size());
    
    for (const auto& vec : vectors) {
        auto codes = encode(vec);
        if (!codes) return tl::unexpected(codes.error());
        codes_batch.push_back(std::move(*codes));
    }
    
    return codes_batch;
}

// ============================================================================
// Decoding
// ============================================================================

Result<Vector> ProductQuantizer::decode(std::span<const uint8_t> codes) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::InvalidState, "Quantizer not trained"});
    }
    
    if (codes.size() != config_.num_subquantizers) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Invalid code size"});
    }
    
    Vector reconstructed(config_.dimension);
    
    for (uint32_t sq = 0; sq < config_.num_subquantizers; ++sq) {
        uint8_t code = codes[sq];
        if (code >= config_.num_centroids) {
            return tl::unexpected(Error{ErrorCode::InvalidData, "Invalid code value"});
        }
        
        const auto& centroid = codebooks_[sq][code];
        size_t offset = sq * subvector_dim_;
        
        std::copy(centroid.begin(), centroid.end(), 
            reconstructed.begin() + offset);
    }
    
    return reconstructed;
}

// ============================================================================
// Distance Computation (ADC)
// ============================================================================

Distance ProductQuantizer::compute_distance(VectorView query,
    std::span<const uint8_t> codes) const 
{
    if (!trained_ || codes.size() != config_.num_subquantizers) {
        return std::numeric_limits<Distance>::max();
    }
    
    Distance total_dist = 0.0f;
    
    for (uint32_t sq = 0; sq < config_.num_subquantizers; ++sq) {
        uint8_t code = codes[sq];
        if (code >= config_.num_centroids) continue;
        
        Vector query_subvec = extract_subvector(query, sq);
        const auto& centroid = codebooks_[sq][code];
        
        Distance d = squared_euclidean(query_subvec.data(), 
            centroid.data(), subvector_dim_);
        total_dist += d;
    }
    
    return std::sqrt(total_dist);
}

std::vector<Distance> ProductQuantizer::precompute_distance_table(
    VectorView query) const 
{
    if (!trained_) return {};
    
    size_t table_size = config_.num_subquantizers * config_.num_centroids;
    std::vector<Distance> table(table_size);
    
    for (uint32_t sq = 0; sq < config_.num_subquantizers; ++sq) {
        Vector query_subvec = extract_subvector(query, sq);
        
        for (uint32_t c = 0; c < config_.num_centroids; ++c) {
            const auto& centroid = codebooks_[sq][c];
            Distance d = squared_euclidean(query_subvec.data(),
                centroid.data(), subvector_dim_);
            table[sq * config_.num_centroids + c] = d;
        }
    }
    
    return table;
}

Distance ProductQuantizer::compute_distance_precomputed(
    std::span<const uint8_t> codes,
    std::span<const Distance> distance_table) const 
{
    if (codes.size() != config_.num_subquantizers) {
        return std::numeric_limits<Distance>::max();
    }
    
    Distance total_dist = 0.0f;
    
    for (uint32_t sq = 0; sq < config_.num_subquantizers; ++sq) {
        uint8_t code = codes[sq];
        if (code >= config_.num_centroids) continue;
        
        total_dist += distance_table[sq * config_.num_centroids + code];
    }
    
    return std::sqrt(total_dist);
}

// ============================================================================
// Helper Methods
// ============================================================================

Vector ProductQuantizer::extract_subvector(VectorView vector, uint32_t subq_idx) const {
    size_t start = subq_idx * subvector_dim_;
    Vector subvec(subvector_dim_);
    std::copy(vector.data() + start, vector.data() + start + subvector_dim_,
        subvec.begin());
    return subvec;
}

uint8_t ProductQuantizer::find_nearest_centroid(const Vector& subvector, 
    uint32_t subq_idx) const 
{
    float min_dist = std::numeric_limits<float>::max();
    uint8_t best_code = 0;
    
    for (uint32_t c = 0; c < config_.num_centroids; ++c) {
        float d = squared_euclidean(subvector.data(),
            codebooks_[subq_idx][c].data(), subvector_dim_);
        if (d < min_dist) {
            min_dist = d;
            best_code = static_cast<uint8_t>(c);
        }
    }
    
    return best_code;
}

Result<void> ProductQuantizer::validate_config() const {
    if (config_.dimension == 0) {
        return tl::unexpected(Error{ErrorCode::InvalidDimension, "Zero dimension"});
    }
    if (config_.num_subquantizers == 0) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Zero subquantizers"});
    }
    if (config_.dimension % config_.num_subquantizers != 0) {
        return tl::unexpected(Error{ErrorCode::InvalidDimension, 
            "Dimension must be divisible by num_subquantizers"});
    }
    if (config_.num_centroids == 0 || config_.num_centroids > 256) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, 
            "num_centroids must be 1-256"});
    }
    return {};
}

Result<void> ProductQuantizer::validate_vector(VectorView vector) const {
    if (vector.dim() != config_.dimension) {
        return tl::unexpected(Error{ErrorCode::InvalidDimension, 
            "Vector dimension mismatch"});
    }
    return {};
}

// ============================================================================
// Persistence
// ============================================================================

Result<void> ProductQuantizer::save(std::string_view path) const {
    std::ofstream file(std::string(path), std::ios::binary);
    if (!file) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to open file"});
    }
    
    // Write header
    file.write(reinterpret_cast<const char*>(&config_.dimension), sizeof(Dim));
    file.write(reinterpret_cast<const char*>(&config_.num_subquantizers), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&config_.num_centroids), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&trained_), sizeof(bool));
    
    // Write codebooks
    for (const auto& codebook : codebooks_) {
        for (const auto& centroid : codebook) {
            file.write(reinterpret_cast<const char*>(centroid.data()),
                centroid.size() * sizeof(Scalar));
        }
    }
    
    return {};
}

Result<ProductQuantizer> ProductQuantizer::load(std::string_view path) {
    std::ifstream file(std::string(path), std::ios::binary);
    if (!file) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to open file"});
    }
    
    ProductQuantizerConfig config;
    bool trained;
    
    file.read(reinterpret_cast<char*>(&config.dimension), sizeof(Dim));
    file.read(reinterpret_cast<char*>(&config.num_subquantizers), sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(&config.num_centroids), sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(&trained), sizeof(bool));
    
    ProductQuantizer pq(config);
    pq.trained_ = trained;
    
    if (trained) {
        Dim subvector_dim = config.dimension / config.num_subquantizers;
        
        for (uint32_t sq = 0; sq < config.num_subquantizers; ++sq) {
            pq.codebooks_[sq].resize(config.num_centroids);
            for (uint32_t c = 0; c < config.num_centroids; ++c) {
                pq.codebooks_[sq][c].resize(subvector_dim);
                file.read(reinterpret_cast<char*>(pq.codebooks_[sq][c].data()),
                    subvector_dim * sizeof(Scalar));
            }
        }
    }
    
    return pq;
}

}} // namespace vdb::quantization
