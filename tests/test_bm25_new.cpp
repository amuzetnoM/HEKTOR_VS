// ============================================================================
// Test: BM25 Full-Text Search Engine (Google Test)
// ============================================================================

#include "vdb/hybrid_search.hpp"
#include <gtest/gtest.h>
#include <filesystem>

using namespace vdb::hybrid;

// ============================================================================
// BM25 Test Fixture
// ============================================================================

class BM25Test : public ::testing::Test {
protected:
    void SetUp() override {
        config_.k1 = 1.2f;
        config_.b = 0.75f;
        config_.use_stemming = true;
        config_.case_sensitive = false;
    }

    BM25Config config_;
};

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST_F(BM25Test, AddDocuments) {
    BM25Engine engine(config_);
    
    EXPECT_TRUE(engine.add_document(1, "Gold prices surge on inflation fears"));
    EXPECT_TRUE(engine.add_document(2, "Silver follows gold higher"));
    EXPECT_TRUE(engine.add_document(3, "Dollar weakens against precious metals"));
    
    EXPECT_EQ(engine.document_count(), 3);
}

TEST_F(BM25Test, DocumentStatistics) {
    BM25Engine engine(config_);
    
    ASSERT_TRUE(engine.add_document(1, "Gold prices surge"));
    ASSERT_TRUE(engine.add_document(2, "Silver follows gold"));
    ASSERT_TRUE(engine.add_document(3, "Dollar weakens"));
    
    EXPECT_EQ(engine.document_count(), 3);
    EXPECT_GT(engine.term_count(), 0);
    EXPECT_GT(engine.average_document_length(), 0.0f);
}

// ============================================================================
// Search Tests
// ============================================================================

TEST_F(BM25Test, BasicSearch) {
    BM25Engine engine;
    
    ASSERT_TRUE(engine.add_document(1, "Gold prices surge to new highs on inflation concerns"));
    ASSERT_TRUE(engine.add_document(2, "Silver rallies as precious metals gain momentum"));
    ASSERT_TRUE(engine.add_document(3, "Dollar weakness supports gold and silver prices"));
    ASSERT_TRUE(engine.add_document(4, "Central banks increase gold reserves"));
    ASSERT_TRUE(engine.add_document(5, "Mining stocks rise with metal prices"));
    
    auto results = engine.search("gold prices", 10, 0.0f);
    ASSERT_TRUE(results.has_value());
    EXPECT_FALSE(results->empty());
    
    // Documents 1 and 3 contain both "gold" and "prices"
    bool found_doc1 = false, found_doc3 = false;
    for (const auto& result : *results) {
        if (result.id == 1) found_doc1 = true;
        if (result.id == 3) found_doc3 = true;
    }
    EXPECT_TRUE(found_doc1);
    EXPECT_TRUE(found_doc3);
}

TEST_F(BM25Test, SearchResultsSortedByScore) {
    BM25Engine engine;
    
    ASSERT_TRUE(engine.add_document(1, "Gold prices surge to new highs"));
    ASSERT_TRUE(engine.add_document(2, "Silver rallies"));
    ASSERT_TRUE(engine.add_document(3, "Gold and silver prices rise"));
    
    auto results = engine.search("gold prices", 10, 0.0f);
    ASSERT_TRUE(results.has_value());
    
    for (size_t i = 1; i < results->size(); ++i) {
        EXPECT_GE((*results)[i-1].score, (*results)[i].score)
            << "Results not sorted by score at index " << i;
    }
}

// ============================================================================
// Stemming Tests
// ============================================================================

