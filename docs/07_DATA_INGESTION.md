---
title: "Data Ingestion"
description: "Data adapters, batch processing, and ingestion patterns"
version: "4.0.0"
last_updated: "2026-01-22"
sidebar_position: 7
category: "Guide"
---

![Version](https://img.shields.io/badge/version-4.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--22-green?style=flat-square)
![Adapters](https://img.shields.io/badge/adapters-10_types-success?style=flat-square)

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Implemented Adapters](#implemented-adapters)
4. [Chunking Strategies](#chunking-strategies)
5. [Usage Examples](#usage-examples)
6. [Integration with VectorDatabase](#integration-with-vectordatabase)
7. [Batch Processing](#batch-processing)
8. [Custom Adapter Development](#custom-adapter-development)
9. [Error Handling](#error-handling)
10. [Performance Considerations](#performance-considerations)
11. [Testing](#testing)
12. [API Reference](#api-reference)

---

## Overview

The Data Ingestion Module provides a unified interface for importing data from multiple formats into Vector Studio. It automatically detects file types, normalizes content, and prepares data for vector embedding and semantic search.

### Key Features

- ✅ **Automatic Format Detection**: Identifies file types from extensions and content
- ✅ **Universal Normalization**: Converts all formats to a common structure
- ✅ **Intelligent Chunking**: Multiple strategies for splitting large documents
- ✅ **Metadata Extraction**: Automatic extraction of structured metadata
- ✅ **Batch Processing**: Efficient handling of multiple files simultaneously
- ✅ **Extensible Architecture**: Easy addition of custom adapters
- ✅ **Type Safety**: Strong validation and sanitization
- ✅ **Bidirectional Support**: Read and write capabilities for all adapters
- ✅ **Comprehensive Logging**: Thread-safe runtime monitoring with anomaly detection
- ✅ **Security Hardened**: SQL injection protection, input sanitization, and validation

### Design Principles

1. **Zero Configuration**: Works out-of-the-box with sensible defaults
2. **Format Agnostic**: Treats all data sources uniformly after normalization
3. **Composable**: Adapters can be mixed and registered dynamically
4. **Error Tolerant**: Graceful handling of malformed data

---

## Architecture

### Core Components

```
┌─────────────────────────────────────────────────────────────┐
│                    DataAdapterManager                        │
│                  (Central Orchestrator)                      │
└────────┬───────────────────────────────────────┬────────────┘
         │                                       │
         │ 1. Detect Format                     │ 4. Return
         │ 2. Select Adapter                    │    NormalizedData
         │ 3. Parse & Normalize                 │
         │                                       │
         ▼                                       │
┌────────────────────────────────────────────────┼────────────┐
│              IDataAdapter Interface            │            │
│         (Base for all adapters)                │            │
├────────────────────────────────────────────────┤            │
│                                                │            │
│  ┌────────────┐  ┌────────────┐  ┌──────────┐│            │
│  │    CSV     │  │    JSON    │  │   Text   ││            │
│  │  Adapter   │  │  Adapter   │  │ Adapter  ││            │
│  └────────────┘  └────────────┘  └──────────┘│            │
│                                                │            │
│  ┌────────────┐  ┌────────────┐               │            │
│  │    PDF     │  │   Excel    │               │            │
│  │  Adapter   │  │  Adapter   │               │            │
│  └────────────┘  └────────────┘               │            │
└────────────────────────────────────────────────┴────────────┘
                        │
                        ▼
            ┌──────────────────────┐
            │   NormalizedData     │
            │  (Unified Output)    │
            └──────────────────────┘
                        │
                        ▼
            ┌──────────────────────┐
            │  VectorDatabase /    │
            │  Embedding Pipeline  │
            └──────────────────────┘
```

### Component Descriptions

#### 1. DataAdapterManager
Central orchestrator that:
- Registers and manages all adapters
- Detects file formats automatically
- Routes files to appropriate adapters
- Handles batch processing
- Manages error recovery

#### 2. IDataAdapter Interface
Base interface that all adapters implement:
```cpp
class IDataAdapter {
public:
    // Format detection
    virtual bool can_handle(const fs::path& path) const = 0;
    virtual bool can_handle(std::string_view content) const = 0;
    
    // Read operations
    virtual Result<NormalizedData> parse(const fs::path& path, const ChunkConfig& config) = 0;
    virtual Result<NormalizedData> parse_content(std::string_view content, const ChunkConfig& config, std::string_view source_hint) = 0;
    
    // Write operations (NEW in v2.0)
    virtual Result<void> write(const NormalizedData& data, const fs::path& path) = 0;
    virtual bool supports_write() const = 0;
    
    // Data processing
    virtual Result<void> sanitize(NormalizedData& data) = 0;
    
    // Metadata
    virtual std::string name() const = 0;
    virtual std::vector<DataFormat> supported_formats() const = 0;
};
```

#### 3. NormalizedData
Unified output format:
```cpp
struct NormalizedData {
    std::vector<DataChunk> chunks;                             // Document chunks
    DataFormat format;                                         // Source format
    std::string source_path;                                   // Original path
    std::unordered_map<std::string, std::string> global_metadata;  // Doc metadata
    float confidence;                                          // Detection confidence
    bool sanitized;                                            // Sanitization status
    std::vector<std::string> warnings;                         // Processing warnings
};
```

---

## Implemented Adapters

### 1. CSV Adapter ✅

**Purpose**: Tabular data, spreadsheet exports, time series data

**Supported Files**: `.csv`, `.tsv`

**Features**:
- Configurable delimiter (comma, tab, semicolon, etc.)
- Custom quote character
- Header detection and custom column names
- Row-level or column-level chunking
- Automatic numerical feature extraction
- Empty row skipping

**Configuration**:
```cpp
CSVConfig config;
config.delimiter = ',';           // Field separator
config.quote = '"';               // Quote character
config.has_header = true;         // First row contains headers
config.skip_empty_rows = true;    // Ignore blank rows
config.column_names = {};         // Override header names
```

**Example**:
```cpp
#include "vdb/adapters/csv_adapter.hpp"

using namespace vdb::adapters;

CSVConfig config;
config.delimiter = ',';
config.has_header = true;

CSVAdapter adapter(config);
auto result = adapter.parse("financial_data.csv");

if (result) {
    std::cout << "Parsed " << result->chunks.size() << " rows\n";
    for (const auto& chunk : result->chunks) {
        // Each chunk represents one row
        std::cout << "Row " << chunk.metadata.at("row_number") << ": " 
                  << chunk.content << "\n";
    }
}
```

**Use Cases**:
- Financial data (stock prices, economic indicators)
- Sensor readings and time series
- Database exports
- Survey results
- Log files in tabular format

---

### 2. JSON Adapter ✅

**Purpose**: Structured data from APIs, exports, and configurations

**Supported Files**: `.json`, `.jsonl`

**Supported Structures**:
1. **Single Object**: One JSON object per file
2. **Array of Objects**: `[{...}, {...}, {...}]`
3. **JSON Lines (JSONL)**: One object per line, newline-separated
4. **Nested Objects**: Automatically flattened with path-based keys

**Features**:
- Automatic structure detection
- Nested object flattening
- Array element extraction
- Type-aware parsing (strings, numbers, booleans, nulls)
- Metadata extraction from known fields
- Pretty-print writing support

**Example - Single Object**:
```cpp
#include "vdb/adapters/json_adapter.hpp"

using namespace vdb::adapters;

JSONAdapter adapter;
auto result = adapter.parse("document.json");

if (result) {
    for (const auto& chunk : result->chunks) {
        std::cout << "Content: " << chunk.content << "\n";
        if (chunk.title) {
            std::cout << "Title: " << *chunk.title << "\n";
        }
    }
}
```

**Example - JSON Lines**:
```cpp
// Input file: logs.jsonl
// {"timestamp": "2025-12-01T10:00:00Z", "level": "INFO", "message": "Server started"}
// {"timestamp": "2025-12-01T10:05:00Z", "level": "WARN", "message": "High memory usage"}

JSONAdapter adapter;
auto result = adapter.parse("logs.jsonl");

// Each line becomes a separate chunk
std::cout << "Parsed " << result->chunks.size() << " log entries\n";
```

**Metadata Extraction**:
The adapter automatically recognizes and extracts these fields:
- `id`, `_id`, `identifier` → stored as `id`
- `title`, `name` → stored as `title`
- `date`, `timestamp`, `created_at`, `updated_at` → stored as `date`
- `source`, `url`, `link` → stored as `source`
- `type`, `category`, `tags` → stored in metadata

**Use Cases**:
- API responses
- NoSQL database exports (MongoDB, CouchDB)
- Application logs (structured logging)
- Configuration files
- Web scraping results

---

### 3. Plain Text Adapter ✅

**Purpose**: General text documents, logs, transcripts

**Supported Files**: `.txt`, `.log`, `.md` (markdown)

**Features**:
- Automatic encoding detection (UTF-8, UTF-16, Latin-1, etc.)
- Encoding conversion to UTF-8
- Markdown format detection
- Code block detection
- Whitespace normalization (optional)
- Language detection (with ICU library)

**Configuration**:
```cpp
TextConfig config;
config.auto_detect_encoding = true;    // Auto-detect character encoding
config.default_encoding = "UTF-8";     // Fallback encoding
config.detect_language = false;        // Detect text language
config.detect_markdown = true;         // Check if content is markdown
config.detect_code = true;             // Detect code blocks
config.preserve_whitespace = false;    // Keep original whitespace
```

**Example**:
```cpp
#include "vdb/adapters/text_adapter.hpp"

using namespace vdb::adapters;

TextConfig config;
config.auto_detect_encoding = true;
config.detect_markdown = true;

TextAdapter adapter(config);
auto result = adapter.parse("notes.txt");

if (result) {
    std::cout << "Format: " << format_to_string(result->format) << "\n";
    std::cout << "Confidence: " << result->confidence << "\n";
    
    // If markdown was detected, format will be DataFormat::Markdown
    if (result->format == DataFormat::Markdown) {
        std::cout << "This file contains markdown formatting\n";
    }
}
```

**Use Cases**:
- Plain text documents
- Application logs
- Meeting transcripts
- Code files
- Configuration files
- README files

---

### 4. PDF Adapter ✅

**Purpose**: Academic papers, reports, documentation, e-books

**Supported Files**: `.pdf`

**Features**:
- Text extraction using Poppler library
- Page-by-page processing
- Table detection and extraction
- Document metadata parsing (title, author, keywords, dates)
- Table of Contents (TOC) extraction
- Layout preservation (optional)
- Page range selection

**Configuration**:
```cpp
PDFConfig config;
config.extract_tables = true;      // Detect and extract tables
config.extract_images = false;     // Extract embedded images (not yet implemented)
config.preserve_layout = true;     // Keep spatial text layout
config.start_page = 0;             // First page (0-indexed)
config.end_page = -1;              // -1 = all pages
config.extract_toc = true;         // Extract table of contents
```

**Example**:
```cpp
#include "vdb/adapters/pdf_adapter.hpp"

using namespace vdb::adapters;

PDFConfig config;
config.extract_tables = true;
config.extract_toc = true;
config.start_page = 0;
config.end_page = 10;  // Only first 10 pages

PDFAdapter adapter(config);
auto result = adapter.parse("research_paper.pdf");

if (result) {
    std::cout << "Pages: " << result->global_metadata["page_count"] << "\n";
    std::cout << "Title: " << result->global_metadata["title"] << "\n";
    std::cout << "Author: " << result->global_metadata["author"] << "\n";
    
    for (const auto& chunk : result->chunks) {
        std::cout << "Page " << chunk.metadata.at("page_number") 
                  << ": " << chunk.content.substr(0, 100) << "...\n";
    }
}
```

**Metadata Extracted**:
- Document title
- Author(s)
- Creation date
- Modification date
- Keywords
- Page count
- Producer/Creator software

**Limitations**:
- Requires text-based PDFs (scanned images require OCR, not yet implemented)
- Password-protected PDFs not supported
- Complex layouts may not extract perfectly

**Use Cases**:
- Academic papers and research
- Technical documentation
- Business reports
- Books and manuscripts
- Legal documents

---

### 5. Excel Adapter ✅

**Purpose**: Spreadsheets, financial models, structured datasets

**Supported Files**: `.xlsx`, `.xls`

**Features**:
- Multi-sheet reading and writing
- Header row detection
- Formula evaluation (optional, slower)
- Type-aware cell parsing (string, number, boolean, date, error)
- Chart metadata extraction (optional)
- Range selection (start row/column)
- Sheet selection by name

**Configuration**:
```cpp
ExcelConfig config;
config.sheet_names = {};           // Empty = all sheets, or specify names
config.has_header = true;          // First row is header
config.skip_empty_rows = true;     // Ignore blank rows
config.evaluate_formulas = false;  // Evaluate formulas (slower)
config.extract_charts = false;     // Extract chart metadata
config.start_row = 0;              // Starting row (0-indexed)
config.start_col = 0;              // Starting column (0-indexed)
```

**Example**:
```cpp
#include "vdb/adapters/excel_adapter.hpp"

using namespace vdb::adapters;

ExcelConfig config;
config.sheet_names = {"Sales", "Inventory"};  // Only these sheets
config.has_header = true;
config.evaluate_formulas = false;

ExcelAdapter adapter(config);
auto result = adapter.parse("financial_model.xlsx");

if (result) {
    for (const auto& chunk : result->chunks) {
        std::cout << "Sheet: " << chunk.metadata.at("sheet_name") 
                  << ", Row: " << chunk.metadata.at("row_number") << "\n";
        std::cout << "Content: " << chunk.content << "\n";
    }
}
```

**Cell Types**:
```cpp
enum class CellType {
    Empty,     // Blank cell
    String,    // Text content
    Number,    // Floating point value (includes dates as serial numbers)
    Boolean,   // TRUE/FALSE
    Error      // Formula error (#DIV/0!, #N/A, etc.)
};
```

**Use Cases**:
- Financial models and spreadsheets
- Database tables exported to Excel
- Survey results
- Inventory tracking
- Scientific data tables

---

### 6. XML Adapter ✅

**Purpose**: Structured documents, configuration files, data exchange

**Supported Files**: `.xml`

**Capabilities**: ✅ Read | ✅ Write

**Features**:
- DOM-based XML parsing and generation
- Text content extraction from all nodes
- Attribute extraction and metadata storage
- Hierarchical path preservation
- Namespace handling
- CDATA support
- **Write support**: Generate well-formed XML with proper escaping

**Configuration**:
```cpp
XMLConfig config;
config.extract_text_content = true;    // Extract all text nodes
config.extract_attributes = true;      // Extract element attributes
config.preserve_hierarchy = true;      // Maintain XML structure in metadata
config.flatten_namespaces = true;      // Remove namespace prefixes
```

**Read Example**:
```cpp
#include "vdb/adapters/xml_adapter.hpp"
#include "vdb/logging.hpp"

using namespace vdb::adapters;

XMLAdapter adapter;
auto result = adapter.parse("catalog.xml");

if (result) {
    LOG_INFO("Parsed XML file with " + std::to_string(result->chunks.size()) + " chunks");
    
    for (const auto& chunk : result->chunks) {
        std::cout << "Root: " << chunk.title.value_or("unknown") << "\n";
        std::cout << "Content: " << chunk.content << "\n";
        
        // Access XML attributes from metadata
        for (const auto& [key, value] : chunk.metadata) {
            std::cout << "  " << key << ": " << value << "\n";
        }
    }
} else {
    LOG_ERROR("Failed to parse XML: " + result.error().message);
}
```

**Write Example** (NEW in v2.0):
```cpp
#include "vdb/adapters/xml_adapter.hpp"

// Create NormalizedData from your content
NormalizedData data;
DataChunk chunk;
chunk.content = "Product information and specifications";
chunk.metadata["product_id"] = "12345";
chunk.metadata["category"] = "Electronics";
chunk.title = "Product Catalog";
data.chunks.push_back(chunk);

// Write to XML file
XMLAdapter adapter;
auto write_result = adapter.write(data, "output.xml");

if (write_result) {
    LOG_INFO("Successfully exported to XML");
} else {
    LOG_ERROR("Export failed: " + write_result.error().message);
}
```

**Generated XML Structure**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<data>
  <chunk index="0" total="1">
    <title>Product Catalog</title>
    <content>Product information and specifications</content>
    <metadata>
      <product_id>12345</product_id>
      <category>Electronics</category>
    </metadata>
  </chunk>
</data>
```

**Security**: Automatic XML escaping for special characters (&, <, >, ", ')

**Use Cases**:
- Configuration files (Maven, Spring)
- RSS/Atom feeds
- Web service responses
- SVG metadata extraction
- Office document internals
- Data interchange with legacy systems

---

### 7. Parquet Adapter ✅

**Purpose**: Columnar data storage, big data analytics

**Supported Files**: `.parquet`

**Capabilities**: ✅ Read | ✅ Write (with Apache Arrow)

**Features**:
- **Full Apache Arrow C++ integration** (NEW in v2.0)
- Columnar data reading and writing
- Row-based or column-based chunking
- Schema extraction and preservation
- Batch processing for large files
- Type-aware parsing (STRING, INT64, DOUBLE, FLOAT, BOOL)
- Numerical feature extraction for embeddings
- Comprehensive error handling

**Configuration**:
```cpp
ParquetConfig config;
config.row_based_chunks = true;        // Extract by row (vs. by column)
config.batch_size = 10000;             // Process rows in batches
config.extract_schema = true;          // Include schema metadata
config.include_column_names = true;    // Include column names in content
```

**Installation** (Required for full functionality):
```bash
# Ubuntu/Debian
sudo apt-get install libarrow-dev libparquet-dev

# macOS
brew install apache-arrow

# Windows (vcpkg)
vcpkg install arrow:x64-windows

# Build with Arrow support
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Read Example**:
```cpp
#include "vdb/adapters/parquet_adapter.hpp"
#include "vdb/logging.hpp"

using namespace vdb::adapters;

ParquetConfig config;
config.row_based_chunks = true;
config.batch_size = 1000;

ParquetAdapter adapter(config);
auto result = adapter.parse("analytics_data.parquet");

if (result) {
    LOG_INFO("Parsed Parquet file");
    
    // Access schema information
    std::cout << "Schema: " << result->global_metadata.at("schema") << "\n";
    std::cout << "Rows: " << result->global_metadata.at("num_rows") << "\n";
    std::cout << "Columns: " << result->global_metadata.at("num_columns") << "\n";
    
    // Process data
    for (const auto& chunk : result->chunks) {
        std::cout << "Row " << chunk.metadata.at("row_number") << ": ";
        std::cout << chunk.content << "\n";
        
        // Access numerical features (extracted automatically)
        if (!chunk.numerical_features.empty()) {
            std::cout << "  Numerical values: ";
            for (float val : chunk.numerical_features) {
                std::cout << val << " ";
            }
            std::cout << "\n";
        }
    }
} else {
    LOG_ERROR("Failed to parse Parquet: " + result.error().message);
}
```

**Write Example** (NEW in v2.0):
```cpp
#include "vdb/adapters/parquet_adapter.hpp"

// Create NormalizedData
NormalizedData data;
data.format = DataFormat::Parquet;

// Add chunks (rows of data)
for (int i = 0; i < 1000; ++i) {
    DataChunk chunk;
    chunk.content = "Row " + std::to_string(i) + " data";
    chunk.chunk_index = i;
    chunk.total_chunks = 1000;
    chunk.metadata["id"] = std::to_string(i);
    chunk.metadata["category"] = "test";
    chunk.numerical_features = {1.0f, 2.5f, 3.7f};
    data.chunks.push_back(chunk);
}

// Write to Parquet file with Apache Arrow
ParquetAdapter adapter;
auto write_result = adapter.write(data, "output.parquet");

if (write_result) {
    LOG_INFO("Successfully exported to Parquet format");
    std::cout << "Written " << data.chunks.size() << " rows\n";
} else {
    LOG_ERROR("Export failed: " + write_result.error().message);
}
```

**Performance**:
- Row-based: Ideal for record processing (e.g., logs, transactions)
- Column-based: Ideal for analytics (e.g., aggregations, ML features)
- Batch processing: Efficient memory usage for files > 1GB
- Zero-copy with Arrow: Minimal memory overhead

**Supported Types**:
- `STRING` / `LARGE_STRING`: Text data
- `INT64`: Integer values
- `DOUBLE` / `FLOAT`: Floating-point numbers (auto-extracted for embeddings)
- `BOOL`: Boolean values
- Additional types supported via Arrow schema

**Use Cases**:
- Data lake exports (Databricks, Snowflake)
- Pandas DataFrame storage
- Apache Spark outputs
- ML dataset storage
- Analytics pipelines
- Time-series data archives

---

### 8. SQLite Adapter ✅

**Purpose**: Relational database data extraction and creation

**Supported Files**: `.db`, `.sqlite`, `.sqlite3`, `.sql`

**Capabilities**: ✅ Read | ✅ Write

**Features**:
- Multiple table support
- Custom SQL queries
- Schema extraction
- Row-based or table-based chunking
- Type-aware column parsing
- Batch reading
- **Write support**: Create and populate databases (NEW in v2.0)
- **Security**: SQL injection protection via identifier quoting and parameterized queries

**Configuration**:
```cpp
SQLiteConfig config;
config.tables = {"products", "customers"};  // Specific tables (empty = all tables)
config.row_based_chunks = true;             // One chunk per row
config.batch_size = 1000;                   // Batch size for reading
config.extract_column_names = true;         // Include column names
config.custom_query = "";                   // Optional custom SQL query
```

**Read Example**:
```cpp
#include "vdb/adapters/sqlite_adapter.hpp"
#include "vdb/logging.hpp"

using namespace vdb::adapters;

SQLiteConfig config;
config.tables = {"users", "posts"};  // Read specific tables

SQLiteAdapter adapter(config);
auto result = adapter.parse("app.db");

if (result) {
    LOG_INFO("Parsed SQLite database");
    
    std::cout << "Tables: " << result->global_metadata.at("num_tables") << "\n";
    std::cout << "Rows: " << result->chunks.size() << "\n";
    
    for (const auto& chunk : result->chunks) {
        std::cout << "Table: " << chunk.metadata.at("table") << "\n";
        std::cout << "Row: " << chunk.metadata.at("row_number") << "\n";
        std::cout << "Data: " << chunk.content << "\n";
    }
} else {
    LOG_ERROR("Failed to parse SQLite: " + result.error().message);
}
```

**Read with Custom Query**:
```cpp
SQLiteConfig config;
config.custom_query = "SELECT * FROM orders WHERE date > '2024-01-01' AND status = 'completed'";

SQLiteAdapter adapter(config);
auto result = adapter.parse("sales.db");

if (result) {
    std::cout << "Query returned " << result->chunks.size() << " rows\n";
}
```

**Write Example** (NEW in v2.0):
```cpp
#include "vdb/adapters/sqlite_adapter.hpp"

// Create NormalizedData with database records
NormalizedData data;
data.format = DataFormat::SQLite;

// Add records
for (int i = 0; i < 100; ++i) {
    DataChunk chunk;
    chunk.content = "User record " + std::to_string(i);
    chunk.metadata["user_id"] = std::to_string(i);
    chunk.metadata["username"] = "user" + std::to_string(i);
    chunk.metadata["email"] = "user" + std::to_string(i) + "@example.com";
    chunk.metadata["created_at"] = "2024-01-01";
    chunk.metadata["table"] = "users";  // Specify target table
    data.chunks.push_back(chunk);
}

// Write to SQLite database
SQLiteAdapter adapter;
auto write_result = adapter.write(data, "output.db");

if (write_result) {
    LOG_INFO("Successfully created SQLite database");
    std::cout << "Written " << data.chunks.size() << " records\n";
} else {
    LOG_ERROR("Database creation failed: " + write_result.error().message);
}
```

**Generated Database Structure**:
- Automatically creates tables based on metadata keys
- Uses parameterized INSERT statements (SQL injection safe)
- Preserves data types from metadata
- Supports multiple tables in single database

**Security Features**:
- **Identifier quoting**: Prevents SQL injection in table/column names
- **Parameterized queries**: Prevents SQL injection in values
- **Input validation**: Type checking and bounds validation
- **Error handling**: Comprehensive error messages without exposing internals

**Use Cases**:
- Application databases (mobile apps, desktop apps)
- Configuration databases
- Data exports and backups
- Offline sync and caching
- ETL pipelines
- Data warehousing
- Testing and development databases

---

### 9. pgvector Adapter ✅

**Purpose**: Distributed vector storage with PostgreSQL

**Supported**: PostgreSQL connection with pgvector extension

**Capabilities**: ✅ Read | ✅ Write (Bidirectional Sync)

**Features**:
- Bidirectional vector sync (read/write)
- Native similarity search (cosine, L2, inner product)
- IVFFlat and HNSW indexes
- Batch operations for high throughput
- Custom metadata columns
- Connection pooling support
- **Security**: SQL injection protection via identifier quoting and literal escaping

**Configuration**:
```cpp
PgvectorConfig config;
config.host = "localhost";
config.port = 5432;
config.database = "vector_db";
config.user = "postgres";
config.password = "secure_password";  // Use environment variables in production
config.table = "embeddings";
config.vector_column = "embedding";
config.content_column = "content";
config.metadata_columns = {"author", "date", "source"};
config.dimension = 512;  // Vector dimension
```

**Setup** (One-time PostgreSQL configuration):
```sql
-- Install pgvector extension
CREATE EXTENSION IF NOT EXISTS vector;

-- Create table for vectors
CREATE TABLE embeddings (
    id SERIAL PRIMARY KEY,
    embedding vector(512),  -- Adjust dimension as needed
    content TEXT,
    author TEXT,
    date TEXT,
    source TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create HNSW index for fast similarity search
CREATE INDEX ON embeddings USING hnsw (embedding vector_cosine_ops);

-- Or use IVFFlat for larger datasets
CREATE INDEX ON embeddings USING ivfflat (embedding vector_cosine_ops) WITH (lists = 100);
```

**Read Example** (Query similar vectors):
```cpp
#include "vdb/adapters/pgvector_adapter.hpp"
#include "vdb/logging.hpp"

using namespace vdb::adapters;

PgvectorConfig config;
config.host = "localhost";
config.database = "vectors";
config.user = "postgres";
config.table = "embeddings";
config.vector_column = "embedding";

PgvectorAdapter adapter(config);

// Connect to PostgreSQL
auto conn = adapter.connect();
if (!conn) {
    LOG_ERROR("Failed to connect: " + conn.error().message);
    return;
}

// Query similar vectors
std::vector<float> query_vector(512, 0.0f);  // Your query embedding
auto results = adapter.query_similar(query_vector, 10);  // Top 10 results

if (results) {
    LOG_INFO("Found " + std::to_string(results->chunks.size()) + " similar vectors");
    
    for (const auto& chunk : results->chunks) {
        std::cout << "Content: " << chunk.content << "\n";
        std::cout << "Distance: " << chunk.metadata.at("distance") << "\n";
        std::cout << "Author: " << chunk.metadata.at("author") << "\n\n";
    }
}
```

**Write Example** (Insert vectors):
```cpp
#include "vdb/adapters/pgvector_adapter.hpp"

PgvectorAdapter adapter(config);
adapter.connect();

// Prepare vectors to insert
std::vector<std::vector<float>> vectors;
std::vector<std::string> contents;
std::vector<std::unordered_map<std::string, std::string>> metadata;

for (int i = 0; i < 1000; ++i) {
    // Generate or encode vectors
    std::vector<float> vec(512);
    // ... fill vector ...
    vectors.push_back(vec);
    
    contents.push_back("Document " + std::to_string(i));
    
    metadata.push_back({
        {"author", "user@example.com"},
        {"date", "2024-01-01"},
        {"source", "import_batch_1"}
    });
}

// Batch insert
auto result = adapter.insert_vectors(vectors, contents, metadata);

if (result) {
    LOG_INFO("Inserted " + std::to_string(*result) + " vectors");
    std::cout << "Successfully synced to PostgreSQL\n";
} else {
    LOG_ERROR("Insert failed: " + result.error().message);
}
```

**Similarity Search Options**:
```cpp
// Cosine similarity (default, normalized vectors)
auto cosine_results = adapter.query_similar(query_vec, 10, "cosine");

// L2 distance (Euclidean)
auto l2_results = adapter.query_similar(query_vec, 10, "l2");

// Inner product (for non-normalized vectors)
auto ip_results = adapter.query_similar(query_vec, 10, "inner_product");
```

**Performance Tips**:
- Use HNSW index for < 1M vectors
- Use IVFFlat index for > 1M vectors
- Batch inserts for better throughput (1000-10000 vectors per batch)
- Use connection pooling for concurrent access
- Tune `lists` parameter in IVFFlat based on dataset size

**Security Features**:
- **Identifier quoting**: Prevents SQL injection in table/column names
- **Literal escaping**: Prevents SQL injection in string values
- **Parameterized queries**: Safe value insertion
- **Connection encryption**: SSL support via connection string

**Use Cases**:
- Distributed vector search across multiple nodes
- Shared vector database for team/organization
- Hybrid search (vector + metadata filtering)
- Real-time vector updates and queries
- Integration with existing PostgreSQL infrastructure
- Multi-tenancy with row-level security
```

**Setup PostgreSQL**:
```sql
-- Install pgvector extension
CREATE EXTENSION vector;

-- Create table
CREATE TABLE embeddings (
    id SERIAL PRIMARY KEY,
    content TEXT,
    embedding vector(384),
    author TEXT,
    date DATE
);

-- Create index
CREATE INDEX ON embeddings USING ivfflat (embedding vector_cosine_ops);
```

**Example - Reading**:
```cpp
#include "vdb/adapters/pgvector_adapter.hpp"

PgvectorConfig config;
config.host = "localhost";
config.database = "vector_db";
config.user = "postgres";
config.table = "embeddings";

PgvectorAdapter adapter(config);

// Connect and read
auto conn = adapter.connect();
auto result = adapter.parse({});  // Read all vectors

if (result) {
    for (const auto& chunk : result->chunks) {
        std::cout << chunk.content << "\n";
    }
}
```

**Example - Similarity Search**:
```cpp
// Query similar vectors
std::vector<float> query = {0.1, 0.2, ...};  // Your query vector
auto similar = adapter.query_similar(query, /*k=*/10, /*threshold=*/0.5);

if (similar) {
    for (const auto& result : *similar) {
        float distance = result.numerical_features[0];
        std::cout << "Distance: " << distance << "\n";
        std::cout << "Content: " << result.content << "\n";
    }
}
```

**Example - Writing**:
```cpp
// Insert vectors
std::vector<std::vector<float>> vectors = {...};
std::vector<std::string> contents = {...};
std::vector<std::unordered_map<std::string, std::string>> metadata = {...};

auto inserted = adapter.insert_vectors(vectors, contents, metadata);
std::cout << "Inserted " << *inserted << " vectors\n";
```

**Use Cases**:
- Distributed vector storage
- Enterprise-scale deployments
- Hybrid SQL + vector queries
- Multi-tenant applications
- Real-time analytics

---

### 10. PDF Adapter ✅

**Purpose**: PDF document reading and creation

**Supported Files**: `.pdf`

**Capabilities**: ✅ Read (with Poppler) | ✅ Write (basic PDF 1.4)

**Features**:
- **Read**: Text extraction via Poppler library
- **Write**: Generate valid PDF 1.4 documents (NEW in v2.0)
- Word wrapping and multi-line support
- PDF string escaping for special characters
- Metadata preservation
- Ready for libharu integration for advanced features

**Read Example** (existing functionality):
```cpp
#include "vdb/adapters/pdf_adapter.hpp"

PDFAdapter adapter;
auto result = adapter.parse("document.pdf");

if (result) {
    for (const auto& chunk : result->chunks) {
        std::cout << "Page " << chunk.metadata.at("page") << ": ";
        std::cout << chunk.content << "\n";
    }
}
```

**Write Example** (NEW in v2.0):
```cpp
#include "vdb/adapters/pdf_adapter.hpp"
#include "vdb/logging.hpp"

// Create NormalizedData
NormalizedData data;
DataChunk chunk;
chunk.title = "Research Report";
chunk.content = "This is a comprehensive research report discussing "
                "various topics related to data science and machine learning. "
                "The document includes multiple sections with detailed analysis.";
data.chunks.push_back(chunk);

// Write to PDF
PDFAdapter adapter;
auto write_result = adapter.write(data, "report.pdf");

if (write_result) {
    LOG_INFO("Successfully created PDF document");
} else {
    LOG_ERROR("PDF creation failed: " + write_result.error().message);
}
```

**Generated PDF Features**:
- Valid PDF 1.4 format
- Basic text rendering with automatic word wrapping
- PDF string escaping for parentheses and backslashes
- Simple page layout (can be enhanced with libharu)

**Use Cases**:
- Report generation
- Document exports
- Data visualization reports
- Academic papers
- Legal documents
- Invoices and receipts

---

## Logging System Integration

**NEW in v2.0**: Comprehensive runtime logging with anomaly detection

All data ingestion operations are automatically logged for monitoring, debugging, and compliance. The logging system provides zero-blind-spot observability.

### Logging Features

- ✅ **Thread-safe**: Multi-level logging (DEBUG, INFO, WARN, ERROR, CRITICAL, ANOMALY)
- ✅ **Dual output**: Console (with ANSI colors) and file logging
- ✅ **Separate anomaly log**: Dedicated `anomalies.log` for unusual events
- ✅ **Automatic log rotation**: Configurable file size limits
- ✅ **Source location tracking**: File and line numbers in logs
- ✅ **15 anomaly types**: Including PARSE_ERROR, SQL_INJECTION_ATTEMPT, DATA_CORRUPTION, PERFORMANCE_DEGRADATION, SECURITY_VIOLATION

### Configuration

```cpp
#include "vdb/logging.hpp"

// Initialize logging system (once at startup)
LoggerConfig config;
config.min_level = LogLevel::INFO;                           // Minimum log level
config.log_to_console = true;                                // Console output
config.log_to_file = true;                                   // File output
config.log_file_path = "/var/log/vectordb/vectordb.log";    // Main log
config.anomaly_log_path = "/var/log/vectordb/anomalies.log";// Anomaly log
config.max_file_size = 100 * 1024 * 1024;                   // 100MB rotation
config.enable_colors = true;                                 // ANSI colors

Logger::instance().initialize(config);
```

### Usage in Adapters

All adapters automatically log operations:

```cpp
#include "vdb/adapters/xml_adapter.hpp"
#include "vdb/logging.hpp"

XMLAdapter adapter;
auto result = adapter.parse("data.xml");

// Adapter automatically logs:
// [INFO] Parsing XML file: data.xml
// [INFO] Extracted 42 chunks from XML
// [DEBUG] Processing element: <product>
// [WARN] Empty element encountered: <description>

if (!result) {
    // Errors are automatically logged
    LOG_ERROR("Parse failed: " + result.error().message);
    
    // Log anomaly for investigation
    LOG_ANOMALY(AnomalyType::PARSE_ERROR, 
               "XML parse error in data.xml: " + result.error().message);
}
```

### Anomaly Detection

The system automatically detects and logs anomalies:

- **PARSE_ERROR**: Malformed file or unexpected structure
- **DATA_CORRUPTION**: Inconsistent or corrupted data
- **SQL_INJECTION_ATTEMPT**: Potential security violation
- **MEMORY_ANOMALY**: Unusual memory usage patterns
- **PERFORMANCE_DEGRADATION**: Slow operations
- **SECURITY_VIOLATION**: Access control or privilege issues
- **UNEXPECTED_SIZE**: File/data size outside expected range
- **MISSING_DATA**: Required fields or data not found
- **INVALID_FORMAT**: Format doesn't match specification
- **CONNECTION_FAILURE**: Network or database connection issues

### Monitoring Example

```cpp
#include "vdb/logging.hpp"

// Log data ingestion start
LOG_INFO("Starting batch data ingestion");

// Process files
for (const auto& file : files) {
    LOG_DEBUG("Processing file: " + file.string());
    
    auto result = manager.auto_parse(file);
    
    if (result) {
        LOG_INFO("Successfully parsed: " + file.string() + 
                " (" + std::to_string(result->chunks.size()) + " chunks)");
    } else {
        LOG_ERROR("Failed to parse: " + file.string());
        LOG_ANOMALY(AnomalyType::PARSE_ERROR, 
                   "Parse failure: " + file.string());
    }
}

LOG_INFO("Batch ingestion complete");
```

### Log Output Example

**Console Output** (with colors):
```
[2024-01-04 10:30:45.123] [INFO] Starting batch data ingestion
[2024-01-04 10:30:45.234] [DEBUG] Processing file: data.xml
[2024-01-04 10:30:45.456] [INFO] Successfully parsed: data.xml (42 chunks)
[2024-01-04 10:30:45.567] [WARN] Large file detected: data.parquet (2.5GB)
[2024-01-04 10:30:46.789] [ERROR] Failed to parse: corrupt.db
[2024-01-04 10:30:46.790] [ANOMALY] PARSE_ERROR: corrupt.db - invalid header
```

**Anomaly Log** (`anomalies.log`):
```
[2024-01-04 10:30:46.790] [ANOMALY] PARSE_ERROR
  Message: Parse failure: corrupt.db - invalid header
  Source: /home/user/project/src/adapters/sqlite_adapter.cpp:123
  
[2024-01-04 10:35:12.456] [ANOMALY] PERFORMANCE_DEGRADATION
  Message: Slow parse operation: 15.2 seconds for 100MB file
  Source: /home/runner/work/vector_studio/vector_studio/src/adapters/parquet_adapter.cpp:234
```

### Production Deployment

**Recommended Configuration**:
```cpp
// Production environment
LoggerConfig prod_config;
prod_config.min_level = LogLevel::INFO;  // INFO and above
prod_config.log_to_console = false;      // Disable console in production
prod_config.log_to_file = true;          // Enable file logging
prod_config.log_file_path = "/var/log/vectordb/vectordb.log";
prod_config.anomaly_log_path = "/var/log/vectordb/anomalies.log";
prod_config.max_file_size = 100 * 1024 * 1024;  // 100MB rotation
prod_config.enable_colors = false;       // No ANSI colors in files

Logger::instance().initialize(prod_config);
```

**Development Configuration**:
```cpp
// Development environment
LoggerConfig dev_config;
dev_config.min_level = LogLevel::DEBUG;  // All logs
dev_config.log_to_console = true;        // Enable console
dev_config.log_to_file = true;           // Also log to file
dev_config.enable_colors = true;         // ANSI colors for readability

Logger::instance().initialize(dev_config);
```

### Monitoring and Alerts

Monitor the anomaly log for production issues:

```bash
# Tail anomaly log
tail -f /var/log/vectordb/anomalies.log

# Count anomalies by type
grep -oP 'ANOMALY\] \K[A-Z_]+' /var/log/vectordb/anomalies.log | sort | uniq -c

# Alert on critical anomalies
watch -n 60 'grep -c "SECURITY_VIOLATION\|SQL_INJECTION_ATTEMPT" /var/log/vectordb/anomalies.log'
```

**See Also**: [docs/LOGGING.md](LOGGING.md) for complete logging system documentation

---

## Chunking Strategies

Chunking divides large documents into smaller, manageable pieces suitable for embedding. The choice of strategy significantly impacts search quality and performance.

### Available Strategies

```cpp
enum class ChunkStrategy {
    Fixed,      // Fixed character/token count
    Sentence,   // Split by sentences (planned)
    Paragraph,  // Split by paragraphs (default)
    Semantic,   // Semantic similarity-based (planned)
    Sliding,    // Sliding window with overlap
    Document,   // One chunk per document (no splitting)
};
```

### Configuration

```cpp
ChunkConfig config;
config.strategy = ChunkStrategy::Paragraph;  // Choose strategy
config.max_chunk_size = 1000;                // Maximum characters per chunk
config.overlap = 100;                        // Overlap for sliding window
config.preserve_structure = true;            // Keep formatting/headers
```

### Strategy Details

#### 1. Paragraph (Default)
Splits text on double newlines (`\n\n`).

**Pros**:
- Preserves semantic boundaries
- Natural for documents
- Variable size adapts to content

**Cons**:
- Inconsistent chunk sizes
- May create very large or very small chunks

**Best For**: Articles, documentation, books, reports

#### 2. Fixed
Splits text every `max_chunk_size` characters.

**Pros**:
- Predictable, uniform chunk sizes
- Fast processing
- Consistent memory usage

**Cons**:
- May split mid-sentence or mid-word
- Can break semantic units

**Best For**: Large volumes of uniform text, performance-critical applications

#### 3. Sliding Window
Fixed-size chunks with configurable overlap.

**Pros**:
- Prevents information loss at boundaries
- Good for context-dependent text
- Captures cross-chunk relationships

**Cons**:
- Redundant storage (same text appears multiple times)
- More chunks to process
- Higher memory usage

**Best For**: Q&A systems, contexts where boundary information is critical

#### 4. Document
No splitting - entire document is one chunk.

**Pros**:
- Preserves complete document context
- Simplest strategy
- No boundary issues

**Cons**:
- Only suitable for small documents (< 1000 characters)
- Large chunks may exceed embedding model limits
- Inefficient for large files

**Best For**: Short documents, tweets, messages, small notes

#### 5. Sentence (Planned)
Splits on sentence boundaries, groups until reaching `max_chunk_size`.

**Best For**: Q&A systems, chatbots, precise retrieval

#### 6. Semantic (Planned)
Analyzes content similarity and groups related sentences together.

**Best For**: Highest quality search, when processing time is not critical

---

## Usage Examples

### Basic Auto-Detection

The simplest way to ingest data - just provide a file path:

```cpp
#include "vdb/adapters/data_adapter.hpp"

using namespace vdb::adapters;

int main() {
    DataAdapterManager manager;
    
    // Automatically detect format and parse
    auto result = manager.auto_parse("data.csv");
    
    if (!result) {
        std::cerr << "Error: " << result.error().message << "\n";
        return 1;
    }
    
    std::cout << "Parsed " << result->chunks.size() << " chunks\n";
    std::cout << "Format: " << format_to_string(result->format) << "\n";
    std::cout << "Confidence: " << result->confidence << "\n";
    
    for (const auto& chunk : result->chunks) {
        std::cout << "Chunk " << chunk.chunk_index << ": " 
                  << chunk.content << "\n";
    }
    
    return 0;
}
```

### Custom Chunking Configuration

Control how documents are split:

```cpp
DataAdapterManager manager;

ChunkConfig config;
config.strategy = ChunkStrategy::Sliding;
config.max_chunk_size = 500;   // 500 characters per chunk
config.overlap = 50;            // 50 character overlap
config.preserve_structure = true;

auto result = manager.auto_parse("long_document.txt", config);

if (result) {
    std::cout << "Created " << result->chunks.size() << " overlapping chunks\n";
}
```

### Batch Processing Multiple Files

Process many files efficiently:

```cpp
DataAdapterManager manager;

std::vector<fs::path> files = {
    "data1.csv",
    "data2.json",
    "document.pdf",
    "spreadsheet.xlsx",
    "notes.txt"
};

ChunkConfig config;
config.strategy = ChunkStrategy::Paragraph;
config.max_chunk_size = 1000;

auto results = manager.parse_batch(files, config, /*max_parallel=*/4);

if (results) {
    std::cout << "Successfully processed " << results->size() << " files\n";
    
    for (size_t i = 0; i < results->size(); ++i) {
        const auto& data = (*results)[i];
        std::cout << "File " << files[i] << ": " 
                  << data.chunks.size() << " chunks\n";
    }
} else {
    std::cerr << "Batch processing failed: " << results.error().message << "\n";
}
```

### Custom Adapter Registration

Add your own adapter:

```cpp
class MyCustomAdapter : public IDataAdapter {
public:
    bool can_handle(const fs::path& path) const override {
        return path.extension() == ".custom";
    }
    
    bool can_handle(std::string_view content) const override {
        return content.starts_with("CUSTOM_FORMAT");
    }
    
    Result<NormalizedData> parse(const fs::path& path, const ChunkConfig& config) override {
        // Your parsing logic here
        NormalizedData data;
        // ... populate data ...
        return data;
    }
    
    // ... implement other methods ...
    
    std::string name() const override { return "CustomFormat"; }
    std::vector<DataFormat> supported_formats() const override {
        return {DataFormat::Unknown};  // Or define custom format
    }
};

// Register the adapter
DataAdapterManager manager;
manager.register_adapter(std::make_unique<MyCustomAdapter>());

// Now can parse .custom files
auto result = manager.auto_parse("mydata.custom");
```

### Content-Based Parsing

Parse content directly without files:

```cpp
DataAdapterManager manager;

std::string json_content = R"(
{
  "id": "123",
  "title": "Sample Document",
  "content": "This is the main content of the document."
}
)";

auto result = manager.auto_parse_content(json_content, {}, "json");

if (result) {
    std::cout << "Detected format: " << format_to_string(result->format) << "\n";
    std::cout << "Content: " << result->chunks[0].content << "\n";
}
```

---

## Integration with VectorDatabase

### Complete Ingestion Pipeline

```cpp
#include "vdb/vector_database.hpp"
#include "vdb/adapters/data_adapter.hpp"
#include "vdb/text_encoder.hpp"

using namespace vdb;
using namespace vdb::adapters;

int main() {
    // 1. Initialize database
    DatabaseConfig db_config;
    db_config.dimension = 384;  // MiniLM embedding dimension
    VectorDatabase db(db_config);
    
    // 2. Initialize text encoder
    TextEncoder encoder("models/all-MiniLM-L6-v2.onnx");
    
    // 3. Initialize adapter manager
    DataAdapterManager adapter_manager;
    
    // 4. Ingest data
    auto data = adapter_manager.auto_parse("financial_reports.csv");
    if (!data) {
        std::cerr << "Failed to parse: " << data.error().message << "\n";
        return 1;
    }
    
    // 5. Process each chunk
    for (const auto& chunk : data->chunks) {
        // Generate embedding
        auto embedding = encoder.encode(chunk.content);
        if (!embedding) {
            std::cerr << "Encoding failed for chunk " << chunk.chunk_index << "\n";
            continue;
        }
        
        // Create metadata
        Metadata meta;
        meta.content = chunk.content;
        if (chunk.title) {
            meta.metadata["title"] = *chunk.title;
        }
        if (chunk.date) {
            meta.metadata["date"] = *chunk.date;
        }
        if (chunk.source) {
            meta.metadata["source"] = *chunk.source;
        }
        for (const auto& [key, value] : chunk.metadata) {
            meta.metadata[key] = value;
        }
        
        // Add to database
        auto id = db.add_vector(embedding->view(), meta);
        std::cout << "Added chunk " << chunk.chunk_index 
                  << " with ID " << id << "\n";
    }
    
    // 6. Save database
    db.save();
    
    std::cout << "Successfully ingested " << data->chunks.size() 
              << " chunks into database\n";
    
    return 0;
}
```

### Python Integration

```python
import pyvdb

# Create database
db = pyvdb.VectorDatabase("./my_vectors", dimension=384)

# Ingest CSV file
db.ingest_file("data.csv", chunk_strategy="paragraph", max_chunk_size=1000)

# Ingest multiple files
files = ["report1.pdf", "data.json", "notes.txt"]
for file in files:
    db.ingest_file(file)

# Search
results = db.search("quarterly financial results", k=10)
for result in results:
    print(f"Score: {result.score:.3f}")
    print(f"Content: {result.content[:100]}...")
    print(f"Source: {result.metadata.get('source', 'unknown')}")
```

---

## Batch Processing

### Parallel Batch Processing

Process multiple files in parallel for improved performance:

```cpp
DataAdapterManager manager;

std::vector<fs::path> files;
for (const auto& entry : fs::directory_iterator("./data")) {
    if (entry.is_regular_file()) {
        files.push_back(entry.path());
    }
}

ChunkConfig config;
config.strategy = ChunkStrategy::Paragraph;
config.max_chunk_size = 1000;

// Process up to 8 files in parallel
auto results = manager.parse_batch(files, config, /*max_parallel=*/8);

if (results) {
    size_t total_chunks = 0;
    for (const auto& data : *results) {
        total_chunks += data.chunks.size();
    }
    std::cout << "Processed " << files.size() << " files, "
              << total_chunks << " total chunks\n";
}
```

### Error Handling in Batch Mode

```cpp
auto results = manager.parse_batch(files, config);

if (!results) {
    std::cerr << "Batch processing failed: " << results.error().message << "\n";
    return 1;
}

// Check individual file warnings
for (size_t i = 0; i < results->size(); ++i) {
    const auto& data = (*results)[i];
    if (!data.warnings.empty()) {
        std::cout << "Warnings for " << files[i] << ":\n";
        for (const auto& warning : data.warnings) {
            std::cout << "  - " << warning << "\n";
        }
    }
}
```

---

## Custom Adapter Development

### Step-by-Step Guide

1. **Create Adapter Class**

```cpp
#include "vdb/adapters/data_adapter.hpp"

namespace vdb::adapters {

class MyFormatAdapter : public IDataAdapter {
public:
    explicit MyFormatAdapter(/* config params */) {}
    
    // Step 2: Implement detection
    bool can_handle(const fs::path& path) const override {
        return path.extension() == ".myformat";
    }
    
    bool can_handle(std::string_view content) const override {
        // Check content signature/magic bytes
        return content.starts_with("MYFORMAT_MAGIC");
    }
    
    // Step 3: Implement parsing
    Result<NormalizedData> parse(const fs::path& path, const ChunkConfig& config) override {
        // Read file
        std::ifstream file(path);
        if (!file) {
            return Error{ErrorCode::IoError, "Failed to open file"};
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        return parse_content(content, config, path.string());
    }
    
    Result<NormalizedData> parse_content(std::string_view content, 
                                         const ChunkConfig& config,
                                         std::string_view source_hint) override {
        NormalizedData data;
        data.format = DataFormat::Unknown;  // Or custom format
        data.source_path = std::string(source_hint);
        data.confidence = 1.0f;
        
        // Your parsing logic here
        // - Extract text content
        // - Extract metadata
        // - Apply chunking strategy
        // - Extract numerical features
        
        DataChunk chunk;
        chunk.content = "...";  // Extracted text
        chunk.metadata["custom_field"] = "value";
        chunk.chunk_index = 0;
        chunk.total_chunks = 1;
        
        data.chunks.push_back(chunk);
        
        // Step 4: Sanitize
        auto sanitize_result = sanitize(data);
        if (!sanitize_result) {
            return sanitize_result.error();
        }
        
        data.sanitized = true;
        return data;
    }
    
    // Step 5: Implement sanitization
    Result<void> sanitize(NormalizedData& data) override {
        for (auto& chunk : data.chunks) {
            // Remove control characters
            chunk.content = sanitize_text(chunk.content);
            
            // Validate metadata
            for (const auto& [key, value] : chunk.metadata) {
                if (key.empty() || value.empty()) {
                    data.warnings.push_back("Empty metadata key or value");
                }
            }
        }
        return {};
    }
    
    // Step 6: Provide metadata
    std::string name() const override { return "MyFormat"; }
    
    std::vector<DataFormat> supported_formats() const override {
        return {DataFormat::Unknown};  // Define custom enum value if needed
    }
};

} // namespace vdb::adapters
```

2. **Register Adapter**

```cpp
DataAdapterManager manager;
manager.register_adapter(std::make_unique<MyFormatAdapter>());
```

3. **Use CMakeLists.txt**

```cmake
target_sources(vdb_core PRIVATE
    src/adapters/my_format_adapter.cpp
)

target_include_directories(vdb_core PUBLIC
    include/vdb/adapters
)
```

4. **Write Tests**

```cpp
TEST(MyFormatAdapterTest, CanDetectFormat) {
    MyFormatAdapter adapter;
    EXPECT_TRUE(adapter.can_handle("test.myformat"));
    EXPECT_FALSE(adapter.can_handle("test.txt"));
}

TEST(MyFormatAdapterTest, ParsesContent) {
    MyFormatAdapter adapter;
    std::string content = "MYFORMAT_MAGIC\nSome content here";
    
    auto result = adapter.parse_content(content, {}, "test");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->chunks.size(), 1);
    EXPECT_EQ(result->format, DataFormat::Unknown);
}
```

---

## Error Handling

### Error Types

All adapters return `Result<T>` types that can contain errors:

```cpp
enum class ErrorCode {
    Success,
    IoError,           // File I/O errors
    ParseError,        // Malformed data
    InvalidFormat,     // Unsupported or invalid format
    InvalidArgument,   // Bad function arguments
    OutOfMemory,       // Memory allocation failed
    NotImplemented,    // Feature not yet implemented
};

struct Error {
    ErrorCode code;
    std::string message;
    std::optional<std::string> details;
};
```

### Handling Errors

```cpp
auto result = manager.auto_parse("data.csv");

if (!result) {
    const auto& error = result.error();
    std::cerr << "Error: " << error.message << "\n";
    
    if (error.details) {
        std::cerr << "Details: " << *error.details << "\n";
    }
    
    switch (error.code) {
        case ErrorCode::IoError:
            std::cerr << "Check file permissions and path\n";
            break;
        case ErrorCode::ParseError:
            std::cerr << "File may be corrupted or invalid\n";
            break;
        case ErrorCode::InvalidFormat:
            std::cerr << "Format not supported\n";
            break;
        default:
            break;
    }
    
    return 1;
}

// Success path
const auto& data = *result;
```

### Warnings

Non-fatal issues are reported as warnings:

```cpp
auto result = manager.auto_parse("data.csv");

if (result && !result->warnings.empty()) {
    std::cout << "Parsed successfully with warnings:\n";
    for (const auto& warning : result->warnings) {
        std::cout << "  - " << warning << "\n";
    }
}
```

---

## Performance Considerations

### Optimization Strategies

1. **Lazy Loading**: Large files are read in chunks, not entirely into memory
2. **Streaming Parsers**: CSV and JSON use streaming parsers for memory efficiency
3. **Batch Processing**: Process multiple files in parallel
4. **Caching**: Repeated files can be cached (not yet implemented)

### Performance Benchmarks

| Format | Throughput | Latency (per file) | Memory Overhead |
|--------|-----------|-------------------|-----------------|
| CSV (10MB) | ~10 MB/sec | ~1 sec | ~1.5x file size |
| JSON (5MB) | ~5 MB/sec | ~1 sec | ~2x file size |
| PDF (10 pages) | ~2 pages/sec | ~5 sec | ~3x file size |
| Excel (5 sheets) | ~1 sheet/sec | ~5 sec | ~2x file size |
| Text (10MB) | ~20 MB/sec | ~0.5 sec | ~1.2x file size |

### Tuning Tips

1. **Chunk Size**: Smaller chunks = more overhead. Recommended: 500-2000 characters
2. **Batch Size**: Process 4-8 files in parallel for best throughput
3. **Format-Specific**:
   - CSV: Disable header detection if you know the structure
   - JSON: Use JSONL for large datasets
   - PDF: Limit page range if only a subset is needed
   - Excel: Specify exact sheets instead of loading all

---

## Testing

### Unit Tests

```cpp
#include <gtest/gtest.h>
#include "vdb/adapters/data_adapter.hpp"

TEST(DataAdapterTest, AutoDetectCSV) {
    DataAdapterManager manager;
    auto format = manager.detect_format("test.csv");
    EXPECT_EQ(format, DataFormat::CSV);
}

TEST(DataAdapterTest, ParseCSV) {
    DataAdapterManager manager;
    auto result = manager.auto_parse("test_data.csv");
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->chunks.size(), 0);
    EXPECT_EQ(result->format, DataFormat::CSV);
    EXPECT_TRUE(result->sanitized);
}

TEST(DataAdapterTest, ChunkingStrategy) {
    DataAdapterManager manager;
    
    ChunkConfig config;
    config.strategy = ChunkStrategy::Fixed;
    config.max_chunk_size = 100;
    
    auto result = manager.auto_parse("long_text.txt", config);
    
    ASSERT_TRUE(result.has_value());
    for (const auto& chunk : result->chunks) {
        EXPECT_LE(chunk.content.size(), 100);
    }
}
```

### Integration Tests

```cpp
TEST(IntegrationTest, EndToEndPipeline) {
    // Setup
    VectorDatabase db(DatabaseConfig{.dimension = 384});
    TextEncoder encoder("models/all-MiniLM-L6-v2.onnx");
    DataAdapterManager adapter;
    
    // Ingest
    auto data = adapter.auto_parse("test_document.pdf");
    ASSERT_TRUE(data.has_value());
    
    // Embed and store
    for (const auto& chunk : data->chunks) {
        auto embedding = encoder.encode(chunk.content);
        ASSERT_TRUE(embedding.has_value());
        
        Metadata meta;
        meta.content = chunk.content;
        auto id = db.add_vector(embedding->view(), meta);
        EXPECT_GT(id, 0);
    }
    
    // Query
    auto query_embedding = encoder.encode("test query");
    ASSERT_TRUE(query_embedding.has_value());
    
    auto results = db.search(query_embedding->view(), 5);
    EXPECT_GT(results.size(), 0);
}
```

---

## API Reference

### DataAdapterManager

```cpp
class DataAdapterManager {
public:
    DataAdapterManager();
    
    // Register custom adapter
    void register_adapter(std::unique_ptr<IDataAdapter> adapter);
    
    // Auto-detect and parse from file
    Result<NormalizedData> auto_parse(
        const fs::path& path,
        const ChunkConfig& config = {}
    );
    
    // Auto-detect and parse from content
    Result<NormalizedData> auto_parse_content(
        std::string_view content,
        const ChunkConfig& config = {},
        std::string_view hint = ""
    );
    
    // Detect format without parsing
    DataFormat detect_format(const fs::path& path) const;
    DataFormat detect_format_content(std::string_view content) const;
    
    // Get supported formats
    std::vector<DataFormat> supported_formats() const;
    
    // Batch processing
    Result<std::vector<NormalizedData>> parse_batch(
        const std::vector<fs::path>& paths,
        const ChunkConfig& config = {},
        size_t max_parallel = 4
    );
};
```

### Helper Functions

```cpp
// Convert format enum to string
std::string format_to_string(DataFormat format);

// Detect format from file extension
DataFormat detect_from_extension(const fs::path& path);

// Sanitize text content
std::string sanitize_text(std::string_view text);

// Extract numerical features
std::vector<float> extract_numerical_features(std::string_view text);

// Chunk text based on strategy
std::vector<DataChunk> chunk_text(
    std::string_view text,
    const ChunkConfig& config,
    const std::unordered_map<std::string, std::string>& metadata = {}
);
```

---

## Next Steps

- **See Also**:
  - [06_DATA_FORMATS.md](06_DATA_FORMATS.md) - Complete format specifications
  - [05_ARCHITECTURE.md](05_ARCHITECTURE.md) - System design details
  - [20_API_REFERENCE.md](20_API_REFERENCE.md) - Full API documentation
  - [04_USER_GUIDE.md](04_USER_GUIDE.md) - User guide and tutorials
  - [LOGGING.md](LOGGING.md) - Comprehensive logging system documentation
  - [REAL_WORLD_APPLICATIONS.md](REAL_WORLD_APPLICATIONS.md) - Production use cases and examples

- **Recently Implemented** (v2.0):
  - ✅ Full Apache Arrow integration for Parquet
  - ✅ XML adapter with read/write support
  - ✅ SQLite adapter with read/write support
  - ✅ pgvector adapter for distributed storage
  - ✅ PDF write support (basic PDF 1.4)
  - ✅ Comprehensive logging system with anomaly detection
  - ✅ Security hardening (SQL injection protection, input sanitization)
  - ✅ Bidirectional read/write for all adapters

- **Future Enhancements**:
  - HTML/web scraping adapter
  - Semantic chunking strategy with embeddings
  - Sentence-based chunking with NLP
  - Automatic language detection
  - OCR for scanned PDFs
  - Word document (.docx) adapter
  - PowerPoint (.pptx) adapter
  - Video/audio metadata extraction
  - Advanced PDF features with libharu integration

---

**Version**: 3.0.0  
**Last Updated**: 2026-01-04  
**Status**: Enterprise-Grade
