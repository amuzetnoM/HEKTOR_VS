// Copyright (c) 2024 Vector Studio
// RAG Engine Tests

#include "vdb/framework_integration.hpp"
#include <iostream>
#include <cassert>

using namespace vdb;

void test_rag_chunking_fixed() {
    std::cout << "Testing RAG fixed chunking...\n";
    
    RAGConfig config;
    config.chunking_strategy = "fixed";
    config.chunk_size = 50;
    config.chunk_overlap = 10;
    
    RAGEngine engine(config);
    
    std::string document = "This is a test document. It has multiple sentences. "
                          "We want to chunk it into smaller pieces for processing.";
    
    auto result = engine.chunk_document(document);
    assert(result.is_ok());
    
    auto chunks = result.unwrap();
    assert(!chunks.empty());
    
    std::cout << "  Created " << chunks.size() << " chunks\n";
    for (size_t i = 0; i < chunks.size(); ++i) {
        std::cout << "  Chunk " << (i+1) << " (" << chunks[i].length() << " chars): " 
                  << chunks[i].substr(0, 30) << "...\n";
    }
    
    std::cout << "  PASSED\n";
}

void test_rag_chunking_sentence() {
    std::cout << "Testing RAG sentence chunking...\n";
    
    RAGConfig config;
    config.chunking_strategy = "sentence";
    config.chunk_size = 100;
    
    RAGEngine engine(config);
    
    std::string document = "First sentence here. Second sentence follows. "
                          "Third sentence is longer and has more content. "
                          "Fourth sentence wraps it up.";
    
    auto result = engine.chunk_document(document);
    assert(result.is_ok());
    
    auto chunks = result.unwrap();
    assert(!chunks.empty());
    
    std::cout << "  Created " << chunks.size() << " chunks\n";
    for (const auto& chunk : chunks) {
        std::cout << "  - " << chunk << "\n";
    }
    
    std::cout << "  PASSED\n";
}

void test_rag_chunking_paragraph() {
    std::cout << "Testing RAG paragraph chunking...\n";
    
    RAGConfig config;
    config.chunking_strategy = "paragraph";
    config.chunk_size = 200;
    
    RAGEngine engine(config);
    
    std::string document = "First paragraph with some content.\n\n"
                          "Second paragraph with different content.\n\n"
                          "Third paragraph to test chunking.";
    
    auto result = engine.chunk_document(document);
    assert(result.is_ok());
    
    auto chunks = result.unwrap();
    assert(!chunks.empty());
    
    std::cout << "  Created " << chunks.size() << " chunks\n";
    std::cout << "  PASSED\n";
}

void test_rag_context_building() {
    std::cout << "Testing RAG context building...\n";
    
    RAGConfig config;
    config.max_context_length = 100;
    config.relevance_threshold = 0.5f;
    
    RAGEngine engine(config);
    
    // Create mock search results
    std::vector<QueryResult> results;
    
    QueryResult r1;
    r1.id = 1;
    r1.score = 0.9f;
    Metadata meta1;
    meta1["text"] = "Gold prices surge on market fears.";
    r1.metadata = meta1;
    results.push_back(r1);
    
    QueryResult r2;
    r2.id = 2;
    r2.score = 0.7f;
    Metadata meta2;
    meta2["text"] = "Silver follows gold higher.";
    r2.metadata = meta2;
    results.push_back(r2);
    
    QueryResult r3;
    r3.id = 3;
    r3.score = 0.3f; // Below threshold
    Metadata meta3;
    meta3["text"] = "Unrelated content.";
    r3.metadata = meta3;
    results.push_back(r3);
    
    auto context_result = engine.build_context("gold prices", results);
    assert(context_result.is_ok());
    
    auto context = context_result.unwrap();
    
    // Should have filtered out low-relevance results
    assert(context.retrieved_chunks.size() <= 2);
    assert(context.relevance_scores.size() == context.retrieved_chunks.size());
    assert(!context.formatted_context.empty());
    assert(context.total_tokens > 0);
    
    std::cout << "  Context tokens: " << context.total_tokens << "\n";
    std::cout << "  Retrieved chunks: " << context.retrieved_chunks.size() << "\n";
    std::cout << "  PASSED\n";
}

void test_rag_prompt_formatting() {
    std::cout << "Testing RAG prompt formatting...\n";
    
    RAGEngine engine;
    
    RAGContext context;
    context.retrieved_chunks = {"Doc 1 content", "Doc 2 content"};
    context.relevance_scores = {0.9f, 0.7f};
    context.formatted_context = "[Document 1 (score: 0.9)]\nDoc 1 content\n\n"
                               "[Document 2 (score: 0.7)]\nDoc 2 content\n\n";
    
    std::string prompt = engine.format_prompt(
        "What is the price trend?",
        context,
        "You are a helpful assistant."
    );
    
    assert(!prompt.empty());
    assert(prompt.find("Context:") != std::string::npos);
    assert(prompt.find("Question:") != std::string::npos);
    assert(prompt.find("Answer:") != std::string::npos);
    
    std::cout << "  Generated prompt length: " << prompt.length() << " chars\n";
    std::cout << "  PASSED\n";
}

void test_rag_reranking() {
    std::cout << "Testing RAG reranking...\n";
    
    RAGEngine engine;
    
    std::vector<QueryResult> results;
    QueryResult r1; r1.id = 1; r1.score = 0.5f;
    QueryResult r2; r2.id = 2; r2.score = 0.9f;
    QueryResult r3; r3.id = 3; r3.score = 0.7f;
    results.push_back(r1);
    results.push_back(r2);
    results.push_back(r3);
    
    auto reranked_result = engine.rerank("query", results);
    assert(reranked_result.is_ok());
    
    auto reranked = reranked_result.unwrap();
    assert(reranked.size() == 3);
    
    // Should be sorted by score (descending)
    assert(reranked[0].score >= reranked[1].score);
    assert(reranked[1].score >= reranked[2].score);
    
    std::cout << "  Reranked scores: " << reranked[0].score 
              << ", " << reranked[1].score 
              << ", " << reranked[2].score << "\n";
    std::cout << "  PASSED\n";
}

void test_rag_empty_document() {
    std::cout << "Testing RAG empty document handling...\n";
    
    RAGEngine engine;
    
    auto result = engine.chunk_document("");
    assert(!result.is_ok());
    
    std::cout << "  Correctly rejected empty document\n";
    std::cout << "  PASSED\n";
}

int main() {
    std::cout << "Running RAG Engine Tests...\n\n";
    
    test_rag_chunking_fixed();
    test_rag_chunking_sentence();
    test_rag_chunking_paragraph();
    test_rag_context_building();
    test_rag_prompt_formatting();
    test_rag_reranking();
    test_rag_empty_document();
    
    std::cout << "\nAll RAG Engine tests passed!\n";
    return 0;
}
