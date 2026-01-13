#pragma once
// ============================================================================
// VectorDB - Hybrid Search (Vector + Lexical/Full-Text)
// ============================================================================

#include "core.hpp"
#include "database.hpp"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace vdb {
namespace hybrid {

// ============================================================================
// BM25 Full-Text Search Engine
// ============================================================================

struct BM25Config {
    float k1 = 1.2f;      // Term frequency saturation parameter
    float b = 0.75f;      // Length normalization parameter
    size_t min_term_length = 2;
    bool use_stemming = true;
    bool case_sensitive = false;
};

struct Term {
    std::string text;
    uint32_t frequency;
    std::vector<size_t> positions;
};

struct Document {
    VectorId id;
    std::string content;
    std::map<std::string, Term> terms;
    size_t length = 0;  // Number of terms
};

struct BM25Result {
    VectorId id;
    float score;
    std::vector<std::string> matched_terms;
    
    bool operator<(const BM25Result& other) const {
        return score > other.score;
    }
};

class BM25Engine {
public:
    explicit BM25Engine(const BM25Config& config = {});
    ~BM25Engine();
    
    // Move semantics
    BM25Engine(BM25Engine&&) noexcept = default;
    BM25Engine& operator=(BM25Engine&&) noexcept = default;
    
    // Delete copy semantics
    BM25Engine(const BM25Engine&) = delete;
    BM25Engine& operator=(const BM25Engine&) = delete;
    
    // Index management
    Result<void> add_document(VectorId id, const std::string& content);
    Result<void> remove_document(VectorId id);
    Result<void> update_document(VectorId id, const std::string& content);
    
    // Search
    Result<std::vector<BM25Result>> search(
        const std::string& query,
        size_t k = 10,
        float min_score = 0.0f
    ) const;
    
    // Statistics
    size_t document_count() const;
    size_t term_count() const;
    float average_document_length() const;
    
    // Persistence
    Result<void> save(const std::string& path) const;
    static Result<BM25Engine> load(const std::string& path);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Keyword Extraction and Ranking
// ============================================================================

struct KeywordConfig {
    size_t max_keywords = 10;
    float min_score = 0.1f;
    bool use_tfidf = true;
    bool use_position_weight = true;
};

struct Keyword {
    std::string term;
    float score;
    uint32_t frequency;
    std::vector<size_t> positions;
    
    bool operator<(const Keyword& other) const {
        return score > other.score;
    }
};

class KeywordExtractor {
public:
    explicit KeywordExtractor(const KeywordConfig& config = {});
    ~KeywordExtractor();
    
    // Extract keywords from text
    Result<std::vector<Keyword>> extract(const std::string& text) const;
    
    // Train on corpus for TF-IDF
    Result<void> train(const std::vector<std::string>& documents);
    
    // Persistence
    Result<void> save(const std::string& path) const;
    static Result<KeywordExtractor> load(const std::string& path);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Hybrid Search Combiner
// ============================================================================

enum class FusionMethod {
    WeightedSum,          // Weighted combination of scores
    RRF,                  // Reciprocal Rank Fusion
    CombSUM,              // Sum of normalized scores
    CombMNZ,              // CombSUM * number of matching systems
    Borda                 // Borda count voting
};

struct HybridSearchConfig {
    float vector_weight = 0.7f;     // Weight for vector search
    float lexical_weight = 0.3f;    // Weight for lexical search
    FusionMethod fusion = FusionMethod::RRF;
    size_t rrf_k = 60;              // RRF constant
    bool rerank = true;             // Apply reranking
};

struct HybridResult {
    VectorId id;
    float combined_score;
    float vector_score;
    float lexical_score;
    std::optional<Metadata> metadata;
    std::vector<std::string> matched_keywords;
    
    bool operator<(const HybridResult& other) const {
        return combined_score > other.combined_score;
    }
};

class HybridSearchEngine {
public:
    explicit HybridSearchEngine(const HybridSearchConfig& config = {});
    ~HybridSearchEngine();
    
    // Combine results from vector and lexical search
    Result<std::vector<HybridResult>> combine(
        const std::vector<QueryResult>& vector_results,
        const std::vector<BM25Result>& lexical_results,
        size_t k = 10
    ) const;
    
    // Score fusion methods
    static float weighted_sum(float vec_score, float lex_score, float vec_weight);
    static float reciprocal_rank_fusion(size_t vec_rank, size_t lex_rank, size_t k);
    static float comb_sum(float vec_score, float lex_score);
    static float comb_mnz(float vec_score, float lex_score, size_t num_systems);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Query Rewriter
// ============================================================================

struct RewriteConfig {
    bool expand_synonyms = true;
    bool correct_spelling = false;
    bool add_stemmed_terms = true;
    size_t max_expansions = 5;
};

class QueryRewriter {
public:
    explicit QueryRewriter(const RewriteConfig& config = {});
    ~QueryRewriter();
    
    // Rewrite query for better results
    Result<std::string> rewrite(const std::string& query) const;
    
    // Add synonyms
    Result<void> add_synonym(const std::string& term, const std::vector<std::string>& synonyms);
    
    // Load synonym dictionary
    Result<void> load_synonyms(const std::string& path);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace hybrid
} // namespace vdb
