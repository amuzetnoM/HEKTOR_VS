// ============================================================================
// VectorDB Tests - FlatIndex
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/index.hpp"
#include <random>
#include <filesystem>
#include <thread>

namespace vdb::test {

class FlatIndexTest : public ::testing::Test {
protected:
    static constexpr Dim DIM = 128;
    static constexpr size_t NUM_VECTORS = 100;
    
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
    
    void TearDown() override {
        // Clean up test files
        namespace fs = std::filesystem;
        if (fs::exists(test_file_path_)) {
            fs::remove(test_file_path_);
        }
    }
    
    std::vector<Vector> vectors_;
    std::filesystem::path test_file_path_ = std::filesystem::temp_directory_path() / 
        ("flat_index_test_" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ".bin");
};

TEST_F(FlatIndexTest, Construction) {
    FlatIndex index(DIM, DistanceMetric::Cosine);
    EXPECT_EQ(index.dimension(), DIM);
    EXPECT_EQ(index.size(), 0);
}

TEST_F(FlatIndexTest, AddAndSearch) {
    FlatIndex index(DIM, DistanceMetric::Cosine);
    
    // Add vectors
    for (size_t i = 0; i < 50; ++i) {
        auto result = index.add(i + 1, vectors_[i]);
        EXPECT_TRUE(result.has_value());
    }
    
    EXPECT_EQ(index.size(), 50);
    
    // Search for exact match
    auto results = index.search(vectors_[0], 1);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 1);
    EXPECT_NEAR(results[0].distance, 0.0f, 1e-5f);
}

TEST_F(FlatIndexTest, SaveAndLoad) {
    // Create and populate index
    FlatIndex index1(DIM, DistanceMetric::Cosine);
    
    for (size_t i = 0; i < 50; ++i) {
        ASSERT_TRUE(index1.add(i + 1, vectors_[i]).has_value());
    }
    
    // Save to file
    auto save_result = index1.save(test_file_path_.string());
    EXPECT_TRUE(save_result.has_value());
    
    // Load from file
    auto load_result = FlatIndex::load(test_file_path_.string());
    ASSERT_TRUE(load_result.has_value());
    
    FlatIndex& index2 = *load_result;
    
    // Verify dimensions and size
    EXPECT_EQ(index2.dimension(), DIM);
    EXPECT_EQ(index2.size(), 50);
    
    // Verify all vectors are present and searchable
    for (size_t i = 0; i < 50; ++i) {
        EXPECT_TRUE(index2.contains(i + 1));
    }
    
    // Verify search results match
    auto results1 = index1.search(vectors_[10], 5);
    auto results2 = index2.search(vectors_[10], 5);
    
    EXPECT_EQ(results1.size(), results2.size());
    for (size_t i = 0; i < results1.size(); ++i) {
        EXPECT_EQ(results1[i].id, results2[i].id);
        EXPECT_NEAR(results1[i].distance, results2[i].distance, 1e-5f);
    }
}

TEST_F(FlatIndexTest, LoadNonExistentFile) {
    auto result = FlatIndex::load("non_existent_file.bin");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IoError);
}

TEST_F(FlatIndexTest, GetVector) {
    FlatIndex index(DIM, DistanceMetric::Cosine);
    
    ASSERT_TRUE(index.add(1, vectors_[0]).has_value());
    
    auto vec = index.get_vector(1);
    ASSERT_TRUE(vec.has_value());
    EXPECT_EQ(vec->dim(), DIM);
    
    // Verify vector contents match
    for (Dim d = 0; d < DIM; ++d) {
        EXPECT_NEAR((*vec)[d], vectors_[0][d], 1e-5f);
    }
    
    // Non-existent vector
    auto non_existent = index.get_vector(999);
    EXPECT_FALSE(non_existent.has_value());
}

} // namespace vdb::test
