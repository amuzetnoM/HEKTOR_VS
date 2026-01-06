// ============================================================================
// VectorDB - Hybrid Search Engine Implementation
// ============================================================================

#include "vdb/hybrid_search.hpp"
#include "vdb/logging.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace vdb {
namespace hybrid {

// ============================================================================
// HybridSearchEngine Implementation
// ============================================================================

struct HybridSearchEngine::Impl {
    HybridSearchConfig config;
    
    Impl(const HybridSearchConfig& cfg) : config(cfg) {}
    
    Result<std::vector<HybridResult>> combine(
        const std::vector<QueryResult>& vector_results,
        const std::vector<BM25Result>& lexical_results,
        size_t k
    ) const {
        if (vector_results.empty() && lexical_results.empty()) {
            return std::vector<HybridResult>();
        }
        
        // Create result map indexed by document ID
        std::unordered_map<VectorId, HybridResult> result_map;
        
        // Process vector results
        for (size_t i = 0; i < vector_results.size(); ++i) {
            const auto& vr = vector_results[i];
            auto& hr = result_map[vr.id];
            hr.id = vr.id;
            hr.vector_score = vr.score;
            hr.lexical_score = 0.0f;
            hr.metadata = vr.metadata;
        }
        
        // Process lexical results
        for (size_t i = 0; i < lexical_results.size(); ++i) {
            const auto& lr = lexical_results[i];
            auto& hr = result_map[lr.id];
            hr.id = lr.id;
            hr.lexical_score = lr.score;
            hr.matched_keywords = lr.matched_terms;
        }
        
        // Apply fusion method
        switch (config.fusion) {
            case FusionMethod::RRF:
                apply_rrf_fusion(vector_results, lexical_results, result_map);
                break;
            case FusionMethod::WeightedSum:
                apply_weighted_sum_fusion(result_map);
                break;
            case FusionMethod::CombSUM:
                apply_combsum_fusion(vector_results, lexical_results, result_map);
                break;
            case FusionMethod::CombMNZ:
                apply_combmnz_fusion(vector_results, lexical_results, result_map);
                break;
            case FusionMethod::Borda:
                apply_borda_fusion(vector_results, lexical_results, result_map);
                break;
        }
        
        // Convert to vector and sort
        std::vector<HybridResult> results;
        results.reserve(result_map.size());
        for (auto& [id, hr] : result_map) {
            results.push_back(std::move(hr));
        }
        
        std::sort(results.begin(), results.end());
        
        // Return top k
        if (results.size() > k) {
            results.resize(k);
        }
        
        return results;
    }
    
private:
    void apply_rrf_fusion(
        const std::vector<QueryResult>& vector_results,
        const std::vector<BM25Result>& lexical_results,
        std::unordered_map<VectorId, HybridResult>& result_map
    ) const {
        // Create rank maps
        std::unordered_map<VectorId, size_t> vector_ranks;
        std::unordered_map<VectorId, size_t> lexical_ranks;
        
        for (size_t i = 0; i < vector_results.size(); ++i) {
            vector_ranks[vector_results[i].id] = i + 1;
        }
        
        for (size_t i = 0; i < lexical_results.size(); ++i) {
            lexical_ranks[lexical_results[i].id] = i + 1;
        }
        
        // Calculate RRF scores
        for (auto& [id, hr] : result_map) {
            float rrf_score = 0.0f;
            
            if (vector_ranks.count(id)) {
                rrf_score += 1.0f / (config.rrf_k + vector_ranks[id]);
            }
            
            if (lexical_ranks.count(id)) {
                rrf_score += 1.0f / (config.rrf_k + lexical_ranks[id]);
            }
            
            hr.combined_score = rrf_score;
        }
    }
    
    void apply_weighted_sum_fusion(
        std::unordered_map<VectorId, HybridResult>& result_map
    ) const {
        // Normalize scores to [0, 1]
        float max_vector_score = 0.0f;
        float max_lexical_score = 0.0f;
        
        for (const auto& [id, hr] : result_map) {
            max_vector_score = std::max(max_vector_score, hr.vector_score);
            max_lexical_score = std::max(max_lexical_score, hr.lexical_score);
        }
        
        // Avoid division by zero
        if (max_vector_score == 0.0f) max_vector_score = 1.0f;
        if (max_lexical_score == 0.0f) max_lexical_score = 1.0f;
        
        // Calculate weighted sum
        for (auto& [id, hr] : result_map) {
            float norm_vector = hr.vector_score / max_vector_score;
            float norm_lexical = hr.lexical_score / max_lexical_score;
            hr.combined_score = config.vector_weight * norm_vector + 
                              config.lexical_weight * norm_lexical;
        }
    }
    
