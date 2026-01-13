// ============================================================================
// VectorDB Tests - HNSW Index
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/index.hpp"
#include <random>
#include <fstream>
#include <filesystem>

namespace vdb::test
{

    class HNSWTest : public ::testing::Test
    {
    protected:
        static constexpr Dim DIM = 128;
        static constexpr size_t NUM_VECTORS = 1000;

        void SetUp() override
        {
            // Generate random vectors
            std::mt19937 gen(42);
            std::normal_distribution<float> dist(0.0f, 1.0f);

            for (size_t i = 0; i < NUM_VECTORS; ++i)
            {
                Vector v(DIM);
                float norm = 0.0f;
                for (Dim d = 0; d < DIM; ++d)
                {
                    v[d] = dist(gen);
                    norm += v[d] * v[d];
                }
                norm = std::sqrt(norm);
                for (Dim d = 0; d < DIM; ++d)
                {
                    v[d] /= norm;
                }
                vectors_.push_back(std::move(v));
            }
        }

        std::vector<Vector> vectors_;
    };

    TEST_F(HNSWTest, Construction)
    {
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = NUM_VECTORS;

        HnswIndex index(config);
        EXPECT_EQ(index.dimension(), DIM);
        EXPECT_EQ(index.size(), 0);
    }

    TEST_F(HNSWTest, AddSingleVector)
    {
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = 100;

        HnswIndex index(config);
        auto result = index.add(1, vectors_[0]);

        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(index.size(), 1);
        EXPECT_TRUE(index.contains(1));
    }

    TEST_F(HNSWTest, AddMultipleVectors)
    {
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = NUM_VECTORS;

        HnswIndex index(config);

        for (size_t i = 0; i < 100; ++i)
        {
            auto result = index.add(i + 1, vectors_[i]);
            EXPECT_TRUE(result.has_value());
        }

        EXPECT_EQ(index.size(), 100);
    }

    TEST_F(HNSWTest, SearchReturnsClosest)
    {
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = NUM_VECTORS;

        HnswIndex index(config);

        // Add first 100 vectors
        for (size_t i = 0; i < 100; ++i)
        {
            index.add(i + 1, vectors_[i]);
        }

        // Search for the first vector
        auto results = index.search(vectors_[0], 1);

        EXPECT_EQ(results.size(), 1);
        EXPECT_EQ(results[0].id, 1); // Should find itself
        EXPECT_NEAR(results[0].distance, 0.0f, 1e-5f);
    }

    TEST_F(HNSWTest, SearchReturnsKResults)
    {
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = NUM_VECTORS;

        HnswIndex index(config);

        for (size_t i = 0; i < 100; ++i)
        {
            index.add(i + 1, vectors_[i]);
        }

        auto results = index.search(vectors_[0], 10);

        EXPECT_EQ(results.size(), 10);

        // Results should be sorted by distance
        for (size_t i = 1; i < results.size(); ++i)
        {
            EXPECT_LE(results[i - 1].distance, results[i].distance);
        }
    }

    TEST_F(HNSWTest, RemoveVector)
    {
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = 100;

        HnswIndex index(config);
        index.add(1, vectors_[0]);
        index.add(2, vectors_[1]);

        EXPECT_EQ(index.size(), 2);
        EXPECT_TRUE(index.contains(1));

        auto result = index.remove(1);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(index.size(), 1);
        EXPECT_FALSE(index.contains(1));
        EXPECT_TRUE(index.contains(2));
    }

