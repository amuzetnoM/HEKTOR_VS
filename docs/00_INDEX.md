---
title: "Documentation Index"
description: "Complete documentation for Vector Studio - High-performance vector database with hybrid search, ML framework integration, and distributed features"
version: "3.0.0"
last_updated: "2026-01-06"
sidebar_position: 0
category: "Introduction"
---


![Version](https://img.shields.io/badge/version-3.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--06-green?style=flat-square)
![Status](https://img.shields.io/badge/status-production-success?style=flat-square)

**Welcome to Vector Studio** - The AI vector studio with hybrid search, distributed features, and comprehensive ML framework integration.

## 📚 Documentation Structure

Our documentation is organized into logical sections for easy navigation:

### 🎯 Getting Started (01-04)
Essential guides to get you up and running quickly.

### 📖 Core Concepts (05-09)
Deep dive into architecture, data handling, and core functionality.

### 🔧 Advanced Features (10-15)
Hybrid search, distributed systems, and ML framework integration.

### 🚀 Deployment & Operations (16-19)
Production deployment, monitoring, and real-world applications.

### 🧪 Development & Extension (20-23)
API reference, custom development, and contribution guides.

---

## 📑 Complete Documentation Index

| # | Document | Category | Description | Audience |
|---|----------|----------|-------------|----------|
| **GETTING STARTED** |
| 01 | [**Introduction**](01_INTRODUCTION.md) | Intro | Overview, key features, quick start | All users |
| 02 | [**Installation**](02_INSTALLATION.md) | Setup | System requirements, installation guide | All users |
| 03 | [**Quick Start**](03_QUICKSTART.md) | Tutorial | First database, basic operations | Beginners |
| 04 | [**User Guide**](04_USER_GUIDE.md) | Guide | Complete usage guide | All users |
| **CORE CONCEPTS** |
| 05 | [**Architecture**](05_ARCHITECTURE.md) | Technical | System design, components, data flow | Architects |
| 06 | [**Data Formats**](06_DATA_FORMATS.md) | Reference | Supported formats, specifications | Data engineers |
| 07 | [**Data Ingestion**](07_DATA_INGESTION.md) | Guide | Adapters, batch processing | Data engineers |
| 08 | [**Embeddings & Models**](08_EMBEDDINGS_MODELS.md) | Technical | Text/image encoders, model specs | ML engineers |
| 09 | [**Vector Operations**](09_VECTOR_OPERATIONS.md) | Technical | HNSW, distance metrics, math | ML engineers |
| **ADVANCED FEATURES** |
| 10 | [**Hybrid Search**](10_HYBRID_SEARCH.md) | Feature | BM25, fusion methods, RAG | Developers |
| 11 | [**Distributed System**](11_DISTRIBUTED_SYSTEM.md) | Feature | Replication, sharding, gRPC | Architects |
| 12 | [**ML Framework Integration**](12_ML_FRAMEWORK.md) | Feature | TensorFlow, PyTorch, training | ML engineers |
| 13 | [**LLM Engine**](13_LLM_ENGINE.md) | Feature | Local inference with llama.cpp | AI developers |
| 14 | [**Quantization**](14_QUANTIZATION.md) | Optimization | Compression techniques | Performance engineers |
| 15 | [**Logging & Monitoring**](15_LOGGING_MONITORING.md) | Operations | Logging, metrics, observability | DevOps |
| **DEPLOYMENT & OPERATIONS** |
| 16 | [**Deployment Guide**](16_DEPLOYMENT.md) | Operations | Docker, Kubernetes, production | DevOps |
| 17 | [**Performance Tuning**](17_PERFORMANCE_TUNING.md) | Optimization | Benchmarks, optimization | Performance engineers |
| 18 | [**Security**](18_SECURITY.md) | Operations | Security best practices | Security engineers |
| 19 | [**Real-World Applications**](19_REAL_WORLD_APPLICATIONS.md) | Case Studies | Production use cases | Solution architects |
| **DEVELOPMENT & EXTENSION** |
| 20 | [**API Reference**](20_API_REFERENCE.md) | Reference | Complete API documentation | Developers |
| 21 | [**Python Bindings**](21_PYTHON_BINDINGS.md) | Reference | Python API, examples | Python developers |
| 22 | [**Custom Development**](22_CUSTOM_DEVELOPMENT.md) | Guide | Custom adapters, extensions | Contributors |
| 23 | [**Contributing**](23_CONTRIBUTING.md) | Guide | Contribution guidelines | Contributors |

---

## 🚀 Quick Navigation

### By Role

#### **New Users**
1. Start with [Introduction](01_INTRODUCTION.md)
2. Follow [Installation](02_INSTALLATION.md)
3. Try [Quick Start](03_QUICKSTART.md)
4. Read [User Guide](04_USER_GUIDE.md)

#### **Developers**
1. Review [Architecture](05_ARCHITECTURE.md)
2. Study [API Reference](20_API_REFERENCE.md)
3. Explore [Python Bindings](21_PYTHON_BINDINGS.md)
4. Check [Custom Development](22_CUSTOM_DEVELOPMENT.md)

#### **Data Engineers**
1. Understand [Data Formats](06_DATA_FORMATS.md)
2. Learn [Data Ingestion](07_DATA_INGESTION.md)
3. Review [Performance Tuning](17_PERFORMANCE_TUNING.md)

#### **ML Engineers**
1. Study [Embeddings & Models](08_EMBEDDINGS_MODELS.md)
2. Explore [ML Framework Integration](12_ML_FRAMEWORK.md)
3. Learn [Hybrid Search](10_HYBRID_SEARCH.md)
4. Review [Quantization](14_QUANTIZATION.md)

#### **DevOps/SRE**
1. Read [Deployment Guide](16_DEPLOYMENT.md)
2. Configure [Logging & Monitoring](15_LOGGING_MONITORING.md)
3. Review [Security](18_SECURITY.md)
4. Study [Distributed System](11_DISTRIBUTED_SYSTEM.md)

### By Task

#### **Getting Started**
- [System Requirements](02_INSTALLATION.md#system-requirements)
- [Installation Steps](02_INSTALLATION.md#installation)
- [First Database](03_QUICKSTART.md#creating-your-first-database)
- [Basic Operations](03_QUICKSTART.md#basic-operations)

#### **Data Ingestion**
- [Supported Formats](06_DATA_FORMATS.md)
- [CSV Adapter](07_DATA_INGESTION.md#csv-adapter)
- [JSON Adapter](07_DATA_INGESTION.md#json-adapter)
- [PDF Adapter](07_DATA_INGESTION.md#pdf-adapter)
- [Batch Processing](07_DATA_INGESTION.md#batch-processing)

#### **Search & Retrieval**
- [Vector Search](04_USER_GUIDE.md#vector-search)
- [Hybrid Search](10_HYBRID_SEARCH.md)
- [Filtering](04_USER_GUIDE.md#metadata-filtering)
- [Ranking](10_HYBRID_SEARCH.md#fusion-methods)

#### **ML & AI**
- [Text Embeddings](08_EMBEDDINGS_MODELS.md#text-encoders)
- [Image Embeddings](08_EMBEDDINGS_MODELS.md#image-encoders)
- [Custom Models](12_ML_FRAMEWORK.md)
- [LLM Integration](13_LLM_ENGINE.md)

#### **Production Deployment**
- [Docker Setup](16_DEPLOYMENT.md#docker)
- [Kubernetes](16_DEPLOYMENT.md#kubernetes)
- [Monitoring](15_LOGGING_MONITORING.md)
- [Scaling](11_DISTRIBUTED_SYSTEM.md)

---

## 📖 What's New in v2.3.0

### 🎉 Major Features

#### **Hybrid Search Engine**
Complete BM25 + vector fusion with 5 algorithms (RRF, WeightedSum, CombSUM, CombMNZ, Borda).
→ See [Hybrid Search Guide](10_HYBRID_SEARCH.md)

#### **Distributed System**
Production-ready replication, sharding, and gRPC networking.
→ See [Distributed System Guide](11_DISTRIBUTED_SYSTEM.md)

#### **ML Framework Integration**
Full TensorFlow and PyTorch C++ API integration with GPU acceleration.
→ See [ML Framework Guide](12_ML_FRAMEWORK.md)

#### **Enhanced Monitoring**
Prometheus metrics, OpenTelemetry tracing, comprehensive logging.
→ See [Logging & Monitoring](15_LOGGING_MONITORING.md)

### 📚 Documentation Updates

- ✅ Reorganized with logical numbering (01-23)
- ✅ Added frontmatter to all documents
- ✅ Professional flow: Intro → Setup → Core → Advanced → Operations
- ✅ Role-based navigation guides
- ✅ Task-based quick links
- ✅ Comprehensive cross-references

---

## 🎯 Learning Paths

### Path 1: Basic User (1-2 hours)
```
01_INTRODUCTION → 02_INSTALLATION → 03_QUICKSTART → 04_USER_GUIDE
```

### Path 2: Developer (4-6 hours)
```
01_INTRODUCTION → 05_ARCHITECTURE → 20_API_REFERENCE → 
21_PYTHON_BINDINGS → 22_CUSTOM_DEVELOPMENT
```

### Path 3: ML Engineer (6-8 hours)
```
01_INTRODUCTION → 08_EMBEDDINGS_MODELS → 10_HYBRID_SEARCH → 
12_ML_FRAMEWORK → 14_QUANTIZATION
```

### Path 4: DevOps Engineer (4-6 hours)
```
01_INTRODUCTION → 16_DEPLOYMENT → 15_LOGGING_MONITORING → 
18_SECURITY → 11_DISTRIBUTED_SYSTEM
```

### Path 5: Complete Mastery (20+ hours)
```
Read all documents in order (01-23)
```

---

## 🔍 Search Tips

- **By Feature**: Use the category column in the index
- **By Role**: Follow the role-based navigation above
- **By Task**: Use the task-based quick links
- **Full-Text**: Use your IDE's search across all .md files

---

## 📞 Getting Help

### Documentation Issues
- Found a typo or error? See [Contributing](23_CONTRIBUTING.md)
- Missing information? Open an issue on GitHub

### Technical Support
- **GitHub Issues**: https://github.com/amuzetnoM/vector_studio/issues
- **Discussions**: https://github.com/amuzetnoM/vector_studio/discussions

### Community
- **Discord**: [Coming soon]
- **Stack Overflow**: Tag `vector-studio`

---

## 📄 Document Conventions

### Frontmatter
All documents include YAML frontmatter with:
- `title`: Document title
- `description`: Brief description
- `version`: Version number
- `last_updated`: Last update date
- `sidebar_position`: Navigation order
- `category`: Document category

### Code Examples
- **Python**: Primary language for examples
- **C++**: For advanced/performance-critical code
- **Shell**: For CLI and deployment

### Diagrams
- **ASCII Art**: For simple diagrams
- **Mermaid**: For complex flows (where supported)

---

## 🗺️ Documentation Roadmap

### v2.3.0 (Current)
- ✅ Complete reorganization with numbering
- ✅ Frontmatter standardization
- ✅ Role-based navigation
- ✅ Hybrid search documentation
- ✅ Distributed system documentation
- ✅ ML framework documentation

### v2.4.0 (Planned)
- [ ] Interactive tutorials
- [ ] Video walkthroughs
- [ ] API playground
- [ ] Performance calculator

### v3.0.0 (Future)
- [ ] Multi-language support
- [ ] Interactive diagrams
- [ ] Live code examples
- [ ] Community cookbook

---

## 📊 Documentation Statistics

- **Total Documents**: 23 core documents
- **Total Pages**: ~400 pages
- **Code Examples**: 200+ examples
- **Diagrams**: 50+ diagrams
- **Last Major Update**: 2026-01-06
- **Coverage**: 100% of features

---

**Ready to get started?** → [Introduction](01_INTRODUCTION.md)

**Need help?** → [User Guide](04_USER_GUIDE.md)

**Want to contribute?** → [Contributing Guide](23_CONTRIBUTING.md)
