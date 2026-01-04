#pragma once
// ============================================================================
// VectorDB - Unified Storage Backend (SQLite + pgvector)
// ============================================================================

#include "../core.hpp"
#include "../storage.hpp"
#include "sqlite_store.hpp"
#include "pgvector_store.hpp"
#include <memory>
#include <atomic>

namespace vdb {
namespace storage {

// ============================================================================
// Storage Backend Type
// ============================================================================

enum class StorageBackend {
    SQLite,
    PgVector,
    Hybrid  // SQLite for cache, pgvector for persistent
};

// ============================================================================
// Unified Storage Configuration
// ============================================================================

struct UnifiedStoreConfig {
    StorageBackend backend = StorageBackend::SQLite;
    
    // SQLite config (for SQLite or Hybrid mode)
    SqliteConfig sqlite_config;
    
    // pgvector config (for PgVector or Hybrid mode)
    PgVectorConfig pgvector_config;
    
    // Hybrid mode settings
    bool enable_write_through = true;       // Write to both backends
    bool enable_read_cache = true;          // Cache reads in SQLite
    size_t cache_max_items = 10000;         // Max cached vectors
    uint32_t cache_ttl_seconds = 3600;      // Cache expiration
    
    // Fallback settings
    bool enable_fallback = true;            // Fall back to SQLite if pgvector fails
    uint32_t reconnect_interval_ms = 5000;  // Retry interval
    size_t max_reconnect_attempts = 3;
};

// ============================================================================
// Unified Store Implementation
// ============================================================================

class UnifiedStore {
public:
    explicit UnifiedStore(const UnifiedStoreConfig& config);
    ~UnifiedStore();
    
    // Initialization
    Result<void> init(Dim dimension, DistanceMetric metric);
    Result<void> close();
    
    // Vector operations
    Result<VectorId> add(VectorView vector, const Metadata& metadata);
    Result<void> add_batch(const std::vector<std::pair<VectorView, Metadata>>& items);
    Result<bool> remove(VectorId id);
    Result<void> remove_batch(const std::vector<VectorId>& ids);
    Result<std::optional<Vector>> get(VectorId id) const;
    Result<std::optional<Metadata>> get_metadata(VectorId id) const;
    Result<void> update_metadata(VectorId id, const Metadata& metadata);
    
    // Search operations
    Result<std::vector<std::pair<VectorId, Distance>>>
    search(VectorView query, size_t k, const std::function<bool(const Metadata&)>& filter = nullptr) const;
    
    // Statistics
    Result<size_t> count() const;
    Result<size_t> size_bytes() const;
    
    // Sync operations
    Result<void> sync();
    Result<void> vacuum();
    
    // Backend management
    Result<StorageBackend> get_active_backend() const;
    Result<void> switch_backend(StorageBackend backend);
    Result<bool> is_backend_healthy(StorageBackend backend) const;
    
    // Cache management (Hybrid mode)
    Result<void> flush_cache();
    Result<void> warm_cache(size_t num_items);
    Result<size_t> get_cache_size() const;
    Result<float> get_cache_hit_rate() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    UnifiedStoreConfig config_;
    Dim dimension_ = 0;
    DistanceMetric metric_ = DistanceMetric::Cosine;
    
    // Helper methods
    Result<void> write_through(VectorId id, VectorView vector, const Metadata& metadata);
    Result<void> try_reconnect_pgvector();
};

}} // namespace vdb::storage