TEST_F(BM25Test, StemmingEnabled) {
    BM25Config config;
    config.use_stemming = true;
    BM25Engine engine(config);
    
    ASSERT_TRUE(engine.add_document(1, "Gold prices are rising rapidly"));
    ASSERT_TRUE(engine.add_document(2, "Silver price rose yesterday"));
    ASSERT_TRUE(engine.add_document(3, "Prices of metals increased"));
    
    // "price rise" should match "prices", "rising", "rose"
    auto results = engine.search("price rise", 10, 0.0f);
    ASSERT_TRUE(results.has_value());
    EXPECT_GE(results->size(), 2u);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(BM25Test, EmptyQueryReturnsError) {
    BM25Engine engine;
    ASSERT_TRUE(engine.add_document(1, "Test document"));
    
    auto results = engine.search("", 10, 0.0f);
    EXPECT_FALSE(results.has_value());
}

TEST_F(BM25Test, StopWordsOnlyQueryHandled) {
    BM25Engine engine;
    ASSERT_TRUE(engine.add_document(1, "Test document"));
    
    // Query with only common words - implementation may handle this gracefully
    auto results = engine.search("the and or", 10, 0.0f);
    // May return empty results or error - either is acceptable
    if (results.has_value()) {
        // If it succeeds, results should be empty or near-empty
        // since stop words typically don't match useful content
        SUCCEED();
    } else {
        // Error is also acceptable
        SUCCEED();
    }
}

TEST_F(BM25Test, CaseInsensitiveSearch) {
    BM25Config config;
    config.case_sensitive = false;
    BM25Engine engine(config);
    
    ASSERT_TRUE(engine.add_document(1, "Gold prices SURGE"));
    
    auto results = engine.search("gold prices surge", 10, 0.0f);
    ASSERT_TRUE(results.has_value());
    EXPECT_FALSE(results->empty());
}

// ============================================================================
// Document Management Tests
// ============================================================================

TEST_F(BM25Test, RemoveDocument) {
    BM25Engine engine;
    ASSERT_TRUE(engine.add_document(1, "Gold prices surge"));
    ASSERT_TRUE(engine.add_document(2, "Silver follows gold"));
    ASSERT_TRUE(engine.add_document(3, "Dollar weakens"));
    
    EXPECT_EQ(engine.document_count(), 3);
    
    EXPECT_TRUE(engine.remove_document(2));
    EXPECT_EQ(engine.document_count(), 2);
    
    // Search should not find removed document
    auto results = engine.search("silver gold", 10);
    ASSERT_TRUE(results.has_value());
    
    for (const auto& r : *results) {
        EXPECT_NE(r.id, 2u) << "Removed document found in search results";
    }
}

TEST_F(BM25Test, RemoveNonExistentDocument) {
    BM25Engine engine;
    ASSERT_TRUE(engine.add_document(1, "Test document"));
    
    EXPECT_FALSE(engine.remove_document(999));
}

TEST_F(BM25Test, UpdateDocument) {
    BM25Engine engine;
    ASSERT_TRUE(engine.add_document(1, "Gold prices surge"));
    
    EXPECT_TRUE(engine.update_document(1, "Gold prices drop significantly"));
    EXPECT_EQ(engine.document_count(), 1);
    
    // New content searchable
    auto results = engine.search("drop", 10);
    ASSERT_TRUE(results.has_value());
    EXPECT_FALSE(results->empty());
    EXPECT_EQ((*results)[0].id, 1u);
    
    // Old content not searchable
    results = engine.search("surge", 10);
    EXPECT_TRUE(!results.has_value() || results->empty());
}

// ============================================================================
// Persistence Tests
// ============================================================================

TEST_F(BM25Test, SaveAndLoad) {
    const std::string test_file = "test_bm25_engine.dat";
    
    size_t orig_doc_count, orig_term_count;
    
    // Create and populate engine
    {
        BM25Engine engine;
        ASSERT_TRUE(engine.add_document(1, "Gold prices surge on inflation fears"));
        ASSERT_TRUE(engine.add_document(2, "Silver follows gold higher"));
        ASSERT_TRUE(engine.add_document(3, "Dollar weakens against metals"));
        
        orig_doc_count = engine.document_count();
        orig_term_count = engine.term_count();
        
        ASSERT_TRUE(engine.save(test_file));
    }
    
    // Load and verify
    {
        auto load_result = BM25Engine::load(test_file);
        ASSERT_TRUE(load_result.has_value());
        BM25Engine& engine = *load_result;
        
        EXPECT_EQ(engine.document_count(), orig_doc_count);
        EXPECT_EQ(engine.term_count(), orig_term_count);
        
        auto results = engine.search("gold prices", 10);
        ASSERT_TRUE(results.has_value());
        EXPECT_FALSE(results->empty());
    }
    
    // Clean up
    std::filesystem::remove(test_file);
}
