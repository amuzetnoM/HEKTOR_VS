# .studio Directory

## Purpose

The `.studio` directory serves as HEKTOR's **centralized technical hub**, containing machine-readable, exhaustive system specifications designed for rapid system understanding by developers, AI systems, and automated tools.

## Contents

### system_map.md (2,864 lines)

This is the **authoritative technical specification** for HEKTOR v4.0.0, providing a comprehensive blueprint of the entire system.

**Last Updated**: 2026-01-04  
**Version**: 4.0.0  
**Status**: EXHAUSTIVE, MACHINE-READABLE

## Document Structure

The system map is organized into 10 major sections:

| Section | Title | Description |
|---------|-------|-------------|
| **§1** | Architecture Topology | 6-layer modular architecture (L0-L5) |
| **§2** | Namespace Map | Organization under `vdb` namespace with 11 submodules |
| **§3** | Type Definitions | Primitive types, 24 enums, core structs |
| **§4** | Enumerations | Complete enum definitions with descriptions |
| **§5** | Classes | 60+ class definitions across 15 categories |
| **§6** | Functions | Function maps organized by category |
| **§7** | CLI Commands | 8 command groups, 30+ commands |
| **§8** | Python API | pybind11 bindings complete reference |
| **§9** | REST API | FastAPI endpoint specifications |
| **§10** | Build System | CMake configuration with 15+ options |

## Quick Reference

### System Identity
```yaml
SYSTEM_ID: HEKTOR
VERSION: 4.0.0 (CMake) | 2.0.0 (version.hpp)
TYPE: HIGH_PERFORMANCE_VECTOR_DATABASE
LANGUAGE: C++23
BUILD_SYSTEM: CMake 3.20+
LICENSE: MIT
API_LEVEL: 2
```

### Core Architecture (6 Layers)
```
L0: Core Types & Operations
    ├── types.hpp, distance.hpp, vector_ops.hpp
L1: Storage Layer
    ├── mmap_store, sqlite_store, pgvector_store, metadata_store
L2: Index Layer
    ├── hnsw_index, flat_index, metadata_index
L3: Embeddings Layer
    ├── text_encoder, image_encoder, onnx_runtime
L4: Search Layer
    ├── vector_search, bm25_engine, hybrid_search
L5: API Layer
    ├── cli_interface, python_bindings, rest_api
```

### Key Technologies
- **Language**: C++23 with SIMD (SSE4, AVX2, AVX-512)
- **Build**: CMake 3.20+ with Ninja
- **Storage**: Memory-mapped files, SQLite, PostgreSQL/pgvector
- **Indexing**: HNSW (M=16, ef_construction=200)
- **Embeddings**: ONNX Runtime (MiniLM-L6-v2, CLIP ViT-B/32)
- **Search**: Vector + BM25 hybrid with 5 fusion methods
- **LLM**: llama.cpp integration
- **Observability**: Prometheus, OpenTelemetry, eBPF

## Usage

### For Developers
1. Review architecture topology (§1) for system overview
2. Study class definitions (§5) for implementation details
3. Reference function maps (§6) for available operations
4. Check type definitions (§3) for data structures

### For AI/LLM Systems
- Parse the structured YAML/code blocks for machine understanding
- Use the comprehensive type system for code generation
- Reference the complete API specifications for integration

### For Operations Teams
- Review CLI commands (§7) for operational tasks
- Study REST API (§9) for monitoring integration
- Check build system (§10) for deployment configuration

## Relationship to Main Documentation

The `.studio/system_map.md` complements the main `docs/` directory:

| Resource | Audience | Format | Purpose |
|----------|----------|--------|---------|
| **system_map.md** | Developers, AI, Tools | Structured YAML/code | Technical reference, code generation |
| **docs/** | Users, Architects | Markdown prose | Tutorials, guides, explanations |

## Maintenance

This directory is maintained alongside the main codebase. When making architectural changes:

1. Update `system_map.md` to reflect new:
   - Classes/structs
   - Functions/methods
   - CLI commands
   - API endpoints
   - Build options

2. Maintain section numbering (§1-§10)
3. Keep YAML/code blocks machine-parseable
4. Update version and build date

## Contributing

When adding new features to HEKTOR, ensure the system map is updated:

- [ ] Add new types to §3 (Type Definitions)
- [ ] Document new enums in §4 (Enumerations)
- [ ] Define new classes in §5 (Classes)
- [ ] List new functions in §6 (Functions)
- [ ] Add CLI commands to §7 (CLI Commands)
- [ ] Update Python API in §8 if bindings added
- [ ] Document REST endpoints in §9 if API changed
- [ ] Reflect build options in §10 if CMake changed

---

**See Also**:
- [Main README](../README.md) - Project overview
- [Documentation Index](../docs/00_INDEX.md) - User-facing docs
- [Architecture Guide](../docs/05_ARCHITECTURE.md) - Design details
- [EXPLORATION_REPORT.md](../EXPLORATION_REPORT.md) - Complete repository analysis
