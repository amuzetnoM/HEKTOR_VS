---
title: "Installation"
description: "System requirements, installation steps, and initial configuration"
version: "4.0.0"
last_updated: "2026-01-22"
sidebar_position: 2
category: "Setup"
---

![Version](https://img.shields.io/badge/version-4.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--22-green?style=flat-square)
![Difficulty](https://img.shields.io/badge/difficulty-beginner-brightgreen?style=flat-square)


## Table of Contents

1. [Quick Start](#quick-start)
2. [Installation](#installation)
3. [Building from Source](#building-from-source)
4. [Basic Usage](#basic-usage)
5. [Gold Standard Integration](#gold-standard-integration)
6. [Python API Tutorial](#python-api-tutorial)
7. [CLI Reference](#cli-reference)
8. [Configuration](#configuration)
9. [Best Practices](#best-practices)
10. [Troubleshooting](#troubleshooting)

---

## Quick Start

### 30-Second Setup

```powershell
# Clone and enter directory
cd vector_database

# Install dependencies
python scripts/setup.py --auto-install

# Download models
python scripts/download_models.py

# Build
.\scripts\build.ps1

# Verify
.\build\Release\vdb --version
```

### First Database

```python
import pyvdb

# Create database
db = pyvdb.create_database("./my_vectors")

# Add a document
db.add_text("Gold prices surge on Fed pivot expectations", {
    "type": "Journal",
    "date": "2025-12-01",
    "bias": "BULLISH"
})

# Search
results = db.search("gold bullish outlook", k=5)
for r in results:
    print(f"{r.score:.3f}: {r.metadata}")
```

---

## Installation

### Quick Install (Recommended)

The easiest way to install Hektor VDB is via pip:

```bash
# Install from source (requires CMake and a C++ compiler)
pip install hektor-vdb

# Or install with ML dependencies
pip install hektor-vdb[ml]

# Or install with all optional dependencies
pip install hektor-vdb[all]
```

**Note:** This requires:
- Python 3.10 or higher
- CMake 3.20 or higher
- A C++23 compatible compiler (GCC 13+, Clang 16+, MSVC 2022 17.3+)

If you encounter build issues, see [Building from Source](#building-from-source) below.

---

### System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| OS | Windows 10 (1903+) / Ubuntu 22.04+ / macOS 13+ | Windows 11 / Ubuntu 24.04 / macOS 14 |
| CPU | x64 with AVX2 | Intel 11th gen+ / AMD Zen3+ |
| RAM | 8 GB | 16+ GB |
| Storage | 5 GB | SSD with 20+ GB |
| Python | 3.10 | 3.12+ |
| Compiler | GCC 13+ / Clang 16+ / MSVC 19.33+ | GCC 14+ / Clang 18+ / MSVC Latest |

### Prerequisites

#### Windows

```powershell
# Check for winget
winget --version

# Install required tools (run as Administrator if needed)
winget install Kitware.CMake
winget install Microsoft.VisualStudio.2022.BuildTools
winget install Ninja-build.Ninja
```

#### Python Environment

```powershell
# Create virtual environment
python -m venv venv
.\venv\Scripts\Activate.ps1

# Install Python dependencies
pip install -r requirements.txt
```

### Download ONNX Models

```powershell
# Download all required models
python scripts/download_models.py

# Models will be saved to: ~/.cache/vector_studio/models/
# - all-MiniLM-L6-v2.onnx (~23 MB)
# - clip-vit-base-patch32.onnx (~340 MB)
```

---

## Building from Source

### Automatic Build

The easiest way to build:

```powershell
# Full build with auto-install of missing tools
.\scripts\build.ps1 -AutoInstall

# Check dependencies only
.\scripts\build.ps1 -CheckOnly

# Release build (default)
.\scripts\build.ps1 -Release

# Debug build
.\scripts\build.ps1 -Debug

# Clean build
.\scripts\build.ps1 -Clean
```

### Manual Build

```powershell
# Create build directory
mkdir build
cd build

# Configure (Release)
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Or use ninja directly
ninja

# Run tests
ctest --output-on-failure
```

### Build Options

| CMake Option | Default | Description |
|--------------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Debug, Release, RelWithDebInfo |
| `VDB_BUILD_TESTS` | ON | Build unit tests |
| `VDB_BUILD_CLI` | ON | Build command-line tool |
| `VDB_BUILD_PYTHON` | ON | Build Python bindings |
| `VDB_ENABLE_AVX2` | ON | Enable AVX2 SIMD |
| `VDB_ENABLE_AVX512` | OFF | Enable AVX-512 SIMD |
| `VDB_USE_OPENMP` | ON | Enable OpenMP parallelization |

Example with options:

```powershell
cmake .. -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DVDB_ENABLE_AVX512=ON `
    -DVDB_BUILD_PYTHON=OFF
```

---

## Basic Usage

### Creating a Database

```python
import pyvdb

# Method 1: Standard empty database
db = pyvdb.create_database("./vectors")

# Method 2: Gold Standard configured database
db = pyvdb.create_gold_standard_db("./gold_vectors")

# Method 3: Custom configuration
config = pyvdb.DatabaseConfig()
config.dimension = 512
config.hnsw_m = 16
config.hnsw_ef_construction = 200
config.max_elements = 1_000_000
db = pyvdb.Database.create("./custom_db", config)
```

### Adding Documents

#### Text Documents

```python
# Simple add
doc_id = db.add_text("Market analysis text here")

# With metadata
metadata = pyvdb.Metadata()
metadata.type = pyvdb.DocumentType.Journal
metadata.date = "2025-12-01"
metadata.bias = "BULLISH"
metadata.gold_price = 4220.50

doc_id = db.add_text("Gold broke above key resistance at $4,200", metadata)
```

#### Images (Charts)

```python
# Add chart image
metadata = pyvdb.Metadata()
metadata.type = pyvdb.DocumentType.Chart
metadata.date = "2025-12-01"
metadata.asset = "GOLD"

chart_id = db.add_image("./charts/gold_daily.png", metadata)
```

#### Batch Operations

```python
# Batch add for efficiency (uses parallel embedding)
documents = [
    ("First document text", {"type": "Journal", "date": "2025-12-01"}),
    ("Second document text", {"type": "Analysis", "date": "2025-12-01"}),
    # ...
]

ids = db.add_batch(documents)
print(f"Added {len(ids)} documents")
```

### Searching

#### Basic Search

```python
# Simple text search
results = db.search("gold bullish momentum", k=10)

for result in results:
    print(f"Score: {result.score:.4f}")
    print(f"ID: {result.id}")
    print(f"Type: {result.metadata.type}")
    print(f"Date: {result.metadata.date}")
    print("---")
```

#### Filtered Search

```python
# Search with filters
options = pyvdb.QueryOptions()
options.k = 10
options.type_filter = pyvdb.DocumentType.Journal
options.date_from = "2025-11-01"
options.date_to = "2025-12-31"
options.min_score = 0.7

results = db.query_text("resistance breakout", options)
```

#### Image Search

```python
# Find similar charts
results = db.query_image("./query_chart.png", pyvdb.QueryOptions(k=5))

# Search for charts using text description
results = db.search("bullish flag pattern daily chart", k=5)
```

### Managing Data

#### Update Metadata

```python
# Get existing metadata
meta = db.get_metadata(doc_id)

# Modify and update
meta.bias = "NEUTRAL"
db.update_metadata(doc_id, meta)
```

#### Delete Documents

```python
# Delete single document
db.remove(doc_id)

# Delete by date (useful for cleanup)
removed_count = db.remove_by_date("2025-11-01")
print(f"Removed {removed_count} documents from 2025-11-01")
```

#### Database Management

```python
# Get database statistics
stats = db.stats()
print(f"Total vectors: {stats.count}")
print(f"Index size: {stats.index_size_mb:.1f} MB")
print(f"Vector storage: {stats.vector_size_mb:.1f} MB")

# Force sync to disk
db.sync()

# Optimize index (compact, rebuild)
db.optimize()

# Close database
db.close()
```

---

## Gold Standard Integration

### Automatic Ingestion

Vector Studio integrates seamlessly with Gold Standard journal outputs:

```python
from pyvdb import create_gold_standard_db, ingest_gold_standard

# Create database configured for Gold Standard
db = create_gold_standard_db("./gold_standard_vectors")

# Ingest all reports from output directory
stats = ingest_gold_standard(
    db,
    output_dir="./gold_standard/output",
    start_date="2025-01-01"
)

print(f"Ingested {stats.journals} journals")
print(f"Ingested {stats.catalysts} catalyst reports")
print(f"Ingested {stats.charts} charts")
```

### Document Types

Gold Standard document types are automatically detected:

| Type | Files | Example |
|------|-------|---------|
| `Journal` | `Journal_*.md` | Daily analysis |
| `Premarket` | `premarket_*.md` | Pre-market notes |
| `CatalystWatchlist` | `catalysts_*.md` | Upcoming events |
| `InstitutionalMatrix` | `inst_matrix_*.md` | Holdings data |
| `Chart` | `charts/*.png` | Technical charts |
| `WeeklyRundown` | `weekly_rundown_*.md` | Weekly summary |
| `EconomicCalendar` | `economic_calendar_*.md` | Economic events |

### Integration with GUI

Add to Gold Standard GUI for semantic search:

```python
# In gold_standard/gui.py

from pyvdb import open_database, QueryOptions

class GoldStandardGUI:
    def __init__(self):
        # ... existing init ...
        self.vector_db = open_database("./data/vectors")

    def semantic_search(self, query: str) -> list:
        """Search past journals and reports semantically."""
        options = QueryOptions()
        options.k = 10
        options.include_metadata = True

        results = self.vector_db.query_text(query, options)

        return [
            {
                "date": r.metadata.date,
                "type": r.metadata.type,
                "score": r.score,
                "file": r.metadata.source_file
            }
            for r in results
        ]
```

---

## Python API Tutorial

### Complete Example: Building a Search System

```python
#!/usr/bin/env python3
"""
Vector Studio - Complete Search System Example
"""

import pyvdb
from pathlib import Path
from datetime import datetime

def create_search_system(db_path: str) -> pyvdb.Database:
    """Initialize or open a search database."""
    db_path = Path(db_path)

    if db_path.exists():
        print(f"Opening existing database: {db_path}")
        return pyvdb.open_database(str(db_path))
    else:
        print(f"Creating new database: {db_path}")
        return pyvdb.create_gold_standard_db(str(db_path))

def ingest_markdown_files(db: pyvdb.Database, directory: str) -> int:
    """Ingest all markdown files from a directory."""
    directory = Path(directory)
    count = 0

    for md_file in directory.glob("**/*.md"):
        try:
            text = md_file.read_text(encoding="utf-8")

            # Parse date from filename if present
            date = None
            if "_202" in md_file.stem:
                date_part = md_file.stem.split("_")[-1]
                try:
                    date = datetime.strptime(date_part, "%Y-%m-%d").strftime("%Y-%m-%d")
                except ValueError:
                    date = datetime.now().strftime("%Y-%m-%d")

            # Detect type from filename
            doc_type = detect_document_type(md_file.name)

            # Create metadata
            meta = pyvdb.Metadata()
            meta.type = doc_type
            meta.date = date or datetime.now().strftime("%Y-%m-%d")
            meta.source_file = str(md_file)

            # Add to database
            db.add_text(text, meta)
            count += 1

        except Exception as e:
            print(f"Error processing {md_file}: {e}")

    return count

def detect_document_type(filename: str) -> pyvdb.DocumentType:
    """Detect document type from filename."""
    filename = filename.lower()

    if "journal" in filename:
        return pyvdb.DocumentType.Journal
    elif "catalyst" in filename:
        return pyvdb.DocumentType.CatalystWatchlist
    elif "premarket" in filename:
        return pyvdb.DocumentType.Premarket
    elif "inst_matrix" in filename:
        return pyvdb.DocumentType.InstitutionalMatrix
    elif "weekly" in filename:
        return pyvdb.DocumentType.WeeklyRundown
    elif "economic" in filename:
        return pyvdb.DocumentType.EconomicCalendar
    else:
        return pyvdb.DocumentType.Unknown

def search(db: pyvdb.Database, query: str, **kwargs) -> list:
    """
    Perform a semantic search.

    Args:
        db: Database instance
        query: Search query string
        k: Number of results (default: 10)
        type_filter: Filter by document type
        date_from: Start date filter
        date_to: End date filter
        min_score: Minimum similarity score

    Returns:
        List of search results
    """
    options = pyvdb.QueryOptions()
    options.k = kwargs.get("k", 10)

    if "type_filter" in kwargs:
        options.type_filter = kwargs["type_filter"]
    if "date_from" in kwargs:
        options.date_from = kwargs["date_from"]
    if "date_to" in kwargs:
        options.date_to = kwargs["date_to"]
    if "min_score" in kwargs:
        options.min_score = kwargs["min_score"]

    options.include_metadata = True

    return db.query_text(query, options)

def pretty_print_results(results: list) -> None:
    """Print search results in a readable format."""
    if not results:
        print("No results found.")
        return

    print(f"\n{'='*60}")
    print(f"Found {len(results)} results")
    print(f"{'='*60}\n")

    for i, r in enumerate(results, 1):
        print(f"[{i}] Score: {r.score:.4f}")
        print(f"    Type: {r.metadata.type}")
        print(f"    Date: {r.metadata.date}")
        if r.metadata.source_file:
            print(f"    File: {r.metadata.source_file}")
        print()

def main():
    """Main demonstration."""
    # Create or open database
    db = create_search_system("./demo_vectors")

    # Ingest some documents
    print("\nIngesting documents...")
    count = ingest_markdown_files(db, "./gold_standard/output")
    print(f"Ingested {count} documents")

    # Perform searches
    print("\n" + "="*60)
    print("SEARCH: 'gold bullish breakout'")
    results = search(db, "gold bullish breakout", k=5)
    pretty_print_results(results)

    print("\n" + "="*60)
    print("SEARCH: 'fed interest rate decision'")
    results = search(db, "fed interest rate decision", k=5)
    pretty_print_results(results)

    print("\n" + "="*60)
    print("SEARCH with date filter (2025-12-01 only)")
    results = search(
        db,
        "market analysis",
        k=5,
        date_from="2025-12-01",
        date_to="2025-12-01"
    )
    pretty_print_results(results)

    # Show stats
    stats = db.stats()
    print("\nDatabase Statistics:")
    print(f"  Total vectors: {stats.count}")
    print(f"  Index size: {stats.index_size_mb:.2f} MB")
    print(f"  Vector storage: {stats.vector_size_mb:.2f} MB")

    # Clean up
    db.sync()
    db.close()
    print("\nDatabase closed.")

if __name__ == "__main__":
    main()
```

---

## CLI Reference

### Basic Commands

```powershell
# Show help
vdb --help

# Show version
vdb --version

# Create new database
vdb create ./my_database

# Database info
vdb info ./my_database
```

### Adding Data

```powershell
# Add text file
vdb add ./my_database --text document.txt

# Add with metadata
vdb add ./my_database --text doc.txt --type Journal --date 2025-12-01

# Add image
vdb add ./my_database --image chart.png --type Chart --asset GOLD

# Batch add from directory
vdb add ./my_database --dir ./reports --type Journal
```

### Searching

```powershell
# Basic search
vdb search ./my_database "gold bullish momentum"

# Search with options
vdb search ./my_database "resistance breakout" --k 10 --min-score 0.7

# Filtered search
vdb search ./my_database "analysis" --type Journal --date-from 2025-11-01

# JSON output
vdb search ./my_database "query" --json > results.json
```

### Maintenance

```powershell
# Show statistics
vdb stats ./my_database

# Verify integrity
vdb verify ./my_database

# Optimize index
vdb optimize ./my_database

# Export for training
vdb export ./my_database --output training_data.json
```

---

## Configuration

### Configuration File

Create `vdb.config.json` in database directory:

```json
{
  "dimension": 512,
  "distance_metric": "cosine",
  "hnsw": {
    "m": 16,
    "m_max0": 32,
    "ef_construction": 200,
    "ef_search": 50
  },
  "storage": {
    "max_elements": 1000000,
    "initial_capacity": 10000,
    "memory_map": true,
    "auto_sync": true,
    "sync_interval_ms": 30000
  },
  "embedding": {
    "models_dir": "~/.cache/vector_studio/models",
    "text_model": "all-MiniLM-L6-v2.onnx",
    "image_model": "clip-vit-base-patch32.onnx",
    "num_threads": 0,
    "batch_size": 32
  }
}
```

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `VDB_MODELS_DIR` | Model directory | `~/.cache/vector_studio/models` |
| `VDB_LOG_LEVEL` | Log level (DEBUG, INFO, WARN, ERROR) | `INFO` |
| `VDB_NUM_THREADS` | Thread count (0 = auto) | `0` |
| `VDB_SIMD` | SIMD level (avx512, avx2, sse4, none) | `auto` |

---

## Best Practices

### Performance Optimization

1. **Use Batch Operations**
   ```python
   # Instead of:
   for doc in documents:
       db.add_text(doc)

   # Use:
   db.add_batch(documents)  # 3-5x faster
   ```

2. **Configure ef_search Based on Needs**
   ```python
   # Speed-focused (95% recall)
   options.ef_search = 50

   # Accuracy-focused (99%+ recall)
   options.ef_search = 200
   ```

3. **Use Filters to Reduce Search Space**
   ```python
   # Filtering happens AFTER approximate search, so be generous with k
   options.k = 50
   options.type_filter = pyvdb.DocumentType.Journal
   final_k = 10  # Filter results down
   ```

4. **Memory-Map for Large Databases**
   ```python
   config.memory_map = True  # Default, don't disable
   ```

### Data Quality

1. **Clean Text Before Embedding**
   ```python
   # Remove excessive whitespace, special characters
   text = " ".join(text.split())
   text = text.strip()
   ```

2. **Use Consistent Date Formats**
   ```python
   # Always: YYYY-MM-DD
   metadata.date = "2025-12-01"
   ```

3. **Add Meaningful Metadata**
   ```python
   # More metadata = better filtering
   meta.type = pyvdb.DocumentType.Journal
   meta.date = "2025-12-01"
   meta.bias = "BULLISH"
   meta.gold_price = 4220.50
   meta.source_file = "output/Journal_2025-12-01.md"
   ```

### Maintenance

1. **Regular Syncs for Durability**
   ```python
   # After batch operations
   db.sync()
   ```

2. **Periodic Optimization**
   ```python
   # Weekly or after major changes
   db.optimize()
   ```

3. **Monitor Statistics**
   ```python
   stats = db.stats()
   if stats.fragmentation > 0.3:
       db.optimize()
   ```

---

## Troubleshooting

### Common Issues

#### "Model file not found"

```
Error: Failed to load model: all-MiniLM-L6-v2.onnx not found
```

**Solution**: Download models first:
```powershell
python scripts/download_models.py
```

#### "Dimension mismatch"

```
Error: Vector dimension 384 doesn't match database dimension 512
```

**Solution**: Don't mix raw model output with projected vectors. Use the API consistently:
```python
# Wrong: adding raw 384-dim embedding
db.add_vector(raw_embedding)

# Right: let the API handle projection
db.add_text("text here")
```

#### "Database locked"

```
Error: Cannot open database: lock file exists
```

**Solution**: Ensure no other process has the database open. Remove stale lock:
```powershell
Remove-Item ./my_database/.lock
```

#### Build Failures

**CMake not found**:
```powershell
.\scripts\build.ps1 -AutoInstall
```

**ONNX Runtime issues**:
```powershell
pip install --force-reinstall onnxruntime
```

### Debug Mode

Enable verbose logging:

```python
import pyvdb

pyvdb.set_log_level(pyvdb.LogLevel.DEBUG)
db = pyvdb.open_database("./vectors")
```

Or via environment:

```powershell
$env:VDB_LOG_LEVEL = "DEBUG"
python my_script.py
```

### Getting Help

1. Check logs in database directory: `./my_database/vdb.log`
2. Verify database integrity: `vdb verify ./my_database`
3. Export diagnostics: `vdb diagnostics ./my_database > diag.txt`

---

## Appendix: Quick Reference Card

```
╔══════════════════════════════════════════════════════════════════╗
║                    VECTOR STUDIO QUICK REFERENCE                 ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                  ║
║  CREATE DATABASE                                                 ║
║  ├─ pyvdb.create_database("path")                               ║
║  └─ pyvdb.create_gold_standard_db("path")                       ║
║                                                                  ║
║  ADD DOCUMENTS                                                   ║
║  ├─ db.add_text("text", metadata)                               ║
║  ├─ db.add_image("image.png", metadata)                         ║
║  └─ db.add_batch([(text, meta), ...])                           ║
║                                                                  ║
║  SEARCH                                                          ║
║  ├─ db.search("query", k=10)                                    ║
║  ├─ db.query_text("query", options)                             ║
║  └─ db.query_image("image.png", options)                        ║
║                                                                  ║
║  MANAGE                                                          ║
║  ├─ db.get_metadata(id)                                         ║
║  ├─ db.update_metadata(id, meta)                                ║
║  ├─ db.remove(id)                                               ║
║  ├─ db.sync()                                                   ║
║  ├─ db.optimize()                                               ║
║  └─ db.close()                                                  ║
║                                                                  ║
║  CLI                                                             ║
║  ├─ vdb create <path>                                           ║
║  ├─ vdb add <path> --text file.txt                              ║
║  ├─ vdb search <path> "query" --k 10                            ║
║  └─ vdb stats <path>                                            ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
```
