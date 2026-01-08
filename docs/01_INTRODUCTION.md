---
title: "Introduction"
description: "Overview of Vector Studio, key features, and quick start guide"
version: "3.0.0"
last_updated: "2026-01-06"
sidebar_position: 1
category: "Introduction"
---

![Version](https://img.shields.io/badge/version-3.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--06-green?style=flat-square)
![Docs](https://img.shields.io/badge/docs-17_files-orange?style=flat-square)


# ⬜HECKTOR 
> Advance Vector Database

Meet Hecktor, the AI Vector Studio. This comprehensive guide covers everything from basic usage to advanced topics and API reference.

## Documentation Index

| # | Document | Description | Audience |
|---|----------|-------------|----------|
| 01 | [**README**](01_README.md) | This file - documentation overview and quick start | All users |
| 02 | [**GETTING_STARTED**](02_INSTALLATION.md) | Installation, quick start, basic usage, CLI reference | Beginners |
| 03 | [**USER_GUIDE**](04_USER_GUIDE.md) | Complete user guide from installation to advanced usage | All users |
| 04 | [**DATA_FORMATS**](06_DATA_FORMATS.md) | Supported data types, formats, and specifications | Data engineers, developers |
| 05 | [**DATA_INGESTION**](07_DATA_INGESTION.md) | Data ingestion module with adapters (CSV, JSON, PDF, Excel, Text) | Data engineers |
| 06 | [**ARCHITECTURE**](05_ARCHITECTURE.md) | System design, data flow, component diagrams | Architects, contributors |
| 07 | [**API_REFERENCE**](20_API_REFERENCE.md) | Detailed API documentation for all classes and functions | Developers |
| 08 | [**MODELS**](08_EMBEDDINGS_MODELS.md) | Model specifications, benchmarks, integration | ML practitioners |
| 09 | [**MATH**](09_VECTOR_OPERATIONS.md) | Mathematical foundations, HNSW algorithm, distance metrics | ML engineers |
| 10 | [**AI_TRAINING**](10_AI_TRAINING.md) | Training custom models, fine-tuning, contrastive learning | AI researchers |
| 11 | [**LOGGING**](15_LOGGING_MONITORING.md) | Comprehensive logging system with anomaly detection | DevOps, developers |
| 12 | [**USAGE_EXAMPLES**](12_USAGE_EXAMPLES.md) | Code examples and common usage patterns | All developers |
| 13 | [**DEPLOYMENT**](16_DEPLOYMENT.md) | Production deployment guide | DevOps, architects |
| 14 | [**REAL_WORLD_APPLICATIONS**](19_REAL_WORLD_APPLICATIONS.md) | Production use cases and benchmarks | Solution architects |
| 15 | [**LLM_ENGINE**](13_LLM_ENGINE.md) | **NEW** - Local text generation with llama.cpp | AI developers |
| 16 | [**QUANTIZATION**](14_QUANTIZATION.md) | **NEW** - Vector compression techniques (4-32x) | Performance engineers |
| 17 | [**HTTP_ADAPTER**](17_HTTP_ADAPTER.md) | **NEW** - HTTP adapter for web API data ingestion | Data engineers |

## Quick Links by Task

### Getting Started
- [Installation Guide](04_USER_GUIDE.md#installation)
- [Quick Start Tutorial](02_INSTALLATION.md#quick-start)
- [First Database](02_INSTALLATION.md#first-database)

### Data Ingestion
- [Supported Data Formats](06_DATA_FORMATS.md) - **Complete format specifications**
- [Universal Data Adapters](07_DATA_INGESTION.md)
- [CSV Adapter](07_DATA_INGESTION.md#1-csv-adapter)
- [JSON Adapter](07_DATA_INGESTION.md#2-json-adapter)
- [Text Adapter](07_DATA_INGESTION.md#3-plain-text-adapter)
- [PDF Adapter](07_DATA_INGESTION.md#4-pdf-adapter)
- [Excel Adapter](07_DATA_INGESTION.md#5-excel-adapter)

### Core Operations
- [Adding Vectors](04_USER_GUIDE.md#adding-vectors)
- [Searching](04_USER_GUIDE.md#searching)
- [Batch Operations](04_USER_GUIDE.md#batch-operations)

### API Reference
- [VectorDatabase API](20_API_REFERENCE.md#vectordatabase)
- [DataAdapterManager API](20_API_REFERENCE.md#dataadaptermanager)
- [TextEncoder API](20_API_REFERENCE.md#textencoder)
- [Python Bindings](20_API_REFERENCE.md#python-bindings-api)

### Advanced Topics
- [Performance Tuning](04_USER_GUIDE.md#performance-tuning)
- [Custom Adapters](07_DATA_INGESTION.md#custom-adapter-development)
- [HNSW Configuration](04_USER_GUIDE.md#hnsw-index)
- [Distributed Deployment](04_USER_GUIDE.md#distributed-deployment)

## What is Vector Studio?

Vector Studio is a high-performance vector database and AI training platform designed for semantic search and machine learning applications. It provides:

- **Fast Similarity Search**: Sub-millisecond queries on millions of vectors
- **Universal Data Ingestion**: CSV, JSON, PDF, Excel, Plain Text formats with automatic detection
- **Local Embedding Generation**: ONNX-based inference for text and images
- **Gold Standard Integration**: Seamless connection with Gold Standard journal system
- **AI Training Toolkit**: Tools for fine-tuning and training custom models
- **Python Bindings**: Easy integration with Python workflows

## Architecture Overview

```
┌────────────────────────────────────────────────────────────────────────────┐
│                              VECTOR STUDIO                                  │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  USER INTERFACES                                                          │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   │
│  │   Python    │   │   C++ CLI   │   │  REST API   │   │    GUI      │   │
│  │   pyvdb     │   │     vdb     │   │  (planned)  │   │  (planned)  │   │
│  └──────┬──────┘   └──────┬──────┘   └──────┬──────┘   └──────┬──────┘   │
│         │                 │                 │                 │           │
│         └────────────────┬┴─────────────────┴─────────────────┘           │
│                          │                                                 │
│  CORE ENGINE (C++23)                                                      │
│  ┌───────────────────────┴───────────────────────────────────────────┐    │
│  │                                                                     │    │
│  │  DATA INGESTION              VECTOR OPS         STORAGE           │    │
│  │  ┌──────────────┐          ┌───────────┐    ┌────────────┐       │    │
│  │  │  Adapters    │          │   HNSW    │    │  Memory    │       │    │
│  │  │  • CSV       │  ──────→ │   Index   │    │  Mapped    │       │    │
│  │  │  • JSON      │          │           │    │  Storage   │       │    │
│  │  │  • PDF       │          │  O(log n) │    │            │       │    │
│  │  │  • Excel     │          │  Search   │    │  vectors   │       │    │
│  │  │  • Text      │          │           │    │  .bin      │       │    │
│  │  └──────────────┘          │  99%+     │    │  index     │       │    │
│  │                            │  Recall   │    │  .hnsw     │       │    │
│  │                            └───────────┘    └────────────┘       │    │
│  │                                                                     │    │
│  │  EMBEDDINGS                                                        │    │
│  │  ┌───────────────────────────────────────────────────────────┐    │    │
│  │  │  ONNX Runtime                                              │    │    │
│  │  │  • Text: MiniLM-L6 (384-dim)                              │    │    │
│  │  │  • Image: CLIP ViT (512-dim)                              │    │    │
│  │  │  • Local inference, no API calls                          │    │    │
│  │  └───────────────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
└────────────────────────────────────────────────────────────────────────────┘
```
└────────────────────────────────────────────────────────────────────────────┘
```

## Getting Started

### 1. Installation

```powershell
cd vector_database
python scripts/setup.py --auto-install
python scripts/download_models.py
.\scripts\build.ps1
```

### 2. First Database

```python
import pyvdb

# Create database
db = pyvdb.create_gold_standard_db("./my_vectors")

# Add documents
db.add_text("Gold analysis for today", {
    "type": "Journal",
    "date": "2025-12-01"
})

# Search
results = db.search("gold outlook", k=5)
```

### 3. Learn More

- **Beginners**: Start with [02_INSTALLATION.md](02_INSTALLATION.md)
- **Data Formats**: Review [06_DATA_FORMATS.md](06_DATA_FORMATS.md) for complete format specifications
- **Developers**: Read [05_ARCHITECTURE.md](05_ARCHITECTURE.md)
- **Researchers**: Explore [09_VECTOR_OPERATIONS.md](09_VECTOR_OPERATIONS.md) and [10_AI_TRAINING.md](10_AI_TRAINING.md)
- **Model Selection**: See [08_EMBEDDINGS_MODELS.md](08_EMBEDDINGS_MODELS.md)

## Key Features

### 🚀 Performance

| Metric | Value |
|--------|-------|
| Query latency (1M vectors) | ~3ms |
| Insertion throughput | 10,000/sec |
| Recall@10 | 99%+ |
| Memory per vector | ~1.2 KB |

### 🧠 AI Capabilities

- **Text Embeddings**: MiniLM, MPNet, E5 models
- **Image Embeddings**: CLIP ViT (cross-modal search)
- **Custom Training**: Fine-tune or train from scratch
- **Local Inference**: No cloud dependency

### 🔧 Integration

- **Gold Standard**: Automatic journal ingestion
- **Python**: Full pyvdb bindings
- **CLI**: vdb command-line tool
- **Export**: Training data extraction

## Document Summaries

### 02_INSTALLATION.md

Practical quick start guide including:
- Step-by-step installation
- Building from source
- First database creation
- Python API tutorial
- CLI reference
- Basic troubleshooting

### 04_USER_GUIDE.md

Complete user guide covering:
- Detailed installation and configuration
- Core concepts and operations
- Advanced usage patterns
- Performance tuning
- Best practices
- Comprehensive troubleshooting

### 06_DATA_FORMATS.md **(NEW)**

**Comprehensive data format specifications:**
- Complete list of supported formats (CSV, JSON, PDF, Excel, Text, Markdown)
- Format requirements and constraints
- Data type specifications
- Metadata structure guidelines
- Chunking strategy explanations
- Data preparation best practices
- Format-specific examples

### 07_DATA_INGESTION.md **(UPDATED)**

Data ingestion module guide:
- Architecture overview
- Implemented adapters (CSV, JSON, Text, PDF, Excel)
- Usage examples and integration
- Batch processing
- Custom adapter development
- Performance considerations

### 05_ARCHITECTURE.md

Comprehensive system design document covering:
- Component diagrams and data flow
- Memory layout and file formats
- Threading model and concurrency
- API design patterns
- Performance characteristics

### 09_VECTOR_OPERATIONS.md

Mathematical foundations:
- Vector spaces and embeddings
- Distance metrics (cosine, Euclidean)
- HNSW algorithm analysis
- Complexity proofs
- Quantization techniques

### 10_AI_TRAINING.md

Complete AI training guide:
- Pretrained model usage
- Fine-tuning strategies
- Training from scratch
- Contrastive learning
- Multi-modal training
- ONNX deployment

### 08_EMBEDDINGS_MODELS.md

Model reference:
- Detailed specifications
- Benchmark results
- Model cards
- Custom integration
- Provider selection

## License

Vector Studio is released under the MIT License. See [../LICENSE](../LICENSE) for details.

## Contributing

See [CONTRIBUTING.md](../CONTRIBUTING.md) for contribution guidelines.
