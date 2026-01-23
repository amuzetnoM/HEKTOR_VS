# HEKTOR FULL N-API IMPLEMENTATION PLAN

## SCOPE
Implement complete N-API wrapper for HEKTOR C++23 vector database covering:
- 15+ namespaces
- 300+ functions  
- 60+ classes
- All features from system_map.md (2,864 lines)

## IMPLEMENTATION STRUCTURE

### Phase 1: Core Infrastructure (CRITICAL)
1. **Enhanced binding.gyp** - Link all dependencies
2. **Type system** - Complete type definitions for all enums/structs
3. **Error handling** - Comprehensive Result<T> wrapping
4. **ObjectWrap classes** - For all major C++ classes

### Phase 2: Core Database Operations
1. **VectorDatabase** class wrapper
2. **HnswIndex** and **FlatIndex** wrappers
3. **Vector/VectorView** operations
4. **Metadata** operations

### Phase 3: Storage Layer
1. **VectorStore, MetadataStore** 
2. **SqliteStore** wrapper
3. **PgVectorStore** wrapper  
4. **MemoryMappedFile** wrapper

### Phase 4: Embeddings
1. **TextEncoder** wrapper (all-MiniLM-L6-v2)
2. **ImageEncoder** wrapper (CLIP)
3. **OnnxSession** wrapper
4. **Tokenizer** wrapper

### Phase 5: Search & Hybrid
1. **BM25Engine** wrapper
2. **HybridSearchEngine** wrapper
3. **QueryRewriter** wrapper
4. **KeywordExtractor** wrapper

### Phase 6: Quantization
1. **ProductQuantizer** wrapper
2. **ScalarQuantizer** wrapper  
3. **DisplayAwareQuantizer** wrapper
4. **Perceptual curves** (PQ, HLG, Gamma)

### Phase 7: Data Ingestion
1. **11+ Adapter** wrappers (CSV, JSON, XML, PDF, Excel, Parquet, etc.)
2. **DataAdapterManager** wrapper
3. **GoldStandardIngest** wrapper
4. **HTTPAdapter** + **HttpClient** wrapper

### Phase 8: RAG & LLM
1. **RAGEngine** wrapper
2. **LLMEngine** wrapper (llama.cpp)
3. **LangChainAdapter** wrapper
4. **LlamaIndexAdapter** wrapper
5. **DocumentChunker** wrapper

### Phase 9: Distributed Systems
1. **ReplicationManager** wrapper
2. **ShardingManager** wrapper
3. **DistributedVectorDatabase** wrapper

### Phase 10: Framework Integration
1. **TensorFlowEmbedder** wrapper
2. **PyTorchEmbedder** wrapper

### Phase 11: Telemetry & Logging
1. **TelemetrySpan** wrapper
2. **TelemetryMetrics** wrapper
3. **Logger** wrapper

### Phase 12: Utilities
1. Distance functions (dot, cosine, L2, etc.)
2. Vector operations (normalize, add, scale, etc.)
3. Utility functions (timestamps, hashing, etc.)

## FILE STRUCTURE

```
native-addon/
├── include/
│   ├── common.h              # Shared types, macros, Result<T> wrapper
│   ├── database.h            # VectorDatabase wrapper
│   ├── index.h               # HnswIndex, FlatIndex wrappers  
│   ├── storage.h             # Storage layer wrappers
│   ├── embeddings.h          # Embedding engine wrappers
│   ├── search.h              # Search & hybrid wrappers
│   ├── quantization.h        # Quantization wrappers
│   ├── ingestion.h           # Data adapter wrappers
│   ├── rag.h                 # RAG & LLM wrappers
│   ├── distributed.h         # Replication/sharding wrappers
│   ├── framework.h           # TF/PyTorch wrappers
│   ├── telemetry.h           # Telemetry/logging wrappers
│   └── utils.h               # Utility function wrappers
├── src/
│   ├── binding.cpp           # Main entry point
│   ├── database.cpp
│   ├── index.cpp
│   ├── storage.cpp
│   ├── embeddings.cpp
│   ├── search.cpp
│   ├── quantization.cpp
│   ├── ingestion.cpp
│   ├── rag.cpp
│   ├── distributed.cpp
│   ├── framework.cpp
│   ├── telemetry.cpp
│   └── utils.cpp
└── types/
    └── hektor-native.d.ts    # Complete TypeScript definitions
```

## CRITICAL LINKING REQUIREMENTS

binding.gyp must link:
- libvdb_core.a (5.1MB)
- ONNX Runtime
- SQLite3
- PostgreSQL client (libpq)
- OpenSSL (for hashing)
- Poppler (for PDF)
- xlsxwriter (for Excel)  
- Arrow/Parquet
- CUDA (optional)
- DirectML (optional, Windows)
- llama.cpp / ggml
- TensorFlow C API (optional)
- PyTorch C++ API (optional)

## TYPESCRIPT DEFINITIONS

Complete hektor-native.d.ts with:
- All enums (DistanceMetric, DocumentType, ErrorCode, etc.)
- All interfaces matching C++ structs
- All class definitions with methods
- Promise wrappers for async operations
- Type guards and utility types

## PRELOAD SCRIPT

Update electron-app/preload.ts to expose:
- window.hektorAPI with complete API surface
- Proper contextBridge isolation
- Type-safe IPC channels

## TESTING STRATEGY

1. Unit tests for each wrapper class
2. Integration tests for workflows
3. Performance benchmarks
4. Memory leak testing
5. Error handling validation

## SUCCESS CRITERIA

✅ All 300+ functions from system_map.md exposed
✅ Full TypeScript type coverage
✅ Working Electron integration
✅ Comprehensive error handling
✅ Production-quality code (no stubs/TODOs)
✅ Memory safety (RAII, proper cleanup)
✅ Thread safety where needed
✅ Complete documentation

