// ============================================================================
// Test: BM25 Full-Text Search Engine
// ============================================================================

#include "vdb/hybrid_search.hpp"
#include <iostream>
#include <cassert>

using namespace vdb::hybrid;

void test_bm25_basic() {
    std::cout << "Testing BM25 basic operations...\n";
    
    // Create BM25 engine
    BM25Config config;
    config.k1 = 1.2f;
    config.b = 0.75f;
    config.use_stemming = true;
    config.case_sensitive = false;
    
    BM25Engine engine(config);
    
    // Add documents
    auto result1 = engine.add_document(1, "Gold prices surge on inflation fears");
    assert(result1);
    std::cout << "  [PASS] Added document 1\n";
    
    auto result2 = engine.add_document(2, "Silver follows gold higher");
    assert(result2);
    std::cout << "  [PASS] Added document 2\n";
    
    auto result3 = engine.add_document(3, "Dollar weakens against precious metals");
    assert(result3);
    std::cout << "  [PASS] Added document 3\n";
    
    // Check statistics
    assert(engine.document_count() == 3);
    std::cout << "  [PASS] Document count: " << engine.document_count() << "\n";
    
    assert(engine.term_count() > 0);
    std::cout << "  [PASS] Term count: " << engine.term_count() << "\n";
    
    assert(engine.average_document_length() > 0);
    std::cout << "  [PASS] Average doc length: " << engine.average_document_length() << "\n";
    
    std::cout << "[PASS] Basic operations test passed\n\n";
}

void test_bm25_search() {
    std::cout << "Testing BM25 search...\n";
    
    BM25Engine engine;
    
    // Add test documents
    engine.add_document(1, "Gold prices surge to new highs on inflation concerns");
    engine.add_document(2, "Silver rallies as precious metals gain momentum");
    engine.add_document(3, "Dollar weakness supports gold and silver prices");
    engine.add_document(4, "Central banks increase gold reserves");
    engine.add_document(5, "Mining stocks rise with metal prices");
    
    // Search for "gold prices"
    auto results = engine.search("gold prices", 10, 0.0f);
    assert(results);
    
    std::cout << "  Search results for 'gold prices':\n";
    for (const auto& result : *results) {
        std::cout << "    Doc " << result.id << ": score=" << result.score;
        std::cout << ", matched=[";
        for (size_t i = 0; i < result.matched_terms.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result.matched_terms[i];
        }
        std::cout << "]\n";
    }
    
    // Verify results
    assert(!results->empty());
    std::cout << "  [PASS] Found " << results->size() << " results\n";
    
    // Check that doc 1 and 3 are in results (both contain "gold" and "prices")
    bool found_doc1 = false;
    bool found_doc3 = false;
    for (const auto& result : *results) {
        if (result.id == 1) found_doc1 = true;
        if (result.id == 3) found_doc3 = true;
    }
    assert(found_doc1);
    assert(found_doc3);
    std::cout << "  [PASS] Expected documents found\n";
    
    // Check that results are sorted by score (descending)
    for (size_t i = 1; i < results->size(); ++i) {
        assert((*results)[i-1].score >= (*results)[i].score);
    }
    std::cout << "  [PASS] Results properly sorted\n";
    
    std::cout << "[PASS] Search test passed\n\n";
}

void test_bm25_stemming() {
    std::cout << "Testing BM25 stemming...\n";
    
    BM25Config config;
    config.use_stemming = true;
    
    BM25Engine engine(config);
    
    // Add documents with different word forms
    engine.add_document(1, "Gold prices are rising rapidly");
    engine.add_document(2, "Silver price rose yesterday");
    engine.add_document(3, "Prices of metals increased");
    
    // Search should match stemmed forms
    auto results = engine.search("price rise", 10, 0.0f);
    assert(results);
    
    std::cout << "  Stemming search results:\n";
    for (const auto& result : *results) {
        std::cout << "    Doc " << result.id << ": score=" << result.score << "\n";
    }
    
    // Should find documents with "prices", "rising", "rose"
    assert(!results->empty());
    assert(results->size() >= 2);  // At least docs 1 and 2
    std::cout << "  [PASS] Stemming working\n";
    
    std::cout << "[PASS] Stemming test passed\n\n";
}

void test_bm25_empty_query() {
    std::cout << "Testing BM25 with empty query...\n";
    
    BM25Engine engine;
    engine.add_document(1, "Test document");
    
    // Empty query should return error
    auto results = engine.search("", 10, 0.0f);
    assert(!results);  // Should fail
    std::cout << "  [PASS] Empty query handled correctly\n";
    
    // Query with only stop words
    results = engine.search("the and or", 10, 0.0f);
    assert(!results);  // Should fail (no valid terms)
    std::cout << "  [PASS] Stop-word-only query handled correctly\n";
    
    std::cout << "[PASS] Empty query test passed\n\n";
}

