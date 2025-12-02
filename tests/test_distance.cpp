// ============================================================================
// VectorDB Tests - Distance Functions
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/distance.hpp"
#include <cmath>
#include <random>

namespace vdb::test {

class DistanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create some test vectors
        a_ = Vector{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
        b_ = Vector{8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f};
        unit_ = Vector{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    Vector a_, b_, unit_;
};

TEST_F(DistanceTest, DotProductSymmetric) {
    float ab = dot_product(a_.data(), b_.data(), a_.dim());
    float ba = dot_product(b_.data(), a_.data(), b_.dim());
    EXPECT_FLOAT_EQ(ab, ba);
}

TEST_F(DistanceTest, L2DistanceSymmetric) {
    float ab = euclidean_distance(a_.data(), b_.data(), a_.dim());
    float ba = euclidean_distance(b_.data(), a_.data(), b_.dim());
    EXPECT_FLOAT_EQ(ab, ba);
}

TEST_F(DistanceTest, L2DistanceTriangleInequality) {
    Vector c{4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f};
    
    float ac = euclidean_distance(a_.data(), c.data(), a_.dim());
    float cb = euclidean_distance(c.data(), b_.data(), c.dim());
    float ab = euclidean_distance(a_.data(), b_.data(), a_.dim());
    
    EXPECT_LE(ab, ac + cb + 1e-5f);
}

TEST_F(DistanceTest, CosineSimRange) {
    float sim = cosine_similarity(a_.data(), b_.data(), a_.dim());
    EXPECT_GE(sim, -1.0f);
    EXPECT_LE(sim, 1.0f);
}

TEST_F(DistanceTest, LargeVectorPerformance) {
    const size_t dim = 512;
    std::vector<float> large_a(dim), large_b(dim);
    
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    for (size_t i = 0; i < dim; ++i) {
        large_a[i] = dist(gen);
        large_b[i] = dist(gen);
    }
    
    // Just verify it runs without crashing
    float dot = dot_product(large_a.data(), large_b.data(), dim);
    float l2 = euclidean_distance(large_a.data(), large_b.data(), dim);
    float cos = cosine_similarity(large_a.data(), large_b.data(), dim);
    
    EXPECT_TRUE(std::isfinite(dot));
    EXPECT_TRUE(std::isfinite(l2));
    EXPECT_TRUE(std::isfinite(cos));
}

} // namespace vdb::test
