// ============================================================================
// Test: Hybrid Search Engine (Fusion Methods) - Google Test
// ============================================================================

#include "vdb/hybrid_search.hpp"
#include "vdb/core.hpp"
#include <gtest/gtest.h>
#include <cmath>

using namespace vdb;
using namespace vdb::hybrid;

// ============================================================================
// Hybrid Search Test Fixture
// ============================================================================

class HybridSearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock vector results
        vector_results_ = {
            {1, 0.9f, 0.9f, {}},
            {2, 0.8f, 0.8f, {}},
            {3, 0.7f, 0.7f, {}}
        };
        
        // Create mock lexical results
        lexical_results_ = {
            {2, 5.0f, {"gold"}},
            {1, 4.0f, {"prices"}},
            {4, 3.0f, {"surge"}}
        };
    }

    std::vector<QueryResult> vector_results_;
    std::vector<BM25Result> lexical_results_;
};

// ============================================================================
// Fusion Method Tests
// ============================================================================

TEST_F(HybridSearchTest, RRFFusion) {
    HybridSearchConfig config;
    config.fusion = FusionMethod::RRF;
    config.rrf_k = 60;
    
    HybridSearchEngine engine(config);
    
    auto results = engine.combine(vector_results_, lexical_results_, 10);
    ASSERT_TRUE(results.has_value());
    EXPECT_FALSE(results->empty());
    
    // Doc 2 appears in both lists, should be ranked highly
    bool found_doc2 = false;
    for (size_t i = 0; i < std::min(results->size(), size_t(3)); ++i) {
        if ((*results)[i].id == 2) {
            found_doc2 = true;
            break;
        }
    }
    EXPECT_TRUE(found_doc2) << "Document 2 should be in top results";
    
    // Results should be sorted by combined_score
    for (size_t i = 1; i < results->size(); ++i) {
        EXPECT_GE((*results)[i-1].combined_score, (*results)[i].combined_score);
    }
}

TEST_F(HybridSearchTest, WeightedSumFusion) {
    HybridSearchConfig config;
    config.fusion = FusionMethod::WeightedSum;
    config.vector_weight = 0.7f;
    config.lexical_weight = 0.3f;
    
    HybridSearchEngine engine(config);
    
    auto results = engine.combine(vector_results_, lexical_results_, 10);
    ASSERT_TRUE(results.has_value());
    EXPECT_FALSE(results->empty());
    
    // Verify weights sum to 1.0 (or close)
    EXPECT_NEAR(config.vector_weight + config.lexical_weight, 1.0f, 0.001f);
}

TEST_F(HybridSearchTest, CombSumFusion) {
    HybridSearchConfig config;
    config.fusion = FusionMethod::CombSUM;
    
    HybridSearchEngine engine(config);
    
    auto results = engine.combine(vector_results_, lexical_results_, 10);
    ASSERT_TRUE(results.has_value());
    EXPECT_FALSE(results->empty());
    
    // CombSUM adds normalized scores
    // Doc 2 appears in both, should have highest combined score
    if (!results->empty()) {
        bool doc2_high = false;
        for (size_t i = 0; i < std::min(results->size(), size_t(2)); ++i) {
            if ((*results)[i].id == 2) doc2_high = true;
        }
        EXPECT_TRUE(doc2_high);
    }
}

TEST_F(HybridSearchTest, CombMNZFusion) {
    HybridSearchConfig config;
    config.fusion = FusionMethod::CombMNZ;
    
    HybridSearchEngine engine(config);
    
    auto results = engine.combine(vector_results_, lexical_results_, 10);
    ASSERT_TRUE(results.has_value());
    EXPECT_FALSE(results->empty());
    
    // CombMNZ multiplies by number of lists document appears in
    // Doc 2 appears in both lists - should be boosted
}

TEST_F(HybridSearchTest, BordaFusion) {
    HybridSearchConfig config;
    config.fusion = FusionMethod::Borda;
    
    HybridSearchEngine engine(config);
    
    auto results = engine.combine(vector_results_, lexical_results_, 10);
    ASSERT_TRUE(results.has_value());
    EXPECT_FALSE(results->empty());
}

// ============================================================================
// Static Method Tests
// ============================================================================

TEST(HybridSearchStaticTest, WeightedSum) {
    float result = HybridSearchEngine::weighted_sum(0.8f, 0.6f, 0.7f);
    // weighted_sum: vec_score * vec_weight + lex_score * (1 - vec_weight)
    float expected = 0.8f * 0.7f + 0.6f * 0.3f;
    EXPECT_NEAR(result, expected, 0.001f);
}

TEST(HybridSearchStaticTest, ReciprocalRankFusion) {
    float result = HybridSearchEngine::reciprocal_rank_fusion(1, 2, 60);
    // RRF: 1/(k + rank1) + 1/(k + rank2)
    EXPECT_GT(result, 0.0f);
}

TEST(HybridSearchStaticTest, CombSum) {
    float result = HybridSearchEngine::comb_sum(0.8f, 0.6f);
    EXPECT_NEAR(result, 1.4f, 0.001f);  // Simple sum
}

TEST(HybridSearchStaticTest, CombMNZ) {
    // CombMNZ: sum * num_systems (when both have scores)
    float result = HybridSearchEngine::comb_mnz(0.8f, 0.6f, 2);
    EXPECT_GT(result, HybridSearchEngine::comb_sum(0.8f, 0.6f));  // Should be boosted
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(HybridSearchTest, EmptyVectorResults) {
    HybridSearchConfig config;
    config.fusion = FusionMethod::RRF;
    HybridSearchEngine engine(config);
    
    std::vector<QueryResult> empty_vectors;
    auto results = engine.combine(empty_vectors, lexical_results_, 10);
    
    // Should still work with only lexical results
    ASSERT_TRUE(results.has_value());
}

TEST_F(HybridSearchTest, EmptyLexicalResults) {
    HybridSearchConfig config;
    config.fusion = FusionMethod::RRF;
    HybridSearchEngine engine(config);
    
    std::vector<BM25Result> empty_lexical;
    auto results = engine.combine(vector_results_, empty_lexical, 10);
    
    // Should still work with only vector results
    ASSERT_TRUE(results.has_value());
}

TEST_F(HybridSearchTest, BothResultsEmpty) {
    HybridSearchConfig config;
    config.fusion = FusionMethod::RRF;
    HybridSearchEngine engine(config);
    
    std::vector<QueryResult> empty_vectors;
    std::vector<BM25Result> empty_lexical;
    
    auto results = engine.combine(empty_vectors, empty_lexical, 10);
    ASSERT_TRUE(results.has_value());
    EXPECT_TRUE(results->empty());
}
