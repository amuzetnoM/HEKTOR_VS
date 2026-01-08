---
title: "Hybrid Search"
description: "BM25 full-text search, fusion methods, and RAG toolkit"
version: "3.0.0"
last_updated: "2026-01-06"
sidebar_position: 10
category: "Feature"
---

![Version](https://img.shields.io/badge/version-3.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--06-green?style=flat-square)
![Type](https://img.shields.io/badge/type-theory-purple?style=flat-square)

> **What is Hybrid Search?** <BR>
Hybrid search combines **vector similarity** (semantic understanding) with **BM25 full-text search** (keyword matching) for superior retrieval accuracy.

**Benefits:**
- 15-20% accuracy improvement over vector-only
- Better handling of exact keywords
- Robust across different query types

## Quick Start (C++)

```cpp
#include "vdb/hybrid_search.hpp"

// 1. Create BM25 engine
BM25Engine bm25;
bm25.add_document(1, "Gold prices surge on inflation");
bm25.add_document(2, "Silver follows gold higher");

// 2. Search with BM25
auto lexical_results = bm25.search("gold prices", 10);

// 3. Get vector results (from your vector database)
auto vector_results = vector_db.search(query_embedding, 10);

// 4. Combine with hybrid fusion
HybridSearchEngine hybrid;
auto combined = hybrid.combine(vector_results, lexical_results, 10);

// 5. Use results
for (const auto& result : combined) {
    std::cout << "Doc " << result.id 
              << ": " << result.combined_score << "\n";
}
```

## Quick Start (Python)

```python
import pyvdb

# 1. Create BM25 engine
bm25 = pyvdb.BM25Engine()
bm25.add_document(1, "Gold prices surge on inflation")
bm25.add_document(2, "Silver follows gold higher")

# 2. Search
lexical_results = bm25.search("gold prices", k=10)

# 3. Hybrid fusion
config = pyvdb.HybridSearchConfig()
config.fusion = pyvdb.FusionMethod.RRF  # Reciprocal Rank Fusion

hybrid = pyvdb.HybridSearchEngine(config)
combined = hybrid.combine(vector_results, lexical_results, k=10)

# 4. Use results
for result in combined:
    print(f"Doc {result.id}: {result.combined_score:.3f}")
```

## Fusion Methods

### 1. RRF (Reciprocal Rank Fusion) - Recommended
```cpp
config.fusion = FusionMethod::RRF;
config.rrf_k = 60;  // Default
```
- **Best for:** General purpose
- **Pros:** Robust, no tuning needed
- **Cons:** None significant

### 2. Weighted Sum
```cpp
config.fusion = FusionMethod::WeightedSum;
config.vector_weight = 0.7;
config.lexical_weight = 0.3;
```
- **Best for:** Known importance weights
- **Pros:** Fine control
- **Cons:** Requires tuning

### 3. CombSUM
```cpp
config.fusion = FusionMethod::CombSUM;
```
- **Best for:** Equal weighting
- **Pros:** Simple, fast
- **Cons:** May need score normalization

### 4. CombMNZ
```cpp
config.fusion = FusionMethod::CombMNZ;
```
- **Best for:** Favor docs in both systems
- **Pros:** Rewards consensus
- **Cons:** Can be aggressive

### 5. Borda Count
```cpp
config.fusion = FusionMethod::Borda;
```
- **Best for:** Ranking-based fusion
- **Pros:** Democratic voting
- **Cons:** Slower than others

## Configuration

### BM25 Tuning

```cpp
BM25Config config;
config.k1 = 1.2;               // Term frequency saturation (1.2-2.0)
config.b = 0.75;               // Length normalization (0.5-0.9)
config.min_term_length = 2;    // Minimum term length
config.use_stemming = true;    // Porter stemming
config.case_sensitive = false; // Case insensitive
```

**Tuning Guidelines:**
- `k1 = 1.2`: Good default for most documents
- `k1 = 2.0`: For longer documents
- `b = 0.75`: Standard normalization
- `b = 0.0`: Disable length normalization

### Hybrid Search Tuning

```python
config = pyvdb.HybridSearchConfig()
config.vector_weight = 0.7     # 70% vector importance
config.lexical_weight = 0.3    # 30% lexical importance
config.fusion = pyvdb.FusionMethod.RRF
config.rrf_k = 60              # RRF constant (30-100)
```

## Performance

### Benchmarks

| Dataset | Vector Only | Hybrid (RRF) | Improvement |
|---------|-------------|--------------|-------------|
| 10K     | 2ms         | 7ms          | +12% acc    |
| 100K    | 5ms         | 20ms         | +15% acc    |
| 1M      | 12ms        | 62ms         | +18% acc    |

### Best Practices

1. **Index documents once:**
   ```python
   for doc_id, text in documents:
       bm25.add_document(doc_id, text)
   ```

2. **Persist index:**
   ```python
   bm25.save("index.dat")
   bm25 = pyvdb.BM25Engine.load("index.dat")
   ```

3. **Retrieve more, fuse to fewer:**
   ```python
   # Get top 20 from each
   vector_results = db.search(query, k=20)
   lexical_results = bm25.search(query, k=20)
   
   # Fuse to top 10
   combined = hybrid.combine(vector_results, lexical_results, k=10)
   ```

4. **Use RRF as default:**
   ```python
   config.fusion = pyvdb.FusionMethod.RRF
   ```

## Common Patterns

### Pattern 1: Simple Hybrid

```python
bm25 = pyvdb.BM25Engine()
hybrid = pyvdb.HybridSearchEngine()

# Index
for doc_id, text in documents:
    bm25.add_document(doc_id, text)
    
# Search
lexical = bm25.search(query_text, k=20)
vector = db.search(query_embedding, k=20)
combined = hybrid.combine(vector, lexical, k=10)
```

### Pattern 2: With Custom Weights

```python
config = pyvdb.HybridSearchConfig()
config.fusion = pyvdb.FusionMethod.WeightedSum
config.vector_weight = 0.8  # Prefer vector
config.lexical_weight = 0.2

hybrid = pyvdb.HybridSearchEngine(config)
```

### Pattern 3: Production Setup

```python
# Initialize once
bm25_config = pyvdb.BM25Config()
bm25_config.use_stemming = True
bm25 = pyvdb.BM25Engine(bm25_config)

# Load persisted index
if os.path.exists("index.dat"):
    bm25 = pyvdb.BM25Engine.load("index.dat")
else:
    # Build and save
    for doc in documents:
        bm25.add_document(doc.id, doc.text)
    bm25.save("index.dat")

# Search function
def hybrid_search(query_text, query_embedding, k=10):
    lexical = bm25.search(query_text, k=20)
    vector = vector_db.search(query_embedding, k=20)
    return hybrid.combine(vector, lexical, k)
```

## See Also

- [Advanced Features Guide](18_ADVANCED_FEATURES.md)
- [Python Examples](../examples/python_hybrid_search.py)
- [API Reference](20_API_REFERENCE.md)

## Support

- GitHub Issues: Report bugs
- Discussions: Ask questions
- Examples: See `examples/` directory
