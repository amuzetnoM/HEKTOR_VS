// ============================================================================
// VectorDB Tests - HNSW Index
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/index.hpp"
#include <random>

namespace vdb::test {

class HNSWTest : public ::testing::Test {
protected:
    static constexpr Dim DIM = 128;
    static constexpr size_t NUM_VECTORS = 1000;
    
    void SetUp() override {
        // Generate random vectors
        std::mt19937 gen(42);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (size_t i = 0; i < NUM_VECTORS; ++i) {
            Vector v(DIM);
            float norm = 0.0f;
            for (Dim d = 0; d < DIM; ++d) {
                v[d] = dist(gen);
                norm += v[d] * v[d];
            }
            norm = std::sqrt(norm);
            for (Dim d = 0; d < DIM; ++d) {
                v[d] /= norm;
            }
            vectors_.push_back(std::move(v));
        }
    }
    
    std::vector<Vector> vectors_;
};

TEST_F(HNSWTest, Construction) {
    HnswConfig config;
    config.dimension = DIM;
    config.max_elements = NUM_VECTORS;
    
    HnswIndex index(config);
    EXPECT_EQ(index.dimension(), DIM);
    EXPECT_EQ(index.size(), 0);
}

TEST_F(HNSWTest, AddSingleVector) {
    HnswConfig config;
    config.dimension = DIM;
    config.max_elements = 100;
    
    HnswIndex index(config);
    auto result = index.add(1, vectors_[0]);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(index.size(), 1);
    EXPECT_TRUE(index.contains(1));
}

TEST_F(HNSWTest, AddMultipleVectors) {
    HnswConfig config;
    config.dimension = DIM;
    config.max_elements = NUM_VECTORS;
    
    HnswIndex index(config);
    
    for (size_t i = 0; i < 100; ++i) {
        auto result = index.add(i + 1, vectors_[i]);
        EXPECT_TRUE(result.has_value());
    }
    
    EXPECT_EQ(index.size(), 100);
}

TEST_F(HNSWTest, SearchReturnsClosest) {
    HnswConfig config;
    config.dimension = DIM;
    config.max_elements = NUM_VECTORS;
    
    HnswIndex index(config);
    
    // Add first 100 vectors
    for (size_t i = 0; i < 100; ++i) {
        ASSERT_TRUE(index.add(i + 1, vectors_[i]).has_value());
    }
    
    // Search for the first vector
    auto results = index.search(vectors_[0], 1);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 1);  // Should find itself
    EXPECT_NEAR(results[0].distance, 0.0f, 1e-5f);
}

TEST_F(HNSWTest, SearchReturnsKResults) {
    HnswConfig config;
    config.dimension = DIM;
    config.max_elements = NUM_VECTORS;
    
    HnswIndex index(config);
    
    for (size_t i = 0; i < 100; ++i) {
        ASSERT_TRUE(index.add(i + 1, vectors_[i]).has_value());
    }
    
    auto results = index.search(vectors_[0], 10);
    
    EXPECT_EQ(results.size(), 10);
    
    // Results should be sorted by distance
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_LE(results[i-1].distance, results[i].distance);
    }
}

} // namespace vdb::test
