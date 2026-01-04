#pragma once
// ============================================================================
// VectorDB - Replication & Sharding Support
// ============================================================================

#include "core.hpp"
#include <vector>
#include <string>
#include <functional>

namespace vdb {

// ============================================================================
// Replication Configuration
// ============================================================================

enum class ReplicationMode {
    None,           // Single node
    Async,          // Asynchronous replication
    Sync,           // Synchronous replication
    SemiSync        // Semi-synchronous (ack from at least one replica)
};

struct NodeConfig {
    std::string node_id;
    std::string host;
    uint16_t port;
    bool is_primary = false;
    int priority = 0;  // For failover
};

struct ReplicationConfig {
    ReplicationMode mode = ReplicationMode::None;
    std::vector<NodeConfig> nodes;
    
    // Replication settings
    size_t min_replicas = 1;                // Minimum replicas for writes
    uint32_t sync_timeout_ms = 5000;        // Timeout for sync mode
    uint32_t heartbeat_interval_ms = 1000;  // Health check interval
    uint32_t election_timeout_ms = 5000;    // Leader election timeout
    
    // Conflict resolution
    enum class ConflictResolution {
        LastWriteWins,
        HighestPriority,
        Manual
    } conflict_resolution = ConflictResolution::LastWriteWins;
};

// ============================================================================
// Sharding Configuration
// ============================================================================

enum class ShardingStrategy {
    None,           // No sharding
    Hash,           // Hash-based sharding
    Range,          // Range-based sharding
    Consistent      // Consistent hashing
};

struct ShardConfig {
    std::string shard_id;
    size_t start_range = 0;         // For range sharding
    size_t end_range = 0;
    std::vector<NodeConfig> replicas;
};

struct ShardingConfig {
    ShardingStrategy strategy = ShardingStrategy::None;
    size_t num_shards = 1;
    std::vector<ShardConfig> shards;
    
    // Resharding
    bool enable_auto_resharding = false;
    size_t reshard_threshold_items = 1000000;  // Trigger resharding
    float reshard_threshold_imbalance = 0.2f;   // 20% imbalance
};

// ============================================================================
// Replication Manager
// ============================================================================

class ReplicationManager {
public:
    explicit ReplicationManager(const ReplicationConfig& config);
    ~ReplicationManager();
    
    // Lifecycle
    Result<void> start();
    Result<void> stop();
    
    // Replication operations
    Result<void> replicate_add(VectorId id, VectorView vector, const Metadata& metadata);
    Result<void> replicate_remove(VectorId id);
    Result<void> replicate_update(VectorId id, const Metadata& metadata);
    
    // Node management
    Result<void> add_replica(const NodeConfig& node);
    Result<void> remove_replica(const std::string& node_id);
    Result<std::vector<NodeConfig>> get_replicas() const;
    
    // Health and status
    Result<bool> is_healthy() const;
    Result<std::string> get_primary_node() const;
    Result<void> promote_to_primary();
    Result<void> demote_from_primary();
    
    // Failover
    Result<void> trigger_failover();
    Result<void> set_failover_callback(std::function<void(const std::string&)> callback);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    ReplicationConfig config_;
};

// ============================================================================
// Sharding Manager
// ============================================================================

class ShardingManager {
public:
    explicit ShardingManager(const ShardingConfig& config);
    ~ShardingManager();
    
    // Lifecycle
    Result<void> start();
    Result<void> stop();
    
    // Shard routing
    Result<std::string> get_shard_for_id(VectorId id) const;
    Result<std::string> get_shard_for_key(const std::string& key) const;
    Result<std::vector<std::string>> get_all_shards() const;
    
    // Shard management
    Result<void> add_shard(const ShardConfig& shard);
    Result<void> remove_shard(const std::string& shard_id);
    Result<void> rebalance_shards();
    
    // Statistics
    Result<size_t> get_shard_size(const std::string& shard_id) const;
    Result<float> get_shard_imbalance() const;
    Result<bool> needs_resharding() const;
    
    // Resharding
    Result<void> trigger_resharding();
    Result<void> set_resharding_callback(std::function<void(size_t, size_t)> callback);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    ShardingConfig config_;
    
    // Helper methods
    size_t hash_function(VectorId id) const;
    size_t consistent_hash(const std::string& key) const;
};

// ============================================================================
// Distributed Vector Database
// ============================================================================

class DistributedVectorDatabase {
public:
    DistributedVectorDatabase(
        const ReplicationConfig& replication_config,
        const ShardingConfig& sharding_config
    );
    ~DistributedVectorDatabase();
    
    // Initialization
    Result<void> init(Dim dimension, DistanceMetric metric);
    Result<void> close();
    
    // Vector operations (distributed)
    Result<VectorId> add(VectorView vector, const Metadata& metadata);
    Result<bool> remove(VectorId id);
    Result<std::optional<Vector>> get(VectorId id) const;
    Result<void> update_metadata(VectorId id, const Metadata& metadata);
    
    // Distributed search
    Result<std::vector<QueryResult>> search(
        VectorView query,
        size_t k,
        const std::function<bool(const Metadata&)>& filter = nullptr
    ) const;
    
    // Cluster management
    Result<void> add_node(const NodeConfig& node);
    Result<void> remove_node(const std::string& node_id);
    Result<std::vector<NodeConfig>> get_all_nodes() const;
    Result<bool> is_cluster_healthy() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    ReplicationConfig replication_config_;
    ShardingConfig sharding_config_;
    Dim dimension_ = 0;
    DistanceMetric metric_ = DistanceMetric::Cosine;
};

} // namespace vdb
