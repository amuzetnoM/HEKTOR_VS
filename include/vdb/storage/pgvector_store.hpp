#pragma once
// ============================================================================
// VectorDB - PostgreSQL pgvector Storage Backend
// ============================================================================

#include "../core.hpp"
#include "../storage.hpp"
#include <string>
#include <memory>
#include <optional>

#ifdef HAVE_POSTGRESQL
#include <libpq-fe.h>
#endif

namespace vdb {
namespace storage {

// ============================================================================
// pgvector Configuration
// ============================================================================

struct PgVectorConfig {
    std::string host = "localhost";
    uint16_t port = 5432;
    std::string database = "vectordb";
    std::string user = "postgres";
    std::string password;
    std::string table_name = "vectors";
    
    // Connection pooling
    size_t min_connections = 2;
    size_t max_connections = 10;
    
    // Performance
    size_t batch_size = 1000;
    bool use_ivfflat = true;              // Use IVFFlat index
    size_t ivfflat_lists = 100;           // Number of lists
    
    // Timeouts
    uint32_t connect_timeout_ms = 5000;
    uint32_t query_timeout_ms = 30000;
};

// ============================================================================
// pgvector Store Implementation
// ============================================================================

class PgVectorStore {
public:
    explicit PgVectorStore(const PgVectorConfig& config);
    ~PgVectorStore();
    
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
    
    // Index management
    Result<void> create_index();
    Result<void> drop_index();
    Result<void> reindex();
    
    // Health check
    Result<bool> is_connected() const;
    Result<void> reconnect();
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    PgVectorConfig config_;
    Dim dimension_ = 0;
    DistanceMetric metric_ = DistanceMetric::Cosine;
};

}} // namespace vdb::storage
