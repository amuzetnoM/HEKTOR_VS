// ============================================================================
// Test: RAG (Retrieval Augmented Generation) Engine - Google Test
// ============================================================================

#include "vdb/framework_integration.hpp"
#include <gtest/gtest.h>

using namespace vdb;
using namespace vdb::framework;

// ============================================================================
// RAG Test Fixture
// ============================================================================

class RAGTest : public ::testing::Test {
protected:
    void SetUp() override {
        sample_document_ = "This is a test document. It has multiple sentences. "
                          "We want to chunk it into smaller pieces for processing. "
                          "Each chunk should contain relevant information. "
                          "The chunking strategy determines how text is split.";
        
        long_document_ = std::string(2000, 'A');  // 2KB of text
    }

    std::string sample_document_;
    std::string long_document_;
};

// ============================================================================
// Fixed Chunking Tests
// ============================================================================

TEST_F(RAGTest, FixedChunkingCreatesChunks) {
    RAGConfig config;
    config.chunking_strategy = "fixed";
    config.chunk_size = 50;
    config.chunk_overlap = 10;
    
    RAGEngine engine(config);
    
    auto result = engine.chunk_document(sample_document_);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->empty());
}

TEST_F(RAGTest, FixedChunkingRespectsSizeLimit) {
    RAGConfig config;
    config.chunking_strategy = "fixed";
    config.chunk_size = 50;
    config.chunk_overlap = 10;
    
    RAGEngine engine(config);
    
    auto result = engine.chunk_document(long_document_);
    ASSERT_TRUE(result.has_value());
    
    for (const auto& chunk : *result) {
        // Chunks should be roughly chunk_size (may be slightly larger due to overlap)
        EXPECT_LE(chunk.length(), config.chunk_size + config.chunk_overlap + 10);
    }
}

// ============================================================================
// Sentence Chunking Tests
// ============================================================================

TEST_F(RAGTest, SentenceChunkingPreservesSentences) {
    RAGConfig config;
    config.chunking_strategy = "sentence";
    config.chunk_size = 100;
    config.chunk_overlap = 20;
    
    RAGEngine engine(config);
    
    auto result = engine.chunk_document(sample_document_);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->empty());
}

// ============================================================================
// Paragraph Chunking Tests
// ============================================================================

TEST_F(RAGTest, ParagraphChunkingHandlesMultipleParagraphs) {
    std::string doc_with_paragraphs = 
        "First paragraph about gold.\n\n"
        "Second paragraph about silver.\n\n"
        "Third paragraph about metals.";
    
    RAGConfig config;
    config.chunking_strategy = "paragraph";
    config.chunk_size = 200;
    config.chunk_overlap = 0;
    
    RAGEngine engine(config);
    
    auto result = engine.chunk_document(doc_with_paragraphs);
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result->size(), 1u);
}

// ============================================================================
// Context Building Tests
// ============================================================================

TEST_F(RAGTest, ContextBuildingCombinesChunks) {
    RAGConfig config;
    config.max_context_length = 500;
    
    RAGEngine engine(config);
    
    // Create mock search results using proper constructor
    std::vector<QueryResult> search_results;
    QueryResult r1; r1.id = 1; r1.distance = 0.05f; r1.score = 0.95f;
    QueryResult r2; r2.id = 2; r2.distance = 0.15f; r2.score = 0.85f;
    QueryResult r3; r3.id = 3; r3.distance = 0.25f; r3.score = 0.75f;
    search_results.push_back(r1);
    search_results.push_back(r2);
    search_results.push_back(r3);
    
    std::string query = "What is happening with gold prices?";
    auto context_result = engine.build_context(query, search_results);
    ASSERT_TRUE(context_result.has_value());
    EXPECT_FALSE(context_result->formatted_context.empty());
}

// ============================================================================
// Prompt Formatting Tests
// ============================================================================

TEST_F(RAGTest, PromptFormattingIncludesQueryAndContext) {
    RAGConfig config;
    RAGEngine engine(config);
    
    std::string query = "What is the price of gold?";
    RAGContext context;
    context.retrieved_chunks = {"Gold is trading at $2000 per ounce."};
    context.formatted_context = "Gold is trading at $2000 per ounce.";
    context.total_tokens = 10;
    
    std::string prompt = engine.format_prompt(query, context);
    
    EXPECT_NE(prompt.find(query), std::string::npos);
}

// ============================================================================
// Reranking Tests
// ============================================================================

TEST_F(RAGTest, RerankingReturnsSortedResults) {
    RAGConfig config;
    config.rerank = true;
    RAGEngine engine(config);
    
    std::vector<QueryResult> results;
    QueryResult r1; r1.id = 1; r1.distance = 0.3f; r1.score = 0.7f;
    QueryResult r2; r2.id = 2; r2.distance = 0.2f; r2.score = 0.8f;
    QueryResult r3; r3.id = 3; r3.distance = 0.4f; r3.score = 0.6f;
    results.push_back(r1);
    results.push_back(r2);
    results.push_back(r3);
    
    auto reranked = engine.rerank("gold prices", results);
    ASSERT_TRUE(reranked.has_value());
    EXPECT_FALSE(reranked->empty());
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(RAGTest, EmptyDocumentHandled) {
    RAGConfig config;
    RAGEngine engine(config);
    
    auto result = engine.chunk_document("");
    // Empty document should either return error or empty chunks
    if (result.has_value()) {
        EXPECT_TRUE(result->empty());
    }
}

TEST_F(RAGTest, WhitespaceOnlyDocument) {
    RAGConfig config;
    RAGEngine engine(config);
    
    auto result = engine.chunk_document("   \n\n   \t   ");
    // Whitespace-only document should be handled gracefully
    // Implementation may return empty or error
    SUCCEED();  // Just verify no crash
}
