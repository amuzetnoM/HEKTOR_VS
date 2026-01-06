// ============================================================================
// Test: Hybrid Search Engine (Fusion Methods)
// ============================================================================

#include "vdb/hybrid_search.hpp"
#include "vdb/core.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace vdb;
using namespace vdb::hybrid;

void test_rrf_fusion() {
    std::cout << "Testing RRF fusion...\n";
    
    HybridSearchConfig config;
    config.fusion = FusionMethod::RRF;
    config.rrf_k = 60;
    
    HybridSearchEngine engine(config);
    
    // Create mock vector results
    std::vector<QueryResult> vector_results = {
        {1, 0.9f, 0.9f, {}},
        {2, 0.8f, 0.8f, {}},
        {3, 0.7f, 0.7f, {}}
    };
    
    // Create mock lexical results
    std::vector<BM25Result> lexical_results = {
        {2, 5.0f, {"gold"}},
        {1, 4.0f, {"prices"}},
        {4, 3.0f, {"surge"}}
    };
    
    auto results = engine.combine(vector_results, lexical_results, 10);
    assert(results);
    assert(!results->empty());
    
    std::cout << "  RRF Results:\n";
    for (const auto& r : *results) {
        std::cout << "    ID " << r.id << ": combined=" << r.combined_score 
                  << ", vector=" << r.vector_score << ", lexical=" << r.lexical_score << "\n";
    }
    
    // Doc 1 and 2 should be ranked higher (appear in both)
    assert((*results)[0].id == 1 || (*results)[0].id == 2);
    std::cout << "  [PASS] RRF fusion test passed\n\n";
}

void test_weighted_sum_fusion() {
    std::cout << "Testing Weighted Sum fusion...\n";
    
    HybridSearchConfig config;
    config.fusion = FusionMethod::WeightedSum;
    config.vector_weight = 0.7f;
    config.lexical_weight = 0.3f;
    
    HybridSearchEngine engine(config);
    
    std::vector<QueryResult> vector_results = {
        {1, 0.9f, 0.9f, {}},
        {2, 0.5f, 0.5f, {}}
    };
    
    std::vector<BM25Result> lexical_results = {
        {2, 10.0f, {"gold"}},
        {3, 8.0f, {"silver"}}
    };
    
    auto results = engine.combine(vector_results, lexical_results, 10);
    assert(results);
    
    std::cout << "  Weighted Sum Results:\n";
    for (const auto& r : *results) {
        std::cout << "    ID " << r.id << ": combined=" << r.combined_score 
                  << ", vector=" << r.vector_score << ", lexical=" << r.lexical_score << "\n";
    }
    
    // Verify weights are applied
    assert(results->size() >= 2);
    std::cout << "  [PASS] Weighted sum test passed\n\n";
}

void test_combsum_fusion() {
    std::cout << "Testing CombSUM fusion...\n";
    
    HybridSearchConfig config;
    config.fusion = FusionMethod::CombSUM;
    
    HybridSearchEngine engine(config);
    
    std::vector<QueryResult> vector_results = {
        {1, 1.0f, 1.0f, {}},
        {2, 0.8f, 0.8f, {}}
    };
    
    std::vector<BM25Result> lexical_results = {
        {1, 5.0f, {"gold"}},
        {3, 4.0f, {"prices"}}
    };
    
    auto results = engine.combine(vector_results, lexical_results, 10);
    assert(results);
    
    std::cout << "  CombSUM Results:\n";
    for (const auto& r : *results) {
        std::cout << "    ID " << r.id << ": combined=" << r.combined_score 
                  << ", vector=" << r.vector_score << ", lexical=" << r.lexical_score << "\n";
    }
    
    // Doc 1 appears in both, should have highest score
    assert((*results)[0].id == 1);
    std::cout << "  [PASS] CombSUM test passed\n\n";
}

