# Vector Studio

High-performance C++ vector database and AI training platform designed for the Gold Standard precious metals intelligence system.

## Features

- **SIMD-Optimized** - AVX2/AVX-512 accelerated distance computations
- **HNSW Index** - Hierarchical Navigable Small World for fast approximate nearest neighbor search
- **Local Embeddings** - ONNX Runtime for CPU/GPU text and image embeddings (no API calls)
- **Unified Search** - Query across text documents and chart images
- **Gold Standard Integration** - Native ingestion of journals, reports, and charts
- **AI Training Pipeline** - Export data for custom model training
- **Python Bindings** - Seamless integration with existing Python workflows
- **Cross-Platform** - Windows and Linux support

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                             Vector Studio                               │
├─────────────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────────┐   │
│  │ Text Encoder │  │ Image Encoder│  │     Gold Standard Ingest     │   │
│  │ (MiniLM-L6)  │  │ (CLIP ViT)   │  │  Journals | Charts | Reports │   │
│  └──────┬───────┘  └──────┬───────┘  └───────────────┬──────────────┘   │
│         │                 │                          │                  │
│         └────────┬────────┴──────────────────────────┘                  │
│                  │                                                      │
│         ┌────────▼────────┐                                             │
│         │  Projection     │  (384-dim → 512-dim unified space)          │
│         └────────┬────────┘                                             │
│                  │                                                      │
│  ┌───────────────▼───────────────┐  ┌──────────────────────────────┐    │
│  │         HNSW Index            │  │      Metadata Store          │    │
│  │  M=16, ef_construction=200    │  │   (JSONL, rich attributes)   │    │
│  └───────────────┬───────────────┘  └───────────────┬──────────────┘    │
│                  │                                  │                   │
│  ┌───────────────▼──────────────────────────────────▼───────────────┐   │
│  │                    Memory-Mapped Storage                         │   │
│  │              (vectors.bin + index.hnsw + metadata.jsonl)         │   │
│  └──────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

## Quick Start

### Build

```powershell
# Windows (PowerShell)
.\scripts\build.ps1 -Release

# With GPU support
.\scripts\build.ps1 -Release -GPU

# Debug build with tests
.\scripts\build.ps1 -Debug
```

### CLI Usage

```bash
# Initialize a new database
vdb_cli init ./my_database

# Ingest Gold Standard outputs
vdb_cli ingest ./my_database ../gold_standard/output

# Search
vdb_cli search ./my_database "gold breakout above resistance" -k 5

# View statistics
vdb_cli stats ./my_database

# Export for training
vdb_cli export ./my_database ./training_data.jsonl
```

### Python Usage

```python
import pyvdb

# Create or open database
db = pyvdb.create_gold_standard_db("./my_database")

# Or open existing
db = pyvdb.open_database("./my_database")

# Add text document
metadata = pyvdb.Metadata()
metadata.type = pyvdb.DocumentType.Journal
metadata.date = "2025-12-01"
metadata.bias = "BULLISH"
metadata.gold_price = 4220.50

doc_id = db.add_text(journal_content, metadata)

# Add chart
chart_id = db.add_image("output/charts/GOLD.png")

# Search by text
results = db.search("Fed rate cut impact on gold", k=10)

for r in results:
    print(f"Score: {r.score:.4f} | Date: {r.metadata.date} | Type: {r.metadata.type}")

# Query with filters
options = pyvdb.QueryOptions()
options.k = 5
options.type_filter = pyvdb.DocumentType.Journal
options.date_from = "2025-11-01"

results = db.query_text("bullish momentum", options)

# Ingest Gold Standard outputs
ingest = pyvdb.GoldStandardIngest(db)
config = pyvdb.IngestConfig()
config.gold_standard_output = "../gold_standard/output"
config.incremental = True

stats = ingest.ingest(config)
print(f"Added: {stats.journals_added} journals, {stats.charts_added} charts")
```

## Embedding Models

Vector Studio uses local ONNX models for embedding generation:

| Model | Type | Dimensions | Size | Speed (CPU) |
|-------|------|------------|------|-------------|
| all-MiniLM-L6-v2 | Text | 384 | 23MB | ~5ms |
| CLIP ViT-B/32 | Image | 512 | 350MB | ~50ms |

Text embeddings are projected to 512 dimensions to match image embeddings, enabling unified cross-modal search.

### Downloading Models

```powershell
# Models are downloaded automatically on first use, or manually:
python tools/download_models.py
```

## Document Types

Vector Studio recognizes these Gold Standard document types:

| Type | Description | Source |
|------|-------------|--------|
| Journal | Daily analysis with bias | `output/Journal_*.md` |
| Chart | Asset price charts | `output/charts/*.png` |
| CatalystWatchlist | 11-category catalyst matrix | `output/reports/catalysts_*.md` |
| InstitutionalMatrix | Bank forecasts & scenarios | `output/reports/inst_matrix_*.md` |
| EconomicCalendar | Fed/NFP/CPI events | `output/reports/economic_calendar_*.md` |
| WeeklyRundown | Weekly technical summary | `output/reports/weekly_rundown_*.md` |
| ThreeMonthReport | Tactical 1-3 month outlook | `output/reports/3m_*.md` |
| OneYearReport | Strategic 12-24 month view | `output/reports/1y_*.md` |

## Metadata Fields

Each vector stores rich metadata extracted from Gold Standard:

```python
metadata.id          # Unique vector ID
metadata.type        # DocumentType enum
metadata.date        # YYYY-MM-DD
metadata.source_file # Original file path
metadata.asset       # GOLD, SILVER, DXY, etc.
metadata.bias        # BULLISH, BEARISH, NEUTRAL
metadata.gold_price  # Price at time of document
metadata.silver_price
metadata.gsr         # Gold/Silver ratio
metadata.dxy         # Dollar index
metadata.vix         # Volatility
metadata.yield_10y   # 10Y Treasury yield
```

## Performance

Benchmarks on Intel i7-12700H, 32GB RAM:

| Operation | Vectors | Time |
|-----------|---------|------|
| Add text | 1 | 8ms |
| Add image | 1 | 55ms |
| Search (k=10) | 100,000 | 2ms |
| Batch ingest | 1,000 docs | 12s |

## Configuration

```cpp
DatabaseConfig config;
config.path = "./db";
config.dimension = 512;                    // Unified dimension
config.metric = DistanceMetric::Cosine;    // Best for embeddings
config.hnsw_m = 16;                        // Connections per node
config.hnsw_ef_construction = 200;         // Build quality
config.hnsw_ef_search = 50;                // Search quality
config.max_elements = 1000000;             // Max capacity
config.provider = ExecutionProvider::Auto; // CPU/CUDA/DirectML
config.num_threads = 0;                    // 0 = auto
```

## Requirements

- CMake 3.20+
- C++20 compiler (MSVC 2022, GCC 12+, Clang 14+)
- Python 3.10+ (for bindings)


##  Roadmap

1. **Phase 1** (Current): Vector store with semantic search
2. **Phase 2**: Incremental learning from new data
3. **Phase 3**: Fine-tune small LLM on vectorized corpus
4. **Phase 4**: Custom domain-specific model for gold/macro analysis



