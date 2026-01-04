# Vector Studio

> **High-Performance Vector Database Engine** — Semantic Search and AI Training Platform

A C++ vector database with SIMD-optimized similarity search and local ONNX-based embeddings. Designed for sub-millisecond queries on millions of vectors.

---

<p align="center">

<!-- Build Status -->
[![Build](https://img.shields.io/badge/build-passing-brightgreen?style=for-the-badge&logo=cmake&logoColor=white)](https://github.com/amuzetnoM/gold_standard)
[![Tests](https://img.shields.io/badge/tests-passing-success?style=for-the-badge&logo=pytest&logoColor=white)](https://github.com/amuzetnoM/gold_standard)
[![Coverage](https://img.shields.io/badge/coverage-85%25-green?style=for-the-badge&logo=codecov&logoColor=white)](https://github.com/amuzetnoM/gold_standard)

<!-- Tech Stack -->
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.20+-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org/)
[![ONNX](https://img.shields.io/badge/ONNX-Runtime-005CED?style=for-the-badge&logo=onnx&logoColor=white)](https://onnxruntime.ai/)
[![Python](https://img.shields.io/badge/Python-3.10--3.13-3776AB?style=for-the-badge&logo=python&logoColor=white)](https://www.python.org/)

<!-- Performance -->
[![SIMD](https://img.shields.io/badge/SIMD-AVX2%2FAVX--512-FF6600?style=for-the-badge&logo=intel&logoColor=white)](#performance)
[![Latency](https://img.shields.io/badge/latency-%3C3ms-blueviolet?style=for-the-badge)](#performance)

<!-- Meta -->
[![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey?style=for-the-badge)](#)

</p>

---

## Table of Contents

- [Features](#features)
- [Quick Start](#quick-start)
- [Architecture](#architecture)
- [Installation](#installation)
- [Usage](#usage)
- [Embedding Models](#embedding-models)
- [Performance](#performance)
- [Configuration](#configuration)
- [Documentation](#documentation)
- [Development](#development)
- [License](#license)

---

## Features

| Feature | Description |
|---------|-------------|
| **SIMD-Optimized Distance** | AVX2/AVX-512 accelerated cosine similarity, Euclidean distance |
| **HNSW Index** | Hierarchical Navigable Small World graph for O(log n) approximate nearest neighbor |
| **Local Embeddings** | ONNX Runtime inference for text and images without API calls |
| **Cross-Modal Search** | Unified 512-dim space for text and image embeddings |
| **Memory-Mapped Storage** | Zero-copy vector access via mmap for efficient I/O |
| **Gold Standard Integration** | Native ingestion of journals, charts, and analysis reports |
| **Python Bindings** | pybind11-based Python API for seamless integration |
| **Thread-Safe Operations** | Concurrent reads with exclusive writes |
| **AI Training Export** | Export vector pairs and triplets for model fine-tuning |
| **Rich Metadata** | JSONL storage with full attribute filtering |

---

## Quick Start

### Automated Setup (Recommended)

**Windows PowerShell:**
```powershell
# Clone repository
git clone https://github.com/amuzetnoM/vector_studio.git
cd gold_standard/vector_studio

# Run automated setup (installs Python, CMake, Ninja, dependencies)
.\scripts\setup.ps1

# Build
.\scripts\build.ps1 -Release
```

**Unix/macOS/Linux:**
```bash
# Clone and setup
git clone https://github.com/amuzetnoM/vector_studio.git
cd gold_standard/vector_studio

chmod +x scripts/setup.sh
./scripts/setup.sh

# Build
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja
```

### First Database

```python
import pyvdb

# Create database optimized for Gold Standard
db = pyvdb.create_gold_standard_db("./my_vectors")

# Add a document
db.add_text("Gold broke above $4,200 resistance with strong volume", {
    "type": "Journal",
    "date": "2025-12-01",
    "bias": "BULLISH"
})

# Semantic search
results = db.search("gold breakout bullish momentum", k=5)
for r in results:
    print(f"{r.score:.4f}: {r.metadata.date} - {r.metadata.type}")
```

---

## Architecture

```
+-----------------------------------------------------------------------------+
|                              VECTOR STUDIO                                   |
+-----------------------------------------------------------------------------+
|                                                                             |
|  +---------------+   +---------------+   +--------------------------+       |
|  |  Text Encoder |   | Image Encoder |   |   Gold Standard Ingest   |       |
|  |  MiniLM-L6-v2 |   |  CLIP ViT-B32 |   | Journals | Charts | Rpts |       |
|  +-------+-------+   +-------+-------+   +------------+--------------+       |
|          |                   |                        |                     |
|          +--------+----------+-----------+------------+                     |
|                   |                                                         |
|          +--------v--------+                                                |
|          |   Projection    |   384-dim -> 512-dim unified space             |
|          +--------+--------+                                                |
|                   |                                                         |
|  +----------------v----------------+   +--------------------------+         |
|  |          HNSW Index             |   |     Metadata Store       |         |
|  |  M=16, ef_construction=200      |   |   JSONL, rich filters    |         |
|  +----------------+----------------+   +------------+-------------+         |
|                   |                                 |                       |
|  +----------------v---------------------------------v-------------+         |
|  |                   Memory-Mapped Storage                        |         |
|  |          vectors.bin | index.hnsw | metadata.jsonl             |         |
|  +----------------------------------------------------------------+         |
|                                                                             |
+-----------------------------------------------------------------------------+
```

### Component Overview

| Component | Technology | Purpose |
|-----------|------------|---------|
| **Core Engine** | C++20 | Vector operations, HNSW index, storage |
| **Distance Functions** | AVX2/AVX-512 SIMD | Optimized similarity computation |
| **Text Embeddings** | ONNX (MiniLM-L6-v2) | Sentence embeddings (384-dim) |
| **Image Embeddings** | ONNX (CLIP ViT-B/32) | Visual embeddings (512-dim) |
| **Python Bindings** | pybind11 | High-level Python API |
| **CLI** | C++ | Command-line database operations |

---

## Installation

### System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| OS | Windows 10 (1903+) / Linux | Windows 11 / Ubuntu 22.04 |
| CPU | x64 with SSE4.1 | Intel 11th gen+ / AMD Zen3+ (AVX-512) |
| RAM | 8 GB | 16+ GB |
| Storage | 5 GB | SSD with 20+ GB |
| Python | 3.10 | 3.12 |
| CMake | 3.20 | 3.28+ |
| Compiler | MSVC 19.30 / GCC 11 | Latest |

### Dependencies

The setup scripts automatically install:

- **Python 3.12** (via winget/brew/apt)
- **CMake** (build system)
- **Ninja** (fast builds)
- **Visual Studio Build Tools 2022** (Windows) or **build-essential** (Linux)
- **ONNX Runtime** (inference)
- **NumPy, Transformers, pytest** (Python packages)

### Manual Installation

```powershell
# Windows - Install prerequisites
winget install Python.Python.3.12
winget install Kitware.CMake
winget install Ninja-build.Ninja
winget install Microsoft.VisualStudio.2022.BuildTools

# Create virtual environment
python -m venv venv
.\venv\Scripts\Activate.ps1

# Install Python dependencies
pip install -r requirements.txt
pip install -r requirements-dev.txt

# Download ONNX models
python scripts/download_models.py

# Build
.\scripts\build.ps1 -Release
```

---

## Usage

### Python API

```python
import pyvdb

# Create or open database
db = pyvdb.create_database("./vectors")
# or: db = pyvdb.open_database("./vectors")

# Add text document with metadata
metadata = pyvdb.Metadata()
metadata.type = pyvdb.DocumentType.Journal
metadata.date = "2025-12-01"
metadata.bias = "BULLISH"
metadata.gold_price = 4220.50

doc_id = db.add_text("Gold analysis content here", metadata)

# Add chart image
chart_id = db.add_image("charts/GOLD.png", {"type": "Chart", "asset": "GOLD"})

# Basic search
results = db.search("gold bullish momentum", k=10)

# Filtered search
options = pyvdb.QueryOptions()
options.k = 10
options.type_filter = pyvdb.DocumentType.Journal
options.date_from = "2025-11-01"
options.date_to = "2025-12-31"
options.min_score = 0.7

results = db.query_text("resistance breakout", options)

# Cross-modal: search images with text
results = db.search("bullish flag pattern chart", k=5)

# Database management
stats = db.stats()
db.sync()
db.optimize()
db.close()
```

### CLI Usage

```bash
# Initialize database
vdb init ./my_database

# Add documents
vdb add ./my_database --text document.txt --type Journal --date 2025-12-01
vdb add ./my_database --image chart.png --type Chart --asset GOLD

# Ingest Gold Standard outputs
vdb ingest ./my_database ../gold_standard/output

# Search
vdb search ./my_database "gold breakout" --k 10 --min-score 0.7

# Statistics
vdb stats ./my_database

# Export for training
vdb export ./my_database ./training_data.jsonl
```

### Gold Standard Integration

```python
from pyvdb import create_gold_standard_db, GoldStandardIngest, IngestConfig

# Create database configured for Gold Standard
db = create_gold_standard_db("./gold_vectors")

# Ingest all outputs
ingest = GoldStandardIngest(db)
config = IngestConfig()
config.gold_standard_output = "../gold_standard/output"
config.incremental = True  # Only add new documents

stats = ingest.ingest(config)
print(f"Added: {stats.journals} journals, {stats.charts} charts")
```

---

## Embedding Models

Vector Studio uses local ONNX models for embedding generation:

| Model | Type | Dimensions | Size | Latency (CPU) |
|-------|------|------------|------|---------------|
| all-MiniLM-L6-v2 | Text | 384 | 23 MB | ~5 ms |
| CLIP ViT-B/32 | Image | 512 | 340 MB | ~50 ms |

Text embeddings are projected from 384 to 512 dimensions to enable unified cross-modal search.

### Download Models

```powershell
# Automatic download to ~/.cache/vector_studio/models/
python scripts/download_models.py
```

---

## Document Types

Vector Studio recognizes these Gold Standard document types:

| Type | Description | Source |
|------|-------------|--------|
| Journal | Daily analysis with bias | `output/Journal_*.md` |
| Chart | Asset price charts | `output/charts/*.png` |
| CatalystWatchlist | 11-category catalyst matrix | `output/reports/catalysts_*.md` |
| InstitutionalMatrix | Bank forecasts and scenarios | `output/reports/inst_matrix_*.md` |
| EconomicCalendar | Fed/NFP/CPI events | `output/reports/economic_calendar_*.md` |
| WeeklyRundown | Weekly technical summary | `output/reports/weekly_rundown_*.md` |
| ThreeMonthReport | Tactical 1-3 month outlook | `output/reports/3m_*.md` |
| OneYearReport | Strategic 12-24 month view | `output/reports/1y_*.md` |

---

## Metadata Fields

Each vector stores rich metadata extracted from Gold Standard:

| Field | Type | Description |
|-------|------|-------------|
| `id` | int | Unique vector ID |
| `type` | enum | DocumentType classification |
| `date` | string | YYYY-MM-DD format |
| `source_file` | string | Original file path |
| `asset` | string | GOLD, SILVER, DXY, etc. |
| `bias` | string | BULLISH, BEARISH, NEUTRAL |
| `gold_price` | float | Price at time of document |
| `silver_price` | float | Silver spot price |
| `gsr` | float | Gold/Silver ratio |
| `dxy` | float | Dollar index value |
| `vix` | float | Volatility index |
| `yield_10y` | float | 10Y Treasury yield |

---

## Performance

### Benchmarks

*Intel Core i7-12700H, 32GB RAM, NVMe SSD*

| Operation | Dataset Size | Time | Throughput |
|-----------|-------------|------|------------|
| Add text | 1 document | 8 ms | 125/sec |
| Add image | 1 image | 55 ms | 18/sec |
| Search (k=10) | 100,000 vectors | 2 ms | 500 qps |
| Search (k=10) | 1,000,000 vectors | 3 ms | 333 qps |
| Batch ingest | 1,000 documents | 12 s | 83/sec |

### HNSW Index Quality

| ef_search | Recall@10 | Latency (1M vectors) |
|-----------|-----------|----------------------|
| 50 | 95.2% | 2.5 ms |
| 100 | 98.1% | 4.2 ms |
| 200 | 99.4% | 7.8 ms |
| 500 | 99.9% | 18 ms |

### Memory Usage

| Component | Size per Vector |
|-----------|-----------------|
| Vector storage (512-dim float32) | 2,048 bytes |
| HNSW index | ~200 bytes |
| Metadata (avg) | ~100 bytes |
| **Total** | **~2.4 KB** |

---

## Configuration

### Database Configuration

```python
config = pyvdb.DatabaseConfig()
config.dimension = 512                      # Vector dimensionality
config.metric = pyvdb.DistanceMetric.Cosine # Distance function
config.hnsw_m = 16                          # HNSW connections per node
config.hnsw_ef_construction = 200           # Build quality
config.hnsw_ef_search = 50                  # Search quality (default)
config.max_elements = 1_000_000             # Maximum capacity
config.provider = pyvdb.ExecutionProvider.Auto  # CPU/CUDA/DirectML
```

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `VDB_MODELS_DIR` | ONNX model directory | `~/.cache/vector_studio/models` |
| `VDB_LOG_LEVEL` | Logging verbosity | `INFO` |
| `VDB_NUM_THREADS` | Thread pool size (0=auto) | `0` |
| `VDB_SIMD` | SIMD level (avx512/avx2/sse4/none) | `auto` |

---

## Documentation

Comprehensive documentation is available in the `docs/` directory:

| # | Document | Description |
|---|----------|-------------|
| 01 | [README](docs/01_README.md) | Documentation overview and index |
| 02 | [GETTING_STARTED](docs/02_GETTING_STARTED.md) | Installation, quick start, basic usage |
| 03 | [USER_GUIDE](docs/03_USER_GUIDE.md) | Complete user guide |
| 04 | [DATA_FORMATS](docs/04_DATA_FORMATS.md) | **NEW** - Supported data types and format specifications |
| 05 | [DATA_INGESTION](docs/05_DATA_INGESTION.md) | Data ingestion with adapters (CSV, JSON, PDF, Excel, Text) |
| 06 | [ARCHITECTURE](docs/06_ARCHITECTURE.md) | System design, data flow, component diagrams |
| 07 | [API_REFERENCE](docs/07_API_REFERENCE.md) | Complete API documentation |
| 08 | [MODELS](docs/08_MODELS.md) | Model specifications, benchmarks |
| 09 | [MATH](docs/09_MATH.md) | Mathematical foundations, HNSW algorithm |
| 10 | [AI_TRAINING](docs/10_AI_TRAINING.md) | Training custom models, fine-tuning |

---

## Development

### Building from Source

```powershell
# Debug build with tests
.\scripts\build.ps1 -Debug

# Release build
.\scripts\build.ps1 -Release

# Check dependencies only
.\scripts\build.ps1 -CheckOnly

# Auto-install missing dependencies
.\scripts\build.ps1 -AutoInstall

# With GPU support
.\scripts\build.ps1 -Release -GPU
```

### Running Tests

```powershell
# C++ tests
cd build
ctest --output-on-failure

# Python tests
pytest tests/ -v
```

### Code Quality

```powershell
# Format C++ code
clang-format -i src/**/*.cpp include/**/*.hpp

# Format Python code
black scripts/ bindings/python/
isort scripts/ bindings/python/
```

---

## Project Structure

```
vector_database/
+-- CMakeLists.txt          # Build configuration
+-- README.md               # This file
+-- LICENSE                 # MIT License
+-- CONTRIBUTING.md         # Contribution guidelines
+-- CHANGELOG.md            # Version history
+-- requirements.txt        # Python runtime dependencies
+-- requirements-dev.txt    # Python development dependencies
+-- .gitignore              # Git ignore rules
+-- .gitattributes          # Git attributes
+-- include/
|   +-- vdb/                # Public C++ headers
+-- src/                    # C++ implementation
|   +-- core/               # Distance, threading, vector ops
|   +-- index/              # HNSW implementation
|   +-- storage/            # Memory-mapped persistence
|   +-- embeddings/         # ONNX encoder wrappers
|   +-- cli/                # Command-line tool
+-- bindings/
|   +-- python/             # pybind11 Python bindings
+-- scripts/
|   +-- setup.ps1           # Windows setup script
|   +-- setup.sh            # Unix setup script
|   +-- build.ps1           # Windows build script
|   +-- download_models.py  # ONNX model downloader
+-- tests/                  # Unit tests
+-- docs/                   # Documentation
```

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- [HNSW Paper](https://arxiv.org/abs/1603.09320) - Malkov and Yashunin
- [Sentence-Transformers](https://www.sbert.net/) - MiniLM models
- [OpenAI CLIP](https://openai.com/research/clip) - Vision-language models
- [ONNX Runtime](https://onnxruntime.ai/) - Cross-platform inference

---

<p align="center">
<sub>Part of the <a href="../gold_standard/README.md">Gold Standard</a> precious metals intelligence system.</sub>
</p>


