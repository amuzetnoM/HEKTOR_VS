# Vector Studio - Complete User Guide
## Professional Documentation for High-Performance Vector Database

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Installation](#installation)
4. [Core Concepts](#core-concepts)
5. [Data Ingestion](#data-ingestion)
6. [Vector Operations](#vector-operations)
7. [Search & Retrieval](#search--retrieval)
8. [API Reference](#api-reference)
9. [Python Bindings](#python-bindings)
10. [CLI Tool](#cli-tool)
11. [Performance Tuning](#performance-tuning)
12. [Troubleshooting](#troubleshooting)
13. [Advanced Topics](#advanced-topics)
14. [Examples & Tutorials](#examples--tutorials)

---

## 1. Introduction

### What is Vector Studio?

Vector Studio is a high-fidelity, high-performance vector database built with modern C++23. It provides:

- **Sub-millisecond search** on millions of vectors
- **Universal data ingestion** from CSV, JSON, APIs, and more
- **SIMD-optimized operations** (AVX2/AVX-512)
- **Local embeddings** via ONNX Runtime
- **Thread-safe** concurrent operations
- **Python bindings** for easy integration

### Key Features

| Feature | Description | Benefit |
|---------|-------------|---------|
| **HNSW Indexing** | Hierarchical Navigable Small World graphs | O(log n) search complexity, 99%+ recall |
| **SIMD Optimization** | AVX2/AVX-512 accelerated distance calculations | 4-8x faster than scalar operations |
| **Memory-Mapped Storage** | Zero-copy vector access | Minimal memory overhead, fast I/O |
| **Universal Adapters** | CSV, JSON, PDF, Excel, APIs | Ingest any data format seamlessly |
| **Local Embeddings** | ONNX-based text/image encoding | No external API dependencies |
| **Cross-Modal Search** | Unified embedding space | Search text with images and vice versa |

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                       VECTOR STUDIO                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  USER INTERFACES                                                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │  Python  │  │  C++ CLI │  │ REST API │  │   Web    │       │
│  │  pyvdb   │  │  vdb_cli │  │ (future) │  │ (future) │       │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘       │
│       └────────────┬┴────────────┬─┴──────────────┘             │
│                    │             │                               │
│  CORE ENGINE (C++23)                                            │
│  ┌────────────────┴─────────────┴────────────────────────────┐ │
│  │                                                             │ │
│  │  DATA INGESTION              VECTOR OPS       STORAGE      │ │
│  │  ┌─────────────┐           ┌───────────┐  ┌───────────┐  │ │
│  │  │  Adapters   │           │   HNSW    │  │  Memory   │  │ │
│  │  │  • CSV      │  ───────→ │   Index   │  │  Mapped   │  │ │
│  │  │  • JSON     │           │           │  │  Storage  │  │ │
│  │  │  • PDF      │           │  O(log n) │  │           │  │ │
│  │  │  • Excel    │           │  Search   │  │  vectors  │  │ │
│  │  │  • APIs     │           │           │  │  .bin     │  │ │
│  │  └─────────────┘           └───────────┘  └───────────┘  │ │
│  │                                                             │ │
│  │  EMBEDDINGS                                                │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │  ONNX Runtime                                         │ │ │
│  │  │  • Text: MiniLM-L6 (384-dim)                         │ │ │
│  │  │  • Image: CLIP ViT (512-dim)                         │ │ │
│  │  │  • Local inference, no API calls                     │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Getting Started

### System Requirements

**Minimum:**
- CPU: x86_64 with SSE4.2
- RAM: 4 GB
- Disk: 1 GB free space
- OS: Windows 10/11, Linux (Ubuntu 20.04+), macOS 11+

**Recommended:**
- CPU: x86_64 with AVX2 or AVX-512
- RAM: 16 GB
- Disk: SSD with 10 GB free space
- OS: Linux (Ubuntu 22.04+)

### Quick Installation

**Ubuntu/Debian:**
```bash
# Install dependencies
sudo apt update
sudo apt install build-essential cmake ninja-build python3-dev

# Clone repository
git clone https://github.com/amuzetnoM/vector_studio.git
cd vector_studio

# Build
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja

# Install
sudo ninja install
```

**Windows (PowerShell):**
```powershell
# Install via scripts
git clone https://github.com/amuzetnoM/vector_studio.git
cd vector_studio
.\scripts\setup.ps1
.\scripts\build.ps1 -Release
```

**macOS:**
```bash
# Install dependencies via Homebrew
brew install cmake ninja python3

# Clone and build
git clone https://github.com/amuzetnoM/vector_studio.git
cd vector_studio
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja
```

### First Steps

**1. Create a Database:**
```cpp
#include <vdb/database.hpp>

using namespace vdb;

// Create database
DatabaseConfig config;
config.path = "my_database";
config.dimension = 512;
config.metric = DistanceMetric::Cosine;

auto db_result = create_gold_standard_db("my_database");
if (!db_result) {
    std::cerr << "Error: " << db_result.error().message << std::endl;
    return 1;
}

VectorDatabase db = std::move(*db_result);
```

**2. Ingest Data:**
```cpp
#include <vdb/adapters/data_adapter.hpp>

using namespace vdb::adapters;

DataAdapterManager manager;

// Parse CSV file
auto result = manager.auto_parse("data.csv");
if (result) {
    for (const auto& chunk : result->chunks) {
        // Generate embedding and add to database
        auto embedding = text_encoder->encode(chunk.content);
        
        Metadata meta;
        meta.content = chunk.content;
        meta.source_file = "data.csv";
        
        db.add_vector(*embedding, meta);
    }
}
```

**3. Search:**
```cpp
// Perform similarity search
auto query_embedding = text_encoder->encode("your search query");
auto results = db.search(*query_embedding, 10);  // Top 10 results

for (const auto& result : results) {
    std::cout << "Score: " << result.score << std::endl;
    std::cout << "Content: " << result.metadata.content << std::endl;
}
```

---

## 3. Installation

### Build from Source

#### Prerequisites

**Required:**
- C++23 compatible compiler (GCC 13+, Clang 16+, MSVC 19.35+)
- CMake 3.20+
- Ninja or Make
- Python 3.10+ (for bindings)

**Optional:**
- CUDA 11.8+ (for GPU acceleration)
- AVX2/AVX-512 CPU (for SIMD optimization)

#### Detailed Build Steps

**1. Clone Repository:**
```bash
git clone --recursive https://github.com/amuzetnoM/vector_studio.git
cd vector_studio
```

**2. Install Dependencies:**

*Ubuntu/Debian:*
```bash
sudo apt install \
    build-essential \
    cmake \
    ninja-build \
    python3-dev \
    python3-pip \
    libjson-c-dev \
    zlib1g-dev
```

*Fedora/RHEL:*
```bash
sudo dnf install \
    gcc-c++ \
    cmake \
    ninja-build \
    python3-devel \
    json-c-devel \
    zlib-devel
```

*macOS:*
```bash
brew install cmake ninja python3 json-c zlib
```

**3. Configure Build:**
```bash
mkdir build && cd build

cmake .. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DVDB_BUILD_TESTS=ON \
    -DVDB_BUILD_PYTHON=ON \
    -DVDB_USE_AVX2=ON
```

**Build Options:**

| Option | Values | Description |
|--------|--------|-------------|
| `CMAKE_BUILD_TYPE` | Debug, Release, RelWithDebInfo | Build configuration |
| `VDB_BUILD_TESTS` | ON/OFF | Build unit tests |
| `VDB_BUILD_PYTHON` | ON/OFF | Build Python bindings |
| `VDB_BUILD_BENCHMARKS` | ON/OFF | Build benchmarks |
| `VDB_USE_AVX2` | ON/OFF | Enable AVX2 SIMD |
| `VDB_USE_AVX512` | ON/OFF | Enable AVX-512 SIMD |
| `VDB_ENABLE_GPU` | ON/OFF | Enable GPU acceleration |

**4. Build:**
```bash
ninja
```

**5. Run Tests:**
```bash
./vdb_tests
```

**6. Install:**
```bash
sudo ninja install
```

### Python Package Installation

**From source:**
```bash
cd vector_studio
pip install .
```

**Development mode:**
```bash
pip install -e .
```

**Verify installation:**
```python
import pyvdb
print(pyvdb.__version__)
```

---

## 4. Core Concepts

### Vectors

**What are vectors?**

Vectors are numerical representations of data (text, images, etc.) in a high-dimensional space. Similar items have vectors that are close together.

**Example:**
```
"cat"    → [0.2, 0.8, 0.1, ...]  (384 dimensions)
"kitten" → [0.3, 0.7, 0.2, ...]  (close to "cat")
"car"    → [0.9, 0.1, 0.8, ...]  (far from "cat")
```

### Embeddings

**Embedding Models:**

Vector Studio uses ONNX Runtime for local embedding generation:

| Model | Type | Dimensions | Use Case |
|-------|------|------------|----------|
| MiniLM-L6 | Text | 384 | General text, documents |
| CLIP ViT | Image | 512 | Images, visual search |
| Custom | Any | Configurable | Your trained models |

**Generating Embeddings:**

```cpp
// Text embedding
TextEncoderConfig config;
config.model_path = "models/minilm-l6.onnx";
config.device = Device::CPU;

TextEncoder encoder(config);
auto embedding = encoder.encode("Hello, world!");

// Image embedding
ImageEncoderConfig img_config;
img_config.model_path = "models/clip-vit.onnx";

ImageEncoder img_encoder(img_config);
auto img_embedding = img_encoder.encode("photo.jpg");
```

### Distance Metrics

**Supported Metrics:**

| Metric | Formula | Use Case | Range |
|--------|---------|----------|-------|
| **Cosine** | 1 - (A·B)/(‖A‖‖B‖) | Text, normalized vectors | [0, 2] |
| **Euclidean** | ‖A - B‖ | General purpose | [0, ∞) |
| **Dot Product** | A·B | Magnitude-sensitive | (-∞, ∞) |

**Choosing a Metric:**

- **Cosine:** Best for text and when magnitude doesn't matter
- **Euclidean:** When absolute distance is important
- **Dot Product:** When magnitude carries meaning

### HNSW Index

**How HNSW Works:**

HNSW (Hierarchical Navigable Small World) creates a multi-layer graph structure:

```
Layer 2:  A -------- B           (sparse, long-range connections)
           \        /
Layer 1:   C -- D -- E -- F      (denser connections)
            \ | / \ | /
Layer 0:    G-H-I-J-K-L-M-N     (all points, local connections)
```

**Search Process:**
1. Start at top layer
2. Navigate to closest point
3. Descend to next layer
4. Repeat until reaching bottom
5. Refine search locally

**Performance:**
- Search: O(log n) average case
- Insert: O(log n)
- Memory: O(n × M) where M is connections per node

**Configuration:**

```cpp
HnswConfig config;
config.M = 16;                // Connections per node (trade-off: speed vs accuracy)
config.ef_construction = 200; // Build quality (higher = better, slower build)
config.ef_search = 50;        // Search quality (higher = better, slower search)
```

### Metadata

**What is Metadata?**

Metadata is structured information about your vectors:

```json
{
    "id": 12345,
    "content": "Original text...",
    "type": "Journal",
    "date": "2024-01-15",
    "source_file": "data.csv",
    "custom_field": "value"
}
```

**Filtering:**

```cpp
// Search with metadata filter
SearchFilter filter;
filter.date_range = {"2024-01-01", "2024-12-31"};
filter.document_type = DocumentType::Journal;

auto results = db.search(query_vector, 10, filter);
```

---

## 5. Data Ingestion

### Overview

The data ingestion system automatically detects, parses, and normalizes data from multiple sources.

### Universal Data Adapter

**Architecture:**

```
Input Files/APIs
      ↓
DataAdapterManager (auto-detection)
      ↓
Specific Adapter (CSV, JSON, PDF, etc.)
      ↓
NormalizedData (chunks + metadata)
      ↓
Embedding Generation
      ↓
Vector Database
```

**Basic Usage:**

```cpp
#include <vdb/adapters/data_adapter.hpp>

using namespace vdb::adapters;

DataAdapterManager manager;

// Auto-detect and parse
auto result = manager.auto_parse("data.csv");

if (!result) {
    std::cerr << "Error: " << result.error().message << std::endl;
    return;
}

// Process chunks
for (const auto& chunk : result->chunks) {
    std::cout << "Content: " << chunk.content << std::endl;
    std::cout << "Metadata: " << chunk.metadata.size() << " fields" << std::endl;
}
```

### CSV Adapter

**Features:**
- Configurable delimiters, quotes, headers
- Automatic type detection
- Smart chunking
- Numerical feature extraction

**Configuration:**

```cpp
CSVConfig config;
config.delimiter = ',';
config.quote = '"';
config.has_header = true;
config.skip_empty_rows = true;

CSVAdapter adapter(config);
auto result = adapter.parse("data.csv");
```

**Example CSV:**

```csv
date,price,volume,description
2024-01-15,2050.50,1000,"Gold spot price increased"
2024-01-16,2048.75,1200,"Market consolidation"
```

**Output:**

```
Chunk 1:
  content: "date: 2024-01-15, price: 2050.50, volume: 1000, description: Gold spot price increased"
  metadata: {row_start: "1", row_end: "1"}
  numerical_features: [2050.50, 1000]

Chunk 2:
  content: "date: 2024-01-16, price: 2048.75, volume: 1200, description: Market consolidation"
  metadata: {row_start: "2", row_end: "2"}
  numerical_features: [2048.75, 1200]
```

### JSON Adapter

**Features:**
- Objects, arrays, JSONL support
- Nested structure flattening
- Automatic metadata extraction
- Type-aware parsing

**Example:**

```cpp
JSONAdapter adapter;
auto result = adapter.parse("data.json");
```

**Input (JSON):**

```json
{
  "id": 1,
  "title": "Market Analysis",
  "date": "2024-01-15",
  "content": "Gold prices surged today...",
  "metrics": {
    "price": 2050.50,
    "volume": 1000
  }
}
```

**Output:**

```
content: |
  id: 1
  title: Market Analysis
  date: 2024-01-15
  content: Gold prices surged today...
  metrics.price: 2050.50
  metrics.volume: 1000

metadata: {
  "id": "1",
  "title": "Market Analysis",
  "date": "2024-01-15"
}
```

### Chunking Strategies

**Available Strategies:**

| Strategy | Description | Best For |
|----------|-------------|----------|
| **Document** | No chunking, entire document | Small documents |
| **Fixed** | Fixed character count | Uniform processing |
| **Paragraph** | Split on double newlines | Natural text boundaries |
| **Sentence** | Split on sentence boundaries | Precise semantic units |
| **Sliding** | Overlapping windows | Context preservation |
| **Semantic** | Similarity-based splitting | Maximum coherence |

**Configuration:**

```cpp
ChunkConfig config;
config.strategy = ChunkStrategy::Paragraph;
config.max_chunk_size = 1000;  // characters
config.overlap = 100;           // for sliding window
config.preserve_structure = true;

auto result = manager.auto_parse("document.txt", config);
```

### Batch Processing

**Process multiple files:**

```cpp
std::vector<fs::path> files = {
    "data1.csv",
    "data2.json",
    "report.pdf"
};

ChunkConfig config;
config.strategy = ChunkStrategy::Paragraph;

// Process in parallel
auto results = manager.parse_batch(files, config, /*max_parallel=*/4);

for (const auto& result : *results) {
    std::cout << "Source: " << result.source_path << std::endl;
    std::cout << "Chunks: " << result.chunks.size() << std::endl;
}
```

---

## 6. Vector Operations

### Adding Vectors

**Single Vector:**

```cpp
// Generate embedding
auto embedding = text_encoder->encode("Hello, world!");

// Create metadata
Metadata meta;
meta.content = "Hello, world!";
meta.type = DocumentType::Text;
meta.date = "2024-01-15";

// Add to database
auto result = db.add_vector(*embedding, meta);

if (result) {
    VectorId id = *result;
    std::cout << "Added vector with ID: " << id << std::endl;
}
```

**Batch Insert:**

```cpp
std::vector<Vector> vectors;
std::vector<Metadata> metadata_list;

for (const auto& text : texts) {
    auto emb = text_encoder->encode(text);
    vectors.push_back(std::move(*emb));
    
    Metadata meta;
    meta.content = text;
    metadata_list.push_back(meta);
}

auto result = db.add_batch(vectors, metadata_list);
if (result) {
    std::cout << "Added " << result->size() << " vectors" << std::endl;
}
```

### Searching

**Basic Search:**

```cpp
auto query = text_encoder->encode("search query");
auto results = db.search(*query, /*k=*/10);

for (const auto& result : results) {
    std::cout << "ID: " << result.id << std::endl;
    std::cout << "Score: " << result.score << std::endl;
    std::cout << "Content: " << result.metadata.content << std::endl;
}
```

**Filtered Search:**

```cpp
SearchFilter filter;
filter.date_range = {"2024-01-01", "2024-12-31"};
filter.document_type = DocumentType::Journal;
filter.min_score = 0.7;

auto results = db.search(*query, 10, filter);
```

**Multi-Vector Search:**

```cpp
// Find documents similar to multiple queries
std::vector<Vector> queries = {
    *text_encoder->encode("gold price"),
    *text_encoder->encode("market analysis"),
    *text_encoder->encode("economic indicators")
};

auto results = db.multi_search(queries, 5);  // Top 5 per query
```

### Updating

**Update Metadata:**

```cpp
VectorId id = 12345;

Metadata new_meta;
new_meta.content = "Updated content";
new_meta.tags = {"important", "reviewed"};

auto result = db.update_metadata(id, new_meta);
```

**Update Vector:**

```cpp
auto new_embedding = text_encoder->encode("updated text");
auto result = db.update_vector(id, *new_embedding);
```

### Deleting

**Single Vector:**

```cpp
VectorId id = 12345;
auto result = db.remove(id);

if (result) {
    std::cout << "Vector removed successfully" << std::endl;
}
```

**Batch Delete:**

```cpp
std::vector<VectorId> ids = {1, 2, 3, 4, 5};
auto result = db.remove_batch(ids);
```

**Filtered Delete:**

```cpp
// Remove all vectors older than a date
SearchFilter filter;
filter.date_range = {"2020-01-01", "2022-12-31"};

auto result = db.remove_filtered(filter);
```

### Persistence

**Save Database:**

```cpp
auto result = db.sync();
if (result) {
    std::cout << "Database synchronized to disk" << std::endl;
}
```

**Database Files:**

```
my_database/
├── config.json          # Database configuration
├── vectors.bin          # Memory-mapped vectors
├── index.hnsw          # HNSW graph structure
└── metadata.jsonl      # Metadata entries
```

---

## 7. Search & Retrieval

### Search Algorithms

**Exact Search:**

```cpp
// Brute-force, guaranteed exact results
db.set_search_mode(SearchMode::Exact);
auto results = db.search(*query, 10);
```

**Approximate Search (HNSW):**

```cpp
// Fast, 99%+ recall
db.set_search_mode(SearchMode::Approximate);
auto results = db.search(*query, 10);
```

### Search Parameters

**HNSW Parameters:**

```cpp
HnswConfig config;
config.ef_search = 100;  // Higher = better recall, slower search

db.set_hnsw_config(config);
```

**Effect of ef_search:**

| ef_search | Recall | Speed | Use Case |
|-----------|--------|-------|----------|
| 10 | ~85% | Fastest | Real-time, low latency |
| 50 | ~95% | Fast | Production default |
| 100 | ~98% | Medium | High accuracy |
| 200+ | ~99%+ | Slow | Offline, critical queries |

### Filtering

**Date Filtering:**

```cpp
SearchFilter filter;
filter.date_range = {"2024-01-01", "2024-12-31"};

auto results = db.search(*query, 10, filter);
```

**Type Filtering:**

```cpp
filter.document_type = DocumentType::Journal;
filter.asset = "GOLD";
```

**Custom Filtering:**

```cpp
filter.custom_fields = {
    {"category", "analysis"},
    {"priority", "high"}
};
```

**Score Filtering:**

```cpp
filter.min_score = 0.8;  // Only results with score >= 0.8
filter.max_score = 1.0;
```

### Ranking

**Score Interpretation:**

For Cosine distance:
- 0.0 = Identical vectors
- 0.5 = Moderately similar
- 1.0 = Orthogonal (unrelated)
- 2.0 = Opposite

**Custom Ranking:**

```cpp
// Re-rank results based on custom logic
auto results = db.search(*query, 100);

std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
    // Custom scoring combining vector similarity and recency
    float score_a = a.score + (is_recent(a.metadata.date) ? 0.1 : 0);
    float score_b = b.score + (is_recent(b.metadata.date) ? 0.1 : 0);
    return score_a < score_b;
});

// Take top 10
results.resize(10);
```

---

## 8. API Reference

### C++ API

#### VectorDatabase

```cpp
class VectorDatabase {
public:
    explicit VectorDatabase(const DatabaseConfig& config);
    
    // Initialization
    [[nodiscard]] Result<void> init();
    [[nodiscard]] bool is_ready() const;
    
    // Vector operations
    [[nodiscard]] Result<VectorId> add_vector(
        VectorView vector,
        const Metadata& metadata
    );
    
    [[nodiscard]] Result<std::vector<VectorId>> add_batch(
        const std::vector<Vector>& vectors,
        const std::vector<Metadata>& metadata
    );
    
    [[nodiscard]] std::vector<SearchResult> search(
        VectorView query,
        size_t k,
        const SearchFilter& filter = {}
    );
    
    [[nodiscard]] Result<void> remove(VectorId id);
    
    [[nodiscard]] Result<void> update_metadata(
        VectorId id,
        const Metadata& metadata
    );
    
    // Persistence
    [[nodiscard]] Result<void> sync();
    
    // Statistics
    [[nodiscard]] DatabaseStats stats() const;
};
```

#### DataAdapterManager

```cpp
class DataAdapterManager {
public:
    DataAdapterManager();
    
    // Register custom adapters
    void register_adapter(std::unique_ptr<IDataAdapter> adapter);
    
    // Parse data
    [[nodiscard]] Result<NormalizedData> auto_parse(
        const fs::path& path,
        const ChunkConfig& config = {}
    );
    
    [[nodiscard]] Result<NormalizedData> auto_parse_content(
        std::string_view content,
        const ChunkConfig& config = {},
        std::string_view hint = ""
    );
    
    // Batch processing
    [[nodiscard]] Result<std::vector<NormalizedData>> parse_batch(
        const std::vector<fs::path>& paths,
        const ChunkConfig& config = {},
        size_t max_parallel = 4
    );
    
    // Format detection
    [[nodiscard]] DataFormat detect_format(const fs::path& path) const;
};
```

#### TextEncoder

```cpp
class TextEncoder {
public:
    explicit TextEncoder(const TextEncoderConfig& config);
    
    [[nodiscard]] Result<Vector> encode(std::string_view text);
    
    [[nodiscard]] Result<std::vector<Vector>> encode_batch(
        const std::vector<std::string>& texts
    );
};
```

#### ImageEncoder

```cpp
class ImageEncoder {
public:
    explicit ImageEncoder(const ImageEncoderConfig& config);
    
    [[nodiscard]] Result<Vector> encode(const fs::path& image_path);
    
    [[nodiscard]] Result<Vector> encode(
        const uint8_t* image_data,
        size_t width,
        size_t height,
        size_t channels
    );
};
```

---

## 9. Python Bindings

### Installation

```bash
pip install pyvdb
```

### Basic Usage

```python
import pyvdb

# Create database
config = pyvdb.DatabaseConfig()
config.path = "my_database"
config.dimension = 512
config.metric = pyvdb.DistanceMetric.Cosine

db = pyvdb.create_gold_standard_db("my_database")

# Add vectors
vector = [0.1] * 512  # Example vector
metadata = {
    "content": "Hello, world!",
    "type": "text",
    "date": "2024-01-15"
}

vector_id = db.add_vector(vector, metadata)

# Search
query = [0.2] * 512
results = db.search(query, k=10)

for result in results:
    print(f"ID: {result.id}, Score: {result.score}")
    print(f"Content: {result.metadata['content']}")
```

### Data Ingestion

```python
from pyvdb.adapters import DataAdapterManager, ChunkConfig, ChunkStrategy

# Create manager
manager = DataAdapterManager()

# Parse CSV
result = manager.auto_parse("data.csv")

# Process chunks
for chunk in result.chunks:
    print(f"Content: {chunk.content}")
    print(f"Metadata: {chunk.metadata}")
    
    # Generate embedding (using your preferred method)
    embedding = generate_embedding(chunk.content)
    
    # Add to database
    db.add_vector(embedding, chunk.metadata)
```

### Text Encoding

```python
# Initialize encoder
config = pyvdb.TextEncoderConfig()
config.model_path = "models/minilm-l6.onnx"
config.device = pyvdb.Device.CPU

encoder = pyvdb.TextEncoder(config)

# Encode text
embedding = encoder.encode("Hello, world!")
print(f"Embedding shape: {len(embedding)}")

# Batch encoding
texts = ["text 1", "text 2", "text 3"]
embeddings = encoder.encode_batch(texts)
```

### Image Encoding

```python
# Initialize encoder
config = pyvdb.ImageEncoderConfig()
config.model_path = "models/clip-vit.onnx"

encoder = pyvdb.ImageEncoder(config)

# Encode image
embedding = encoder.encode("photo.jpg")

# Add to database
db.add_vector(embedding, {"type": "image", "path": "photo.jpg"})
```

### Filtering

```python
# Create filter
filter_obj = pyvdb.SearchFilter()
filter_obj.date_range = ("2024-01-01", "2024-12-31")
filter_obj.document_type = pyvdb.DocumentType.Journal
filter_obj.min_score = 0.7

# Search with filter
results = db.search(query, k=10, filter=filter_obj)
```

---

## 10. CLI Tool

### Basic Commands

**Initialize Database:**

```bash
vdb_cli init --path my_database --dimension 512 --metric cosine
```

**Ingest Data:**

```bash
vdb_cli ingest --db my_database --input data.csv
```

**Search:**

```bash
vdb_cli search --db my_database --query "gold price analysis" --k 10
```

**Statistics:**

```bash
vdb_cli stats --db my_database
```

### Advanced Usage

**Batch Ingestion:**

```bash
vdb_cli ingest --db my_database --input data/*.csv --parallel 4
```

**Filtered Search:**

```bash
vdb_cli search \
    --db my_database \
    --query "market trends" \
    --k 10 \
    --date-from 2024-01-01 \
    --date-to 2024-12-31 \
    --type Journal
```

**Export Results:**

```bash
vdb_cli search \
    --db my_database \
    --query "analysis" \
    --k 100 \
    --output results.json
```

**Database Maintenance:**

```bash
# Compact database
vdb_cli compact --db my_database

# Rebuild index
vdb_cli rebuild-index --db my_database

# Verify integrity
vdb_cli verify --db my_database
```

---

## 11. Performance Tuning

### Hardware Optimization

**CPU:**
- Enable AVX2 or AVX-512 for 4-8x faster distance calculations
- Use `-march=native` flag during compilation
- Allocate more cores for parallel operations

**Memory:**
- Memory-map large databases to reduce RAM usage
- Pre-allocate vector storage for batch inserts
- Use huge pages for better TLB performance

**Storage:**
- Use SSD for database files
- Place index and vectors on separate drives
- Enable filesystem caching

### Configuration Tuning

**HNSW Parameters:**

For **high recall** (99%+):
```cpp
HnswConfig config;
config.M = 32;
config.ef_construction = 400;
config.ef_search = 200;
```

For **balanced** (95% recall, good speed):
```cpp
config.M = 16;
config.ef_construction = 200;
config.ef_search = 50;
```

For **high speed** (85% recall, fastest):
```cpp
config.M = 8;
config.ef_construction = 100;
config.ef_search = 20;
```

**Memory vs Speed Trade-offs:**

| Setting | Memory | Build Time | Search Speed | Recall |
|---------|--------|------------|--------------|--------|
| M=8, ef_c=100 | Low | Fast | Fastest | ~85% |
| M=16, ef_c=200 | Medium | Medium | Fast | ~95% |
| M=32, ef_c=400 | High | Slow | Medium | ~99% |

### Batch Operations

**Optimize Inserts:**

```cpp
// BAD: Insert one at a time
for (const auto& vector : vectors) {
    db.add_vector(vector, metadata);
}

// GOOD: Batch insert
db.add_batch(vectors, metadata_list);
```

**Optimize Searches:**

```cpp
// BAD: Search one at a time
for (const auto& query : queries) {
    auto results = db.search(query, 10);
}

// GOOD: Multi-search
auto results = db.multi_search(queries, 10);
```

### Profiling

**Enable Profiling:**

```cpp
db.enable_profiling(true);

// Perform operations
db.search(*query, 10);

// Get statistics
auto stats = db.profiling_stats();
std::cout << "Average search time: " << stats.avg_search_ms << "ms" << std::endl;
```

**Identify Bottlenecks:**

```bash
# Linux: Use perf
perf record -g ./vdb_cli search --db my_database --query "test"
perf report

# Analyze hot spots
```

---

## 12. Troubleshooting

### Common Issues

**Build Errors:**

*Issue: "C++23 not supported"*
```bash
# Solution: Update compiler
sudo apt install gcc-13 g++-13
export CXX=g++-13
```

*Issue: "Cannot find ONNX Runtime"*
```bash
# Solution: Install or specify path
cmake .. -DONNX_RUNTIME_DIR=/path/to/onnxruntime
```

**Runtime Errors:**

*Issue: "Failed to load model"*
- Check model file exists
- Verify ONNX Runtime installation
- Ensure model is compatible with ONNX Runtime version

*Issue: "Out of memory during search"*
- Reduce `ef_search` parameter
- Enable memory mapping
- Upgrade RAM

*Issue: "Slow search performance"*
- Enable AVX2/AVX-512
- Increase `ef_construction` during build
- Use SSD storage

### Debug Mode

**Enable Debug Logging:**

```cpp
db.set_log_level(LogLevel::Debug);
```

**Verify Database Integrity:**

```cpp
auto result = db.verify();
if (!result) {
    std::cerr << "Database corruption detected" << std::endl;
}
```

### Performance Issues

**Diagnose Slow Searches:**

1. Check index quality:
```cpp
auto stats = db.stats();
std::cout << "Index recall: " << stats.estimated_recall << std::endl;
```

2. Profile search:
```cpp
auto start = std::chrono::high_resolution_clock::now();
auto results = db.search(*query, 10);
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "Search took: " << duration.count() << " μs" << std::endl;
```

3. Optimize parameters based on results

---

## 13. Advanced Topics

### Custom Adapters

**Create Custom Data Adapter:**

```cpp
class MyCustomAdapter : public IDataAdapter {
public:
    bool can_handle(const fs::path& path) const override {
        return path.extension() == ".custom";
    }
    
    bool can_handle(std::string_view content) const override {
        return content.starts_with("CUSTOM_FORMAT");
    }
    
    Result<NormalizedData> parse(
        const fs::path& path,
        const ChunkConfig& config
    ) override {
        // Your parsing logic
        NormalizedData data;
        // ... populate data
        return data;
    }
    
    Result<NormalizedData> parse_content(
        std::string_view content,
        const ChunkConfig& config,
        std::string_view source_hint
    ) override {
        // Your parsing logic
        return parse_content_impl(content, config);
    }
    
    Result<void> sanitize(NormalizedData& data) override {
        // Your sanitization logic
        return {};
    }
    
    std::string name() const override {
        return "MyCustomAdapter";
    }
    
    std::vector<DataFormat> supported_formats() const override {
        return {DataFormat::Unknown};
    }
};

// Register adapter
DataAdapterManager manager;
manager.register_adapter(std::make_unique<MyCustomAdapter>());
```

### Custom Distance Metrics

```cpp
class CustomDistance {
public:
    static float distance(VectorView a, VectorView b) {
        // Your custom distance calculation
        float dist = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            dist += custom_metric(a[i], b[i]);
        }
        return dist;
    }
};

// Use custom metric
db.set_custom_distance_function(&CustomDistance::distance);
```

### Distributed Deployment

**Multi-Node Setup (Planned):**

```cpp
// Node 1: Shard 0-999,999
DatabaseConfig config1;
config1.shard_id = 0;
config1.total_shards = 2;
VectorDatabase db1(config1);

// Node 2: Shard 1,000,000-1,999,999
DatabaseConfig config2;
config2.shard_id = 1;
config2.total_shards = 2;
VectorDatabase db2(config2);

// Coordinator aggregates results
auto results1 = db1.search(*query, 10);
auto results2 = db2.search(*query, 10);
auto merged = merge_and_rank(results1, results2, 10);
```

---

## 14. Examples & Tutorials

### Example 1: Document Search System

```cpp
#include <vdb/database.hpp>
#include <vdb/adapters/data_adapter.hpp>

int main() {
    // 1. Create database
    auto db_result = create_gold_standard_db("docs_db");
    if (!db_result) {
        std::cerr << "Failed to create database" << std::endl;
        return 1;
    }
    VectorDatabase db = std::move(*db_result);
    
    // 2. Initialize text encoder
    TextEncoderConfig enc_config;
    enc_config.model_path = "models/minilm-l6.onnx";
    TextEncoder encoder(enc_config);
    
    // 3. Ingest documents
    DataAdapterManager manager;
    auto docs = manager.auto_parse("documents/*.txt");
    
    for (const auto& chunk : docs->chunks) {
        auto embedding = encoder.encode(chunk.content);
        
        Metadata meta;
        meta.content = chunk.content;
        meta.source_file = docs->source_path;
        
        db.add_vector(*embedding, meta);
    }
    
    // 4. Search
    auto query = encoder.encode("machine learning algorithms");
    auto results = db.search(*query, 10);
    
    // 5. Display results
    for (const auto& result : results) {
        std::cout << "Score: " << result.score << std::endl;
        std::cout << "Content: " << result.metadata.content << std::endl;
        std::cout << "---" << std::endl;
    }
    
    return 0;
}
```

### Example 2: CSV Data Analysis

```python
import pyvdb
from pyvdb.adapters import DataAdapterManager

# Initialize database
db = pyvdb.create_gold_standard_db("financial_db")

# Initialize data adapter
manager = DataAdapterManager()

# Ingest CSV file with financial data
result = manager.auto_parse("financial_data.csv")

# Process and add to database
for chunk in result.chunks:
    # Each chunk is a row from the CSV
    # Generate embedding from content
    embedding = db.encode_text(chunk.content)
    
    # Add to database with metadata
    db.add_vector(embedding, chunk.metadata)

# Search for similar patterns
query = "Q4 revenue growth above 10%"
results = db.search_text(query, k=10)

print("Similar financial patterns:")
for result in results:
    print(f"{result.metadata.get('date', 'N/A')}: {result.content[:100]}...")
```

### Example 3: Image Search

```cpp
#include <vdb/database.hpp>
#include <vdb/embeddings/image.hpp>

int main() {
    // Create database for images
    auto db = create_gold_standard_db("image_db");
    
    // Initialize image encoder
    ImageEncoderConfig config;
    config.model_path = "models/clip-vit.onnx";
    ImageEncoder encoder(config);
    
    // Index images
    std::vector<fs::path> images = {
        "photos/cat1.jpg",
        "photos/cat2.jpg",
        "photos/dog1.jpg",
        // ...
    };
    
    for (const auto& img_path : images) {
        auto embedding = encoder.encode(img_path);
        
        Metadata meta;
        meta.source_file = img_path.string();
        meta.type = DocumentType::Image;
        
        db->add_vector(*embedding, meta);
    }
    
    // Search by image
    auto query_img = encoder.encode("query.jpg");
    auto results = db->search(*query_img, 10);
    
    // Display similar images
    for (const auto& result : results) {
        std::cout << "Similar image: " << result.metadata.source_file << std::endl;
        std::cout << "Similarity: " << (1.0 - result.score) << std::endl;
    }
    
    return 0;
}
```

---

## Appendix

### A. Configuration Reference

**DatabaseConfig:**
```cpp
struct DatabaseConfig {
    fs::path path;                  // Database directory
    size_t dimension = 512;         // Vector dimensionality
    DistanceMetric metric = Cosine; // Distance metric
    size_t hnsw_m = 16;            // HNSW connections per node
    size_t ef_construction = 200;  // HNSW build quality
    size_t max_elements = 1000000; // Initial capacity
};
```

**TextEncoderConfig:**
```cpp
struct TextEncoderConfig {
    fs::path model_path;           // Path to ONNX model
    Device device = Device::CPU;   // Computation device
    size_t max_length = 512;      // Max token length
    bool normalize = true;         // L2 normalize output
};
```

**ChunkConfig:**
```cpp
struct ChunkConfig {
    ChunkStrategy strategy = Paragraph;
    size_t max_chunk_size = 1000;
    size_t overlap = 100;
    bool preserve_structure = true;
};
```

### B. Error Codes

| Code | Description | Resolution |
|------|-------------|------------|
| `IoError` | File I/O failure | Check file permissions, disk space |
| `InvalidData` | Malformed input | Verify data format, encoding |
| `NotFound` | Resource not found | Check path, ID exists |
| `AlreadyExists` | Duplicate resource | Use different ID or path |
| `OutOfMemory` | Memory allocation failed | Reduce batch size, upgrade RAM |
| `NotImplemented` | Feature not available | Check version, enable feature |

### C. Glossary

- **Vector**: Numerical representation of data in high-dimensional space
- **Embedding**: Process of converting data to vectors
- **HNSW**: Hierarchical Navigable Small World graph algorithm
- **SIMD**: Single Instruction Multiple Data (parallel processing)
- **Cosine Similarity**: Measure of angle between two vectors
- **Recall**: Percentage of relevant results found
- **Chunking**: Splitting documents into smaller pieces

### D. Resources

**Official:**
- GitHub: https://github.com/amuzetnoM/vector_studio
- Documentation: https://vector-studio.readthedocs.io
- Issues: https://github.com/amuzetnoM/vector_studio/issues

**Community:**
- Discord: https://discord.gg/vector-studio
- Forum: https://forum.vector-studio.io

**Related Projects:**
- ONNX Runtime: https://onnxruntime.ai
- HNSW Library: https://github.com/nmslib/hnswlib
- pybind11: https://pybind11.readthedocs.io

---

**Version:** 2.0.0  
**Last Updated:** 2026-01-04  
**License:** MIT

© 2024 Vector Studio Contributors
