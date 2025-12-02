#pragma once
// ============================================================================
// VectorDB - Distance Functions (SIMD Optimized)
// ============================================================================

#include "core.hpp"

namespace vdb {

// ============================================================================
// Distance Type (alias for compatibility)
// ============================================================================

using DistanceType = DistanceMetric;

// ============================================================================
// Low-level Distance Functions (raw pointers for performance)
// ============================================================================

/// Dot product of two float arrays
[[nodiscard]] float dot_product(const Scalar* a, const Scalar* b, Dim n);

/// Euclidean distance between two float arrays
[[nodiscard]] float euclidean_distance(const Scalar* a, const Scalar* b, Dim n);

/// Squared Euclidean distance (no sqrt, faster)
[[nodiscard]] float squared_euclidean(const Scalar* a, const Scalar* b, Dim n);

/// Cosine similarity between two float arrays
[[nodiscard]] float cosine_similarity(const Scalar* a, const Scalar* b, Dim n);

/// Cosine distance (1 - cosine_similarity)
[[nodiscard]] float cosine_distance(const Scalar* a, const Scalar* b, Dim n);

// ============================================================================
// Distance Function Interface (VectorView)
// ============================================================================

/// Compute distance between two vectors
[[nodiscard]] Distance compute_distance(
    VectorView a, 
    VectorView b, 
    DistanceMetric metric = DistanceMetric::Cosine
);

/// Compute L2 (Euclidean) distance
[[nodiscard]] Distance l2_distance(VectorView a, VectorView b);

/// Compute squared L2 distance (faster, no sqrt)
[[nodiscard]] Distance l2_squared_distance(VectorView a, VectorView b);

/// Compute cosine distance (1 - cosine_similarity)
[[nodiscard]] Distance cosine_distance(VectorView a, VectorView b);

/// Compute dot product (for normalized vectors, negate for distance)
[[nodiscard]] Distance dot_product(VectorView a, VectorView b);

/// Compute cosine similarity [-1, 1]
[[nodiscard]] Distance cosine_similarity(VectorView a, VectorView b);

// ============================================================================
// Vector Operations
// ============================================================================

/// Normalize vector to unit length (in-place)
void normalize(Vector& v);

/// Normalize vector to unit length (returns new vector)
[[nodiscard]] Vector normalized(VectorView v);

/// Compute L2 norm (magnitude)
[[nodiscard]] Distance l2_norm(VectorView v);

/// Add two vectors: result = a + b
[[nodiscard]] Vector add(VectorView a, VectorView b);

/// Subtract vectors: result = a - b
[[nodiscard]] Vector subtract(VectorView a, VectorView b);

/// Scale vector: result = v * scalar
[[nodiscard]] Vector scale(VectorView v, Scalar s);

/// Add scaled vector: result = a + b * scale
[[nodiscard]] Vector add_scaled(VectorView a, VectorView b, Scalar scale);

/// Element-wise multiply
[[nodiscard]] Vector multiply(VectorView a, VectorView b);

/// Mean of multiple vectors
[[nodiscard]] Vector mean(std::span<const Vector> vectors);

// ============================================================================
// Batch Operations (for efficiency)
// ============================================================================

/// Compute distances from query to all targets
[[nodiscard]] std::vector<Distance> batch_distance(
    VectorView query,
    std::span<const Vector> targets,
    DistanceMetric metric = DistanceMetric::Cosine
);

/// Find k nearest neighbors (brute force)
[[nodiscard]] SearchResults brute_force_knn(
    VectorView query,
    std::span<const Vector> vectors,
    size_t k,
    DistanceMetric metric = DistanceMetric::Cosine
);

// ============================================================================
// Projection (for unified embedding space)
// ============================================================================

/// Linear projection matrix for dimension mapping
class ProjectionMatrix {
public:
    ProjectionMatrix() = default;
    ProjectionMatrix(Dim input_dim, Dim output_dim);
    
    /// Project vector to new dimension
    [[nodiscard]] Vector project(VectorView input) const;
    
    /// Initialize with random orthogonal projection (preserves distances)
    void init_random_orthogonal(uint64_t seed = 42);
    
    /// Load from file
    [[nodiscard]] Result<void> load(std::string_view path);
    
    /// Save to file
    [[nodiscard]] Result<void> save(std::string_view path) const;
    
    [[nodiscard]] Dim input_dim() const { return input_dim_; }
    [[nodiscard]] Dim output_dim() const { return output_dim_; }

private:
    Dim input_dim_ = 0;
    Dim output_dim_ = 0;
    std::vector<Scalar> weights_;  // Row-major: output_dim x input_dim
};

} // namespace vdb
