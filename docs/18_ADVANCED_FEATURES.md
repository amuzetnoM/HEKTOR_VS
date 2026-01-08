---
title: "Advanced Features - Distributed, Hybrid, and Framework Integration"
version: "3.0.0"
last_updated: "2026-01-06"
sidebar_position: 18
---

![Version](https://img.shields.io/badge/version-3.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--06-green?style=flat-square)
![Type](https://img.shields.io/badge/type-operations-yellow?style=flat-square)

## Table of Contents

1. [Overview](#overview)
2. [Multi-Node & Distributed Features](#multi-node--distributed-features)
3. [Hybrid Search](#hybrid-search)
4. [Deep Learning Framework Integration](#deep-learning-framework-integration)
5. [RAG Toolkit](#rag-toolkit)
6. [Vector Quantization GPU Acceleration](#vector-quantization-gpu-acceleration)
7. [Performance Benchmarks](#performance-benchmarks)
8. [Best Practices](#best-practices)

---

## Overview

This document covers the advanced features added in Vector Studio v2.2, including:

- **Multi-node support** with replication and sharding
- **Hybrid search** combining vector and lexical search
- **Deep learning framework integration** (TensorFlow, PyTorch)
- **RAG toolkit** with LangChain and LlamaIndex adapters
- **GPU acceleration** for vector quantization
- **Production-grade distributed features**

---

## Multi-Node & Distributed Features

### Replication

Vector Studio supports three replication modes for high availability:

#### Async Replication

Fire-and-forget replication with minimal latency impact.

```cpp
#include "vdb/replication.hpp"

using namespace vdb;

// Configure async replication
ReplicationConfig config;
config.mode = ReplicationMode::Async;
config.min_replicas = 2;

// Add replica nodes
config.nodes = {
 {.node_id = "primary", .host = "localhost", .port = 8080, .is_primary = true, .priority = 100},
 {.node_id = "replica1", .host = "localhost", .port = 8081, .is_primary = false, .priority = 50},
 {.node_id = "replica2", .host = "localhost", .port = 8082, .is_primary = false, .priority = 50}
};

// Create replication manager
ReplicationManager replication(config);
replication.start();

// Operations are automatically replicated
replication.replicate_add(id, vector, metadata);
```

#### Sync Replication

Synchronous replication ensuring data durability at the cost of latency.

```cpp
config.mode = ReplicationMode::Sync;
config.sync_timeout_ms = 5000; // Wait up to 5 seconds
config.min_replicas = 2; // Must replicate to at least 2 nodes

ReplicationManager replication(config);
replication.start();

// Blocks until replicated to min_replicas
auto result = replication.replicate_add(id, vector, metadata);
if (!result) {
 std::cerr << "Replication failed: " << result.error().message << "\n";
}
```

#### Semi-Sync Replication

Balance between performance and durability.

```cpp
config.mode = ReplicationMode::SemiSync;
config.min_replicas = 1; // Only need one replica to acknowledge
```

### Sharding

Distribute data across multiple nodes for horizontal scalability.

#### Hash-Based Sharding

```cpp
#include "vdb/replication.hpp"

using namespace vdb;

ShardingConfig shard_config;
shard_config.strategy = ShardingStrategy::Hash;
shard_config.num_shards = 4;

// Define shards
shard_config.shards = {
 {.shard_id = "shard0", .replicas = {primary1, replica1}},
 {.shard_id = "shard1", .replicas = {primary2, replica2}},
 {.shard_id = "shard2", .replicas = {primary3, replica3}},
 {.shard_id = "shard3", .replicas = {primary4, replica4}}
};

ShardingManager sharding(shard_config);
sharding.start();

// Automatically routes to correct shard
auto shard = sharding.get_shard_for_id(vector_id);
```

#### Range-Based Sharding

```cpp
shard_config.strategy = ShardingStrategy::Range;

shard_config.shards = {
 {.shard_id = "shard0", .start_range = 0, .end_range = 250000},
 {.shard_id = "shard1", .start_range = 250000, .end_range = 500000},
 {.shard_id = "shard2", .start_range = 500000, .end_range = 750000},
 {.shard_id = "shard3", .start_range = 750000, .end_range = 1000000}
};
```

#### Consistent Hashing

For dynamic resharding without massive data movement.

```cpp
shard_config.strategy = ShardingStrategy::Consistent;
shard_config.enable_auto_resharding = true;
shard_config.reshard_threshold_items = 1000000;
shard_config.reshard_threshold_imbalance = 0.2f; // 20%
```

### Distributed Vector Database

Combine replication and sharding for a fully distributed system.

```cpp
#include "vdb/replication.hpp"

using namespace vdb;

// Configure replication and sharding
ReplicationConfig repl_config;
repl_config.mode = ReplicationMode::Async;
repl_config.min_replicas = 2;

ShardingConfig shard_config;
shard_config.strategy = ShardingStrategy::Hash;
shard_config.num_shards = 4;

// Create distributed database
DistributedVectorDatabase dist_db(repl_config, shard_config);
dist_db.init(512, DistanceMetric::Cosine);

// Use like a regular database
auto id = dist_db.add(vector, metadata);
auto results = dist_db.search(query, 10);

// Cluster management
dist_db.add_node(new_node_config);
bool healthy = dist_db.is_cluster_healthy();
```

### Failover and High Availability

```cpp
// Set up failover callback
replication.set_failover_callback([](const std::string& new_primary) {
 LOG_INFO("Failover to new primary: " + new_primary);
 // Reconfigure clients to point to new primary
});

// Manual failover trigger
if (!replication.is_healthy()) {
 replication.trigger_failover();
}

// Promote replica to primary
replication.promote_to_primary();
```

---

## Hybrid Search

Combine vector similarity with lexical/full-text search for best results.

### BM25 Full-Text Search

```cpp
#include "vdb/hybrid_search.hpp"

using namespace vdb::hybrid;

// Create BM25 engine
BM25Config bm25_config;
bm25_config.k1 = 1.2f; // Term frequency saturation
bm25_config.b = 0.75f; // Length normalization
bm25_config.use_stemming = true;

BM25Engine bm25(bm25_config);

// Index documents
bm25.add_document(1, "Gold prices surge on inflation fears");
bm25.add_document(2, "Silver follows gold higher");
bm25.add_document(3, "Dollar weakens against precious metals");

// Search
auto lexical_results = bm25.search("gold silver prices", 10);

for (const auto& result : *lexical_results) {
 std::cout << "Doc " << result.id << ": " << result.score << "\n";
 std::cout << "Matched terms: ";
 for (const auto& term : result.matched_terms) {
 std::cout << term << " ";
 }
 std::cout << "\n";
}
```

### Hybrid Search Combiner

Combine vector and lexical results using various fusion methods.

```cpp
#include "vdb/hybrid_search.hpp"

using namespace vdb::hybrid;

HybridSearchConfig hybrid_config;
hybrid_config.vector_weight = 0.7f;
hybrid_config.lexical_weight = 0.3f;
hybrid_config.fusion = FusionMethod::RRF; // Reciprocal Rank Fusion

HybridSearchEngine hybrid(hybrid_config);

// Get results from both engines
auto vector_results = vector_db.search(query_vector, 20);
auto lexical_results = bm25.search(query_text, 20);

// Combine results
auto combined = hybrid.combine(*vector_results, *lexical_results, 10);

for (const auto& result : *combined) {
 std::cout << "ID: " << result.id << "\n";
 std::cout << "Combined score: " << result.combined_score << "\n";
 std::cout << "Vector score: " << result.vector_score << "\n";
 std::cout << "Lexical score: " << result.lexical_score << "\n";
}
```

### Fusion Methods

#### Reciprocal Rank Fusion (RRF)

```cpp
hybrid_config.fusion = FusionMethod::RRF;
hybrid_config.rrf_k = 60; // RRF constant
```

**Formula**: `score = Σ(1 / (k + rank))`

Best for: General purpose, robust across different score distributions

#### Weighted Sum

```cpp
hybrid_config.fusion = FusionMethod::WeightedSum;
hybrid_config.vector_weight = 0.7f;
hybrid_config.lexical_weight = 0.3f;
```

**Formula**: `score = w₁ × vec_score + w₂ × lex_score`

Best for: When you know relative importance of each system

#### CombSUM

```cpp
hybrid_config.fusion = FusionMethod::CombSUM;
```

**Formula**: `score = vec_score + lex_score`

Best for: Equal weighting, score normalization built-in

### Keyword Extraction

Extract and rank important keywords from documents.

```cpp
KeywordConfig kw_config;
kw_config.max_keywords = 10;
kw_config.use_tfidf = true;
kw_config.use_position_weight = true;

KeywordExtractor extractor(kw_config);

// Train on corpus
std::vector<std::string> corpus = load_documents();
extractor.train(corpus);

// Extract keywords
auto keywords = extractor.extract("Gold and silver prices surge amid inflation fears");

for (const auto& kw : *keywords) {
 std::cout << kw.term << ": " << kw.score << " (freq=" << kw.frequency << ")\n";
}
```

### Query Rewriting

Improve search quality by expanding queries.

```cpp
RewriteConfig rewrite_config;
rewrite_config.expand_synonyms = true;
rewrite_config.add_stemmed_terms = true;

QueryRewriter rewriter(rewrite_config);

// Add synonyms
rewriter.add_synonym("gold", {"bullion", "precious metal", "au"});
rewriter.add_synonym("price", {"cost", "value", "rate"});

// Rewrite query
auto rewritten = rewriter.rewrite("gold price");
// Result: "gold price bullion cost precious metal value"
```

---

## Deep Learning Framework Integration

### TensorFlow Integration

#### Using TensorFlow Models

```cpp
#include "vdb/framework_integration.hpp"

using namespace vdb::framework;

// Load TensorFlow SavedModel
TensorFlowConfig tf_config;
tf_config.model_path = "models/my_encoder/";
tf_config.use_gpu = true;

TensorFlowEmbedder embedder(tf_config);

// Embed text
auto embedding = embedder.embed("This is a test document");

// Batch embedding
std::vector<std::string> texts = {"doc1", "doc2", "doc3"};
auto embeddings = embedder.embed_batch(texts);

// Add to database
for (size_t i = 0; i < embeddings->size(); ++i) {
 db.add((*embeddings)[i], create_metadata(texts[i]));
}
```

#### Export for TensorFlow Training

```cpp
// Export database for model training
std::vector<Vector> vectors;
std::vector<std::string> labels;

for (const auto& item : dataset) {
 vectors.push_back(item.embedding);
 labels.push_back(item.label);
}

TensorFlowEmbedder::export_for_training(
 vectors,
 labels,
 "training_data/tfrecords/"
);
```

### PyTorch Integration

#### Using PyTorch Models

```cpp
#include "vdb/framework_integration.hpp"

using namespace vdb::framework;

// Load TorchScript model
PyTorchConfig pt_config;
pt_config.model_path = "models/my_encoder.pt";
pt_config.device = "cuda"; // Use GPU
pt_config.use_half_precision = true; // FP16 for speed

PyTorchEmbedder embedder(pt_config);

// Embed text
auto embedding = embedder.embed("This is a test");

// Batch embedding (more efficient)
auto embeddings = embedder.embed_batch(texts);
```

#### Export for PyTorch Training

```cpp
// Export for contrastive learning
std::vector<std::pair<Vector, Vector>> positive_pairs;
std::vector<std::pair<Vector, Vector>> negative_pairs;

// Generate pairs from database
for (size_t i = 0; i < dataset.size(); ++i) {
 for (size_t j = i + 1; j < dataset.size(); ++j) {
 if (dataset[i].label == dataset[j].label) {
 positive_pairs.push_back({dataset[i].vec, dataset[j].vec});
 } else {
 negative_pairs.push_back({dataset[i].vec, dataset[j].vec});
 }
 }
}

TrainingExporter exporter;
exporter.export_contrastive_pairs(
 positive_pairs,
 negative_pairs,
 "training_data/pairs.jsonl"
);
```

---

## RAG Toolkit

### RAG Engine

Complete toolkit for Retrieval Augmented Generation.

```cpp
#include "vdb/framework_integration.hpp"

using namespace vdb::framework;

// Configure RAG
RAGConfig rag_config;
rag_config.top_k = 5;
rag_config.max_context_length = 2048;
rag_config.relevance_threshold = 0.7f;
rag_config.chunking_strategy = "sentence";
rag_config.chunk_size = 512;
rag_config.chunk_overlap = 50;

RAGEngine rag(rag_config);

// Chunk and index documents
std::string document = load_long_document();
auto chunks = rag.chunk_document(document);

for (const auto& chunk : *chunks) {
 auto embedding = embedder.embed(chunk);
 db.add(*embedding, create_metadata(chunk));
}

// Query with RAG
std::string query = "What is the impact of inflation on gold prices?";
auto query_embedding = embedder.embed(query);
auto search_results = db.search(*query_embedding, 10);

// Build context
auto context = rag.build_context(query, *search_results);

// Format prompt for LLM
std::string system_prompt = "You are a helpful financial analyst.";
std::string prompt = rag.format_prompt(query, *context, system_prompt);

// Send to LLM
auto response = llm.generate(prompt);
```

### LangChain Adapter

Drop-in replacement for LangChain's vector store.

```cpp
#include "vdb/framework_integration.hpp"

using namespace vdb::framework;

LangChainConfig lc_config;
lc_config.use_mmr = true; // Maximal Marginal Relevance
lc_config.mmr_lambda = 0.5f;

LangChainAdapter langchain(lc_config);

// Standard similarity search
auto docs = langchain.similarity_search("gold prices", 4);

// With scores
auto results_with_scores = langchain.similarity_search_with_score("gold", 4);

// MMR search (diversity)
auto diverse_results = langchain.max_marginal_relevance_search(
 "gold prices",
 4, // k: final results
 20, // fetch_k: initial retrieval
 0.5f // lambda: 0=diversity, 1=relevance
);

// As retriever (returns text only)
auto texts = langchain.as_retriever("gold prices", 4);
```

### LlamaIndex Adapter

Compatible with LlamaIndex framework.

```cpp
#include "vdb/framework_integration.hpp"

using namespace vdb::framework;

LlamaIndexConfig li_config;
li_config.chunk_size = 1024;
li_config.chunk_overlap = 20;

LlamaIndexAdapter llama_index(li_config);

// Build index
std::vector<std::string> documents = load_documents();
std::vector<Metadata> metadata = load_metadata();

llama_index.build_index(documents, metadata);

// Query
auto context = llama_index.query("What factors affect gold prices?", 5);

// Update index
llama_index.insert("New document about gold", metadata);
llama_index.delete_document(old_doc_id);

// Persist
llama_index.save("index.llamaindex");
auto loaded = LlamaIndexAdapter::load("index.llamaindex");
```

### Document Chunking

Smart document splitting strategies.

```cpp
ChunkingConfig chunk_config;
chunk_config.strategy = ChunkingStrategy::Semantic; // Best for RAG
chunk_config.chunk_size = 512;
chunk_config.chunk_overlap = 50;

DocumentChunker chunker(chunk_config);

// Simple chunking
auto chunks = chunker.chunk(long_document);

// With metadata preservation
auto chunks_with_meta = chunker.chunk_with_metadata(
 long_document,
 base_metadata
);

for (const auto& chunk : *chunks_with_meta) {
 std::cout << "Chunk [" << chunk.start_char << ":" << chunk.end_char << "]\n";
 std::cout << chunk.text << "\n";
 std::cout << "Metadata: " << chunk.metadata.to_json() << "\n";
}
```

---

## Vector Quantization GPU Acceleration

GPU-accelerated quantization for faster training and encoding.

### Product Quantization with GPU

```cpp
#include "vdb/quantization/product_quantizer.hpp"

using namespace vdb::quantization;

ProductQuantizerConfig pq_config;
pq_config.dimension = 512;
pq_config.num_subquantizers = 8;
pq_config.num_centroids = 256;
pq_config.use_gpu = true; // Enable GPU acceleration

ProductQuantizer pq(pq_config);

// Train on GPU (10-100x faster)
auto train_result = pq.train(training_vectors);

// Encode on GPU (batch processing)
auto encoded = pq.encode_batch(vectors);
```

### IVF Index with Quantization

Inverted File index with Product Quantization.

```cpp
#include "vdb/quantization/ivf_index.hpp"

IVFConfig ivf_config;
ivf_config.num_clusters = 256; // Number of Voronoi cells
ivf_config.probe_count = 8; // Cells to search
ivf_config.use_pq = true; // Product Quantization
ivf_config.pq_subquantizers = 8;
ivf_config.use_gpu = true;

IVFIndex ivf(ivf_config);

// Train
ivf.train(training_vectors);

// Add vectors
for (const auto& vec : vectors) {
 ivf.add(vec, metadata);
}

// Search (much faster for large datasets)
auto results = ivf.search(query, 10);
```

---

## Performance Benchmarks

### Hybrid Search

| Dataset Size | Vector Only | Lexical Only | Hybrid (RRF) | Accuracy Improvement |
|--------------|-------------|--------------|--------------|---------------------|
| 10K docs | 2ms | 5ms | 7ms | +12% |
| 100K docs | 5ms | 15ms | 20ms | +15% |
| 1M docs | 12ms | 50ms | 62ms | +18% |

### Distributed Performance

| Configuration | Throughput | Latency (p99) | Availability |
|--------------|------------|---------------|--------------|
| Single node | 500 qps | 5ms | 99.5% |
| 2x replicas | 800 qps | 6ms | 99.99% |
| 4x shards | 1800 qps | 8ms | 99.99% |
| 4x shards + 2x replicas | 3000 qps | 10ms | 99.999% |

### Framework Integration

| Operation | TensorFlow | PyTorch | ONNX Runtime |
|-----------|-----------|---------|--------------|
| Model load | 2.5s | 1.8s | 0.5s |
| Single embed | 8ms | 6ms | 5ms |
| Batch (100) | 250ms | 180ms | 150ms |

---

## Best Practices

### Distributed Systems

1. **Start with replication** before sharding
2. **Use async replication** for better latency
3. **Monitor health checks** and set up alerting
4. **Test failover** regularly in staging
5. **Plan resharding** during low-traffic periods

### Hybrid Search

1. **Use RRF fusion** as default (most robust)
2. **Tune weights** based on your data
3. **Index important fields** for lexical search
4. **Apply query rewriting** for better recall
5. **Rerank top results** for precision

### RAG Applications

1. **Chunk size**: 512-1024 chars for most cases
2. **Overlap**: 10-20% of chunk size
3. **Top-k**: 5-10 chunks for context
4. **Relevance threshold**: 0.7+ for quality
5. **Rerank**: Always rerank for best results

### Training Integration

1. **Export regularly** for continuous learning
2. **Use contrastive learning** for embeddings
3. **Fine-tune** on domain-specific data
4. **Monitor drift** and retrain when needed
5. **Version models** with semantic versioning

---

## See Also

- [Quantization Guide](14_QUANTIZATION.md) - Detailed quantization techniques
- [Architecture](05_ARCHITECTURE.md) - System architecture
- [API Reference](20_API_REFERENCE.md) - Complete API documentation
- [Deployment](16_DEPLOYMENT.md) - Production deployment guide

---

**Last Updated**: 2026-01-06
**Version**: 3.0.0
**Status**: Production Ready
