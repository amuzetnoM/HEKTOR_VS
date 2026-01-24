# Hektor Vector Database - Comprehensive Feature Reference

**Version**: 4.1.0  
**Author**: Ali A. Shakil / ARTIFACT VIRTUAL  
**Last Updated**: 2026-01-24

---

## Executive Summary

Hektor is a high-performance C++ vector database designed for production AI/ML workloads. This document serves as the authoritative source of truth for all features, capabilities, and specifications.

---

## 1. Core Vector Database Features

### 1.1 Vector Storage & Indexing

| Feature | Status | Description |
|---------|--------|-------------|
| HNSW Index | ✅ Production | Hierarchical Navigable Small World graph |
| Memory-Mapped Storage | ✅ Production | Zero-copy vector access via mmap |
| Multi-Index Support | ✅ Production | Multiple indexes per database |
| Dynamic Updates | ✅ Production | Insert/Update/Delete without rebuild |
| Incremental Persistence | ✅ Production | Durability without full sync |

### 1.2 Distance Metrics

| Metric | SIMD Support | Notes |
|--------|--------------|-------|
| Cosine Similarity | AVX-512, AVX2, SSE4.1 | Normalized dot product |
| Euclidean (L2) | AVX-512, AVX2, SSE4.1 | Squared distance |
| Inner Product | AVX-512, AVX2, SSE4.1 | Dot product |
| Manhattan (L1) | AVX2, SSE4.1 | Taxicab distance |
| Hamming | AVX-512 POPCNT | Binary vectors |

### 1.3 HNSW Parameters

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| M | 16 | 4-64 | Connections per node |
| ef_construction | 200 | 50-500 | Build quality |
| ef_search | 50 | 10-500 | Search quality |
| max_elements | 1M | 1K-1B | Capacity |

---

## 2. Search Capabilities

### 2.1 Vector Search

| Feature | Status | Performance |
|---------|--------|-------------|
| k-NN Search | ✅ Production | <3ms @ 1M vectors |
| Range Search | ✅ Production | Returns all within radius |
| Filtered Search | ✅ Production | Metadata predicates |
| Multi-Vector Query | ✅ Production | Batch processing |

### 2.2 Hybrid Search (Vector + Lexical)

| Component | Status | Description |
|-----------|--------|-------------|
| BM25 Full-Text | ✅ Production | Porter stemming, stopword removal |
| Inverted Index | ✅ Production | Compressed posting lists |
| Tokenization | ✅ Production | Unicode-aware |

### 2.3 Fusion Methods

| Method | Status | Best For |
|--------|--------|----------|
| RRF (Reciprocal Rank Fusion) | ✅ Production | General purpose |
| Weighted Sum | ✅ Production | Known importance ratio |
| CombSUM | ✅ Production | High recall |
| CombMNZ | ✅ Production | High precision |
| Borda Count | ✅ Production | Fair combination |

---

## 3. Quantization & Compression

### 3.1 Scalar Quantization

| Type | Bits | Compression | Recall Loss |
|------|------|-------------|-------------|
| FP32 (baseline) | 32 | 1x | 0% |
| FP16 | 16 | 2x | <0.1% |
| INT8 | 8 | 4x | <3% |
| INT4 | 4 | 8x | <8% |

### 3.2 Perceptual Quantization (Dolby Compatible)

| Transfer Function | Status | Standards |
|-------------------|--------|-----------|
| SMPTE ST 2084 (PQ) | ✅ Production | HDR10, Dolby Vision |
| HLG (Rec. 2100) | ✅ Production | BBC/NHK HDR |
| Gamma 2.2 | ✅ Production | sRGB |
| Gamma 2.4 | ✅ Production | BT.1886 |

### 3.3 Display Profiles

| Profile | Peak Luminance | Use Case |
|---------|----------------|----------|
| SDR | 100 nits | Standard monitors |
| HDR10 | 1,000 nits | Consumer HDR |
| DolbyVision | 4,000 nits | Premium HDR |
| HLG | 1,000 nits | Broadcast HDR |
| HDR1000 | 1,000 nits | Generic HDR |

---

## 4. Embedding Models

### 4.1 Text Embeddings

| Model | Dimensions | Size | Latency |
|-------|------------|------|---------|
| all-MiniLM-L6-v2 | 384 | 23 MB | ~5 ms |
| all-mpnet-base-v2 | 768 | 420 MB | ~15 ms |
| e5-large-v2 | 1024 | 1.2 GB | ~25 ms |

