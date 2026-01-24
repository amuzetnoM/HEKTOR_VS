# HEKTOR Quantization Studio

**High-Performance Vector Database Visualization & Management**

**Status**: Production Ready (N-API Implementation Complete)  
**Version**: 4.0.0  
**Date**: 2026-01-24  
**Author**: Ali A. Shakil / ARTIFACT VIRTUAL

---

## Overview

HEKTOR Quantization Studio is an Electron-based desktop application providing:
- **3D Vector Visualization** - Interactive Three.js rendering of vector spaces
- **Database Management** - Full CRUD operations via native C++ addon
- **Perceptual Quantization** - HDR-aware vector compression tools
- **Real-time Analytics** - Live performance metrics and monitoring

## Quick Start

### Prerequisites

- **Node.js** 18+ (with npm)
- **Python** 3.10+ (for node-gyp)
- **C++23 Compiler**:
  - Windows: Visual Studio 2022 (17.3+)
  - Linux: GCC 13+ or Clang 16+
  - macOS: Xcode 15+ (macOS 13+ SDK)
- **hektor-vdb** library built (see main repo)

### Installation

```bash
# Navigate to studio directory
cd studio/hektor-app

# Install dependencies
npm install

# Build native addon (requires hektor-vdb built first)
npm run build:native

# Start development server
npm run dev
```

### Building for Production

```bash
# Build all components
npm run build

# Start production build
npm start

# Package for distribution
npm run package        # Current platform
npm run package:all    # All platforms (Windows, macOS, Linux)
```

## Project Structure

```
hektor-app/
├── binding.gyp              # Native addon build configuration
├── package.json             # Application manifest
├── native-addon/
│   ├── include/             # C++ headers (17 files)
│   │   ├── database.h       # VectorDatabase wrapper
│   │   ├── search.h         # Search operations
│   │   ├── hybrid.h         # BM25 + hybrid search
│   │   ├── rag.h            # RAG pipeline
│   │   ├── distributed.h    # Cluster management
│   │   └── ...
│   ├── src/                 # C++ implementations (18 files, 4,066+ lines)
│   │   ├── database.cpp     # VectorDatabase - full implementation
│   │   ├── search.cpp       # Vector/text/hybrid search
│   │   ├── hybrid.cpp       # BM25Engine, HybridSearch
│   │   ├── rag.cpp          # RAGEngine, LLMEngine
│   │   └── ...
│   └── types/               # TypeScript definitions
│       └── hektor-native.d.ts
├── electron-app/
│   ├── main/                # Main process (Electron)
│   │   └── index.ts
│   ├── preload/             # IPC bridge
│   │   └── index.ts
│   └── renderer/            # React UI
│       ├── index.html
│       └── src/
│           ├── App.tsx
│           ├── main.tsx
│           ├── index.css
│           ├── lib/
│           │   └── theme-system.tsx
│           └── components/
│               └── 3d/
│                   └── VectorSpace3D.tsx
├── themes/                  # Theme files (JSON)
│   ├── midnight-purple-dark.json  (default)
│   ├── quantum-slate-dark.json
│   ├── quantum-slate-light.json
│   ├── ocean-breeze-dark.json
│   ├── forest-green-dark.json
│   └── sunset-amber-light.json
└── docs/
    ├── PERCEPTUAL_QUANTIZATION_DESIGN.md
    ├── ANALYTICS_MONITORING_DESIGN.md
    └── THEME_SYSTEM.md
```

## Features

### ✅ Complete Native Addon (338+ API Methods)

| Category | Methods | Description |
|----------|---------|-------------|
| Database | 35+ | VectorDatabase CRUD, collections, transactions |
| Search | 42+ | Vector similarity, text search, filtering |
| Hybrid | 28+ | BM25 engine, query rewriting, fusion |
| RAG | 32+ | Document chunking, context building, LLM |
| Distributed | 28+ | Replication, sharding, cluster management |
| Storage | 25+ | MMap, SQLite, PostgreSQL adapters |
| Index | 22+ | HNSW, Flat, Metadata indices |
| Ingestion | 28+ | CSV, JSON, Parquet, Arrow adapters |
| Embeddings | 35+ | ONNX, text/image encoders |
| Quantization | 18+ | PQ, SQ compression |
| Telemetry | 16+ | Logging, metrics, tracing |
| Framework | 15+ | TensorFlow, PyTorch, ONNX integration |

### ✅ Electron Application

- **React 18** with TypeScript 5.3
- **Vite** for fast development builds
- **Three.js** 3D visualization with React Three Fiber
- **6 Theme System** with hot-swapping
- **IPC Bridge** for secure main/renderer communication

### ✅ Build System

- **binding.gyp** configured for Windows, macOS, Linux
- **C++23** with AVX2/FMA optimizations
- **Cross-platform** library linking

## Scripts

| Script | Description |
|--------|-------------|
| `npm install` | Install dependencies |
| `npm run build:native` | Build C++ native addon |
| `npm run dev` | Start development (Electron + Vite) |
| `npm run build` | Build for production |
| `npm start` | Run production build |
| `npm run package` | Package for current platform |
| `npm run package:all` | Package for all platforms |
| `npm run lint` | Lint TypeScript code |
| `npm run format` | Format with Prettier |

## Technology Stack

| Component | Technology |
|-----------|------------|
| Desktop Framework | Electron 40.x |
| UI Framework | React 18.x |
| Language | TypeScript 5.3.x |
| Build Tool | Vite 5.x |
| Styling | Tailwind CSS 3.x |
| 3D Graphics | Three.js 0.160.x |
| State Management | Zustand 4.x |
| Animation | Framer Motion 10.x |
| Native Addon | N-API (node-addon-api 7.x) |
| Charts | D3.js 7.x |

## Documentation

- [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) - Full project overview
- [ROADMAP.md](ROADMAP.md) - Implementation roadmap
- [docs/THEME_SYSTEM.md](docs/THEME_SYSTEM.md) - Theme development
- [docs/PERCEPTUAL_QUANTIZATION_DESIGN.md](docs/PERCEPTUAL_QUANTIZATION_DESIGN.md) - PQ specification
- [docs/ANALYTICS_MONITORING_DESIGN.md](docs/ANALYTICS_MONITORING_DESIGN.md) - Analytics design

## Contributing

1. Ensure hektor-vdb is built first
2. Follow C++23 standards for native addon
3. Use TypeScript strict mode
4. Run linting before commits

## License

MIT License - See [LICENSE](../../LICENSE) for details.

---

**Repository**: https://github.com/amuzetnoM/hektor
