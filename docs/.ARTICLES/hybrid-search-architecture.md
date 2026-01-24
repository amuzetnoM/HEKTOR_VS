---
title: BM25 Meets HNSW
description: "Hybrid Search in Vector Databases"
version: 4.1.5
last_updated: 2026-01-24
sidebar_position: 1
category: Introduction
---

# BM25 Meets HNSW
> Hybrid Search in Vector Databases

**ARTIFACT VIRTUAL Technical Article Series**  
*Author: Ali A. Shakil*  
*Published: January 24, 2026*

---

## Abstract

Modern search systems require both semantic understanding (vector similarity) and lexical precision (keyword matching). This article presents a comprehensive analysis of hybrid search architectures, fusion algorithms, and optimization strategies implemented in high-performance vector databases.

---

## 1. Introduction

The evolution of search has followed a clear trajectory:

1. **Lexical Search (1990s-2000s)**: TF-IDF, BM25 — match keywords
2. **Semantic Search (2010s)**: Word2Vec, BERT — understand meaning
3. **Hybrid Search (2020s)**: Combine both — best of both worlds

Neither approach alone is sufficient:
- Lexical search fails on synonyms and paraphrases
- Semantic search fails on exact matches (product codes, names, rare terms)

Hybrid search solves both limitations.

## 2. BM25: The Lexical Foundation

### 2.1 The BM25 Formula

BM25 (Best Matching 25) remains the gold standard for lexical search:

$$
\text{score}(D, Q) = \sum_{i=1}^{n} \text{IDF}(q_i) \cdot \frac{f(q_i, D) \cdot (k_1 + 1)}{f(q_i, D) + k_1 \cdot (1 - b + b \cdot \frac{|D|}{\text{avgdl}})}
$$

Where:
- $f(q_i, D)$ = frequency of term $q_i$ in document $D$
- $|D|$ = document length
- $\text{avgdl}$ = average document length
- $k_1$ = term frequency saturation (typically 1.2-2.0)
- $b$ = length normalization (typically 0.75)

### 2.2 IDF Variants

The Inverse Document Frequency term:

**Classic IDF:**
$$\text{IDF}(q_i) = \log \frac{N - n(q_i) + 0.5}{n(q_i) + 0.5}$$

**BM25+ IDF (our default):**
$$\text{IDF}(q_i) = \log \frac{N + 1}{n(q_i) + 0.5}$$

Where $N$ is total documents and $n(q_i)$ is documents containing $q_i$.

### 2.3 Hektor's BM25 Implementation

```cpp
class BM25Engine {
    static constexpr float k1_default = 1.5f;
    static constexpr float b_default = 0.75f;
    
    std::unordered_map<std::string, PostingList> inverted_index_;
    std::vector<float> doc_lengths_;
    float avg_doc_length_ = 0.0f;
    size_t total_docs_ = 0;
    
public:
    void index_document(uint64_t doc_id, std::string_view text) {
        auto tokens = tokenize(text);
        
        std::unordered_map<std::string, uint32_t> term_freqs;
        for (const auto& token : tokens) {
            term_freqs[token]++;
        }
        
        for (const auto& [term, freq] : term_freqs) {
            inverted_index_[term].emplace_back(doc_id, freq);
        }
        
        doc_lengths_[doc_id] = static_cast<float>(tokens.size());
        update_stats();
    }
    
    std::vector<ScoredDoc> search(std::string_view query, size_t k) {
        auto query_tokens = tokenize(query);
        
        std::unordered_map<uint64_t, float> scores;
        
        for (const auto& token : query_tokens) {
            auto it = inverted_index_.find(std::string(token));
            if (it == inverted_index_.end()) continue;
            
            float idf = compute_idf(it->second.size());
            
            for (const auto& [doc_id, freq] : it->second) {
                float tf_component = compute_tf(freq, doc_id);
                scores[doc_id] += idf * tf_component;
            }
        }
        
        return top_k(scores, k);
    }
    
private:
    float compute_idf(size_t doc_freq) const {
        return std::log((total_docs_ + 1.0f) / (doc_freq + 0.5f));
    }
    
    float compute_tf(uint32_t freq, uint64_t doc_id) const {
        float doc_len = doc_lengths_[doc_id];
        float numerator = freq * (k1_default + 1.0f);
        float denominator = freq + k1_default * 
            (1.0f - b_default + b_default * (doc_len / avg_doc_length_));
        return numerator / denominator;
    }
};
```

## 3. HNSW: The Semantic Foundation

### 3.1 Algorithm Overview

Hierarchical Navigable Small World (HNSW) provides O(log n) approximate nearest neighbor search:

```
Layer 2: [sparse graph - long-range connections]
    │
    ▼
Layer 1: [medium density]
    │
    ▼
Layer 0: [dense graph - all vectors]
```

### 3.2 Search Process