void test_bm25_case_sensitivity() {
    std::cout << "Testing BM25 case sensitivity...\n";
    
    // Case insensitive (default)
    BM25Config config_insensitive;
    config_insensitive.case_sensitive = false;
    
    BM25Engine engine_insensitive(config_insensitive);
    engine_insensitive.add_document(1, "Gold prices SURGE");
    
    auto results = engine_insensitive.search("gold prices surge", 10, 0.0f);
    assert(results && !results->empty());
    std::cout << "  [PASS] Case insensitive search working\n";
    
    std::cout << "[PASS] Case sensitivity test passed\n\n";
}

void test_bm25_remove_document() {
    std::cout << "Testing BM25 remove document...\n";
    
    BM25Engine engine;
    engine.add_document(1, "Gold prices surge");
    engine.add_document(2, "Silver follows gold");
    engine.add_document(3, "Dollar weakens");
    
    assert(engine.document_count() == 3);
    
    // Remove document
    auto result = engine.remove_document(2);
    assert(result);
    std::cout << "  [PASS] Document removed\n";
    
    assert(engine.document_count() == 2);
    std::cout << "  [PASS] Document count updated\n";
    
    // Search should not find removed document
    auto search_results = engine.search("silver gold", 10);
    assert(search_results);
    
    bool found_doc2 = false;
    for (const auto& r : *search_results) {
        if (r.id == 2) found_doc2 = true;
    }
    assert(!found_doc2);
    std::cout << "  [PASS] Removed document not in search results\n";
    
    // Try to remove non-existent document
    result = engine.remove_document(999);
    assert(!result);
    std::cout << "  [PASS] Non-existent document handled correctly\n";
    
    std::cout << "[PASS] Remove document test passed\n\n";
}

void test_bm25_update_document() {
    std::cout << "Testing BM25 update document...\n";
    
    BM25Engine engine;
    engine.add_document(1, "Gold prices surge");
    
    // Update document
    auto result = engine.update_document(1, "Gold prices drop significantly");
    assert(result);
    std::cout << "  [PASS] Document updated\n";
    
    assert(engine.document_count() == 1);
    std::cout << "  [PASS] Document count unchanged\n";
    
    // Search for new content
    auto results = engine.search("drop", 10);
    assert(results && !results->empty());
    assert((*results)[0].id == 1);
    std::cout << "  [PASS] Updated content searchable\n";
    
    // Old content should not be found
    results = engine.search("surge", 10);
    assert(!results || results->empty());
    std::cout << "  [PASS] Old content not searchable\n";
    
    std::cout << "[PASS] Update document test passed\n\n";
}

void test_bm25_save_load() {
    std::cout << "Testing BM25 save and load...\n";
    
    const std::string test_file = "test_bm25_engine.dat";
    
    // Create and populate engine
    BM25Engine engine1;
    engine1.add_document(1, "Gold prices surge on inflation fears");
    engine1.add_document(2, "Silver follows gold higher");
    engine1.add_document(3, "Dollar weakens against metals");
    
    size_t orig_doc_count = engine1.document_count();
    size_t orig_term_count = engine1.term_count();
    
    // Save engine
    auto save_result = engine1.save(test_file);
    assert(save_result);
    std::cout << "  [PASS] Engine saved\n";
    
    // Load engine
    auto load_result = BM25Engine::load(test_file);
    assert(load_result);
    BM25Engine& engine2 = *load_result;
    std::cout << "  [PASS] Engine loaded\n";
    
    // Verify statistics
    assert(engine2.document_count() == orig_doc_count);
    std::cout << "  [PASS] Document count preserved\n";
    
    assert(engine2.term_count() == orig_term_count);
    std::cout << "  [PASS] Term count preserved\n";
    
    // Verify search works
    auto results = engine2.search("gold prices", 10);
    assert(results && !results->empty());
    std::cout << "  [PASS] Search works on loaded engine\n";
    
    // Clean up
    std::remove(test_file.c_str());
    
    std::cout << "[PASS] Save and load test passed\n\n";
}

int main() {
    std::cout << "==============================================\n";
    std::cout << "BM25 Full-Text Search Engine Tests\n";
    std::cout << "==============================================\n\n";
    
    try {
        test_bm25_basic();
        test_bm25_search();
        test_bm25_stemming();
        test_bm25_empty_query();
        test_bm25_case_sensitivity();
        test_bm25_remove_document();
        test_bm25_update_document();
        test_bm25_save_load();
        
        std::cout << "==============================================\n";
        std::cout << "All tests passed! [PASS]\n";
        std::cout << "==============================================\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
