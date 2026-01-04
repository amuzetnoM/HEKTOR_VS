#pragma once
// ============================================================================
// VectorDB - HNSW Index (Hierarchical Navigable Small World)
// ============================================================================

#include "core.hpp"
#include "distance.hpp"
#include <shared_mutex>
#include <random>
#include <functional>

namespace vdb {

// ============================================================================
// HNSW Configuration
// ============================================================================

struct HnswConfig {
    Dim dimension = UNIFIED_DIM;
    size_t max_elements = HNSW_MAX_ELEMENTS;
    size_t M = HNSW_M;                          // Max connections per layer
    size_t ef_construction = HNSW_EF_CONSTRUCTION;
    size_t ef_search = HNSW_EF_SEARCH;
    DistanceMetric metric = DistanceMetric::Cosine;
    uint64_t seed = 42;
    bool allow_replace = false;                  // Allow replacing existing vectors
    size_t num_threads = 0;                      // 0 = auto-detect
};

// ============================================================================
// HNSW Index
// ============================================================================

class HnswIndex {
public:
    explicit HnswIndex(const HnswConfig& config = {});
    ~HnswIndex();
    
    // Non-copyable, movable
    HnswIndex(const HnswIndex&) = delete;
    HnswIndex& operator=(const HnswIndex&) = delete;
    HnswIndex(HnswIndex&&) noexcept;
    HnswIndex& operator=(HnswIndex&&) noexcept;
    
    // ========================================================================
    // Core Operations
    // ========================================================================
    
    /// Add a vector with given ID
    [[nodiscard]] Result<void> add(VectorId id, VectorView vector);
    
    /// Add multiple vectors (batch, parallelized)
    [[nodiscard]] Result<void> add_batch(
        std::span<const VectorId> ids,
        std::span<const Vector> vectors
    );
    
    /// Search for k nearest neighbors
    [[nodiscard]] SearchResults search(VectorView query, size_t k) const;
    
    /// Search with filter function
    [[nodiscard]] SearchResults search_filtered(
        VectorView query,
        size_t k,
        std::function<bool(VectorId)> filter
    ) const;
    
    /// Remove a vector by ID
    [[nodiscard]] Result<void> remove(VectorId id);
    
    /// Check if vector exists
    [[nodiscard]] bool contains(VectorId id) const;
    
    /// Get vector by ID (if stored)
    [[nodiscard]] std::optional<Vector> get_vector(VectorId id) const;
    
    // ========================================================================
    // Index Management
    // ========================================================================
    
    /// Get current element count
    [[nodiscard]] size_t size() const;
    
    /// Get maximum capacity
    [[nodiscard]] size_t capacity() const { return config_.max_elements; }
    
    /// Check if empty
    [[nodiscard]] bool empty() const { return size() == 0; }
    
    /// Get dimension
    [[nodiscard]] Dim dimension() const { return config_.dimension; }
    
    /// Get configuration
    [[nodiscard]] const HnswConfig& config() const { return config_; }
    
    /// Get statistics
    [[nodiscard]] IndexStats stats() const;
    
    /// Set search ef parameter (trade accuracy for speed)
    void set_ef_search(size_t ef);
    
    /// Resize index (expensive, rebuilds)
    [[nodiscard]] Result<void> resize(size_t new_max_elements);
    
    /// Optimize index (compact memory, rebalance)
    void optimize();
    
    // ========================================================================
    // Persistence
    // ========================================================================
    
    /// Save index to file
    [[nodiscard]] Result<void> save(std::string_view path) const;
    
    /// Load index from file
    [[nodiscard]] static Result<HnswIndex> load(std::string_view path);
    
    /// Serialize to bytes
    [[nodiscard]] std::vector<uint8_t> serialize() const;
    
    /// Deserialize from bytes
    [[nodiscard]] static Result<HnswIndex> deserialize(std::span<const uint8_t> data);

private:
    // Internal node structure
    struct Node {
        VectorId id;
        int level;
        std::vector<std::vector<VectorId>> connections;  // Per-level connections
        Vector vector;  // Store vector for distance computation
        bool deleted = false;  // Lazy deletion flag
    };
    
    // Select random level for new node (exponential distribution)
    [[nodiscard]] int random_level();
    
    // Search layer for closest nodes
    [[nodiscard]] std::vector<VectorId> search_layer(
        VectorView query,
        VectorId entry_point,
        size_t ef,
        int layer
    ) const;
    
    // Select neighbors using heuristic
    [[nodiscard]] std::vector<VectorId> select_neighbors(
        VectorView query,
        std::vector<VectorId>& candidates,
        size_t M,
        int layer
    ) const;
    
    // Get distance to node
    [[nodiscard]] Distance distance_to_node(VectorView query, VectorId node_id) const;
    
    // Mutate connections (thread-safe)
    void connect_nodes(VectorId from, VectorId to, int layer);
    
    HnswConfig config_;
    std::vector<Node> nodes_;
    std::unordered_map<VectorId, size_t> id_to_index_;
    VectorId entry_point_ = 0;
    int max_level_ = 0;
    size_t element_count_ = 0;
    
    mutable std::shared_mutex mutex_;
    std::mt19937_64 rng_;
    double level_mult_;  // 1 / log(M)
};

// ============================================================================
// Flat Index (Brute Force, for small datasets or testing)
// ============================================================================

class FlatIndex {
public:
    explicit FlatIndex(Dim dimension, DistanceMetric metric = DistanceMetric::Cosine);
    
    [[nodiscard]] Result<void> add(VectorId id, VectorView vector);
    [[nodiscard]] SearchResults search(VectorView query, size_t k) const;
    [[nodiscard]] bool contains(VectorId id) const;
    [[nodiscard]] std::optional<Vector> get_vector(VectorId id) const;
    [[nodiscard]] size_t size() const { return vectors_.size(); }
    [[nodiscard]] Dim dimension() const { return dimension_; }
    
    [[nodiscard]] Result<void> save(std::string_view path) const;
    [[nodiscard]] static Result<FlatIndex> load(std::string_view path);

private:
    Dim dimension_;
    DistanceMetric metric_;
    std::vector<VectorId> ids_;
    std::vector<Vector> vectors_;
    std::unordered_map<VectorId, size_t> id_to_index_;
};

} // namespace vdb