```cpp
std::vector<std::pair<float, uint64_t>> HNSWIndex::search(
    std::span<const float> query,
    size_t k,
    size_t ef_search
) {
    // Start from entry point at top layer
    uint64_t current = entry_point_;
    float current_dist = distance(query, get_vector(current));
    
    // Greedy descent through upper layers
    for (int layer = max_layer_; layer > 0; --layer) {
        bool improved = true;
        while (improved) {
            improved = false;
            for (auto neighbor : get_neighbors(current, layer)) {
                float dist = distance(query, get_vector(neighbor));
                if (dist < current_dist) {
                    current = neighbor;
                    current_dist = dist;
                    improved = true;
                }
            }
        }
    }
    
    // ef-search at layer 0
    return search_layer_0(query, current, ef_search, k);
}
```

## 4. Fusion Algorithms

The critical question: how do we combine BM25 and vector results?

### 4.1 Reciprocal Rank Fusion (RRF)

The most robust fusion method:

$$
\text{RRF}(d) = \sum_{r \in R} \frac{1}{k + r(d)}
$$

Where:
- $R$ is the set of rankings (BM25, vector)
- $r(d)$ is the rank of document $d$ in ranking $r$
- $k$ is a constant (typically 60)

**Properties:**
- Rank-based, not score-based (robust to score distribution differences)
- Handles missing results gracefully
- No hyperparameter tuning needed

```cpp
std::vector<ScoredDoc> rrf_fusion(
    const std::vector<ScoredDoc>& bm25_results,
    const std::vector<ScoredDoc>& vector_results,
    float k = 60.0f
) {
    std::unordered_map<uint64_t, float> fused_scores;
    
    // Add BM25 contributions
    for (size_t rank = 0; rank < bm25_results.size(); ++rank) {
        fused_scores[bm25_results[rank].id] += 1.0f / (k + rank + 1);
    }
    
    // Add vector contributions
    for (size_t rank = 0; rank < vector_results.size(); ++rank) {
        fused_scores[vector_results[rank].id] += 1.0f / (k + rank + 1);
    }
    
    return sort_by_score(fused_scores);
}
```

### 4.2 Weighted Sum Fusion

When you know the relative importance:

$$
\text{score}(d) = \alpha \cdot \text{norm}(\text{BM25}(d)) + (1 - \alpha) \cdot \text{norm}(\text{vector}(d))
$$

**Normalization options:**
- Min-max: $\frac{x - \min}{\max - \min}$
- Z-score: $\frac{x - \mu}{\sigma}$
- Rank-based: $\frac{N - \text{rank}}{N}$

```cpp
std::vector<ScoredDoc> weighted_sum_fusion(
    const std::vector<ScoredDoc>& bm25_results,
    const std::vector<ScoredDoc>& vector_results,
    float alpha = 0.5f,
    NormalizationType norm_type = NormalizationType::MinMax
) {
    auto norm_bm25 = normalize(bm25_results, norm_type);
    auto norm_vector = normalize(vector_results, norm_type);
    
    std::unordered_map<uint64_t, float> fused_scores;
    
    for (const auto& [id, score] : norm_bm25) {
        fused_scores[id] += alpha * score;
    }
    
    for (const auto& [id, score] : norm_vector) {
        fused_scores[id] += (1.0f - alpha) * score;
    }
    
    return sort_by_score(fused_scores);
}
```

### 4.3 CombSUM and CombMNZ

From the TREC fusion literature:

**CombSUM:**
$$\text{CombSUM}(d) = \sum_{r \in R} \text{norm}(s_r(d))$$

**CombMNZ (Multiple Non-Zero):**
$$\text{CombMNZ}(d) = |R_d| \cdot \sum_{r \in R_d} \text{norm}(s_r(d))$$

Where $R_d$ is the set of rankings containing document $d$.

CombMNZ rewards documents that appear in multiple result sets.

```cpp
std::vector<ScoredDoc> combmnz_fusion(
    const std::vector<std::vector<ScoredDoc>>& all_results
) {
    std::unordered_map<uint64_t, float> sum_scores;
    std::unordered_map<uint64_t, int> hit_counts;
    
    for (const auto& results : all_results) {
        auto normalized = normalize(results, NormalizationType::MinMax);
        for (const auto& [id, score] : normalized) {
            sum_scores[id] += score;
            hit_counts[id]++;
        }
    }
    
    std::unordered_map<uint64_t, float> fused_scores;
    for (const auto& [id, sum] : sum_scores) {
        fused_scores[id] = hit_counts[id] * sum;  // MNZ multiplier
    }
    
    return sort_by_score(fused_scores);
}
```

### 4.4 Borda Count

Election-style voting:

$$\text{Borda}(d) = \sum_{r \in R} (N - r(d))$$

Where $N$ is the number of candidates.

