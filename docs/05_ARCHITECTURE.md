---
title: "Architecture"
description: "System design, components, and data flow"
version: "4.0.0"
last_updated: "2026-01-22"
sidebar_position: 5
category: "Technical"
---

![Version](https://img.shields.io/badge/version-4.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--22-green?style=flat-square)
![Type](https://img.shields.io/badge/type-technical-red?style=flat-square)

## Table of Contents

1. [Overview](#overview)
2. [Design Philosophy](#design-philosophy)
3. [System Components](#system-components)
4. [Data Flow](#data-flow)
5. [Memory Layout](#memory-layout)
6. [Threading Model](#threading-model)
7. [Storage Architecture](#storage-architecture)
8. [Index Structures](#index-structures)
9. [Embedding Pipeline](#embedding-pipeline)
10. [API Design](#api-design)
11. [Error Handling](#error-handling)
12. [Performance Characteristics](#performance-characteristics)

---

## Overview

Vector Studio is a high-performance vector database and AI training platform engineered for semantic search and machine learning applications. The system combines three critical capabilities:

1. **Vector Storage**: Efficient persistence of high-dimensional vectors with metadata
2. **Similarity Search**: Sub-millisecond approximate nearest neighbor queries
3. **Embedding Generation**: Local inference for text and image embeddings

### Core Metrics

| Metric | Target | Achieved |
|--------|--------|----------|
| Query latency (k=10, 1M vectors) | < 5ms | ~3ms |
| Insertion throughput | > 5,000/sec | ~10,000/sec |
| Memory efficiency | < 2KB/vector | ~1.2KB/vector |
| Index build time (1M vectors) | < 5 min | ~3 min |

---

## Design Philosophy

### Principles

1. **Zero-Copy Where Possible**
   - Memory-mapped files for vector storage
   - View-based APIs to avoid unnecessary copies
   - Direct SIMD operations on mapped memory

2. **Fail-Fast with Recovery**
   - Validate inputs at API boundaries
   - Atomic file operations with rollback
   - Checksums for data integrity

3. **Predictable Performance**
   - O(log n) search complexity via HNSW
   - Bounded memory usage with capacity limits
   - Configurable quality/speed tradeoffs

4. **Local-First**
   - No network dependencies for core operations
   - Embedded ONNX models for inference
   - Works offline after initial model download

### Non-Goals

- Distributed multi-node deployment (use dedicated solutions)
- Real-time streaming ingestion (batch-oriented)
- Sub-microsecond latencies (targets are millisecond-scale)

---

## System Components

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                              VECTOR STUDIO ENGINE                               │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  ┌───────────────────────────────────────────────────────────────────────────┐  │
│  │                           PUBLIC API LAYER                                │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │  │
│  │  │ VectorDB    │  │ QueryAPI    │  │ IngestAPI   │  │ ExportAPI   │      │  │
│  │  │ (CRUD ops)  │  │ (search)    │  │ (batch add) │  │ (training)  │      │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘      │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
│                                      │                                          │
│  ┌───────────────────────────────────▼───────────────────────────────────────┐  │
│  │                          EMBEDDING LAYER                                  │  │
│  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────────┐    │  │
│  │  │   TextEncoder    │  │   ImageEncoder   │  │   ProjectionLayer    │    │  │
│  │  │  MiniLM-L6-v2    │  │   CLIP ViT-B/32  │  │   384 → 512 dim      │    │  │
│  │  │  ONNX Runtime    │  │   ONNX Runtime   │  │   Linear transform   │    │  │
│  │  └────────┬─────────┘  └────────┬─────────┘  └──────────┬───────────┘    │  │
│  │           │    Tokenization     │    Preprocessing      │                 │  │
│  │           └─────────────────────┴────────────────────────┘                 │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
│                                      │                                          │
│  ┌───────────────────────────────────▼───────────────────────────────────────┐  │
│  │                           INDEX LAYER                                     │  │
│  │  ┌──────────────────────────────────────────────────────────────────┐    │  │
│  │  │                        HNSW Index                                │    │  │
│  │  │  • Multi-layer navigable small world graph                       │    │  │
│  │  │  • Layer 0: All vectors (densest)                                │    │  │
│  │  │  • Layer L: exp(-L) probability (sparser)                        │    │  │
│  │  │  • M connections per node, ef_construction quality               │    │  │
│  │  └──────────────────────────────────────────────────────────────────┘    │  │
│  │  ┌──────────────────────────────────────────────────────────────────┐    │  │
│  │  │                     Distance Functions                           │    │  │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐         │    │  │
│  │  │  │ AVX-512  │  │  AVX2    │  │  SSE4.1  │  │  Scalar  │         │    │  │
│  │  │  │ 16 floats│  │ 8 floats │  │ 4 floats │  │ fallback │         │    │  │
│  │  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘         │    │  │
│  │  └──────────────────────────────────────────────────────────────────┘    │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
│                                      │                                          │
│  ┌───────────────────────────────────▼───────────────────────────────────────┐  │
│  │                          STORAGE LAYER                                    │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐   │  │
│  │  │   VectorStore   │  │  MetadataStore  │  │   IndexPersistence      │   │  │
│  │  │   vectors.bin   │  │  metadata.jsonl │  │   index.hnsw            │   │  │
│  │  │   mmap I/O      │  │  append-only    │  │   binary format         │   │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────────────┘   │  │
│  │                                                                           │  │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │  │
│  │  │                   Memory-Mapped File Layer                          │ │  │
│  │  │  • Windows: CreateFileMapping / MapViewOfFile                       │ │  │
│  │  │  • POSIX: mmap / munmap                                             │ │  │
│  │  │  • Automatic growth with remap                                      │ │  │
│  │  └─────────────────────────────────────────────────────────────────────┘ │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## Data Flow

### Insertion Pipeline

```
Input Document/Image
        │
        ▼
┌───────────────────┐
│  Content Analysis │ ─── Detect type, extract date, parse metadata
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│    Tokenization   │ ─── WordPiece (text) or Resize/Normalize (image)
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│   ONNX Inference  │ ─── Forward pass through encoder model
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│    Projection     │ ─── 384→512 dim linear transform (text only)
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│   Normalization   │ ─── L2 normalize to unit sphere
└─────────┬─────────┘
          │
          ├──────────────────────────────────────┐
          ▼                                      ▼
┌───────────────────┐                  ┌───────────────────┐
│   HNSW Insert     │                  │  Storage Write    │
│  Update graph     │                  │  Append vector    │
│  Add connections  │                  │  Write metadata   │
└───────────────────┘                  └───────────────────┘
```

### Query Pipeline

```
Query Text/Image
        │
        ▼
┌───────────────────┐
│  Embed Query      │ ─── Same pipeline as insertion
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│   HNSW Search     │ ─── Navigate graph layers
│   ef_search=50    │     Return k nearest candidates
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│  Distance Refine  │ ─── Exact distance calculation
│  SIMD accelerated │     for candidate set
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│  Metadata Filter  │ ─── Apply type/date/asset filters
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│  Result Assembly  │ ─── Package scores + metadata
└─────────┬─────────┘
          │
          ▼
    QueryResults
```

---

## Memory Layout

### Vector Storage Format

```
vectors.bin:
┌────────────────────────────────────────────────────────────────┐
│ Header (64 bytes)                                              │
├────────────────────────────────────────────────────────────────┤
│ Magic:     "VSTUDIO1" (8 bytes)                                │
│ Version:   1 (uint32)                                          │
│ Dimension: 512 (uint32)                                        │
│ Count:     N (uint64)                                          │
│ Capacity:  M (uint64)                                          │
│ Checksum:  CRC32 (uint32)                                      │
│ Reserved:  (28 bytes)                                          │
├────────────────────────────────────────────────────────────────┤
│ Vector 0: [float32 × 512] = 2048 bytes                         │
│ Vector 1: [float32 × 512] = 2048 bytes                         │
│ Vector 2: [float32 × 512] = 2048 bytes                         │
│ ...                                                            │
│ Vector N-1: [float32 × 512] = 2048 bytes                       │
├────────────────────────────────────────────────────────────────┤
│ Free space for growth (up to capacity M)                       │
└────────────────────────────────────────────────────────────────┘

Total size = 64 + (N × 512 × 4) bytes
1M vectors ≈ 2 GB
```

### HNSW Index Format

```
index.hnsw:
┌────────────────────────────────────────────────────────────────┐
│ Header                                                         │
├────────────────────────────────────────────────────────────────┤
│ Magic:            "HNSW0001" (8 bytes)                         │
│ M:                16 (uint32) - max connections per node       │
│ M_max0:           32 (uint32) - max connections at layer 0     │
│ ef_construction:  200 (uint32) - build quality                 │
│ max_level:        L (uint32) - maximum layer number            │
│ entry_point:      id (uint64) - entry node for search          │
│ element_count:    N (uint64)                                   │
├────────────────────────────────────────────────────────────────┤
│ Level Assignment Table                                         │
│ [level_0, level_1, ..., level_N-1] (N × uint8)                 │
├────────────────────────────────────────────────────────────────┤
│ Adjacency Lists (per level, per node)                          │
│ For each level l in [0, max_level]:                            │
│   For each node with level >= l:                               │
│     neighbor_count (uint16)                                    │
│     neighbors[M] (uint64 × M)                                  │
└────────────────────────────────────────────────────────────────┘
```

### Metadata Format

```
metadata.jsonl (newline-delimited JSON):

{"id":1,"type":"Journal","date":"2025-12-01","bias":"BULLISH","gold_price":4220.5}
{"id":2,"type":"Chart","date":"2025-12-01","asset":"GOLD","source_file":"GOLD.png"}
{"id":3,"type":"CatalystWatchlist","date":"2025-12-01"}
...
```

---

## Threading Model

### Thread Safety Guarantees

| Operation | Lock Type | Concurrent Access |
|-----------|-----------|-------------------|
| Read (search, get) | Shared (read) | Multiple readers OK |
| Write (add, remove) | Exclusive (write) | Single writer only |
| Index build | Exclusive | No concurrent access |
| Metadata read | Lock-free | Always safe |

### Thread Pool Architecture

```cpp
class ThreadPool {
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;
    
public:
    // Submit task and get future
    template<typename F>
    auto submit(F&& f) -> std::future<std::invoke_result_t<F>>;
    
    // Parallel for over range [0, count)
    template<typename F>
    void parallel_for(size_t count, F&& func);
};
```

### Parallelization Points

1. **Distance computation**: Batch queries processed in parallel
2. **HNSW search**: Multiple entry points explored concurrently
3. **Embedding batches**: Multiple documents processed together
4. **Index construction**: Parallel insertion with lock-free updates

---

## Storage Architecture

### File Organization

```
database/
├── config.json         # Database configuration
├── vectors.bin         # Memory-mapped vector storage
├── index.hnsw          # HNSW graph structure
├── metadata.jsonl      # Document metadata
├── id_map.bin          # Vector ID → file offset mapping
└── wal/                # Write-ahead log (optional)
    ├── 000001.log
    └── 000002.log
```

### Durability Guarantees

1. **Metadata**: Immediately flushed after each write
2. **Vectors**: Synced on explicit `sync()` call or auto-sync interval
3. **Index**: Saved on database close or explicit save
4. **WAL (optional)**: Enables crash recovery

### Growth Strategy

```
Initial capacity: 10,000 vectors
Growth factor: 2x
Maximum file size: 4 GB per file (multi-file for larger)

Remap procedure:
1. Acquire exclusive lock
2. Unmap current view
3. Extend file size
4. Map new view
5. Update capacity
6. Release lock
```

---

## Index Structures

### HNSW Algorithm

The Hierarchical Navigable Small World (HNSW) algorithm provides O(log n) approximate nearest neighbor search.

#### Layer Structure

```
Layer 3: ●───────────────────────●  (few nodes, long-range links)
         │                       │
Layer 2: ●───────●───────────────●───────●
         │       │               │       │
Layer 1: ●───●───●───●───────●───●───●───●───●
         │   │   │   │       │   │   │   │   │
Layer 0: ●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●  (all nodes)
```

#### Key Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| M | 16 | Max connections per node (layers > 0) |
| M_max0 | 32 | Max connections at layer 0 |
| ef_construction | 200 | Beam width during build |
| ef_search | 50 | Beam width during query |
| mL | 1/ln(M) | Level generation factor |

#### Search Algorithm

```
function SEARCH(query, k, ef):
    entry_point ← top layer entry
    for level ← max_level down to 1:
        entry_point ← SEARCH_LAYER(query, entry_point, ef=1, level)
    
    candidates ← SEARCH_LAYER(query, entry_point, ef, level=0)
    return TOP_K(candidates, k)

function SEARCH_LAYER(query, entry, ef, level):
    visited ← {entry}
    candidates ← MinHeap([entry])  # by distance
    results ← MaxHeap([entry])     # by distance
    
    while candidates not empty:
        current ← candidates.pop_min()
        if distance(current) > results.top():
            break  # all remaining are farther
        
        for neighbor in get_neighbors(current, level):
            if neighbor not in visited:
                visited.add(neighbor)
                if distance(neighbor) < results.top() or |results| < ef:
                    candidates.push(neighbor)
                    results.push(neighbor)
                    if |results| > ef:
                        results.pop_max()
    
    return results
```

### Distance Functions

#### Cosine Similarity (Primary)

Best for normalized embeddings. Measures angle between vectors.

```
cosine_similarity(a, b) = dot(a, b) / (||a|| × ||b||)
cosine_distance(a, b) = 1 - cosine_similarity(a, b)
```

#### Euclidean Distance

```
euclidean(a, b) = sqrt(sum((a[i] - b[i])²))
```

#### Dot Product

For pre-normalized vectors (equivalent to cosine).

```
dot_product(a, b) = sum(a[i] × b[i])
```

### SIMD Implementation

```cpp
// AVX2 dot product (8 floats at a time)
float dot_product_avx2(const float* a, const float* b, size_t n) {
    __m256 sum = _mm256_setzero_ps();
    
    for (size_t i = 0; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        sum = _mm256_fmadd_ps(va, vb, sum);  // fused multiply-add
    }
    
    // Horizontal sum
    __m128 hi = _mm256_extractf128_ps(sum, 1);
    __m128 lo = _mm256_castps256_ps128(sum);
    __m128 sum128 = _mm_add_ps(lo, hi);
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    
    return _mm_cvtss_f32(sum128);
}
```

---

## Embedding Pipeline

### Text Encoder (MiniLM-L6-v2)

```
Input: "Gold broke above $4,200 resistance with strong volume"
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ Tokenization (WordPiece)                                  │
│ [CLS] gold broke above $ 4 , 200 resistance ... [SEP]    │
│ Token IDs: [101, 2751, 3631, 2682, 1002, ...]            │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ Transformer Encoder (6 layers)                            │
│ • Self-attention                                          │
│ • Feed-forward                                            │
│ • Layer normalization                                     │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ Mean Pooling                                              │
│ Average all token embeddings (ignoring [PAD])             │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ Projection: 384 → 512 dimensions                          │
│ Linear layer: W[512×384] × embedding + b[512]             │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ L2 Normalization                                          │
│ embedding ← embedding / ||embedding||₂                    │
└───────────────────────────────────────────────────────────┘
        │
        ▼
Output: float[512] unit vector
```

### Image Encoder (CLIP ViT-B/32)

```
Input: 224×224 RGB image (chart)
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ Preprocessing                                             │
│ • Resize to 224×224                                       │
│ • Normalize: (pixel - mean) / std                         │
│   mean = [0.48145466, 0.4578275, 0.40821073]             │
│   std = [0.26862954, 0.26130258, 0.27577711]             │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ Patch Embedding                                           │
│ • Split into 7×7 = 49 patches of 32×32                   │
│ • Linear projection to 768 dim                            │
│ • Add positional embeddings                               │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ Vision Transformer (12 layers)                            │
│ • Multi-head self-attention                               │
│ • MLP blocks                                              │
│ • Layer normalization                                     │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ [CLS] token → Final projection                            │
│ Project 768 → 512 dimensions                              │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│ L2 Normalization                                          │
└───────────────────────────────────────────────────────────┘
        │
        ▼
Output: float[512] unit vector
```

---

## API Design

### C++ Core API

```cpp
namespace vdb {

class VectorDatabase {
public:
    // Lifecycle
    static Result<VectorDatabase> create(const DatabaseConfig& config);
    static Result<VectorDatabase> open(const fs::path& path);
    Result<void> close();
    
    // Text operations
    Result<VectorId> add_text(std::string_view text, const Metadata& meta);
    Result<QueryResults> query_text(std::string_view query, const QueryOptions& opts);
    
    // Image operations  
    Result<VectorId> add_image(const fs::path& path, const Metadata& meta);
    Result<QueryResults> query_image(const fs::path& path, const QueryOptions& opts);
    
    // Raw vector operations
    Result<VectorId> add_vector(VectorView vec, const Metadata& meta);
    Result<QueryResults> query_vector(VectorView vec, const QueryOptions& opts);
    
    // Metadata
    std::optional<Metadata> get_metadata(VectorId id) const;
    Result<void> update_metadata(VectorId id, const Metadata& meta);
    
    // Deletion
    Result<void> remove(VectorId id);
    Result<size_t> remove_by_date(std::string_view date);
    
    // Management
    size_t size() const;
    IndexStats stats() const;
    Result<void> sync();
    void optimize();
    
    // Export
    Result<void> export_training_data(const fs::path& output);
};

}  // namespace vdb
```

### Python Bindings

```python
import pyvdb

# Database lifecycle
db = pyvdb.create_gold_standard_db("./my_db")
db = pyvdb.open_database("./my_db")
db.close()

# Adding documents
meta = pyvdb.Metadata()
meta.type = pyvdb.DocumentType.Journal
meta.date = "2025-12-01"
meta.bias = "BULLISH"

doc_id = db.add_text("Gold analysis...", meta)
img_id = db.add_image("chart.png", meta)

# Searching
results = db.search("gold breakout", k=10)
results = db.query_text("resistance level", options)
results = db.query_image("query_chart.png", options)

# Query options
opts = pyvdb.QueryOptions()
opts.k = 10
opts.type_filter = pyvdb.DocumentType.Journal
opts.date_from = "2025-11-01"
opts.date_to = "2025-12-01"
opts.include_metadata = True
```

---

## Error Handling

### Error Types

```cpp
enum class ErrorCode {
    Success = 0,
    
    // I/O errors
    IoError,
    FileNotFound,
    PermissionDenied,
    DiskFull,
    
    // Data errors
    InvalidDimension,
    InvalidData,
    CorruptedIndex,
    ChecksumMismatch,
    
    // State errors
    NotInitialized,
    AlreadyExists,
    NotFound,
    InvalidState,
    
    // Resource errors
    OutOfMemory,
    CapacityExceeded,
    
    // Model errors
    ModelLoadError,
    InferenceError
};

struct Error {
    ErrorCode code;
    std::string message;
    std::source_location location;  // C++20
};

template<typename T>
using Result = std::expected<T, Error>;  // C++23
```

### Recovery Strategies

| Error | Recovery |
|-------|----------|
| CorruptedIndex | Rebuild from vectors.bin |
| ChecksumMismatch | Rollback to last checkpoint |
| OutOfMemory | Reduce batch size, retry |
| ModelLoadError | Download model, retry |

---

## Performance Characteristics

### Time Complexity

| Operation | Average | Worst Case |
|-----------|---------|------------|
| Insert | O(log n) | O(n) during resize |
| Search (k results) | O(log n) | O(n) pathological |
| Delete | O(log n) | O(M × log n) |
| Build index (n vectors) | O(n log n) | O(n²) |

### Space Complexity

| Component | Size |
|-----------|------|
| Vector storage | 512 × 4 = 2048 bytes/vector |
| HNSW index | ~200 bytes/vector average |
| Metadata | ~100 bytes/vector average |
| **Total** | **~2.4 KB/vector** |

### Benchmark Results

*Intel i7-12700H, 32GB RAM, NVMe SSD, Windows 11*

#### Insertion Performance

| Vectors | Time | Throughput |
|---------|------|------------|
| 1,000 | 0.1s | 10,000/sec |
| 10,000 | 1.2s | 8,300/sec |
| 100,000 | 15s | 6,700/sec |
| 1,000,000 | 180s | 5,500/sec |

#### Query Performance (k=10)

| Vectors | ef_search=50 | ef_search=100 | ef_search=200 |
|---------|--------------|---------------|---------------|
| 10,000 | 0.3ms | 0.5ms | 0.9ms |
| 100,000 | 0.8ms | 1.4ms | 2.5ms |
| 1,000,000 | 2.5ms | 4.2ms | 7.8ms |

#### Recall@10 (vs exact brute force)

| ef_search | Recall |
|-----------|--------|
| 50 | 95.2% |
| 100 | 98.1% |
| 200 | 99.4% |
| 500 | 99.9% |

---

## Appendix: Configuration Reference

```cpp
struct DatabaseConfig {
    fs::path path;                              // Database directory
    
    // Vector configuration
    Dim dimension = 512;                        // Vector dimensionality
    DistanceMetric metric = Cosine;             // Distance function
    
    // HNSW parameters
    size_t hnsw_m = 16;                         // Connections per node
    size_t hnsw_ef_construction = 200;          // Build quality
    size_t hnsw_ef_search = 50;                 // Default search quality
    size_t max_elements = 1'000'000;            // Capacity limit
    
    // Storage
    size_t initial_capacity = 10'000;           // Initial vector capacity
    bool memory_map = true;                     // Use memory-mapped I/O
    bool auto_sync = true;                      // Auto-persist changes
    size_t sync_interval_ms = 30'000;           // Auto-sync interval
    
    // Embedding
    fs::path models_dir;                        // ONNX model directory
    ExecutionProvider provider = Auto;          // CPU/CUDA/DirectML
    size_t num_threads = 0;                     // 0 = auto-detect
    
    // Limits
    size_t max_batch_size = 64;                 // Max embedding batch
    size_t max_text_length = 512;               // Max tokens
};
```
