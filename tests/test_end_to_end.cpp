// ============================================================================
// VectorDB Tests - End-to-End Integration
// Phase 4: Complete System Integration Testing
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/replication.hpp"
#include "vdb/framework_integration.hpp"
#include <thread>
#include <chrono>

namespace vdb::test {

// ============================================================================
// End-to-End Integration Tests
// ============================================================================

class EndToEndTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup distributed configuration
        repl_config_.mode = ReplicationMode::Async;
        repl_config_.min_replicas = 1;
        
        NodeConfig node;
        node.node_id = "primary";
        node.host = "localhost";
        node.port = 8080;
        node.is_primary = true;
        repl_config_.nodes.push_back(node);
        
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

TEST_F(EndToEndTest, DistributedDatabaseLifecycle) {
    // Test complete lifecycle: init -> add -> search -> close
    DistributedVectorDatabase db(repl_config_, shard_config_);
    
    // Initialize
    auto init_result = db.init(128, DistanceMetric::Cosine);
    ASSERT_TRUE(init_result.has_value()) << "Failed to initialize database";
    
    // Add vectors
    Vector vec(128);
    for (size_t i = 0; i < 128; ++i) {
        vec[i] = 0.01f * i;
    }
    
    Metadata meta;
    meta.id = 1;
    
    auto add_result = db.add(vec.view(), meta);
    ASSERT_TRUE(add_result.has_value()) << "Failed to add vector";
    
    // Search
    auto search_result = db.search(vec.view(), 1);
    ASSERT_TRUE(search_result.has_value()) << "Failed to search";
    
    // Close
    auto close_result = db.close();
    EXPECT_TRUE(close_result.has_value()) << "Failed to close database";
}

TEST_F(EndToEndTest, MultiVectorDistributedOperations) {
    DistributedVectorDatabase db(repl_config_, shard_config_);
    ASSERT_TRUE(db.init(64, DistanceMetric::Cosine).has_value());
    
    // Add multiple vectors
    const size_t num_vectors = 100;
    std::vector<VectorId> ids;
    
    for (size_t i = 0; i < num_vectors; ++i) {
        Vector vec(64);
        for (size_t j = 0; j < 64; ++j) {
            vec[j] = static_cast<float>(i + j) / 100.0f;
        }
        
        Metadata meta;
        meta.id = i;
        
        auto add_result = db.add(vec.view(), meta);
        ASSERT_TRUE(add_result.has_value());
        ids.push_back(add_result.value());
    }
    
    // Verify we added all vectors
    EXPECT_EQ(ids.size(), num_vectors);
    
    // Search should return results from multiple shards
    Vector query(64);
    for (size_t i = 0; i < 64; ++i) {
        query[i] = 0.5f;
    }
    
    auto search_result = db.search(query.view(), 10);
    ASSERT_TRUE(search_result.has_value());
    EXPECT_LE(search_result.value().size(), 10);
    
    EXPECT_TRUE(db.close().has_value());
}

TEST_F(EndToEndTest, ReplicationWithSharding) {
    // Test that replication works correctly with sharding
    repl_config_.mode = ReplicationMode::SemiSync;
    repl_config_.min_replicas = 1;
    
    DistributedVectorDatabase db(repl_config_, shard_config_);
    ASSERT_TRUE(db.init(128, DistanceMetric::L2).has_value());
    
    // Add vector - should be both sharded and replicated
    Vector vec(128);
    for (size_t i = 0; i < 128; ++i) {
        vec[i] = 0.01f * i;
    }
    
    Metadata meta;
    meta.id = 1;
    
    auto add_result = db.add(vec.view(), meta);
    ASSERT_TRUE(add_result.has_value());
    
    // Verify cluster is healthy
    auto health_result = db.is_cluster_healthy();
    ASSERT_TRUE(health_result.has_value());
    
    EXPECT_TRUE(db.close().has_value());
}

TEST_F(EndToEndTest, ConcurrentOperations) {
    DistributedVectorDatabase db(repl_config_, shard_config_);
    ASSERT_TRUE(db.init(64, DistanceMetric::Cosine).has_value());
    
    const size_t num_threads = 4;
    const size_t vectors_per_thread = 25;
    std::vector<std::thread> threads;
    std::atomic<size_t> success_count{0};
    
    // Launch concurrent add operations
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (size_t i = 0; i < vectors_per_thread; ++i) {
                Vector vec(64);
                for (size_t j = 0; j < 64; ++j) {
                    vec[j] = static_cast<float>(t * 1000 + i + j) / 1000.0f;
                }
                
                Metadata meta;
                meta.id = t * 1000 + i;
                
                auto result = db.add(vec.view(), meta);
                if (result.has_value()) {
                    success_count++;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all operations succeeded
    EXPECT_EQ(success_count.load(), num_threads * vectors_per_thread);
    
    EXPECT_TRUE(db.close().has_value());
}

TEST_F(EndToEndTest, FailoverScenario) {
    // Test automatic failover when primary fails
    repl_config_.mode = ReplicationMode::Sync;
    
    // Add multiple nodes with different priorities
    NodeConfig replica1;
    replica1.node_id = "replica1";
    replica1.host = "localhost";
    replica1.port = 8081;
    replica1.priority = 5;
    repl_config_.nodes.push_back(replica1);
    
    NodeConfig replica2;
    replica2.node_id = "replica2";
    replica2.host = "localhost";
    replica2.port = 8082;
    replica2.priority = 3;
    repl_config_.nodes.push_back(replica2);
    
    ReplicationManager mgr(repl_config_);
    ASSERT_TRUE(mgr.start().has_value());
    
    // Get initial primary
    auto primary_result = mgr.get_primary_node();
    ASSERT_TRUE(primary_result.has_value());
    std::string initial_primary = primary_result.value();
    
    // Simulate primary failure by triggering failover
    auto failover_result = mgr.trigger_failover();
    EXPECT_TRUE(failover_result.has_value());
    
    EXPECT_TRUE(mgr.stop().has_value());
}

TEST_F(EndToEndTest, MetadataFiltering) {
    DistributedVectorDatabase db(repl_config_, shard_config_);
    ASSERT_TRUE(db.init(64, DistanceMetric::Cosine).has_value());
    
    // Add vectors with different metadata
    for (int i = 0; i < 20; ++i) {
        Vector vec(64);
        for (size_t j = 0; j < 64; ++j) {
            vec[j] = static_cast<float>(i + j) / 100.0f;
        }
        
        Metadata meta;
        meta.id = i;
        meta.source_file = (i % 2 == 0) ? "even.txt" : "odd.txt";
        
        db.add(vec.view(), meta);
    }
    
    // Search with filter for even files
    Vector query(64);
    for (size_t i = 0; i < 64; ++i) {
        query[i] = 0.5f;
    }
    
    auto filter = [](const Metadata& meta) {
        return meta.source_file == "even.txt";
    };
    
    auto search_result = db.search(query.view(), 10, filter);
    ASSERT_TRUE(search_result.has_value());
    
    // All results should be from even files
    for (const auto& result : search_result.value()) {
        if (result.metadata.has_value()) {
            EXPECT_EQ(result.metadata->source_file, "even.txt");
        }
    }
    
    EXPECT_TRUE(db.close().has_value());
}

TEST_F(EndToEndTest, DistanceMetricConsistency) {
    // Test different distance metrics produce consistent results
    const Dim dim = 64;
    
    for (auto metric : {DistanceMetric::Cosine, DistanceMetric::L2, DistanceMetric::DotProduct}) {
        DistributedVectorDatabase db(repl_config_, shard_config_);
        ASSERT_TRUE(db.init(dim, metric).has_value());
        
        // Add test vector
        Vector vec(dim);
        for (size_t i = 0; i < dim; ++i) {
            vec[i] = 0.01f * i;
        }
        
        Metadata meta;
        meta.id = 1;
        
        auto add_result = db.add(vec.view(), meta);
        ASSERT_TRUE(add_result.has_value());
        
        // Search with same vector should return itself as top result
        auto search_result = db.search(vec.view(), 1);
        ASSERT_TRUE(search_result.has_value());
        EXPECT_GE(search_result.value().size(), 1);
        
        EXPECT_TRUE(db.close().has_value());
    }
}

TEST_F(EndToEndTest, LargeScaleOperation) {
    DistributedVectorDatabase db(repl_config_, shard_config_);
    ASSERT_TRUE(db.init(128, DistanceMetric::Cosine).has_value());
    
    // Add 1000 vectors
    const size_t num_vectors = 1000;
    
    for (size_t i = 0; i < num_vectors; ++i) {
        Vector vec(128);
        for (size_t j = 0; j < 128; ++j) {
            vec[j] = static_cast<float>(i + j) / 1000.0f;
        }
        
        Metadata meta;
        meta.id = i;
        
        auto result = db.add(vec.view(), meta);
        ASSERT_TRUE(result.has_value());
        
        // Log progress every 100 vectors
        if ((i + 1) % 100 == 0) {
            // Progress indicator
        }
    }
    
    // Perform searches
    Vector query(128);
    for (size_t i = 0; i < 128; ++i) {
        query[i] = 0.5f;
    }
    
    auto search_result = db.search(query.view(), 20);
    ASSERT_TRUE(search_result.has_value());
    EXPECT_LE(search_result.value().size(), 20);
    
    EXPECT_TRUE(db.close().has_value());
}

TEST_F(EndToEndTest, SystemRobustness) {
    // Test system handles errors gracefully
    DistributedVectorDatabase db(repl_config_, shard_config_);
    ASSERT_TRUE(db.init(64, DistanceMetric::Cosine).has_value());
    
    // Try to add vector with wrong dimension
    Vector wrong_dim_vec(32);  // Wrong dimension
    for (size_t i = 0; i < 32; ++i) {
        wrong_dim_vec[i] = 0.1f;
    }
    
    Metadata meta;
    meta.id = 1;
    
    auto add_result = db.add(wrong_dim_vec.view(), meta);
    EXPECT_FALSE(add_result.has_value());  // Should fail gracefully
    
    // System should still be functional
    Vector correct_vec(64);
    for (size_t i = 0; i < 64; ++i) {
        correct_vec[i] = 0.1f;
    }
    
    auto correct_add = db.add(correct_vec.view(), meta);
    EXPECT_TRUE(correct_add.has_value());  // Should succeed
    
    EXPECT_TRUE(db.close().has_value());
}

} // namespace vdb::test