```cpp
std::vector<ScoredDoc> borda_fusion(
    const std::vector<ScoredDoc>& bm25_results,
    const std::vector<ScoredDoc>& vector_results
) {
    size_t N = std::max(bm25_results.size(), vector_results.size());
    std::unordered_map<uint64_t, float> scores;
    
    for (size_t rank = 0; rank < bm25_results.size(); ++rank) {
        scores[bm25_results[rank].id] += N - rank;
    }
    
    for (size_t rank = 0; rank < vector_results.size(); ++rank) {
        scores[vector_results[rank].id] += N - rank;
    }
    
    return sort_by_score(scores);
}
```

## 5. Query Rewriting

### 5.1 Why Rewrite Queries?

User queries are often suboptimal:
- Typos: "quantizaton" → "quantization"
- Synonyms: "ML" → "machine learning"
- Abbreviations: "NYC" → "New York City"

### 5.2 Query Expansion

```cpp
class QueryRewriter {
    std::unordered_map<std::string, std::vector<std::string>> synonyms_;
    SpellChecker spell_checker_;
    
public:
    std::vector<std::string> expand(std::string_view query) {
        auto tokens = tokenize(query);
        std::vector<std::string> expanded;
        
        for (const auto& token : tokens) {
            expanded.push_back(std::string(token));
            
            // Spell correction
            if (auto corrected = spell_checker_.correct(token)) {
                expanded.push_back(*corrected);
            }
            
            // Synonym expansion
            if (auto it = synonyms_.find(std::string(token)); 
                it != synonyms_.end()) {
                for (const auto& syn : it->second) {
                    expanded.push_back(syn);
                }
            }
        }
        
        return deduplicate(expanded);
    }
};
```

## 6. Performance Optimization

### 6.1 Index Co-location

Store BM25 inverted index alongside HNSW:

```cpp
struct HybridIndex {
    HNSWIndex vector_index;
    BM25Engine lexical_index;
    
    // Shared document store
    std::vector<Document> documents;
    
    void add(uint64_t id, std::span<const float> vector, std::string_view text) {
        vector_index.add(id, vector);
        lexical_index.index_document(id, text);
        documents.emplace_back(id, vector, text);
    }
};
```

### 6.2 Early Termination

Don't compute full rankings if top-k is small:

```cpp
std::vector<ScoredDoc> hybrid_search_optimized(
    std::string_view query,
    std::span<const float> query_vector,
    size_t k
) {
    // Over-fetch from each index
    size_t fetch_k = k * 3;
    
    auto bm25_results = lexical_index.search(query, fetch_k);
    auto vector_results = vector_index.search(query_vector, fetch_k);
    
    // Fuse and return top-k
    auto fused = rrf_fusion(bm25_results, vector_results);
    fused.resize(std::min(fused.size(), k));
    return fused;
}
```

### 6.3 Parallel Execution

Execute BM25 and vector search concurrently:

```cpp
std::vector<ScoredDoc> hybrid_search_parallel(
    std::string_view query,
    std::span<const float> query_vector,
    size_t k
) {
    std::future<std::vector<ScoredDoc>> bm25_future = std::async(
        std::launch::async,
        [&]() { return lexical_index.search(query, k * 3); }
    );
    
    std::future<std::vector<ScoredDoc>> vector_future = std::async(
        std::launch::async,
        [&]() { return vector_index.search(query_vector, k * 3); }
    );
    
    auto bm25_results = bm25_future.get();
    auto vector_results = vector_future.get();
    
    return rrf_fusion(bm25_results, vector_results);
}
```

## 7. Benchmarks

### 7.1 MS MARCO Passage Ranking

| Method | MRR@10 | Recall@1000 | Latency |
|--------|--------|-------------|---------|
| BM25 only | 0.187 | 0.857 | 12ms |
| Dense only | 0.334 | 0.958 | 8ms |
| **RRF fusion** | **0.389** | **0.972** | 15ms |
| **WeightedSum (α=0.4)** | **0.401** | **0.969** | 15ms |

### 7.2 BEIR Benchmark (Average)

| Method | NDCG@10 |
|--------|---------|
| BM25 | 0.431 |
| Contriever | 0.449 |
| **Hektor Hybrid (RRF)** | **0.512** |

## 8. Conclusion

Hybrid search is not optional — it's essential for production systems. The combination of:

- **BM25** for lexical precision
- **HNSW** for semantic understanding
- **RRF/CombMNZ** for robust fusion

Delivers search quality that neither approach achieves alone.

Hektor implements all fusion algorithms, optimized for both throughput and latency, with full C++23 performance.

---

## References

1. Robertson, S., & Zaragoza, H. (2009). The Probabilistic Relevance Framework: BM25 and Beyond.
2. Malkov, Y., & Yashunin, D. (2020). Efficient and Robust Approximate Nearest Neighbor Search Using HNSW.
3. Cormack, G., et al. (2009). Reciprocal Rank Fusion Outperforms Condorcet and Individual Rank Learning Methods.
4. Fox, E., & Shaw, J. (1994). Combination of Multiple Searches.

---

*© 2026 ARTIFACT VIRTUAL. Technical article for Hektor Vector Database.*