### 4.2 Image Embeddings

| Model | Dimensions | Size | Latency |
|-------|------------|------|---------|
| CLIP ViT-B/32 | 512 | 340 MB | ~50 ms |
| CLIP ViT-L/14 | 768 | 890 MB | ~120 ms |

### 4.3 Cross-Modal Support

- Text → 384-dim → Projection → 512-dim unified space
- Image → 512-dim → Unified space
- Enables text-to-image and image-to-text search

---

## 5. RAG (Retrieval-Augmented Generation)

### 5.1 Chunking Strategies

| Strategy | Status | Best For |
|----------|--------|----------|
| Fixed-Size | ✅ Production | Uniform processing |
| Sentence | ✅ Production | Natural language |
| Paragraph | ✅ Production | Structured documents |
| Semantic | ✅ Production | Topic coherence |
| Recursive | ✅ Production | Mixed content |

### 5.2 Framework Adapters

| Framework | Status | Version |
|-----------|--------|---------|
| LangChain | ✅ Production | 0.1+ |
| LlamaIndex | ✅ Production | 0.10+ |

---

## 6. Distributed System

### 6.1 Replication

| Mode | Status | Latency |
|------|--------|---------|
| Async | ✅ Production | <100ms |
| Sync | ✅ Production | <500ms |
| Semi-Sync | ✅ Production | <200ms |

### 6.2 Sharding

| Strategy | Status | Description |
|----------|--------|-------------|
| Hash | ✅ Production | Consistent hashing |
| Range | ✅ Production | Key range partitioning |
| Consistent | ✅ Production | Minimal resharding |

### 6.3 Networking

| Feature | Status | Protocol |
|---------|--------|----------|
| gRPC | ✅ Production | HTTP/2 |
| TLS/mTLS | ✅ Production | TLS 1.3 |
| Service Discovery | ✅ Production | DNS/Consul |
| Load Balancing | ✅ Production | Round-robin, weighted |

---

## 7. ML Framework Integration

### 7.1 TensorFlow C++ API

| Feature | Status |
|---------|--------|
| SavedModel Loading | ✅ Production |
| GPU Acceleration | ✅ Production |
| Training Export (TFRecord) | ✅ Production |

### 7.2 PyTorch C++ (LibTorch)

| Feature | Status |
|---------|--------|
| TorchScript Loading | ✅ Production |
| CUDA/ROCm Support | ✅ Production |
| Mixed Precision | ✅ Production |

---

## 8. Observability

### 8.1 Metrics

| System | Status | Metrics Count |
|--------|--------|---------------|
| Prometheus | ✅ Production | 50+ metrics |
| Custom Exporters | ✅ Production | Configurable |

### 8.2 Tracing

| Feature | Status |
|---------|--------|
| OpenTelemetry | ✅ Production |
| W3C Trace Context | ✅ Production |
| Distributed Tracing | ✅ Production |

### 8.3 Profiling

| Feature | Status |
|---------|--------|
| eBPF Integration | ✅ Production |
| CPU Profiling | ✅ Production |
| Memory Profiling | ✅ Production |

### 8.4 Logging

| Feature | Status |
|---------|--------|
| Structured JSON | ✅ Production |
| Anomaly Detection | ✅ Production |
| 15 Anomaly Types | ✅ Production |

---

## 9. Data Ingestion

### 9.1 Supported Formats

| Format | Read | Write |
|--------|------|-------|
| JSON | ✅ | ✅ |
| JSONL | ✅ | ✅ |
| CSV | ✅ | ✅ |
| Parquet | ✅ | ✅ |
| Excel | ✅ | ❌ |
| PDF | ✅ | ❌ |
| XML | ✅ | ✅ |

### 9.2 Database Connectors

| Database | Status |
|----------|--------|
| SQLite | ✅ Production |
| PostgreSQL | ✅ Production |
| pgvector Extension | ✅ Production |

---

## 10. Studio Native Addon

### 10.1 Build System

| Tool | Status | Notes |
|------|--------|-------|
| cmake-js | ✅ Production | Primary build system |
| node-gyp | ⚠️ Fallback | Limited C++23 support |

### 10.2 Implemented Components

