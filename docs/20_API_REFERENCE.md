---
title: "API Reference"
description: "Complete C++ API documentation"
version: "4.0.0"
last_updated: "2026-01-22"
sidebar_position: 20
category: "Reference"
---

![Version](https://img.shields.io/badge/version-4.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--22-green?style=flat-square)
![Language](https://img.shields.io/badge/language-C%2B%2B23-00599C?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)

## Table of Contents

1. [Core Database API](#core-database-api)
2. [Data Adapters API](#data-adapters-api)
3. [Embedding Models API](#embedding-models-api)
4. [Search & Indexing API](#search--indexing-api)
5. [Hybrid Search API](#hybrid-search-api)
6. [Python Bindings API](#python-bindings-api)
7. [Type Definitions](#type-definitions)
8. [Error Handling](#error-handling)

---

## Core Database API

### VectorDatabase

Main database class for vector storage and retrieval.

#### Constructor

```cpp
explicit VectorDatabase(const DatabaseConfig& config);
```

**Parameters:**
- `config`: Database configuration

**Example:**
```cpp
DatabaseConfig config;
config.path = "my_db";
config.dimension = 512;
config.metric = DistanceMetric::Cosine;

VectorDatabase db(config);
```

#### init()

```cpp
[[nodiscard]] Result<void> init();
```

Initialize the database (create directories, load index).

**Returns:** `Result<void>` - Success or error

**Example:**
```cpp
auto result = db.init();
if (!result) {
    std::cerr << "Init failed: " << result.error().message << std::endl;
}
```

#### is_ready()

```cpp
[[nodiscard]] bool is_ready() const noexcept;
```

Check if database is ready for operations.

**Returns:** `bool` - True if ready

#### add_vector()

```cpp
[[nodiscard]] Result<VectorId> add_vector(
    VectorView vector,
    const Metadata& metadata
);
```

Add a single vector to the database.

**Parameters:**
- `vector`: Vector data (VectorView)
- `metadata`: Associated metadata

**Returns:** `Result<VectorId>` - Assigned vector ID or error

**Example:**
```cpp
std::vector<float> vec = {0.1f, 0.2f, /* ... */};
Metadata meta;
meta.content = "Example";

auto result = db.add_vector(VectorView(vec.data(), vec.size()), meta);
if (result) {
    VectorId id = *result;
    std::cout << "Added vector " << id << std::endl;
}
```

#### add_batch()

```cpp
[[nodiscard]] Result<std::vector<VectorId>> add_batch(
    const std::vector<Vector>& vectors,
    const std::vector<Metadata>& metadata
);
```

Add multiple vectors in a single operation.

**Parameters:**
- `vectors`: Vector of vectors
- `metadata`: Vector of metadata (same size as vectors)

**Returns:** `Result<std::vector<VectorId>>` - Vector IDs or error

**Example:**
```cpp
std::vector<Vector> vectors;
std::vector<Metadata> metadata_list;

// ... populate vectors and metadata ...

auto result = db.add_batch(vectors, metadata_list);
if (result) {
    for (VectorId id : *result) {
        std::cout << "Added " << id << std::endl;
    }
}
```

#### search()

```cpp
[[nodiscard]] std::vector<SearchResult> search(
    VectorView query,
    size_t k,
    const SearchFilter& filter = {}
) const;
```

Search for k nearest neighbors.

**Parameters:**
- `query`: Query vector
- `k`: Number of results to return
- `filter`: Optional search filter

**Returns:** `std::vector<SearchResult>` - Sorted by similarity

**Example:**
```cpp
std::vector<float> query = {/* ... */};
auto results = db.search(VectorView(query.data(), query.size()), 10);

for (const auto& result : results) {
    std::cout << "ID: " << result.id << ", Score: " << result.score << std::endl;
}
```

#### remove()

```cpp
[[nodiscard]] Result<void> remove(VectorId id);
```

Remove a vector from the database.

**Parameters:**
- `id`: Vector ID to remove

**Returns:** `Result<void>` - Success or error

**Example:**
```cpp
auto result = db.remove(12345);
if (!result) {
    std::cerr << "Remove failed" << std::endl;
}
```

#### update_metadata()

```cpp
[[nodiscard]] Result<void> update_metadata(
    VectorId id,
    const Metadata& metadata
);
```

Update metadata for a vector.

**Parameters:**
- `id`: Vector ID
- `metadata`: New metadata

**Returns:** `Result<void>` - Success or error

#### sync()

```cpp
[[nodiscard]] Result<void> sync();
```

Synchronize database to disk.

**Returns:** `Result<void>` - Success or error

**Example:**
```cpp
auto result = db.sync();
if (result) {
    std::cout << "Database synchronized" << std::endl;
}
```

#### stats()

```cpp
[[nodiscard]] DatabaseStats stats() const;
```

Get database statistics.

**Returns:** `DatabaseStats` - Database metrics

**Example:**
```cpp
auto stats = db.stats();
std::cout << "Vectors: " << stats.num_vectors << std::endl;
std::cout << "Size: " << stats.total_size_bytes << " bytes" << std::endl;
```

---

## Data Adapters API

### DataAdapterManager

Central manager for data ingestion.

#### Constructor

```cpp
DataAdapterManager();
```

**Example:**
```cpp
DataAdapterManager manager;
```

#### register_adapter()

```cpp
void register_adapter(std::unique_ptr<IDataAdapter> adapter);
```

Register a custom data adapter.

**Parameters:**
- `adapter`: Unique pointer to adapter

**Example:**
```cpp
manager.register_adapter(std::make_unique<MyCustomAdapter>());
```

#### auto_parse()

```cpp
[[nodiscard]] Result<NormalizedData> auto_parse(
    const fs::path& path,
    const ChunkConfig& config = {}
);
```

Auto-detect format and parse file.

**Parameters:**
- `path`: File path
- `config`: Optional chunking configuration

**Returns:** `Result<NormalizedData>` - Parsed data or error

**Example:**
```cpp
auto result = manager.auto_parse("data.csv");
if (result) {
    std::cout << "Parsed " << result->chunks.size() << " chunks" << std::endl;
}
```

#### parse_batch()

```cpp
[[nodiscard]] Result<std::vector<NormalizedData>> parse_batch(
    const std::vector<fs::path>& paths,
    const ChunkConfig& config = {},
    size_t max_parallel = 4
);
```

Parse multiple files in parallel.

**Parameters:**
- `paths`: Vector of file paths
- `config`: Optional chunking configuration
- `max_parallel`: Maximum parallel operations

**Returns:** `Result<std::vector<NormalizedData>>` - Parsed data or error

### CSVAdapter

CSV file adapter.

#### Constructor

```cpp
explicit CSVAdapter(const CSVConfig& config = {});
```

**Example:**
```cpp
CSVConfig config;
config.delimiter = ',';
config.has_header = true;

CSVAdapter adapter(config);
```

#### parse()

```cpp
[[nodiscard]] Result<NormalizedData> parse(
    const fs::path& path,
    const ChunkConfig& config = {}
) override;
```

Parse CSV file.

### JSONAdapter

JSON/JSONL file adapter.

#### Constructor

```cpp
JSONAdapter();
```

#### parse()

```cpp
[[nodiscard]] Result<NormalizedData> parse(
    const fs::path& path,
    const ChunkConfig& config = {}
) override;
```

Parse JSON file (objects, arrays, JSONL).

---

## Embedding Models API

### TextEncoder

Text embedding encoder using ONNX models.

#### Constructor

```cpp
explicit TextEncoder(const TextEncoderConfig& config);
```

**Example:**
```cpp
TextEncoderConfig config;
config.model_path = "models/minilm-l6.onnx";
config.device = Device::CPU;

TextEncoder encoder(config);
```

#### encode()

```cpp
[[nodiscard]] Result<Vector> encode(std::string_view text);
```

Encode text to vector.

**Parameters:**
- `text`: Input text

**Returns:** `Result<Vector>` - Embedding vector or error

**Example:**
```cpp
auto embedding = encoder.encode("Hello, world!");
if (embedding) {
    std::cout << "Embedding size: " << embedding->size() << std::endl;
}
```

#### encode_batch()

```cpp
[[nodiscard]] Result<std::vector<Vector>> encode_batch(
    const std::vector<std::string>& texts
);
```

Encode multiple texts in batch.

**Parameters:**
- `texts`: Vector of input texts

**Returns:** `Result<std::vector<Vector>>` - Embedding vectors or error

### ImageEncoder

Image embedding encoder using ONNX models.

#### Constructor

```cpp
explicit ImageEncoder(const ImageEncoderConfig& config);
```

**Example:**
```cpp
ImageEncoderConfig config;
config.model_path = "models/clip-vit.onnx";

ImageEncoder encoder(config);
```

#### encode() - From File

```cpp
[[nodiscard]] Result<Vector> encode(const fs::path& image_path);
```

Encode image from file.

**Parameters:**
- `image_path`: Path to image file

**Returns:** `Result<Vector>` - Embedding vector or error

**Example:**
```cpp
auto embedding = encoder.encode("photo.jpg");
```

#### encode() - From Memory

```cpp
[[nodiscard]] Result<Vector> encode(
    const uint8_t* image_data,
    size_t width,
    size_t height,
    size_t channels
);
```

Encode image from memory buffer.

**Parameters:**
- `image_data`: Raw image data
- `width`: Image width
- `height`: Image height
- `channels`: Number of channels (3 for RGB)

**Returns:** `Result<Vector>` - Embedding vector or error

---

## Search & Indexing API

### HnswIndex

HNSW (Hierarchical Navigable Small World) index.

#### Constructor

```cpp
explicit HnswIndex(const HnswConfig& config);
```

**Example:**
```cpp
HnswConfig config;
config.M = 16;
config.ef_construction = 200;
config.dimension = 512;
config.metric = DistanceMetric::Cosine;

HnswIndex index(config);
```

#### add()

```cpp
[[nodiscard]] Result<void> add(VectorId id, VectorView vector);
```

Add vector to index.

**Parameters:**
- `id`: Vector ID
- `vector`: Vector data

**Returns:** `Result<void>` - Success or error

#### search()

```cpp
[[nodiscard]] std::vector<SearchResult> search(
    VectorView query,
    size_t k
) const;
```

Search for k nearest neighbors.

**Parameters:**
- `query`: Query vector
- `k`: Number of results

**Returns:** `std::vector<SearchResult>` - Search results

#### remove()

```cpp
[[nodiscard]] Result<void> remove(VectorId id);
```

Remove vector from index.

**Parameters:**
- `id`: Vector ID to remove

**Returns:** `Result<void>` - Success or error

#### size()

```cpp
[[nodiscard]] size_t size() const noexcept;
```

Get number of vectors in index.

**Returns:** `size_t` - Vector count

#### contains()

```cpp
[[nodiscard]] bool contains(VectorId id) const noexcept;
```

Check if vector exists in index.

**Parameters:**
- `id`: Vector ID

**Returns:** `bool` - True if exists

---

## Hybrid Search API

### BM25Engine

BM25 full-text search engine for lexical search.

#### Constructor

```cpp
explicit BM25Engine(const BM25Config& config = {});
```

**Parameters:**
- `config`: BM25 configuration (optional)

**Example:**
```cpp
BM25Config config;
config.k1 = 1.2f;
config.b = 0.75f;
config.use_stemming = true;

BM25Engine bm25(config);
```

#### add_document()

```cpp
[[nodiscard]] Result<void> add_document(VectorId id, const std::string& content);
```

Add document to BM25 index.

**Parameters:**
- `id`: Document ID
- `content`: Text content

**Returns:** `Result<void>` - Success or error

**Example:**
```cpp
auto result = bm25.add_document(1, "Machine learning is a subset of AI");
if (!result) {
    std::cerr << "Failed to add document" << std::endl;
}
```

#### remove_document()

```cpp
[[nodiscard]] Result<void> remove_document(VectorId id);
```

Remove document from index.

**Parameters:**
- `id`: Document ID

**Returns:** `Result<void>` - Success or error

#### update_document()

```cpp
[[nodiscard]] Result<void> update_document(VectorId id, const std::string& content);
```

Update document content.

**Parameters:**
- `id`: Document ID
- `content`: New text content

**Returns:** `Result<void>` - Success or error

#### search()

```cpp
[[nodiscard]] Result<std::vector<BM25Result>> search(
    const std::string& query,
    size_t k = 10,
    float min_score = 0.0f
) const;
```

Search documents with BM25 algorithm.

**Parameters:**
- `query`: Search query
- `k`: Number of results (default: 10)
- `min_score`: Minimum score threshold (default: 0.0)

**Returns:** `Result<std::vector<BM25Result>>` - Search results or error

**Example:**
```cpp
auto results = bm25.search("machine learning", 20, 0.5f);
if (results) {
    for (const auto& result : *results) {
        std::cout << "ID: " << result.id << ", Score: " << result.score << std::endl;
    }
}
```

#### document_count()

```cpp
[[nodiscard]] size_t document_count() const;
```

Get number of indexed documents.

**Returns:** `size_t` - Document count

#### term_count()

```cpp
[[nodiscard]] size_t term_count() const;
```

Get number of unique terms.

**Returns:** `size_t` - Term count

#### average_document_length()

```cpp
[[nodiscard]] float average_document_length() const;
```

Get average document length.

**Returns:** `float` - Average length

#### save()

```cpp
[[nodiscard]] Result<void> save(const std::string& path) const;
```

Save index to file.

**Parameters:**
- `path`: File path

**Returns:** `Result<void>` - Success or error

#### load() (static)

```cpp
static Result<BM25Engine> load(const std::string& path);
```

Load index from file.

**Parameters:**
- `path`: File path

**Returns:** `Result<BM25Engine>` - Loaded engine or error

**Example:**
```cpp
auto engine = BM25Engine::load("bm25_index.dat");
if (engine) {
    std::cout << "Loaded " << engine->document_count() << " documents" << std::endl;
}
```

### HybridSearchEngine

Combines vector and lexical search results using fusion methods.

#### Constructor

```cpp
explicit HybridSearchEngine(const HybridSearchConfig& config = {});
```

**Parameters:**
- `config`: Hybrid search configuration (optional)

**Example:**
```cpp
HybridSearchConfig config;
config.vector_weight = 0.7f;
config.lexical_weight = 0.3f;
config.fusion = FusionMethod::RRF;

HybridSearchEngine hybrid(config);
```

#### combine()

```cpp
[[nodiscard]] Result<std::vector<HybridResult>> combine(
    const std::vector<QueryResult>& vector_results,
    const std::vector<BM25Result>& lexical_results,
    size_t k = 10
) const;
```

Combine vector and lexical search results.

**Parameters:**
- `vector_results`: Results from vector search
- `lexical_results`: Results from BM25 search
- `k`: Number of final results (default: 10)

**Returns:** `Result<std::vector<HybridResult>>` - Combined results or error

**Example:**
```cpp
// Get results from both systems
auto vector_results = db.search(query_vec, 50);
auto lexical_results = bm25.search("query text", 50);

// Combine
HybridSearchEngine hybrid;
auto combined = hybrid.combine(*vector_results, *lexical_results, 20);

if (combined) {
    for (const auto& result : *combined) {
        std::cout << "ID: " << result.id 
                  << ", Combined: " << result.combined_score
                  << ", Vector: " << result.vector_score
                  << ", Lexical: " << result.lexical_score << std::endl;
    }
}
```

#### Static Fusion Methods

```cpp
static float weighted_sum(float vec_score, float lex_score, float vec_weight);
static float reciprocal_rank_fusion(size_t vec_rank, size_t lex_rank, float k);
static float comb_sum(float vec_score, float lex_score);
static float comb_mnz(float vec_score, float lex_score, size_t num_systems);
```

Utility methods for computing fusion scores.

**Example:**
```cpp
float score = HybridSearchEngine::weighted_sum(0.8f, 0.6f, 0.7f);
float rrf = HybridSearchEngine::reciprocal_rank_fusion(1, 3, 60.0f);
```

---

## Python Bindings API

### pyvdb.VectorDatabase

Python wrapper for VectorDatabase.

#### Constructor

```python
db = pyvdb.VectorDatabase(config: DatabaseConfig)
```

**Example:**
```python
config = pyvdb.DatabaseConfig()
config.path = "my_db"
config.dimension = 512

db = pyvdb.VectorDatabase(config)
db.init()
```

#### add_vector()

```python
vector_id = db.add_vector(
    vector: List[float],
    metadata: Dict[str, Any]
) -> int
```

**Example:**
```python
vector = [0.1] * 512
metadata = {"content": "text", "date": "2024-01-15"}

vector_id = db.add_vector(vector, metadata)
```

#### search()

```python
results = db.search(
    query: List[float],
    k: int,
    filter: Optional[SearchFilter] = None
) -> List[SearchResult]
```

**Example:**
```python
query = [0.2] * 512
results = db.search(query, k=10)

for result in results:
    print(f"ID: {result.id}, Score: {result.score}")
```

### pyvdb.adapters.DataAdapterManager

Python wrapper for DataAdapterManager.

```python
manager = pyvdb.adapters.DataAdapterManager()
result = manager.auto_parse("data.csv")

for chunk in result.chunks:
    print(chunk.content)
```

### pyvdb.TextEncoder

Python wrapper for TextEncoder.

```python
config = pyvdb.TextEncoderConfig()
config.model_path = "models/minilm-l6.onnx"

encoder = pyvdb.TextEncoder(config)
embedding = encoder.encode("Hello, world!")
```

---

## Type Definitions

### DatabaseConfig

```cpp
struct DatabaseConfig {
    fs::path path;                      // Database directory
    size_t dimension = 512;             // Vector dimensionality
    DistanceMetric metric = Cosine;     // Distance metric
    size_t hnsw_m = 16;                // HNSW M parameter
    size_t ef_construction = 200;      // HNSW ef_construction
    size_t max_elements = 1000000;     // Initial capacity
};
```

### Metadata

```cpp
struct Metadata {
    VectorId id = 0;
    std::string content;
    DocumentType type = DocumentType::Unknown;
    std::string date;
    std::string source_file;
    std::string asset;
    std::unordered_map<std::string, std::string> custom_fields;
};
```

### SearchResult

```cpp
struct SearchResult {
    VectorId id;
    float score;          // Distance score (lower = more similar for most metrics)
    Metadata metadata;
};
```

### SearchFilter

```cpp
struct SearchFilter {
    std::optional<std::pair<std::string, std::string>> date_range;
    std::optional<DocumentType> document_type;
    std::optional<std::string> asset;
    std::optional<float> min_score;
    std::optional<float> max_score;
    std::unordered_map<std::string, std::string> custom_fields;
};
```

### NormalizedData

```cpp
struct NormalizedData {
    std::vector<DataChunk> chunks;
    DataFormat format;
    std::string source_path;
    std::unordered_map<std::string, std::string> global_metadata;
    float confidence = 1.0f;
    bool sanitized = false;
    std::vector<std::string> warnings;
};
```

### DataChunk

```cpp
struct DataChunk {
    std::string content;
    std::unordered_map<std::string, std::string> metadata;
    std::optional<std::string> title;
    std::optional<std::string> date;
    std::optional<std::string> source;
    std::vector<float> numerical_features;
    size_t chunk_index = 0;
    size_t total_chunks = 1;
};
```

### ChunkConfig

```cpp
struct ChunkConfig {
    ChunkStrategy strategy = ChunkStrategy::Paragraph;
    size_t max_chunk_size = 1000;
    size_t overlap = 100;
    bool preserve_structure = true;
};
```

### HnswConfig

```cpp
struct HnswConfig {
    size_t M = 16;                  // Connections per node
    size_t ef_construction = 200;   // Build quality
    size_t ef_search = 50;         // Search quality
    size_t dimension = 512;
    DistanceMetric metric = DistanceMetric::Cosine;
};
```

### TextEncoderConfig

```cpp
struct TextEncoderConfig {
    fs::path model_path;
    Device device = Device::CPU;
    size_t max_length = 512;
    bool normalize = true;
};
```

### ImageEncoderConfig

```cpp
struct ImageEncoderConfig {
    fs::path model_path;
    Device device = Device::CPU;
    size_t target_size = 224;
    bool normalize = true;
};
```

---

## Enumerations

### DistanceMetric

```cpp
enum class DistanceMetric {
    Cosine,      // Cosine distance: 1 - (A·B)/(||A||||B||)
    Euclidean,   // L2 distance: ||A - B||
    DotProduct   // Negative dot product: -(A·B)
};
```

### DocumentType

```cpp
enum class DocumentType {
    Unknown,
    Journal,
    Chart,
    Catalyst,
    InstMatrix,
    Report3M,
    Report1Y,
    PreMarket,
    Text,
    Image
};
```

### DataFormat

```cpp
enum class DataFormat {
    // Text formats
    PlainText, Markdown, JSON, CSV, XML,
    
    // Document formats
    PDF, DOCX, TXT,
    
    // Structured data
    SQL, Excel, Parquet,
    
    // Web & APIs
    HTML, API_JSON, RSS,
    
    // External APIs (via HTTP adapter)
    Yahoo_Finance, Alpha_Vantage,
    
    // Images
    PNG, JPEG,
    
    Unknown
};
```

**Note:** The HTTP adapter is now fully functional and can be used to fetch data from any HTTP/HTTPS endpoint. It auto-detects the response format (JSON, XML, CSV) based on Content-Type headers. Use `HTTPAdapter` with URLs for web API data ingestion.

### ChunkStrategy

```cpp
enum class ChunkStrategy {
    Fixed,       // Fixed character count
    Sentence,    // Split by sentences
    Paragraph,   // Split by paragraphs
    Semantic,    // Semantic similarity-based
    Sliding,     // Sliding window with overlap
    Document     // One chunk per document
};
```

### Device

```cpp
enum class Device {
    CPU,
    CUDA,
    DirectML
};
```

### BM25Config

```cpp
struct BM25Config {
    float k1 = 1.2f;               // Term frequency saturation parameter
    float b = 0.75f;               // Length normalization parameter
    size_t min_term_length = 2;    // Minimum term length
    bool use_stemming = true;      // Enable Porter stemming
    bool case_sensitive = false;   // Case sensitive search
};
```

### BM25Result

```cpp
struct BM25Result {
    VectorId id;                       // Document ID
    float score;                       // BM25 score
    std::vector<std::string> matched_terms;  // Matched query terms
};
```

### HybridSearchConfig

```cpp
struct HybridSearchConfig {
    float vector_weight = 0.7f;    // Weight for vector search (0-1)
    float lexical_weight = 0.3f;   // Weight for lexical search (0-1)
    FusionMethod fusion = FusionMethod::RRF;  // Fusion method
    float rrf_k = 60.0f;           // RRF constant (30-100)
    bool rerank = true;            // Apply reranking
};
```

### HybridResult

```cpp
struct HybridResult {
    VectorId id;                   // Document ID
    float combined_score;          // Combined fusion score
    float vector_score;            // Vector similarity score
    float lexical_score;           // Lexical (BM25) score
    std::vector<std::string> matched_keywords;  // Matched keywords
};
```

### FusionMethod

```cpp
enum class FusionMethod {
    WeightedSum,   // Weighted combination of scores
    RRF,           // Reciprocal Rank Fusion
    CombSUM,       // Sum of normalized scores
    CombMNZ,       // CombSUM * number of matching systems
    Borda          // Borda count voting
};
```

---

## Error Handling

### Result<T>

All fallible operations return `Result<T>`:

```cpp
template<typename T>
using Result = std::expected<T, Error>;
```

### Error

```cpp
struct Error {
    ErrorCode code;
    std::string message;
    std::optional<std::string> context;
};
```

### ErrorCode

```cpp
enum class ErrorCode {
    Success,
    IoError,
    InvalidData,
    NotFound,
    AlreadyExists,
    OutOfMemory,
    NotImplemented,
    InternalError
};
```

### Error Handling Patterns

**Pattern 1: Check and use**
```cpp
auto result = db.add_vector(vec, meta);
if (result) {
    VectorId id = *result;
    // Use id
} else {
    std::cerr << "Error: " << result.error().message << std::endl;
}
```

**Pattern 2: Early return**
```cpp
auto result = db.init();
if (!result) {
    return result.error();
}
// Continue...
```

**Pattern 3: Transform**
```cpp
auto result = db.search(query, 10);
return result.transform([](const auto& results) {
    return results.size();
});
```

**Pattern 4: Chain operations**
```cpp
return db.add_vector(vec, meta)
    .and_then([&](VectorId id) {
        return db.update_metadata(id, new_meta);
    })
    .and_then([&]() {
        return db.sync();
    });
```

---

## Factory Functions

### create_gold_standard_db()

```cpp
[[nodiscard]] Result<VectorDatabase> create_gold_standard_db(
    const fs::path& path
);
```

Create a new database with default Gold Standard configuration.

**Parameters:**
- `path`: Database directory path

**Returns:** `Result<VectorDatabase>` - Database instance or error

**Example:**
```cpp
auto db_result = create_gold_standard_db("my_database");
if (!db_result) {
    std::cerr << "Failed to create database" << std::endl;
    return 1;
}

VectorDatabase db = std::move(*db_result);
```

### open_database()

```cpp
[[nodiscard]] Result<VectorDatabase> open_database(
    const fs::path& path
);
```

Open an existing database.

**Parameters:**
- `path`: Database directory path

**Returns:** `Result<VectorDatabase>` - Database instance or error

**Example:**
```cpp
auto db_result = open_database("existing_database");
if (!db_result) {
    std::cerr << "Failed to open database" << std::endl;
    return 1;
}

VectorDatabase db = std::move(*db_result);
```

---

## Helper Functions

### format_to_string()

```cpp
[[nodiscard]] std::string format_to_string(DataFormat format);
```

Convert DataFormat enum to string.

### detect_from_extension()

```cpp
[[nodiscard]] DataFormat detect_from_extension(const fs::path& path);
```

Detect format from file extension.

### sanitize_text()

```cpp
[[nodiscard]] std::string sanitize_text(std::string_view text);
```

Remove control characters and normalize whitespace.

### extract_numerical_features()

```cpp
[[nodiscard]] std::vector<float> extract_numerical_features(
    std::string_view text
);
```

Extract all numbers from text.

### chunk_text()

```cpp
[[nodiscard]] std::vector<DataChunk> chunk_text(
    std::string_view text,
    const ChunkConfig& config,
    const std::unordered_map<std::string, std::string>& metadata = {}
);
```

Split text into chunks based on strategy.

---

## Constants

### Default Values

```cpp
// HNSW defaults
constexpr size_t HNSW_M = 16;
constexpr size_t HNSW_EF_CONSTRUCTION = 200;
constexpr size_t HNSW_EF_SEARCH = 50;

// Dimension defaults
constexpr size_t UNIFIED_DIM = 512;

// Chunking defaults
constexpr size_t DEFAULT_CHUNK_SIZE = 1000;
constexpr size_t DEFAULT_OVERLAP = 100;
```

---

## Thread Safety

### Thread-Safe Operations

- `search()` - Multiple concurrent reads
- `stats()` - Read-only operations
- `contains()` - Check operations

### Mutex-Protected Operations

- `add_vector()` - Exclusive write
- `add_batch()` - Exclusive write
- `remove()` - Exclusive write
- `update_metadata()` - Exclusive write
- `sync()` - Exclusive write

### Concurrency Pattern

```cpp
// Multiple threads can search concurrently
std::thread t1([&]() {
    auto results = db.search(query1, 10);
});

std::thread t2([&]() {
    auto results = db.search(query2, 10);
});

t1.join();
t2.join();

// But only one thread can write at a time
db.add_vector(vec, meta);  // Blocks until complete
```

---

## Versioning

**API Version:** 3.0.0

**Compatibility:**
- Binary compatible within major version
- Source compatible within minor version
- Breaking changes only in major version
- **v2.0 Breaking Changes**: Added write() method to IDataAdapter interface

**Version Checking:**

```cpp
#include <vdb/version.hpp>

std::cout << "VDB Version: " << VDB_VERSION << std::endl;
std::cout << "API Level: " << VDB_API_LEVEL << std::endl;
```

---

**Last Updated:** 2026-01-04  
**Contributors:** Vector Studio Team  
**License:** MIT

© 2024 Vector Studio Contributors