    TEST_F(HNSWTest, RemoveNonExistentVector)
    {
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = 100;

        HnswIndex index(config);
        index.add(1, vectors_[0]);

        auto result = index.remove(999);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code, ErrorCode::VectorNotFound);
    }

    TEST_F(HNSWTest, ResizeIndex)
    {
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = 10;

        HnswIndex index(config);

        for (size_t i = 0; i < 10; ++i)
        {
            index.add(i + 1, vectors_[i]);
        }

        EXPECT_EQ(index.capacity(), 10);

        // Resize to larger capacity
        auto result = index.resize(100);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(index.capacity(), 100);

        // Now we can add more vectors
        for (size_t i = 10; i < 20; ++i)
        {
            auto add_result = index.add(i + 1, vectors_[i]);
            EXPECT_TRUE(add_result.has_value());
        }

        EXPECT_EQ(index.size(), 20);
    }

    TEST_F(HNSWTest, ResizeToSmallerCapacityFails)
    {
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = 100;

        HnswIndex index(config);

        for (size_t i = 0; i < 50; ++i)
        {
            index.add(i + 1, vectors_[i]);
        }

        // Try to resize smaller than current element count
        auto result = index.resize(25);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code, ErrorCode::InvalidInput);
    }

    TEST_F(HNSWTest, SaveLoadAndAddVectors)
    {
        // Test for the segfault bug: adding vectors to a loaded index should work
        HnswConfig config;
        config.dimension = DIM;
        config.max_elements = 100;
        config.ef_construction = 100;
        config.ef_search = 50;
        config.M = 8;

        // Create and populate index
        HnswIndex index(config);
        for (size_t i = 0; i < 50; ++i)
        {
            auto result = index.add(i + 1, vectors_[i]);
            EXPECT_TRUE(result.has_value());
        }

        // Save to file
        auto temp_path = std::filesystem::temp_directory_path() / "test_hnsw_save_load.bin";
        auto save_result = index.save(temp_path.string());
        EXPECT_TRUE(save_result.has_value());

        // Load from file
        auto load_result = HnswIndex::load(temp_path.string());
        EXPECT_TRUE(load_result.has_value());
        
        HnswIndex loaded_index = std::move(*load_result);
        
        // Verify loaded index has correct state
        EXPECT_EQ(loaded_index.size(), 50);
        EXPECT_EQ(loaded_index.dimension(), DIM);
        
        // Verify config fields are preserved
        EXPECT_EQ(loaded_index.config().dimension, config.dimension);
        EXPECT_EQ(loaded_index.config().M, config.M);
        EXPECT_EQ(loaded_index.config().max_elements, config.max_elements);
        EXPECT_EQ(loaded_index.config().ef_construction, config.ef_construction);
        EXPECT_EQ(loaded_index.config().ef_search, config.ef_search);

        // Add more vectors to loaded index (this is where the segfault occurs)
        for (size_t i = 50; i < 75; ++i)
        {
            auto result = loaded_index.add(i + 1, vectors_[i]);
            EXPECT_TRUE(result.has_value()) << "Failed to add vector " << i + 1 << " to loaded index";
        }

        EXPECT_EQ(loaded_index.size(), 75);

        // Verify search still works
        auto results = loaded_index.search(vectors_[0], 5);
        EXPECT_GT(results.size(), 0);
        EXPECT_EQ(results[0].id, 1);

        // Clean up
        std::filesystem::remove(temp_path);
    }

    TEST_F(HNSWTest, BackwardCompatibilityVersion1)
    {
        // Test backward compatibility: loading a version 1 file should work
        // Create a version 1 file manually
        auto temp_path = std::filesystem::temp_directory_path() / "test_hnsw_v1.bin";
        std::ofstream file(temp_path, std::ios::binary);
        
        // Write version 1 header
        // Magic number from src/index/hnsw.cpp
        constexpr uint32_t HNSW_MAGIC = 0x564442;  // "VDB"
        constexpr uint32_t HNSW_VERSION_1 = 1;
        
        file.write(reinterpret_cast<const char*>(&HNSW_MAGIC), sizeof(HNSW_MAGIC));
        file.write(reinterpret_cast<const char*>(&HNSW_VERSION_1), sizeof(HNSW_VERSION_1));
        
        // Write minimal config (version 1 format)
        Dim dimension = DIM;
        size_t M = 16;
        DistanceMetric metric = DistanceMetric::Cosine;
        file.write(reinterpret_cast<const char*>(&dimension), sizeof(dimension));
        file.write(reinterpret_cast<const char*>(&M), sizeof(M));
        file.write(reinterpret_cast<const char*>(&metric), sizeof(metric));
        
        // Write state
        size_t element_count = 2;
        int max_level = 0;
        VectorId entry_point = 1;
        file.write(reinterpret_cast<const char*>(&element_count), sizeof(element_count));
        file.write(reinterpret_cast<const char*>(&max_level), sizeof(max_level));
        file.write(reinterpret_cast<const char*>(&entry_point), sizeof(entry_point));
        
        // Write nodes
        uint64_t node_count = 2;
        file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
        
        for (uint64_t i = 0; i < node_count; ++i)
        {
            VectorId id = i + 1;
            int level = 0;
            file.write(reinterpret_cast<const char*>(&id), sizeof(id));
            file.write(reinterpret_cast<const char*>(&level), sizeof(level));
            
            // Write vector
            file.write(reinterpret_cast<const char*>(vectors_[i].data()),
                       vectors_[i].dim() * sizeof(Scalar));
            
            // Write connections (empty for level 0)
            uint32_t conn_count = 0;
            file.write(reinterpret_cast<const char*>(&conn_count), sizeof(conn_count));
        }
        
        file.close();
        
        // Load the version 1 file
        auto load_result = HnswIndex::load(temp_path.string());
        EXPECT_TRUE(load_result.has_value());
        
        HnswIndex loaded_index = std::move(*load_result);
        
        // Verify loaded index has correct basic state
        EXPECT_EQ(loaded_index.size(), 2);
        EXPECT_EQ(loaded_index.dimension(), DIM);
        
        // Config fields should have default values for version 1 files
        EXPECT_EQ(loaded_index.config().dimension, dimension);
        EXPECT_EQ(loaded_index.config().M, M);
        EXPECT_EQ(loaded_index.config().max_elements, HNSW_MAX_ELEMENTS); // default
        EXPECT_EQ(loaded_index.config().ef_construction, HNSW_EF_CONSTRUCTION); // default
        EXPECT_EQ(loaded_index.config().ef_search, HNSW_EF_SEARCH); // default
        
        // Should be able to add more vectors without segfault
        for (size_t i = 2; i < 10; ++i)
        {
            auto result = loaded_index.add(i + 1, vectors_[i]);
            EXPECT_TRUE(result.has_value()) << "Failed to add vector " << i + 1 << " to v1 loaded index";
        }
        
        EXPECT_EQ(loaded_index.size(), 10);
        
        // Clean up
        std::filesystem::remove(temp_path);
    }

} // namespace vdb::test
