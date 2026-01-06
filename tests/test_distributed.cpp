// ============================================================================
// VectorDB Tests - Distributed System (Replication & Sharding)
// Phase 4: Comprehensive Testing
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/replication.hpp"
#include <thread>
#include <chrono>

namespace vdb::test {

// ============================================================================
// Replication Tests
// ============================================================================

class ReplicationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test configuration with 3 nodes
        config_.mode = ReplicationMode::Async;
        config_.min_replicas = 2;
        config_.heartbeat_interval_ms = 100;
        
        // Primary node
        NodeConfig primary;
        primary.node_id = "node1";
        primary.host = "localhost";
        primary.port = 8081;
        primary.is_primary = true;
        primary.priority = 10;
        config_.nodes.push_back(primary);
        
        // Replica 1
        NodeConfig replica1;
        replica1.node_id = "node2";
        replica1.host = "localhost";
        replica1.port = 8082;
        replica1.priority = 5;
        config_.nodes.push_back(replica1);
        
        // Replica 2
        NodeConfig replica2;
        replica2.node_id = "node3";
        replica2.host = "localhost";
        replica2.port = 8083;
        replica2.priority = 3;
        config_.nodes.push_back(replica2);
    }
    
    ReplicationConfig config_;
};

TEST_F(ReplicationTest, ManagerStartStop) {
    ReplicationManager mgr(config_);
    
    auto start_result = mgr.start();
    ASSERT_TRUE(start_result.has_value());
    
    // Give threads time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    auto stop_result = mgr.stop();
    ASSERT_TRUE(stop_result.has_value());
}

