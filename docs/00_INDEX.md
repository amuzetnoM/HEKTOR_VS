# Documentation Index

> **Hektor Vector Database** - Comprehensive Documentation

**Version**: 4.0.0  
**Last Updated**: 2026-01-22  
**Total Documents**: 25+

---

## Core Documentation

| # | Document | Description | Audience |
|---|----------|-------------|----------|
| 01 | [Introduction](01_INTRODUCTION.md) | System overview, key features, quick start | All users |
| 02 | [Installation](02_INSTALLATION.md) | System requirements, installation, configuration | All users |
| 03 | [Quick Start](03_QUICKSTART.md) | Create first database, basic operations | Beginners |
| 04 | [User Guide](04_USER_GUIDE.md) | Complete user guide covering all features | All users |

## Architecture & Design

| # | Document | Description | Audience |
|---|----------|-------------|----------|
| 05 | [Architecture](05_ARCHITECTURE.md) | System design, data flow, components | Architects |
| 06 | [Data Formats](06_DATA_FORMATS.md) | Supported data types, format specifications | Data engineers |
| 07 | [Data Ingestion](07_DATA_INGESTION.md) | Data adapters, batch processing | Data engineers |

## AI/ML Features

| # | Document | Description | Audience |
|---|----------|-------------|----------|
| 08 | [Embeddings & Models](08_EMBEDDINGS_MODELS.md) | Text/image encoders, model specs | ML practitioners |
| 09 | [Vector Operations](09_VECTOR_OPERATIONS.md) | HNSW algorithm, distance metrics | ML engineers |
| 10 | [AI Training](10_AI_TRAINING.md) | Training, fine-tuning, contrastive learning | AI researchers |
| 12 | [ML Framework Integration](12_ML_FRAMEWORK.md) | TensorFlow, PyTorch C++ API | ML engineers |
| 13 | [LLM Engine](13_LLM_ENGINE.md) | Local text generation with llama.cpp | AI developers |

## Search & RAG

| # | Document | Description | Audience |
|---|----------|-------------|----------|
| 10 | [Hybrid Search](10_HYBRID_SEARCH.md) | BM25 full-text, fusion methods, RAG | Search engineers |

## Performance & Optimization

| # | Document | Description | Audience |
|---|----------|-------------|----------|
| 14 | [Quantization](14_QUANTIZATION.md) | Vector compression (4-32x), PQ/SQ | Performance engineers |
| **17** | **[Performance Tuning](17_PERFORMANCE_TUNING.md)** | **Benchmarks, competitor comparison, optimization** | **All users** |
| **24** | **[Perceptual Quantization](24_PERCEPTUAL_QUANTIZATION_RESEARCH.md)** | **HDR-aware quantization research** | **Advanced users** |

## Distributed & Production

| # | Document | Description | Audience |
|---|----------|-------------|----------|
| 11 | [Distributed System](11_DISTRIBUTED_SYSTEM.md) | Replication, sharding, gRPC networking | DevOps |
| 16 | [Deployment Guide](16_DEPLOYMENT.md) | Docker, Kubernetes, production | DevOps |
| 19 | [Real-World Applications](19_REAL_WORLD_APPLICATIONS.md) | Production use cases, benchmarks | Solution architects |

## Observability

| # | Document | Description | Audience |
|---|----------|-------------|----------|
| 15 | [Logging & Monitoring](15_LOGGING_MONITORING.md) | Logging, Prometheus metrics | DevOps |
| 15 | [OpenTelemetry Tracing](15_OPENTELEMETRY_TRACING.md) | Distributed tracing, W3C Trace Context | DevOps |
| 15 | [eBPF Observability](15_EBPF_OBSERVABILITY.md) | Zero-overhead profiling, kernel-level | Performance engineers |

## API & Development

| # | Document | Description | Audience |
|---|----------|-------------|----------|
| 17 | [HTTP Adapter](17_HTTP_ADAPTER.md) | HTTP adapter for web API ingestion | Data engineers |
| 18 | [Advanced Features](18_ADVANCED_FEATURES.md) | Advanced topics and features | Advanced users |
| 18 | [Security](18_SECURITY.md) | Security best practices | DevOps, security |
| 20 | [API Reference](20_API_REFERENCE.md) | Complete C++ API documentation | Developers |
| 21 | [Usage Examples](21_USAGE_EXAMPLES.md) | Code examples, common patterns | All developers |
| 22 | [Python Bindings](22_PYTHON_BINDINGS.md) | Python API reference | Python developers |
| 23 | [Custom Development](23_CUSTOM_DEVELOPMENT.md) | Custom adapters, extensions | Advanced developers |
| 24 | [Contributing](24_CONTRIBUTING.md) | How to contribute | Contributors |

---

## Quick Navigation by Task

### 🚀 Getting Started
1. [Installation](02_INSTALLATION.md)
2. [Quick Start](03_QUICKSTART.md)
3. [First Database](03_QUICKSTART.md#first-database)

### �� Data Ingestion
1. [Data Formats](06_DATA_FORMATS.md)
2. [Data Adapters](07_DATA_INGESTION.md)
3. [Batch Processing](07_DATA_INGESTION.md#batch-processing)

### 🔍 Search & Retrieval
1. [Vector Search](04_USER_GUIDE.md#searching)
2. [Hybrid Search](10_HYBRID_SEARCH.md)
3. [RAG Engine](10_HYBRID_SEARCH.md#rag-engine)

### ⚡ Performance
1. **[Performance Tuning](17_PERFORMANCE_TUNING.md)** - Benchmarks, competitor comparison
2. [Quantization](14_QUANTIZATION.md) - Vector compression
3. **[Perceptual Quantization](24_PERCEPTUAL_QUANTIZATION_RESEARCH.md)** - HDR-aware

### 🌐 Distributed & Production
1. [Distributed System](11_DISTRIBUTED_SYSTEM.md)
2. [Deployment](16_DEPLOYMENT.md)
3. [Observability](15_LOGGING_MONITORING.md)

### 🔧 Development
1. [API Reference](20_API_REFERENCE.md)
2. [Python Bindings](22_PYTHON_BINDINGS.md)
3. [Custom Development](23_CUSTOM_DEVELOPMENT.md)

---

## System Snapshot (v4.0.0)

See the comprehensive system snapshot and exhaustive analysis in `docs/research/HEKTOR_ANALYSIS.md` (v4.0.0) for architecture details, billion-scale benchmarks, perceptual quantization research, and deployment guidance.

---

## Document Status

| Status | Count | Description |
|--------|-------|-------------|

---

## Document Status

| Status | Count | Description |
|--------|-------|-------------|
| ✅ Complete | 20 | Fully documented with examples |
| 🔄 Updated | 5 | Recently updated for v4.0.0 |
| 🆕 New | 5 | New in v4.0.0 |

---

**Navigation**: [README](../README.md) | [GitHub](https://github.com/amuzetnoM/hektor)  
**Support**: Issues, Discussions, Contributing Guide  
**License**: MIT
