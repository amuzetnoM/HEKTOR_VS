// ============================================================================
// VectorDB Tests - Storage Layer
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/storage.hpp"
#include <filesystem>
#include <random>

namespace vdb::test {

namespace fs = std::filesystem;

class StorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "vdb_test";
        fs::create_directories(test_dir_);
    }
    
    void TearDown() override {
        fs::remove_all(test_dir_);
    }
    
    fs::path test_dir_;
};

// ============================================================================
// VectorStore Tests
// ============================================================================

TEST_F(StorageTest, VectorStoreInit) {
    VectorStoreConfig config;
    config.path = test_dir_;
    config.dimension = 512;
    config.initial_capacity = 100;
    
    VectorStore store(config);
    auto result = store.init();
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(store.size(), 0);
    EXPECT_GE(store.capacity(), 100);
}

TEST_F(StorageTest, VectorStoreAddAndGet) {
    VectorStoreConfig config;
    config.path = test_dir_;
    config.dimension = 4;
    config.initial_capacity = 100;
    
    VectorStore store(config);
    ASSERT_TRUE(store.init().has_value());
    
    std::vector<Scalar> data = {1.0f, 2.0f, 3.0f, 4.0f};
    VectorView view(data.data(), 4);
    
    auto add_result = store.add(1, view);
    EXPECT_TRUE(add_result.has_value());
    EXPECT_EQ(store.size(), 1);
    EXPECT_TRUE(store.contains(1));
    
    auto get_result = store.get(1);
    EXPECT_TRUE(get_result.has_value());
    EXPECT_EQ(get_result->dim(), 4);
}

TEST_F(StorageTest, VectorStoreRemove) {
    VectorStoreConfig config;
    config.path = test_dir_;
    config.dimension = 4;
    config.initial_capacity = 100;
    
    VectorStore store(config);
    ASSERT_TRUE(store.init().has_value());
    
    std::vector<Scalar> data = {1.0f, 2.0f, 3.0f, 4.0f};
    ASSERT_TRUE(store.add(1, VectorView(data.data(), 4)).has_value());
    ASSERT_TRUE(store.add(2, VectorView(data.data(), 4)).has_value());
    
    EXPECT_EQ(store.size(), 2);
    
    auto result = store.remove(1);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(store.size(), 1);
    EXPECT_FALSE(store.contains(1));
    EXPECT_TRUE(store.contains(2));
}

// ============================================================================
// MetadataStore Tests
// ============================================================================

TEST_F(StorageTest, MetadataStoreInit) {
    MetadataStore store(test_dir_ / "metadata.jsonl");
    auto result = store.init();
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(store.size(), 0);
}

TEST_F(StorageTest, MetadataStoreAddAndGet) {
    MetadataStore store(test_dir_ / "metadata.jsonl");
    ASSERT_TRUE(store.init().has_value());
    
    Metadata meta;
    meta.id = 1;
    meta.type = DocumentType::Journal;
    meta.date = "2025-12-01";
    meta.source_file = "Journal_2025-12-01.md";
    meta.bias = "BULLISH";
    meta.gold_price = 4220.50f;
    
    auto add_result = store.add(meta);
    EXPECT_TRUE(add_result.has_value());
    
    auto get_result = store.get(1);
    EXPECT_TRUE(get_result.has_value());
    EXPECT_EQ(get_result->type, DocumentType::Journal);
    EXPECT_EQ(get_result->date, "2025-12-01");
    EXPECT_FLOAT_EQ(get_result->gold_price.value(), 4220.50f);
}

TEST_F(StorageTest, MetadataStorePersistence) {
    fs::path meta_path = test_dir_ / "metadata.jsonl";
    
    // Write
    {
        MetadataStore store(meta_path);
        ASSERT_TRUE(store.init().has_value());
        
        Metadata meta;
        meta.id = 1;
        meta.type = DocumentType::Chart;
        meta.date = "2025-12-01";
        meta.asset = "GOLD";
        ASSERT_TRUE(store.add(meta).has_value());
        ASSERT_TRUE(store.sync().has_value());
    }
    
    // Read back
    {
        MetadataStore store(meta_path);
        ASSERT_TRUE(store.init().has_value());
        
        EXPECT_EQ(store.size(), 1);
        auto meta = store.get(1);
        EXPECT_TRUE(meta.has_value());
        EXPECT_EQ(meta->asset, "GOLD");
    }
}

} // namespace vdb::test