TEST_F(ReplicationTest, AsyncReplication) {
    config_.mode = ReplicationMode::Async;
    ReplicationManager mgr(config_);
    
    ASSERT_TRUE(mgr.start().has_value());
    
    // Create test vector and metadata
    Vector vec{1.0f, 2.0f, 3.0f, 4.0f};
    Metadata meta;
    meta.id = 1;
    
    // Replicate add operation
    auto result = mgr.replicate_add(1, vec.view(), meta);
    EXPECT_TRUE(result.has_value());
    
    // Async should return immediately
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ReplicationTest, SyncReplication) {
    config_.mode = ReplicationMode::Sync;
    config_.sync_timeout_ms = 1000;
    ReplicationManager mgr(config_);
    
    ASSERT_TRUE(mgr.start().has_value());
    
    Vector vec{1.0f, 2.0f, 3.0f, 4.0f};
    Metadata meta;
    meta.id = 1;
    
    auto result = mgr.replicate_add(1, vec.view(), meta);
    EXPECT_TRUE(result.has_value());
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ReplicationTest, SemiSyncReplication) {
    config_.mode = ReplicationMode::SemiSync;
    config_.min_replicas = 2;  // Primary + 1 replica
    ReplicationManager mgr(config_);
    
    ASSERT_TRUE(mgr.start().has_value());
    
    Vector vec{1.0f, 2.0f, 3.0f, 4.0f};
    Metadata meta;
    meta.id = 1;
    
    auto result = mgr.replicate_add(1, vec.view(), meta);
    EXPECT_TRUE(result.has_value());
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ReplicationTest, PrimaryNodeDetection) {
    ReplicationManager mgr(config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    auto primary_result = mgr.get_primary_node();
    ASSERT_TRUE(primary_result.has_value());
    EXPECT_EQ(primary_result.value(), "node1");
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ReplicationTest, HealthMonitoring) {
    ReplicationManager mgr(config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    // Initially should be healthy
    auto health_result = mgr.is_healthy();
    ASSERT_TRUE(health_result.has_value());
    EXPECT_TRUE(health_result.value());
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ReplicationTest, AddRemoveReplica) {
    ReplicationManager mgr(config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    // Add new replica
    NodeConfig new_replica;
    new_replica.node_id = "node4";
    new_replica.host = "localhost";
    new_replica.port = 8084;
    new_replica.priority = 1;
    
    auto add_result = mgr.add_replica(new_replica);
    EXPECT_TRUE(add_result.has_value());
    
    // Remove replica
    auto remove_result = mgr.remove_replica("node4");
    EXPECT_TRUE(remove_result.has_value());
    
    EXPECT_TRUE(mgr.stop().has_value());
}

// ============================================================================
// Sharding Tests
// ============================================================================

class ShardingTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.strategy = ShardingStrategy::Hash;
        config_.num_shards = 4;
        
        // Create 4 shards
        for (size_t i = 0; i < 4; ++i) {
            ShardConfig shard;
            shard.shard_id = "shard" + std::to_string(i);
            shard.start_range = i * 1000000;
            shard.end_range = (i + 1) * 1000000;
            config_.shards.push_back(shard);
        }
    }
    
    ShardingConfig config_;
};

TEST_F(ShardingTest, ManagerStartStop) {
    ShardingManager mgr(config_);
    
    auto start_result = mgr.start();
    ASSERT_TRUE(start_result.has_value());
    
    auto stop_result = mgr.stop();
    ASSERT_TRUE(stop_result.has_value());
}

TEST_F(ShardingTest, HashSharding) {
    config_.strategy = ShardingStrategy::Hash;
    ShardingManager mgr(config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    // Test shard assignment for different IDs
    auto shard1 = mgr.get_shard_for_id(1);
    auto shard2 = mgr.get_shard_for_id(2);
    auto shard100 = mgr.get_shard_for_id(100);
    
    ASSERT_TRUE(shard1.has_value());
    ASSERT_TRUE(shard2.has_value());
    ASSERT_TRUE(shard100.has_value());
    
    // Same ID should always map to same shard
    auto shard1_again = mgr.get_shard_for_id(1);
    ASSERT_TRUE(shard1_again.has_value());
    EXPECT_EQ(shard1.value(), shard1_again.value());
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ShardingTest, RangeSharding) {
    config_.strategy = ShardingStrategy::Range;
    ShardingManager mgr(config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    // IDs should map to shards based on range
    auto shard0 = mgr.get_shard_for_id(500000);    // First shard
    auto shard1 = mgr.get_shard_for_id(1500000);   // Second shard
    auto shard2 = mgr.get_shard_for_id(2500000);   // Third shard
    
    ASSERT_TRUE(shard0.has_value());
    ASSERT_TRUE(shard1.has_value());
    ASSERT_TRUE(shard2.has_value());
    
    EXPECT_EQ(shard0.value(), "shard0");
    EXPECT_EQ(shard1.value(), "shard1");
    EXPECT_EQ(shard2.value(), "shard2");
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ShardingTest, ConsistentHashing) {
    config_.strategy = ShardingStrategy::Consistent;
    ShardingManager mgr(config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    // Test key-based sharding
    auto shard_key1 = mgr.get_shard_for_key("user123");
    auto shard_key2 = mgr.get_shard_for_key("user456");
    
    ASSERT_TRUE(shard_key1.has_value());
    ASSERT_TRUE(shard_key2.has_value());
    
    // Same key should always map to same shard
    auto shard_key1_again = mgr.get_shard_for_key("user123");
    ASSERT_TRUE(shard_key1_again.has_value());
    EXPECT_EQ(shard_key1.value(), shard_key1_again.value());
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ShardingTest, GetAllShards) {
    ShardingManager mgr(config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    auto shards_result = mgr.get_all_shards();
    ASSERT_TRUE(shards_result.has_value());
    
    const auto& shards = shards_result.value();
    EXPECT_EQ(shards.size(), 4);
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ShardingTest, AddRemoveShard) {
    ShardingManager mgr(config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    // Add new shard
    ShardConfig new_shard;
    new_shard.shard_id = "shard4";
    new_shard.start_range = 4000000;
    new_shard.end_range = 5000000;
    
    auto add_result = mgr.add_shard(new_shard);
    EXPECT_TRUE(add_result.has_value());
    
    auto all_shards = mgr.get_all_shards();
    ASSERT_TRUE(all_shards.has_value());
    EXPECT_EQ(all_shards.value().size(), 5);
    
    // Remove shard
    auto remove_result = mgr.remove_shard("shard4");
    EXPECT_TRUE(remove_result.has_value());
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(ShardingTest, ImbalanceDetection) {
    ShardingManager mgr(config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    auto imbalance_result = mgr.get_shard_imbalance();
    ASSERT_TRUE(imbalance_result.has_value());
    
    // Initially should be balanced (0.0)
    EXPECT_GE(imbalance_result.value(), 0.0f);
    
    EXPECT_TRUE(mgr.stop().has_value());
}

// ============================================================================
// Distributed Database Tests
// ============================================================================

class DistributedDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup replication config
        repl_config_.mode = ReplicationMode::Async;
        repl_config_.min_replicas = 2;
        
        NodeConfig node1;
        node1.node_id = "node1";
        node1.host = "localhost";
        node1.port = 8081;
        node1.is_primary = true;
        repl_config_.nodes.push_back(node1);
        
        // Setup sharding config
        shard_config_.strategy = ShardingStrategy::Hash;
        shard_config_.num_shards = 2;
        
        for (size_t i = 0; i < 2; ++i) {
            ShardConfig shard;
            shard.shard_id = "shard" + std::to_string(i);
            shard_config_.shards.push_back(shard);
        }
    }
    
    ReplicationConfig repl_config_;
    ShardingConfig shard_config_;
};

TEST_F(DistributedDatabaseTest, InitAndClose) {
    DistributedVectorDatabase db(repl_config_, shard_config_);
    
    auto init_result = db.init(128, DistanceMetric::Cosine);
    ASSERT_TRUE(init_result.has_value());
    
    auto close_result = db.close();
    EXPECT_TRUE(close_result.has_value());
}

TEST_F(DistributedDatabaseTest, AddVector) {
    DistributedVectorDatabase db(repl_config_, shard_config_);
    ASSERT_TRUE(db.init(128, DistanceMetric::Cosine).has_value());
    
    Vector vec(128);
    for (size_t i = 0; i < 128; ++i) {
        vec[i] = static_cast<float>(i) * 0.01f;
    }
    
    Metadata meta;
    meta.id = 1;
    
    auto add_result = db.add(vec.view(), meta);
    ASSERT_TRUE(add_result.has_value());
    EXPECT_GT(add_result.value(), 0);
    
    EXPECT_TRUE(db.close().has_value());
}

TEST_F(DistributedDatabaseTest, SearchDistributed) {
    DistributedVectorDatabase db(repl_config_, shard_config_);
    ASSERT_TRUE(db.init(128, DistanceMetric::Cosine).has_value());
    
    // Add some vectors
    for (int i = 0; i < 10; ++i) {
        Vector vec(128);
        for (size_t j = 0; j < 128; ++j) {
            vec[j] = static_cast<float>(i + j) * 0.01f;
        }
        Metadata meta;
        meta.id = i;
        db.add(vec.view(), meta);
    }
    
    // Search
    Vector query(128);
    for (size_t i = 0; i < 128; ++i) {
        query[i] = static_cast<float>(i) * 0.01f;
    }
    
    auto search_result = db.search(query.view(), 5);
    ASSERT_TRUE(search_result.has_value());
    
    const auto& results = search_result.value();
    EXPECT_LE(results.size(), 5);
    
    EXPECT_TRUE(db.close().has_value());
}

TEST_F(DistributedDatabaseTest, ClusterHealth) {
    DistributedVectorDatabase db(repl_config_, shard_config_);
    ASSERT_TRUE(db.init(128, DistanceMetric::Cosine).has_value());
    
    auto health_result = db.is_cluster_healthy();
    ASSERT_TRUE(health_result.has_value());
    
    EXPECT_TRUE(db.close().has_value());
}

} // namespace vdb::test
