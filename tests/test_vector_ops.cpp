// ============================================================================
// VectorDB Tests - Vector Operations
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/core.hpp"
#include "vdb/distance.hpp"

namespace vdb::test {

// ============================================================================
// Vector Basics
// ============================================================================

TEST(VectorTest, DefaultConstruction) {
    Vector v;
    EXPECT_EQ(v.dim(), 0);
    EXPECT_TRUE(v.empty());
}

TEST(VectorTest, SizedConstruction) {
    Vector v(512);
    EXPECT_EQ(v.dim(), 512);
    EXPECT_FALSE(v.empty());
}

TEST(VectorTest, InitializerListConstruction) {
    Vector v{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_EQ(v.dim(), 4);
    EXPECT_FLOAT_EQ(v[0], 1.0f);
    EXPECT_FLOAT_EQ(v[3], 4.0f);
}

TEST(VectorTest, VectorViewConversion) {
    Vector v{1.0f, 2.0f, 3.0f};
    VectorView view = v;
    
    EXPECT_EQ(view.dim(), 3);
    EXPECT_FLOAT_EQ(view[0], 1.0f);
}

// ============================================================================
// Distance Operations (Basic tests without fixture)
// ============================================================================

TEST(DistanceBasicTest, DotProduct) {
    std::vector<float> a = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> b = {1.0f, 0.0f, 0.0f, 0.0f};
    
    float result = dot_product(a.data(), b.data(), 4);
    EXPECT_FLOAT_EQ(result, 1.0f);
}

TEST(DistanceBasicTest, DotProductOrthogonal) {
    std::vector<float> a = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> b = {0.0f, 1.0f, 0.0f, 0.0f};
    
    float result = dot_product(a.data(), b.data(), 4);
    EXPECT_FLOAT_EQ(result, 0.0f);
}

TEST(DistanceBasicTest, EuclideanDistance) {
    std::vector<float> a = {0.0f, 0.0f, 0.0f};
    std::vector<float> b = {3.0f, 4.0f, 0.0f};
    
    float result = euclidean_distance(a.data(), b.data(), 3);
    EXPECT_FLOAT_EQ(result, 5.0f);  // 3-4-5 triangle
}

TEST(DistanceBasicTest, CosineSimilaritySame) {
    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    std::vector<float> b = {1.0f, 2.0f, 3.0f};
    
    float result = cosine_similarity(a.data(), b.data(), 3);
    EXPECT_NEAR(result, 1.0f, 1e-6f);
}

TEST(DistanceBasicTest, CosineSimilarityOpposite) {
    std::vector<float> a = {1.0f, 0.0f, 0.0f};
    std::vector<float> b = {-1.0f, 0.0f, 0.0f};
    
    float result = cosine_similarity(a.data(), b.data(), 3);
    EXPECT_NEAR(result, -1.0f, 1e-6f);
}

} // namespace vdb::test
