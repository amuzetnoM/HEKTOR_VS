---
title: "Python Bindings"
description: "Python API reference and examples"
version: "4.1.5"
last_updated: "2026-01-24"
sidebar_position: 23
category: "Reference"
---
![Version](https://img.shields.io/badge/version-4.1.5-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--24-green?style=flat-square)
![Type](https://img.shields.io/badge/type-python-blue?style=flat-square)
![Backend](https://img.shields.io/badge/backend-pybind11-brightgreen?style=flat-square)

# Python Bindings

## Table of Contents

- [Overview](#overview)
- [Installation](#installation)
- [Getting Started](#getting-started)
- [Core Classes](#core-classes)
  - [VectorDatabase](#vectordatabase)
  - [DatabaseConfig](#databaseconfig)
  - [Metadata](#metadata)
  - [QueryOptions](#queryoptions)
  - [QueryResult](#queryresult)
- [Data Ingestion](#data-ingestion)
  - [GoldStandardIngest](#goldstandardingest)
  - [IngestConfig](#ingestconfig)
  - [IngestStats](#ingeststats)
- [Hybrid Search](#hybrid-search)
  - [BM25Engine](#bm25engine)
  - [HybridSearchEngine](#hybridsearchengine)
  - [FusionMethod](#fusionmethod)
- [LLM Engine](#llm-engine)
  - [LLMEngine](#llmengine)
  - [LLMConfig](#llmconfig)
  - [GenerationParams](#generationparams)
- [Enumerations](#enumerations)
- [Examples](#examples)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

---

## Overview

The `pyvdb` module provides Python bindings for the Hektor vector database engine. Built with pybind11, it offers a high-performance interface to all C++ functionality including:

- **Vector Operations**: Add, query, and manage vectors with numpy support
- **Text & Image Search**: Automatic encoding and semantic search
- **Hybrid Search**: Combine vector and BM25 lexical search
- **LLM Integration**: Local text generation with llama.cpp backend
- **Data Ingestion**: Batch processing and custom data pipelines

**Key Features:**
- Zero-copy numpy array integration
- Automatic error handling with Python exceptions
- Thread-safe operations
- Comprehensive type hints (when using stub files)

---

## Installation

### Prerequisites

```bash
# Python 3.8 or higher
python --version

# Build tools
pip install pybind11 numpy
```

### Build from Source

```bash
# Clone repository
git clone https://github.com/amuzetnoM/hektor.git
cd hektor

# Build Python bindings
mkdir build && cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
make -j$(nproc)

# Install
pip install .
```

### Verify Installation

```python
import pyvdb
print(f"pyvdb version: {pyvdb.__version__}")
```

---

## Getting Started

### Quick Start Example

```python
import pyvdb
import numpy as np

# Create database configuration
config = pyvdb.DatabaseConfig()
config.path = "my_database"
config.dimension = 512
config.metric = pyvdb.DistanceMetric.Cosine

# Create and initialize database
db = pyvdb.create_gold_standard_db("my_database")

# Add text with automatic encoding
text_id = db.add_text(
    "This is a sample document about machine learning",
    pyvdb.DocumentType.Journal,
    "2026-01-08"
)

# Search
results = db.search("machine learning", k=10)
for result in results:
    print(f"ID: {result.id}, Score: {result.score}")
```

### Opening Existing Database

```python
# Open existing database
db = pyvdb.open_database("my_database")

if db.is_ready():
    print(f"Database loaded: {db.size()} vectors")
```

---

## Core Classes

### VectorDatabase

Main database class for vector storage and retrieval.

#### Constructor

```python
db = pyvdb.VectorDatabase(config: DatabaseConfig)
```

**Parameters:**
- `config`: DatabaseConfig - Database configuration object

#### Methods

##### init()

```python
db.init() -> None
```

Initialize the database. Must be called before using the database.

**Raises:** `RuntimeError` if initialization fails

**Example:**
```python
db = pyvdb.VectorDatabase(config)
db.init()  # Initialize before use
```

##### is_ready()

```python
db.is_ready() -> bool
```

Check if database is ready for operations.

**Returns:** `bool` - True if database is initialized and ready

##### add_text()

```python
# Method 1: With metadata
db.add_text(text: str, metadata: Metadata) -> int

# Method 2: Simple form
db.add_text(text: str, type: DocumentType, date: str) -> int
```

Add text document with automatic encoding to vector.

**Parameters:**
- `text`: str - Text content to encode
- `metadata`: Metadata - Complete metadata object
- `type`: DocumentType - Document type (method 2)
- `date`: str - Date string (method 2)

**Returns:** `int` - Assigned vector ID

**Example:**
```python
# Simple form
vec_id = db.add_text(
    "Machine learning enables computers to learn",
    pyvdb.DocumentType.Journal,
    "2026-01-08"
)

# With full metadata
meta = pyvdb.Metadata()
meta.type = pyvdb.DocumentType.Chart
meta.date = "2026-01-08"
meta.asset = "GOLD"
meta.gold_price = 2050.0

vec_id = db.add_text("Gold price analysis", meta)
```

##### query_text()

```python
db.query_text(query: str, options: QueryOptions = QueryOptions()) -> List[QueryResult]
```

Search database with text query (automatic encoding).

**Parameters:**
- `query`: str - Query text
- `options`: QueryOptions - Search options (optional)

**Returns:** `List[QueryResult]` - Search results sorted by relevance

**Example:**
```python
options = pyvdb.QueryOptions()
options.k = 20
options.type_filter = pyvdb.DocumentType.Journal
options.date_from = "2026-01-01"

results = db.query_text("market analysis", options)
```

##### search()

```python
db.search(query: str, k: int = 10) -> List[QueryResult]
```

Simple search interface (convenience method).

**Parameters:**
- `query`: str - Query text
- `k`: int - Number of results (default: 10)

**Returns:** `List[QueryResult]` - Top k results

**Example:**
```python
results = db.search("machine learning trends", k=5)
for r in results:
    print(f"{r.id}: {r.score:.3f}")
```

##### add_image()

```python
# Method 1: With metadata
db.add_image(path: str, metadata: Metadata) -> int

# Method 2: Simple form
db.add_image(path: str) -> int
```

Add image with automatic encoding to vector.

**Parameters:**
- `path`: str - Path to image file
- `metadata`: Metadata - Associated metadata (optional)

**Returns:** `int` - Assigned vector ID

**Example:**
```python
# Simple form
img_id = db.add_image("chart.png")

# With metadata
meta = pyvdb.Metadata()
meta.type = pyvdb.DocumentType.Chart
meta.date = "2026-01-08"

img_id = db.add_image("analysis.png", meta)
```

##### query_image()

```python
db.query_image(path: str, options: QueryOptions = QueryOptions()) -> List[QueryResult]
```

Search database with image query.

**Parameters:**
- `path`: str - Path to query image
- `options`: QueryOptions - Search options (optional)

**Returns:** `List[QueryResult]` - Search results

##### add_vector()

```python
db.add_vector(vector: np.ndarray, metadata: Metadata) -> int
```

Add pre-computed vector directly (numpy array).

**Parameters:**
- `vector`: np.ndarray - 1D float32 array
- `metadata`: Metadata - Associated metadata

**Returns:** `int` - Assigned vector ID

**Example:**
```python
import numpy as np

# Create embedding externally
vector = np.random.randn(512).astype(np.float32)

meta = pyvdb.Metadata()
meta.type = pyvdb.DocumentType.Journal
meta.date = "2026-01-08"

vec_id = db.add_vector(vector, meta)
```

##### query_vector()

```python
db.query_vector(vector: np.ndarray, options: QueryOptions = QueryOptions()) -> List[QueryResult]
```

Search with pre-computed vector.

**Parameters:**
- `vector`: np.ndarray - Query vector (1D float32 array)
- `options`: QueryOptions - Search options

**Returns:** `List[QueryResult]` - Search results

##### get_vector()

```python
db.get_vector(id: int) -> Optional[np.ndarray]
```

Retrieve vector by ID as numpy array.

**Parameters:**
- `id`: int - Vector ID

**Returns:** `np.ndarray` or `None` if not found

**Example:**
```python
vec = db.get_vector(12345)
if vec is not None:
    print(f"Vector shape: {vec.shape}")
```

##### get_metadata()

```python
db.get_metadata(id: int) -> Optional[Metadata]
```

Retrieve metadata for a vector.

**Parameters:**
- `id`: int - Vector ID

**Returns:** `Metadata` or `None` if not found

##### find_by_date()

```python
db.find_by_date(date: str) -> List[VectorId]
```

Find all vectors for a specific date.

**Parameters:**
- `date`: str - Date string (e.g., "2026-01-08")

**Returns:** `List[int]` - Vector IDs

##### find_by_type()

```python
db.find_by_type(type: DocumentType) -> List[VectorId]
```

Find all vectors of a specific document type.

**Parameters:**
- `type`: DocumentType - Document type to filter

**Returns:** `List[int]` - Vector IDs

##### find_by_asset()

```python
db.find_by_asset(asset: str) -> List[VectorId]
```

Find all vectors associated with an asset.

**Parameters:**
- `asset`: str - Asset name (e.g., "GOLD", "SILVER")

**Returns:** `List[int]` - Vector IDs

##### remove()

```python
db.remove(id: int) -> None
```

Remove vector from database.

**Parameters:**
- `id`: int - Vector ID to remove

**Raises:** `RuntimeError` if removal fails

##### remove_by_date()

```python
db.remove_by_date(date: str) -> int
```

Remove all vectors from a specific date.

**Parameters:**
- `date`: str - Date string

**Returns:** `int` - Number of vectors removed

##### size()

```python
db.size() -> int
len(db) -> int  # Also works
```

Get total number of vectors in database.

**Returns:** `int` - Vector count

##### count_by_type()

```python
db.count_by_type(type: DocumentType) -> int
```

Count vectors of specific type.

**Parameters:**
- `type`: DocumentType - Type to count

**Returns:** `int` - Count

##### all_dates()

```python
db.all_dates() -> List[str]
```

Get all unique dates in database.

**Returns:** `List[str]` - Sorted list of dates

##### stats()

```python
db.stats() -> IndexStats
```

Get database statistics.

**Returns:** `IndexStats` - Database metrics

**Example:**
```python
stats = db.stats()
print(f"Vectors: {stats.total_vectors}")
print(f"Dimension: {stats.dimension}")
print(f"Memory: {stats.memory_usage_bytes / 1024 / 1024:.2f} MB")
```

##### optimize()

```python
db.optimize() -> None
```

Optimize database index for better search performance.

##### sync()

```python
db.sync() -> None
```

Synchronize database to disk.

**Raises:** `RuntimeError` if sync fails

##### compact()

```python
db.compact() -> None
```

Compact database storage (remove deleted vectors).

**Raises:** `RuntimeError` if compaction fails

##### export_training_data()

```python
db.export_training_data(output_path: str) -> None
```

Export database vectors for training.

**Parameters:**
- `output_path`: str - Path to export file

---

### DatabaseConfig

Configuration for VectorDatabase.

#### Attributes

```python
config = pyvdb.DatabaseConfig()

config.path: str                     # Database directory path
config.dimension: int                # Vector dimensionality (default: 512)
config.metric: DistanceMetric        # Distance metric (default: Cosine)
config.hnsw_m: int                   # HNSW M parameter (default: 16)
config.hnsw_ef_construction: int     # Build quality (default: 200)
config.hnsw_ef_search: int           # Search quality (default: 50)
config.max_elements: int             # Initial capacity (default: 1000000)
config.text_model_path: str          # Path to text encoder model (optional)
config.image_model_path: str         # Path to image encoder model (optional)
config.vocab_path: str               # Path to vocabulary file (optional)
config.num_threads: int              # Thread count (default: auto)
config.memory_only: bool             # In-memory mode (default: False)
config.auto_sync: bool               # Auto-sync to disk (default: True)
```

#### Example

```python
config = pyvdb.DatabaseConfig()
config.path = "my_vectors"
config.dimension = 768
config.metric = pyvdb.DistanceMetric.L2
config.hnsw_m = 32
config.hnsw_ef_construction = 400
config.num_threads = 8

db = pyvdb.VectorDatabase(config)
db.init()
```

---

### Metadata

Metadata associated with vectors.

#### Attributes

```python
meta = pyvdb.Metadata()

meta.id: int                    # Vector ID
meta.type: DocumentType         # Document type
meta.date: str                  # Date string
meta.source_file: str           # Source file path
meta.asset: str                 # Asset name (e.g., "GOLD")
meta.bias: str                  # Bias/sentiment
meta.gold_price: float          # Gold price
meta.silver_price: float        # Silver price
meta.gsr: float                 # Gold/Silver ratio
meta.dxy: float                 # Dollar index
meta.vix: float                 # Volatility index
meta.yield_10y: float           # 10-year treasury yield
```

#### Example

```python
meta = pyvdb.Metadata()
meta.type = pyvdb.DocumentType.Journal
meta.date = "2026-01-08"
meta.asset = "GOLD"
meta.gold_price = 2050.75
meta.bias = "bullish"

vec_id = db.add_text("Market analysis", meta)
```

---

### QueryOptions

Options for search queries.

#### Attributes

```python
options = pyvdb.QueryOptions()

options.k: int                           # Number of results
options.ef_search: int                   # HNSW search quality
options.type_filter: Optional[DocumentType]  # Filter by type
options.date_filter: bool                # Enable date filtering
options.date_from: str                   # Start date
options.date_to: str                     # End date
options.asset_filter: str                # Filter by asset
options.bias_filter: str                 # Filter by bias
options.include_metadata: bool           # Include full metadata
options.deduplicate_by_date: bool        # Remove date duplicates
```

#### Example

```python
options = pyvdb.QueryOptions()
options.k = 50
options.type_filter = pyvdb.DocumentType.Journal
options.date_filter = True
options.date_from = "2025-01-01"
options.date_to = "2026-01-08"
options.asset_filter = "GOLD"
options.include_metadata = True

results = db.query_text("gold market", options)
```

---

### QueryResult

Search result with score and metadata.

#### Attributes

```python
result: QueryResult

result.id: int              # Vector ID (read-only)
result.distance: float      # Raw distance (read-only)
result.score: float         # Normalized score (read-only)
result.metadata: Metadata   # Associated metadata (read-only)
```

#### Example

```python
results = db.search("market analysis", k=10)
for result in results:
    print(f"ID: {result.id}")
    print(f"Score: {result.score:.3f}")
    print(f"Type: {result.metadata.type}")
    print(f"Date: {result.metadata.date}")
```

---

## Data Ingestion

### GoldStandardIngest

Data ingestion pipeline for Gold Standard format.

#### Constructor

```python
ingest = pyvdb.GoldStandardIngest(db: VectorDatabase)
```

**Parameters:**
- `db`: VectorDatabase - Target database

#### Methods

##### ingest()

```python
ingest.ingest(config: IngestConfig) -> IngestStats
```

Batch ingest with configuration.

**Parameters:**
- `config`: IngestConfig - Ingestion configuration

**Returns:** `IngestStats` - Ingestion statistics

**Example:**
```python
config = pyvdb.IngestConfig()
config.gold_standard_output = "/path/to/data"
config.include_journals = True
config.include_charts = True
config.include_reports = True
config.incremental = True
config.since_date = "2026-01-01"

ingest = pyvdb.GoldStandardIngest(db)
stats = ingest.ingest(config)

print(f"Journals: {stats.journals_added}")
print(f"Charts: {stats.charts_added}")
print(f"Reports: {stats.reports_added}")
print(f"Errors: {stats.errors}")
```

##### ingest_journal()

```python
ingest.ingest_journal(path: str) -> int
```

Ingest single journal file.

**Parameters:**
- `path`: str - Path to journal file

**Returns:** `int` - Vector ID

##### ingest_chart()

```python
ingest.ingest_chart(path: str) -> int
```

Ingest single chart image.

**Parameters:**
- `path`: str - Path to chart image

**Returns:** `int` - Vector ID

##### ingest_report()

```python
ingest.ingest_report(path: str) -> int
```

Ingest single report file.

**Parameters:**
- `path`: str - Path to report file

**Returns:** `int` - Vector ID

##### last_stats()

```python
ingest.last_stats() -> IngestStats
```

Get statistics from last ingestion.

**Returns:** `IngestStats` - Statistics

---

### IngestConfig

Configuration for data ingestion.

#### Attributes

```python
config = pyvdb.IngestConfig()

config.gold_standard_output: str    # Data directory path
config.include_journals: bool       # Include journal files
config.include_charts: bool         # Include chart images
config.include_reports: bool        # Include report files
config.overwrite_existing: bool     # Overwrite existing data
config.incremental: bool            # Incremental mode
config.since_date: str              # Process data since date
```

---

### IngestStats

Statistics from ingestion operation.

#### Attributes

```python
stats: IngestStats

stats.journals_added: int           # Journal files added
stats.charts_added: int             # Chart images added
stats.reports_added: int            # Report files added
stats.skipped: int                  # Files skipped
stats.errors: int                   # Error count
stats.error_messages: List[str]     # Error messages
```

---

## Hybrid Search

### BM25Engine

BM25 lexical search engine.

#### Constructor

```python
# Default configuration
bm25 = pyvdb.BM25Engine()

# Custom configuration
config = pyvdb.BM25Config()
config.k1 = 1.5
config.b = 0.75
config.use_stemming = True

bm25 = pyvdb.BM25Engine(config)
```

#### Methods

##### add_document()

```python
bm25.add_document(id: int, content: str) -> None
```

Add document to BM25 index.

**Parameters:**
- `id`: int - Document ID
- `content`: str - Text content

**Example:**
```python
bm25 = pyvdb.BM25Engine()
bm25.add_document(1, "Machine learning is a subset of AI")
bm25.add_document(2, "Deep learning uses neural networks")
```

##### remove_document()

```python
bm25.remove_document(id: int) -> None
```

Remove document from index.

##### update_document()

```python
bm25.update_document(id: int, content: str) -> None
```

Update document content.

##### search()

```python
bm25.search(query: str, k: int = 10, min_score: float = 0.0) -> List[BM25Result]
```

Search documents with BM25.

**Parameters:**
- `query`: str - Search query
- `k`: int - Number of results (default: 10)
- `min_score`: float - Minimum score threshold (default: 0.0)

**Returns:** `List[BM25Result]` - Search results

**Example:**
```python
results = bm25.search("machine learning", k=20, min_score=1.0)
for result in results:
    print(f"ID: {result.id}, Score: {result.score}")
    print(f"Matched: {result.matched_terms}")
```

##### document_count()

```python
bm25.document_count() -> int
```

Get number of indexed documents.

##### term_count()

```python
bm25.term_count() -> int
```

Get number of unique terms.

##### average_document_length()

```python
bm25.average_document_length() -> float
```

Get average document length.

##### save()

```python
bm25.save(path: str) -> None
```

Save index to file.

##### load() (static)

```python
bm25 = pyvdb.BM25Engine.load(path: str)
```

Load index from file.

**Example:**
```python
# Save
bm25.save("bm25_index.bin")

# Load
bm25_loaded = pyvdb.BM25Engine.load("bm25_index.bin")
```

---

### HybridSearchEngine

Combines vector and BM25 search results.

#### Constructor

```python
# Default configuration
hybrid = pyvdb.HybridSearchEngine()

# Custom configuration
config = pyvdb.HybridSearchConfig()
config.vector_weight = 0.7
config.lexical_weight = 0.3
config.fusion = pyvdb.FusionMethod.RRF

hybrid = pyvdb.HybridSearchEngine(config)
```

#### Methods

##### combine()

```python
hybrid.combine(
    vector_results: List[QueryResult],
    lexical_results: List[BM25Result],
    k: int = 10
) -> List[HybridResult]
```

Combine vector and lexical search results.

**Parameters:**
- `vector_results`: List[QueryResult] - Vector search results
- `lexical_results`: List[BM25Result] - BM25 search results
- `k`: int - Number of final results

**Returns:** `List[HybridResult]` - Combined results

**Example:**
```python
# Perform both searches
vec_results = db.search("machine learning", k=50)
bm25_results = bm25.search("machine learning", k=50)

# Combine
hybrid = pyvdb.HybridSearchEngine()
combined = hybrid.combine(vec_results, bm25_results, k=20)

for result in combined:
    print(f"ID: {result.id}")
    print(f"Combined: {result.combined_score:.3f}")
    print(f"Vector: {result.vector_score:.3f}")
    print(f"Lexical: {result.lexical_score:.3f}")
    print(f"Keywords: {result.matched_keywords}")
```

##### Static Fusion Methods

```python
# Weighted sum
score = pyvdb.HybridSearchEngine.weighted_sum(
    vec_score: float,
    lex_score: float,
    vec_weight: float
) -> float

# Reciprocal Rank Fusion
score = pyvdb.HybridSearchEngine.reciprocal_rank_fusion(
    vec_rank: int,
    lex_rank: int,
    k: float
) -> float

# CombSUM
score = pyvdb.HybridSearchEngine.comb_sum(
    vec_score: float,
    lex_score: float
) -> float

# CombMNZ
score = pyvdb.HybridSearchEngine.comb_mnz(
    vec_score: float,
    lex_score: float,
    num_systems: int
) -> float
```

---

### FusionMethod

Enumeration of fusion methods for hybrid search.

```python
pyvdb.FusionMethod.WeightedSum    # Weighted combination of scores
pyvdb.FusionMethod.RRF            # Reciprocal Rank Fusion
pyvdb.FusionMethod.CombSUM        # Sum of normalized scores
pyvdb.FusionMethod.CombMNZ        # CombSUM * number of systems
pyvdb.FusionMethod.Borda          # Borda count voting
```

---

## LLM Engine

### LLMEngine

Local text generation with llama.cpp backend.

**Note:** Only available when compiled with `VDB_USE_LLAMA_CPP=ON`

#### Check Availability

```python
if pyvdb.has_llm_support():
    # LLM features available
    engine = pyvdb.create_llm_engine()
else:
    print("LLM support not compiled")
```

#### Constructor

```python
engine = pyvdb.create_llm_engine()
```

#### Methods

##### load()

```python
engine.load(config: LLMConfig) -> None
```

Load GGUF model file.

**Parameters:**
- `config`: LLMConfig - Model configuration

**Example:**
```python
config = pyvdb.LLMConfig()
config.model_path = "models/llama-2-7b-q4.gguf"
config.n_ctx = 4096
config.n_threads = 8
config.n_gpu_layers = 32

engine.load(config)
```

##### is_loaded()

```python
engine.is_loaded() -> bool
```

Check if model is loaded.

##### unload()

```python
engine.unload() -> None
```

Unload model from memory.

##### generate()

```python
engine.generate(prompt: str, params: GenerationParams = GenerationParams()) -> ChatCompletionResult
```

Generate text from prompt.

**Parameters:**
- `prompt`: str - Input prompt
- `params`: GenerationParams - Generation parameters

**Returns:** `ChatCompletionResult` - Generated text and metadata

**Example:**
```python
params = pyvdb.GenerationParams()
params.max_tokens = 512
params.temperature = 0.7
params.top_p = 0.9

result = engine.generate("Explain quantum computing", params)
print(result.content)
print(f"Tokens: {result.tokens_generated}")
print(f"Time: {result.generation_time_ms}ms")
```

##### chat()

```python
engine.chat(messages: List[Message], params: GenerationParams = GenerationParams()) -> ChatCompletionResult
```

Chat completion with message history.

**Parameters:**
- `messages`: List[Message] - Conversation messages
- `params`: GenerationParams - Generation parameters

**Returns:** `ChatCompletionResult` - Generated response

**Example:**
```python
messages = [
    pyvdb.Message(pyvdb.Role.System, "You are a helpful assistant"),
    pyvdb.Message(pyvdb.Role.User, "What is machine learning?"),
]

result = engine.chat(messages)
print(result.content)
```

##### count_tokens()

```python
engine.count_tokens(text: str) -> int
```

Count tokens in text.

---

### LLMConfig

Configuration for LLM engine.

#### Attributes

```python
config = pyvdb.LLMConfig()

config.model_path: str      # Path to GGUF model file
config.n_ctx: int          # Context window size (default: 2048)
config.n_batch: int        # Batch size (default: 512)
config.n_threads: int      # CPU threads (default: auto)
config.n_gpu_layers: int   # GPU layers (default: 0)
config.use_mmap: bool      # Use memory mapping (default: True)
config.use_mlock: bool     # Lock memory (default: False)
```

---

### GenerationParams

Parameters for text generation.

#### Attributes

```python
params = pyvdb.GenerationParams()

params.max_tokens: int              # Maximum tokens to generate
params.temperature: float           # Sampling temperature (0-2)
params.top_p: float                 # Nucleus sampling (0-1)
params.top_k: int                   # Top-k sampling
params.repeat_penalty: float        # Repetition penalty (1.0+)
params.stop_sequences: List[str]    # Stop generation on these
```

---

## Enumerations

### DistanceMetric

```python
pyvdb.DistanceMetric.Cosine       # Cosine distance
pyvdb.DistanceMetric.L2           # Euclidean distance
pyvdb.DistanceMetric.DotProduct   # Negative dot product
```

### DocumentType

```python
pyvdb.DocumentType.Journal
pyvdb.DocumentType.Chart
pyvdb.DocumentType.CatalystWatchlist
pyvdb.DocumentType.InstitutionalMatrix
pyvdb.DocumentType.EconomicCalendar
pyvdb.DocumentType.WeeklyRundown
pyvdb.DocumentType.ThreeMonthReport
pyvdb.DocumentType.OneYearReport
pyvdb.DocumentType.MonthlyReport
pyvdb.DocumentType.PreMarket
pyvdb.DocumentType.Unknown
```

### Device (if ONNX Runtime enabled)

```python
pyvdb.Device.CPU          # CPU execution
pyvdb.Device.CUDA         # NVIDIA CUDA
pyvdb.Device.DirectML     # DirectML (Windows)
```

### Role (for LLM chat)

```python
pyvdb.Role.System         # System message
pyvdb.Role.User           # User message
pyvdb.Role.Assistant      # Assistant message
```

---

## Examples

### Complete Workflow Example

```python
import pyvdb
import numpy as np

# 1. Create database
config = pyvdb.DatabaseConfig()
config.path = "financial_db"
config.dimension = 512
config.metric = pyvdb.DistanceMetric.Cosine

db = pyvdb.create_gold_standard_db("financial_db")

# 2. Add documents
documents = [
    ("Gold prices surge to new highs", "2026-01-08", "GOLD"),
    ("Silver market shows strength", "2026-01-08", "SILVER"),
    ("Market volatility increases", "2026-01-07", "VIX"),
]

for content, date, asset in documents:
    meta = pyvdb.Metadata()
    meta.type = pyvdb.DocumentType.Journal
    meta.date = date
    meta.asset = asset
    
    vec_id = db.add_text(content, meta)
    print(f"Added: {vec_id}")

# 3. Search
results = db.search("gold market analysis", k=10)
for r in results:
    print(f"Score: {r.score:.3f}, Date: {r.metadata.date}")

# 4. Statistics
stats = db.stats()
print(f"Total vectors: {stats.total_vectors}")
print(f"Memory usage: {stats.memory_usage_bytes / 1024 / 1024:.2f} MB")

# 5. Save
db.sync()
```

### Hybrid Search Example

```python
import pyvdb

# Setup
db = pyvdb.open_database("financial_db")
bm25 = pyvdb.BM25Engine()

# Index documents in BM25
docs = db.all_dates()
for doc_id in range(db.size()):
    meta = db.get_metadata(doc_id)
    if meta:
        # Add text content to BM25 (you'd need to store/retrieve text)
        bm25.add_document(doc_id, meta.asset)  # Example

# Perform hybrid search
query = "gold silver ratio"

# Vector search
vec_results = db.search(query, k=50)

# BM25 search
bm25_results = bm25.search(query, k=50)

# Combine
config = pyvdb.HybridSearchConfig()
config.vector_weight = 0.6
config.lexical_weight = 0.4
config.fusion = pyvdb.FusionMethod.RRF

hybrid = pyvdb.HybridSearchEngine(config)
combined = hybrid.combine(vec_results, bm25_results, k=20)

for result in combined:
    print(f"ID: {result.id}, Combined: {result.combined_score:.3f}")
```

### LLM Integration Example

```python
import pyvdb

if not pyvdb.has_llm_support():
    print("LLM support not available")
    exit(1)

# Create LLM engine
engine = pyvdb.create_llm_engine()

# Load model
config = pyvdb.LLMConfig()
config.model_path = "models/llama-2-7b-q4.gguf"
config.n_ctx = 4096
config.n_threads = 8

engine.load(config)

# Generate
params = pyvdb.GenerationParams()
params.max_tokens = 256
params.temperature = 0.7

result = engine.generate("Explain the gold standard", params)
print(result.content)

# Chat
messages = [
    pyvdb.Message(pyvdb.Role.System, "You are a financial analyst"),
    pyvdb.Message(pyvdb.Role.User, "What affects gold prices?"),
]

chat_result = engine.chat(messages, params)
print(chat_result.content)
```

### Batch Processing Example

```python
import pyvdb
from pathlib import Path

# Setup
db = pyvdb.open_database("financial_db")
ingest = pyvdb.GoldStandardIngest(db)

# Configure ingestion
config = pyvdb.IngestConfig()
config.gold_standard_output = "/data/gold_standard"
config.include_journals = True
config.include_charts = True
config.incremental = True
config.since_date = "2026-01-01"

# Ingest
stats = ingest.ingest(config)

print(f"Journals: {stats.journals_added}")
print(f"Charts: {stats.charts_added}")
print(f"Errors: {stats.errors}")

if stats.errors > 0:
    for msg in stats.error_messages:
        print(f"Error: {msg}")
```

### Numpy Integration Example

```python
import pyvdb
import numpy as np

db = pyvdb.open_database("vectors_db")

# Add vectors from numpy
embeddings = np.random.randn(100, 512).astype(np.float32)

for i, vec in enumerate(embeddings):
    meta = pyvdb.Metadata()
    meta.type = pyvdb.DocumentType.Journal
    meta.date = f"2026-01-{i:02d}"
    
    vec_id = db.add_vector(vec, meta)

# Query with numpy
query_vec = np.random.randn(512).astype(np.float32)
results = db.query_vector(query_vec)

# Retrieve as numpy
for r in results[:5]:
    stored_vec = db.get_vector(r.id)
    if stored_vec is not None:
        similarity = np.dot(query_vec, stored_vec)
        print(f"ID: {r.id}, Similarity: {similarity:.3f}")
```

---

## Best Practices

### Memory Management

```python
# Always sync before closing
db.sync()

# For large datasets, use compact periodically
if db.size() > 1000000:
    db.compact()
```

### Error Handling

```python
try:
    vec_id = db.add_text("content", meta)
except RuntimeError as e:
    print(f"Error adding text: {e}")
```

### Performance Optimization

```python
# Use batch operations when possible
config = pyvdb.DatabaseConfig()
config.num_threads = 16  # Utilize multiple cores
config.hnsw_ef_construction = 400  # Higher quality index

# Adjust search quality
options = pyvdb.QueryOptions()
options.ef_search = 100  # Higher = more accurate but slower
```

### Thread Safety

```python
# Read operations are thread-safe
from concurrent.futures import ThreadPoolExecutor

queries = ["query1", "query2", "query3"]

with ThreadPoolExecutor(max_workers=4) as executor:
    results = list(executor.map(lambda q: db.search(q, k=10), queries))

# Write operations need external synchronization
import threading

lock = threading.Lock()

def add_with_lock(text, meta):
    with lock:
        return db.add_text(text, meta)
```

---

## Troubleshooting

### Import Error

```python
# Error: ModuleNotFoundError: No module named 'pyvdb'
# Solution: Rebuild and install
cd hektor/build
make pyvdb
pip install --force-reinstall .
```

### Dimension Mismatch

```python
# Error: RuntimeError: Expected 1D array
# Solution: Ensure numpy arrays are 1D
vector = np.random.randn(512)  # Correct
vector_2d = np.random.randn(1, 512)  # Wrong - 2D

# Convert if needed
vector = vector_2d.flatten()
```

### Model Loading Issues

```python
# Check if model path is correct
config = pyvdb.DatabaseConfig()
config.text_model_path = "path/to/model.onnx"

import os
if not os.path.exists(config.text_model_path):
    print("Model file not found!")
```

### Memory Issues

```python
# For large databases, use memory_only=False
config = pyvdb.DatabaseConfig()
config.memory_only = False  # Store on disk
config.auto_sync = True     # Auto-save

# Monitor memory
stats = db.stats()
print(f"Memory: {stats.memory_usage_bytes / 1024 / 1024:.2f} MB")
```

### LLM Not Available

```python
# Check if LLM support is compiled
if not pyvdb.has_llm_support():
    print("Rebuild with: cmake .. -DVDB_USE_LLAMA_CPP=ON")
```

### Performance Issues

```python
# Increase HNSW parameters for better performance
config = pyvdb.DatabaseConfig()
config.hnsw_m = 32              # More connections
config.hnsw_ef_construction = 400  # Better quality
config.hnsw_ef_search = 100     # Better search

# Use more threads
config.num_threads = os.cpu_count()
```

---

## API Summary Table

| Category | Method | Description |
|----------|--------|-------------|
| **Database** | `create_gold_standard_db()` | Create new database |
| | `open_database()` | Open existing database |
| | `db.init()` | Initialize database |
| | `db.is_ready()` | Check if ready |
| **Text** | `db.add_text()` | Add text with encoding |
| | `db.query_text()` | Search with text |
| | `db.search()` | Simple text search |
| **Image** | `db.add_image()` | Add image with encoding |
| | `db.query_image()` | Search with image |
| **Vector** | `db.add_vector()` | Add numpy vector |
| | `db.query_vector()` | Search with vector |
| | `db.get_vector()` | Retrieve vector |
| **Metadata** | `db.get_metadata()` | Get metadata |
| | `db.find_by_date()` | Find by date |
| | `db.find_by_type()` | Find by type |
| **Management** | `db.remove()` | Remove vector |
| | `db.sync()` | Save to disk |
| | `db.compact()` | Compact storage |
| | `db.stats()` | Get statistics |
| **Ingest** | `ingest.ingest()` | Batch ingest |
| | `ingest.ingest_journal()` | Ingest journal |
| **Hybrid** | `bm25.add_document()` | Add to BM25 |
| | `bm25.search()` | BM25 search |
| | `hybrid.combine()` | Combine results |
| **LLM** | `engine.load()` | Load model |
| | `engine.generate()` | Generate text |
| | `engine.chat()` | Chat completion |

---

**Next**: [Custom Development](23_CUSTOM_DEVELOPMENT.md)  
**Previous**: [Usage Examples](21_USAGE_EXAMPLES.md)  
**Index**: [Documentation Index](01_INTRODUCTION.md)