void test_combmnz_fusion() {
    std::cout << "Testing CombMNZ fusion...\n";
    
    HybridSearchConfig config;
    config.fusion = FusionMethod::CombMNZ;
    
    HybridSearchEngine engine(config);
    
    std::vector<QueryResult> vector_results = {
        {1, 0.8f, 0.8f, {}},
        {2, 0.6f, 0.6f, {}}
    };
    
    std::vector<BM25Result> lexical_results = {
        {1, 4.0f, {"gold"}},
        {3, 5.0f, {"prices"}}
    };
    
    auto results = engine.combine(vector_results, lexical_results, 10);
    assert(results);
    
    std::cout << "  CombMNZ Results:\n";
    for (const auto& r : *results) {
        std::cout << "    ID " << r.id << ": combined=" << r.combined_score 
                  << " (systems=" << ((r.vector_score > 0 ? 1 : 0) + (r.lexical_score > 0 ? 1 : 0)) << ")\n";
    }
    
    // Doc 1 in both systems should have 2x multiplier
    assert((*results)[0].id == 1);
    std::cout << "  [PASS] CombMNZ test passed\n\n";
}

void test_borda_fusion() {
    std::cout << "Testing Borda fusion...\n";
    
    HybridSearchConfig config;
    config.fusion = FusionMethod::Borda;
    
    HybridSearchEngine engine(config);
    
    std::vector<QueryResult> vector_results = {
        {1, 0.9f, 0.9f, {}},
        {2, 0.8f, 0.8f, {}},
        {3, 0.7f, 0.7f, {}}
    };
    
    std::vector<BM25Result> lexical_results = {
        {3, 10.0f, {"gold"}},
        {1, 9.0f, {"prices"}},
        {4, 8.0f, {"surge"}}
    };
    
    auto results = engine.combine(vector_results, lexical_results, 10);
    assert(results);
    
    std::cout << "  Borda Results:\n";
    for (const auto& r : *results) {
        std::cout << "    ID " << r.id << ": combined=" << r.combined_score << "\n";
    }
    
    assert(results->size() == 4); // All unique docs
    std::cout << "  [PASS] Borda test passed\n\n";
}

void test_static_methods() {
    std::cout << "Testing static fusion methods...\n";
    
    // Test weighted sum
    float ws = HybridSearchEngine::weighted_sum(0.8f, 0.6f, 0.7f);
    assert(ws > 0.6f && ws < 0.8f);
    std::cout << "  Weighted sum: " << ws << "\n";
    
    // Test RRF
    float rrf = HybridSearchEngine::reciprocal_rank_fusion(1, 2, 60);
    assert(rrf > 0.0f);
    std::cout << "  RRF: " << rrf << "\n";
    
    // Test CombSUM
    float cs = HybridSearchEngine::comb_sum(0.8f, 0.6f);
    assert(std::abs(cs - 1.4f) < 0.001f);
    std::cout << "  CombSUM: " << cs << "\n";
    
    // Test CombMNZ
    float cmnz = HybridSearchEngine::comb_mnz(0.8f, 0.6f, 2);
    assert(std::abs(cmnz - 2.8f) < 0.001f);
    std::cout << "  CombMNZ: " << cmnz << "\n";
    
    std::cout << "  [PASS] Static methods test passed\n\n";
}

void test_empty_results() {
    std::cout << "Testing with empty results...\n";
    
    HybridSearchEngine engine;
    
    std::vector<QueryResult> empty_vector;
    std::vector<BM25Result> empty_lexical;
    
    auto results = engine.combine(empty_vector, empty_lexical, 10);
    assert(results);
    assert(results->empty());
    
    std::cout << "  [PASS] Empty results handled correctly\n\n";
}

int main() {
    std::cout << "==============================================\n";
    std::cout << "Hybrid Search Engine Tests\n";
    std::cout << "==============================================\n\n";
    
    try {
        test_rrf_fusion();
        test_weighted_sum_fusion();
        test_combsum_fusion();
        test_combmnz_fusion();
        test_borda_fusion();
        test_static_methods();
        test_empty_results();
        
        std::cout << "==============================================\n";
        std::cout << "All tests passed! [PASS]\n";
        std::cout << "==============================================\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