    void apply_combsum_fusion(
        const std::vector<QueryResult>& vector_results,
        const std::vector<BM25Result>& lexical_results,
        std::unordered_map<VectorId, HybridResult>& result_map
    ) const {
        // Normalize scores
        float max_vector_score = 0.0f;
        float max_lexical_score = 0.0f;
        
        for (const auto& vr : vector_results) {
            max_vector_score = std::max(max_vector_score, vr.score);
        }
        
        for (const auto& lr : lexical_results) {
            max_lexical_score = std::max(max_lexical_score, lr.score);
        }
        
        if (max_vector_score == 0.0f) max_vector_score = 1.0f;
        if (max_lexical_score == 0.0f) max_lexical_score = 1.0f;
        
        // Sum normalized scores
        for (auto& [id, hr] : result_map) {
            hr.combined_score = (hr.vector_score / max_vector_score) + 
                              (hr.lexical_score / max_lexical_score);
        }
    }
    
    void apply_combmnz_fusion(
        const std::vector<QueryResult>& vector_results,
        const std::vector<BM25Result>& lexical_results,
        std::unordered_map<VectorId, HybridResult>& result_map
    ) const {
        // First apply CombSUM
        apply_combsum_fusion(vector_results, lexical_results, result_map);
        
        // Then multiply by number of systems that returned the document
        for (auto& [id, hr] : result_map) {
            size_t num_systems = 0;
            if (hr.vector_score > 0.0f) num_systems++;
            if (hr.lexical_score > 0.0f) num_systems++;
            hr.combined_score *= num_systems;
        }
    }
    
    void apply_borda_fusion(
        const std::vector<QueryResult>& vector_results,
        const std::vector<BM25Result>& lexical_results,
        std::unordered_map<VectorId, HybridResult>& result_map
    ) const {
        size_t n_vector = vector_results.size();
        size_t n_lexical = lexical_results.size();
        
        // Create rank maps
        std::unordered_map<VectorId, size_t> vector_ranks;
        std::unordered_map<VectorId, size_t> lexical_ranks;
        
        for (size_t i = 0; i < vector_results.size(); ++i) {
            vector_ranks[vector_results[i].id] = i;
        }
        
        for (size_t i = 0; i < lexical_results.size(); ++i) {
            lexical_ranks[lexical_results[i].id] = i;
        }
        
        // Calculate Borda count
        for (auto& [id, hr] : result_map) {
            float borda_score = 0.0f;
            
            if (vector_ranks.count(id)) {
                borda_score += (n_vector - vector_ranks[id]);
            }
            
            if (lexical_ranks.count(id)) {
                borda_score += (n_lexical - lexical_ranks[id]);
            }
            
            hr.combined_score = borda_score;
        }
    }
};

// ============================================================================
// HybridSearchEngine Public Interface
// ============================================================================

HybridSearchEngine::HybridSearchEngine(const HybridSearchConfig& config)
    : impl_(std::make_unique<Impl>(config)) {
}

HybridSearchEngine::~HybridSearchEngine() = default;

Result<std::vector<HybridResult>> HybridSearchEngine::combine(
    const std::vector<QueryResult>& vector_results,
    const std::vector<BM25Result>& lexical_results,
    size_t k
) const {
    return impl_->combine(vector_results, lexical_results, k);
}

// Static fusion method implementations
float HybridSearchEngine::weighted_sum(float vec_score, float lex_score, float vec_weight) {
    float lex_weight = 1.0f - vec_weight;
    return vec_weight * vec_score + lex_weight * lex_score;
}

float HybridSearchEngine::reciprocal_rank_fusion(size_t vec_rank, size_t lex_rank, size_t k) {
    return (1.0f / (k + vec_rank)) + (1.0f / (k + lex_rank));
}

float HybridSearchEngine::comb_sum(float vec_score, float lex_score) {
    return vec_score + lex_score;
}

float HybridSearchEngine::comb_mnz(float vec_score, float lex_score, size_t num_systems) {
    return (vec_score + lex_score) * num_systems;
}

} // namespace hybrid
} // namespace vdb