| Component | Status | Description |
|-----------|--------|-------------|
| BM25Engine | ✅ Complete | Full-text search |
| KeywordExtractor | ✅ Complete | TF-IDF extraction |
| HybridSearchEngine | ✅ Complete | Vector + lexical fusion |
| QueryRewriter | ✅ Complete | Query expansion |
| Quantization | ✅ Complete | PQ, SQ, HDR support |

---

## 11. Platform Support

### 11.1 Operating Systems

| OS | Status | Notes |
|----|--------|-------|
| Windows 10/11 | ✅ Production | MSVC 2022+ |
| Ubuntu 22.04+ | ✅ Production | GCC 13+/Clang 16+ |
| Debian 12+ | ✅ Production | GCC 13+ |
| macOS 13+ | ✅ Production | Apple Clang 15+ |
| Raspberry Pi OS | ⚠️ Experimental | ARM64 |

### 11.2 Architectures

| Architecture | Status |
|--------------|--------|
| x86_64 (AMD64) | ✅ Production |
| ARM64 (AArch64) | ✅ Production |
| x86 (32-bit) | ❌ Not Supported |

### 11.3 Compiler Requirements

| Compiler | Minimum | Recommended |
|----------|---------|-------------|
| GCC | 13.0 | 14.0+ |
| Clang | 16.0 | 18.0+ |
| MSVC | 19.33 (VS 17.3) | 19.38+ (VS 17.8) |
| Apple Clang | 15.0 | 15.3+ |

---

## 12. Performance Specifications

### 12.1 Query Performance

| Scale | Latency (p50) | Latency (p99) | Recall@10 |
|-------|---------------|---------------|-----------|
| 100K | 1.2 ms | 2.8 ms | 98.5% |
| 1M | 2.1 ms | 4.8 ms | 98.1% |
| 10M | 4.3 ms | 9.2 ms | 97.5% |
| 100M | 6.8 ms | 15 ms | 96.8% |
| 1B | 8.5 ms | 22 ms | 96.8% |

### 12.2 Throughput

| Configuration | QPS |
|---------------|-----|
| Single Node | 10,000+ |
| Distributed (10 nodes) | 85,000+ |

### 12.3 Memory Usage

| Component | Size per Vector (512-dim) |
|-----------|---------------------------|
| Vector (FP32) | 2,048 bytes |
| HNSW Index | ~200 bytes |
| Metadata (avg) | ~100 bytes |
| **Total** | **~2.4 KB** |

---

## 13. Installation Methods

### 13.1 Package Managers

```bash
# PyPI
pip install hektor-vdb

# With ML dependencies
pip install hektor-vdb[ml]

# With all optional dependencies
pip install hektor-vdb[all]
```

### 13.2 Docker

```bash
# Pull from GHCR
docker pull ghcr.io/amuzetnom/hektor:latest

# Run with docker-compose
docker-compose up -d
```

### 13.3 Source Build

```bash
# Clone and build
git clone https://github.com/amuzetnoM/hektor.git
cd hektor
./build-hektor.sh
```

---

## 14. API Availability

### 14.1 Language Bindings

| Language | Status | Package |
|----------|--------|---------|
| Python | ✅ Production | pyvdb |
| C++ | ✅ Production | libvdb |
| Node.js | ✅ Production | @hektor/native-addon |

### 14.2 Interfaces

| Interface | Status |
|-----------|--------|
| CLI | ✅ Production |
| REST API | ✅ Production |
| gRPC API | ✅ Production |
| Python SDK | ✅ Production |

---

## 15. Security Features

| Feature | Status |
|---------|--------|
| TLS 1.3 | ✅ Production |
| mTLS | ✅ Production |
| API Key Auth | ✅ Production |
| RBAC | ⚠️ Beta |
| Encryption at Rest | ⚠️ Beta |

---

## 16. Documentation

| Document | Location |
|----------|----------|
| Installation Guide | docs/02_INSTALLATION.md |
| Quick Start | docs/03_QUICKSTART.md |
| User Guide | docs/04_USER_GUIDE.md |
| API Reference | docs/20_API_REFERENCE.md |
| Python Bindings | docs/22_PYTHON_BINDINGS.md |
| Deployment Guide | docs/16_DEPLOYMENT.md |

---

## 17. Support & Licensing

- **License**: MIT
- **GitHub**: https://github.com/amuzetnoM/hektor
- **PyPI**: https://pypi.org/project/hektor-vdb/
- **Issues**: https://github.com/amuzetnoM/hektor/issues

---

*This document is the authoritative source of truth for Hektor Vector Database features and capabilities.*
