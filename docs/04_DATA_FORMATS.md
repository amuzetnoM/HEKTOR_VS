# Vector Studio - Supported Data Types and Formats

**Comprehensive guide to data formats, types, and structures supported by Vector Studio**

---

## Table of Contents

1. [Overview](#overview)
2. [Supported Data Formats](#supported-data-formats)
3. [Format Specifications](#format-specifications)
4. [Data Type Requirements](#data-type-requirements)
5. [Metadata Structure](#metadata-structure)
6. [Chunking Strategies](#chunking-strategies)
7. [Numerical Features](#numerical-features)
8. [Data Preparation Guidelines](#data-preparation-guidelines)
9. [Format Examples](#format-examples)

---

## Overview

Vector Studio's data ingestion system is designed to accept a wide variety of data formats and automatically normalize them into a unified structure suitable for vector embedding and semantic search. This document provides complete specifications for all supported formats to help you prepare your data for optimal processing.

### Design Principles

- **Universal Compatibility**: Support common data formats without custom preprocessing
- **Automatic Detection**: Format detection based on file extension and content analysis
- **Lossless Metadata**: Preserve all structural metadata during ingestion
- **Flexible Chunking**: Multiple strategies to segment data for embeddings
- **Type Safety**: Strong validation and sanitization of all inputs

---

## Supported Data Formats

### Text Formats

| Format | Extension(s) | Read | Write | Description |
|--------|-------------|------|-------|-------------|
| **Plain Text** | `.txt` | ✅ | ✅ | UTF-8 encoded text, automatic encoding detection |
| **Markdown** | `.md` | ✅ | ✅ | GitHub-flavored markdown with header hierarchy |
| **JSON** | `.json`, `.jsonl` | ✅ | ✅ | Single objects, arrays, or JSON Lines format |
| **CSV** | `.csv`, `.tsv` | ✅ | ✅ | Comma or tab-separated values with headers |

### Document Formats

| Format | Extension(s) | Read | Write | Description |
|--------|-------------|------|-------|-------------|
| **PDF** | `.pdf` | ✅ | ❌ | Text extraction, table detection, metadata parsing |
| **Excel** | `.xlsx`, `.xls` | ✅ | ✅ | Multi-sheet workbooks with formula support |

### Structured Data

| Format | Extension(s) | Read | Write | Description |
|--------|-------------|------|-------|-------------|
| **XML** | `.xml` | 🚧 | ❌ | XML documents (planned) |
| **Parquet** | `.parquet` | 🚧 | ❌ | Apache Parquet columnar format (planned) |
| **SQL** | `.sql`, `.db` | 🚧 | ❌ | SQLite databases (planned) |

### Legend
- ✅ Fully implemented
- 🚧 Planned for future release
- ❌ Not supported

---

## Format Specifications

### 1. Plain Text (`.txt`)

**Purpose**: General-purpose text documents

**Requirements**:
- Encoding: UTF-8 (preferred), or auto-detected from UTF-16, Latin-1, etc.
- Line endings: Any (CRLF, LF, CR)
- Maximum file size: Limited by available memory (recommend < 100MB per file)

**Features**:
- Automatic encoding detection and conversion
- Markdown detection (if content has markdown syntax)
- Code block detection
- Whitespace normalization (configurable)

**Example**:
```text
This is a plain text document.
It can contain multiple paragraphs.

Each paragraph is separated by blank lines.
```

---

### 2. Markdown (`.md`)

**Purpose**: Structured documentation with formatting

**Requirements**:
- Valid GitHub-flavored markdown syntax
- UTF-8 encoding
- Headers must use `#` syntax (not underline style)

**Features**:
- Header hierarchy extraction (`#`, `##`, `###`, etc.)
- Code block preservation with language tags
- Link extraction and metadata
- Table parsing
- List structure preservation

**Metadata Extracted**:
- `title`: First H1 header
- `headers`: All header levels and content
- `links`: URLs and references
- `code_blocks`: Language and content

**Example**:
```markdown
# Document Title

## Introduction

This is a paragraph with **bold** and *italic* text.

### Subsection

- List item 1
- List item 2

\`\`\`python
def hello():
    print("Hello, World!")
\`\`\`
```

---

### 3. JSON (`.json`, `.jsonl`)

**Purpose**: Structured data from APIs, exports, and configurations

**Supported Structures**:
1. **Single Object**: One JSON object per file
2. **Array**: Array of objects `[{...}, {...}]`
3. **JSON Lines (JSONL)**: One object per line, newline-separated

**Requirements**:
- Valid JSON syntax (RFC 8259 compliant)
- UTF-8 encoding
- Maximum nesting depth: 50 levels
- Maximum file size: 500MB

**Features**:
- Automatic structure detection
- Nested object flattening
- Array element extraction
- Type-aware parsing (strings, numbers, booleans, nulls)
- Metadata extraction from known fields

**Metadata Fields** (automatically detected):
- `id`, `_id`, `identifier` → stored as `id`
- `title`, `name` → stored as `title`
- `date`, `timestamp`, `created_at`, `updated_at` → stored as `date`
- `source`, `url`, `link` → stored as `source`
- `type`, `category`, `tags` → stored as metadata

**Example - Single Object**:
```json
{
  "id": "doc123",
  "title": "Market Analysis",
  "content": "Gold prices continue to rise...",
  "date": "2025-12-01",
  "tags": ["finance", "gold", "markets"]
}
```

**Example - JSON Lines**:
```jsonl
{"id": 1, "text": "First document", "category": "A"}
{"id": 2, "text": "Second document", "category": "B"}
{"id": 3, "text": "Third document", "category": "A"}
```

---

### 4. CSV (`.csv`, `.tsv`)

**Purpose**: Tabular data, spreadsheet exports, time series data

**Requirements**:
- Valid RFC 4180 CSV format
- UTF-8 encoding (or auto-detected)
- Delimiter: comma (`,`), tab (`\t`), or configurable
- Quote character: double quote (`"`) or configurable

**Features**:
- Header row detection and extraction
- Custom column name specification
- Empty row skipping
- Automatic numerical feature extraction
- Per-row or column-based chunking
- Type inference for each column

**Configuration Options**:
```cpp
CSVConfig config;
config.delimiter = ',';           // Field separator
config.quote = '"';                // Quote character
config.has_header = true;          // First row contains headers
config.skip_empty_rows = true;     // Ignore blank rows
config.column_names = {...};       // Override header names
```

**Chunking Strategies**:
- **Row-based**: Each row becomes a chunk (default)
- **Column-based**: Each column becomes a chunk
- **Block-based**: Groups of N rows per chunk

**Metadata Per Chunk**:
- `row_number`: Original row position
- `column_*`: Each column value as metadata field
- Numerical columns → stored in `numerical_features` array

**Example**:
```csv
date,symbol,price,volume,change
2025-12-01,GOLD,2050.50,125000,+1.2%
2025-12-02,GOLD,2055.75,130000,+0.26%
2025-12-03,GOLD,2048.25,118000,-0.36%
```

For this CSV:
- Each row becomes one `DataChunk`
- Column headers stored in metadata
- `price`, `volume` extracted as numerical features
- Text content combines all fields for embedding

---

### 5. PDF (`.pdf`)

**Purpose**: Academic papers, reports, documentation, books

**Requirements**:
- Valid PDF 1.0-2.0 format
- Text-based (not scanned images without OCR)
- Not password-protected
- Maximum file size: 100MB

**Features**:
- Text extraction (using Poppler library)
- Page-by-page processing
- Table detection and extraction
- Metadata parsing (title, author, dates, keywords)
- Table of Contents (TOC) extraction
- Layout preservation (optional)
- Page range selection

**Configuration Options**:
```cpp
PDFConfig config;
config.extract_tables = true;      // Detect and extract tables
config.extract_images = false;     // Extract embedded images
config.preserve_layout = true;     // Keep spatial layout
config.start_page = 0;             // First page (0-indexed)
config.end_page = -1;              // -1 = all pages
config.extract_toc = true;         // Extract table of contents
```

**Metadata Extracted**:
- `title`: Document title from PDF metadata
- `author`: Author names
- `creation_date`: Creation timestamp
- `modification_date`: Last modified timestamp
- `keywords`: Extracted keywords
- `page_count`: Total number of pages
- `page_number`: Page number for each chunk

**Chunking**: Each page typically becomes one or more chunks, depending on page length and chunking strategy.

---

### 6. Excel (`.xlsx`, `.xls`)

**Purpose**: Spreadsheets, financial data, structured datasets

**Requirements**:
- Valid Excel OpenXML format (`.xlsx`) or legacy format (`.xls`)
- Not password-protected
- Maximum file size: 50MB

**Features**:
- Multi-sheet reading and writing
- Header row detection
- Formula evaluation (optional, slower)
- Type-aware cell parsing (string, number, boolean, date)
- Chart metadata extraction (optional)
- Range selection

**Configuration Options**:
```cpp
ExcelConfig config;
config.sheet_names = {...};        // Specific sheets or empty for all
config.has_header = true;          // First row is header
config.skip_empty_rows = true;     // Ignore blank rows
config.evaluate_formulas = false;  // Evaluate formulas (slow)
config.extract_charts = false;     // Extract chart metadata
config.start_row = 0;              // Starting row
config.start_col = 0;              // Starting column
```

**Cell Types**:
```cpp
enum class CellType { 
    Empty,    // Blank cell
    String,   // Text content
    Number,   // Floating point value
    Boolean,  // True/false
    Error     // Formula error
};
```

**Metadata Per Chunk**:
- `sheet_name`: Name of the worksheet
- `row_number`: Row position within sheet
- `column_*`: Each column value
- Numerical cells → stored in `numerical_features`

---

## Data Type Requirements

### Normalized Data Structure

All formats are converted to this unified structure:

```cpp
struct DataChunk {
    std::string content;                                        // Main text content for embedding
    std::unordered_map<std::string, std::string> metadata;     // Key-value metadata
    std::optional<std::string> title;                          // Optional title
    std::optional<std::string> date;                           // Optional date/timestamp
    std::optional<std::string> source;                         // Optional source identifier
    std::vector<float> numerical_features;                     // Extracted numbers
    size_t chunk_index;                                        // Position in document (0-based)
    size_t total_chunks;                                       // Total chunks in document
};

struct NormalizedData {
    std::vector<DataChunk> chunks;                             // All chunks from document
    DataFormat format;                                         // Original format
    std::string source_path;                                   // File path or identifier
    std::unordered_map<std::string, std::string> global_metadata;  // Document-level metadata
    float confidence;                                          // Format detection confidence (0-1)
    bool sanitized;                                            // Has been sanitized
    std::vector<std::string> warnings;                         // Processing warnings
};
```

### Field Constraints

| Field | Type | Max Length | Required | Description |
|-------|------|-----------|----------|-------------|
| `content` | string | 10,000 chars | ✅ | Main text for embedding |
| `metadata` | map | 50 keys | ❌ | Arbitrary key-value pairs |
| `title` | string | 500 chars | ❌ | Document/chunk title |
| `date` | string (ISO 8601) | 32 chars | ❌ | Timestamp in ISO format |
| `source` | string | 1000 chars | ❌ | Source URL or path |
| `numerical_features` | float array | 100 values | ❌ | Extracted numbers |

---

## Metadata Structure

### Standard Metadata Keys

Use these standardized keys for consistency:

| Key | Type | Description | Example |
|-----|------|-------------|---------|
| `id` | string | Unique identifier | `"doc_12345"` |
| `title` | string | Document title | `"Market Analysis Report"` |
| `date` | string | ISO 8601 timestamp | `"2025-12-01T10:30:00Z"` |
| `source` | string | Origin URL or path | `"https://example.com/doc"` |
| `author` | string | Creator name | `"John Doe"` |
| `category` | string | Classification | `"Finance"` |
| `tags` | string (comma-separated) | Keywords | `"gold,analysis,2025"` |
| `language` | string | ISO 639-1 code | `"en"`, `"es"`, `"fr"` |
| `type` | string | Content type | `"Journal"`, `"Report"`, `"Article"` |
| `version` | string | Document version | `"1.2.0"` |

### Custom Metadata

You can add any custom metadata fields. Guidelines:
- Use lowercase keys with underscores: `my_custom_field`
- Keep keys short and descriptive
- Avoid very long values (prefer < 1000 characters)
- Use consistent types across documents

---

## Chunking Strategies

Chunking divides large documents into smaller segments suitable for embedding.

### Strategy Comparison

| Strategy | Best For | Pros | Cons |
|----------|----------|------|------|
| **Fixed** | Uniform processing | Predictable size, fast | May split mid-sentence |
| **Paragraph** | Documents | Preserves semantic units | Variable size |
| **Sentence** | Q&A, chatbots | Complete thoughts | Small chunks, more processing |
| **Semantic** | High-quality search | Meaningful boundaries | Requires embeddings, slower |
| **Sliding** | Context overlap | Prevents information loss | Redundancy, more chunks |
| **Document** | Small files | One chunk per file | Large chunks, memory intensive |

### Configuration

```cpp
ChunkConfig config;
config.strategy = ChunkStrategy::Paragraph;  // Choose strategy
config.max_chunk_size = 1000;                // Max characters per chunk
config.overlap = 100;                        // Overlap for sliding window
config.preserve_structure = true;            // Keep formatting/headers
```

### Strategy Details

#### 1. Fixed Size
- Splits text every `max_chunk_size` characters
- Optional overlap for context continuity
- Fast and predictable

**Use When**: Processing large volumes uniformly, performance is critical

**Example**:
```
Input: "The quick brown fox jumps over the lazy dog. The dog was sleeping."
max_chunk_size: 30
overlap: 5

Chunk 1: "The quick brown fox jumps o"
Chunk 2: "ps over the lazy dog. The d"
Chunk 3: "The dog was sleeping."
```

#### 2. Paragraph
- Splits on double newlines (`\n\n`)
- Preserves natural document structure
- Variable chunk sizes

**Use When**: Processing articles, documentation, books

**Example**:
```
Input:
"First paragraph here.

Second paragraph with more content.

Third paragraph."

→ 3 chunks, one per paragraph
```

#### 3. Sentence
- Splits on sentence boundaries (`. `, `! `, `? `)
- Groups sentences until `max_chunk_size` reached
- More granular than paragraph

**Use When**: Q&A systems, precise retrieval, chatbots

#### 4. Semantic
- Analyzes semantic similarity between sentences
- Groups similar content together
- Requires embedding model

**Use When**: Highest quality results needed, processing time acceptable

#### 5. Sliding Window
- Fixed size with overlap
- Each chunk shares content with neighbors
- Prevents information loss at boundaries

**Use When**: Context is critical, redundancy acceptable

**Example**:
```
Input: "ABCDEFGHIJ"
chunk_size: 4
overlap: 2

Chunk 1: "ABCD"
Chunk 2: "CDEF"  (overlap: CD)
Chunk 3: "EFGH"  (overlap: EF)
Chunk 4: "GHIJ"  (overlap: GH)
```

#### 6. Document
- Entire document is one chunk
- No splitting
- Suitable for small files only

**Use When**: Files are already small (< 1000 chars), don't want splitting

---

## Numerical Features

Vector Studio automatically extracts numerical values from text for enhanced search and analysis.

### Extraction Rules

1. **Integers**: `123`, `-456`, `+789`
2. **Decimals**: `3.14`, `-2.5`, `+0.001`
3. **Scientific Notation**: `1.5e10`, `3.2E-5`
4. **Percentages**: `12.5%` → stored as `12.5`
5. **Currency**: `$1,234.56` → stored as `1234.56`
6. **Dates**: `2025-12-01` → stored as Unix timestamp

### Storage

- Stored in `DataChunk.numerical_features` as `std::vector<float>`
- Order preserved from left-to-right appearance in text
- Maximum 100 features per chunk (additional ones ignored with warning)

### Use Cases

- Filtering by numerical ranges
- Similarity based on numerical values
- Time-based queries
- Price/quantity comparisons

---

## Data Preparation Guidelines

### General Best Practices

1. **Encoding**: Always use UTF-8 when possible
2. **File Size**: Keep individual files under 100MB
3. **Naming**: Use descriptive filenames with proper extensions
4. **Structure**: Maintain consistent structure within file type
5. **Metadata**: Include metadata fields when available
6. **Quality**: Remove or fix corrupted/malformed data

### Format-Specific Tips

#### CSV Files
- ✅ Include header row with descriptive column names
- ✅ Use consistent delimiters throughout file
- ✅ Quote fields containing delimiter or newlines
- ✅ Use ISO 8601 for dates: `YYYY-MM-DD`
- ❌ Don't mix data types within a column
- ❌ Avoid very wide tables (> 100 columns)

#### JSON Files
- ✅ Use consistent field names across objects
- ✅ Keep nesting shallow (< 5 levels preferred)
- ✅ Use JSON Lines for large datasets
- ✅ Include `id` field for tracking
- ❌ Don't use extremely large arrays (> 10,000 items)
- ❌ Avoid binary data (use base64 if necessary)

#### PDF Files
- ✅ Use text-based PDFs (not scanned images)
- ✅ Include document metadata
- ✅ Use standard fonts
- ✅ Keep file size reasonable (< 50MB)
- ❌ Don't password-protect files for ingestion
- ❌ Avoid heavily compressed or damaged PDFs

#### Excel Files
- ✅ Use first row for headers
- ✅ Keep related data on same sheet
- ✅ Name sheets descriptively
- ✅ Remove hidden rows/columns
- ❌ Don't rely on cell formatting for meaning
- ❌ Avoid merged cells
- ❌ Don't use very complex formulas

#### Text Files
- ✅ Use UTF-8 encoding
- ✅ Use consistent line endings
- ✅ Structure content with blank lines
- ✅ Keep lines reasonably short (< 120 chars)
- ❌ Don't use control characters
- ❌ Avoid tabs for formatting (use spaces)

---

## Format Examples

### Complete Example: Financial Data

#### Input CSV (`gold_prices.csv`)
```csv
date,price_usd,volume,change_pct,notes
2025-12-01,2050.50,125000,1.2,Strong opening
2025-12-02,2055.75,130000,0.26,Continued momentum
2025-12-03,2048.25,118000,-0.36,Slight pullback
```

#### Resulting NormalizedData
```cpp
NormalizedData {
    chunks: [
        DataChunk {
            content: "date: 2025-12-01, price_usd: 2050.50, volume: 125000, change_pct: 1.2, notes: Strong opening",
            metadata: {
                {"date", "2025-12-01"},
                {"price_usd", "2050.50"},
                {"volume", "125000"},
                {"change_pct", "1.2"},
                {"notes", "Strong opening"},
                {"row_number", "1"}
            },
            numerical_features: {2050.50, 125000.0, 1.2},
            chunk_index: 0,
            total_chunks: 3
        },
        // ... chunks for other rows
    ],
    format: DataFormat::CSV,
    source_path: "gold_prices.csv",
    global_metadata: {
        {"columns", "date,price_usd,volume,change_pct,notes"},
        {"row_count", "3"}
    },
    confidence: 1.0,
    sanitized: true,
    warnings: {}
}
```

### Complete Example: JSON Document

#### Input JSON (`article.json`)
```json
{
  "id": "art_2025_001",
  "title": "Gold Market Analysis Q4 2025",
  "author": "Jane Smith",
  "date": "2025-12-01T10:00:00Z",
  "tags": ["gold", "precious metals", "market analysis"],
  "content": "The gold market showed strong performance in Q4 2025, with prices reaching new highs. Several factors contributed to this rally, including global economic uncertainty and increased central bank purchases.",
  "metadata": {
    "source": "Financial Times",
    "category": "Markets",
    "word_count": 42
  }
}
```

#### Resulting NormalizedData
```cpp
NormalizedData {
    chunks: [
        DataChunk {
            content: "The gold market showed strong performance in Q4 2025, with prices reaching new highs. Several factors contributed to this rally, including global economic uncertainty and increased central bank purchases.",
            metadata: {
                {"id", "art_2025_001"},
                {"author", "Jane Smith"},
                {"tags", "gold,precious metals,market analysis"},
                {"source", "Financial Times"},
                {"category", "Markets"},
                {"word_count", "42"}
            },
            title: "Gold Market Analysis Q4 2025",
            date: "2025-12-01T10:00:00Z",
            source: "Financial Times",
            numerical_features: {2025.0, 4.0, 2025.0, 42.0},
            chunk_index: 0,
            total_chunks: 1
        }
    ],
    format: DataFormat::JSON,
    source_path: "article.json",
    global_metadata: {},
    confidence: 1.0,
    sanitized: true,
    warnings: {}
}
```

---

## Summary

Vector Studio supports a comprehensive range of data formats designed for semantic search and AI applications:

- **Text Formats**: Plain text, Markdown, JSON, CSV
- **Documents**: PDF, Excel (with multi-sheet support)
- **Future**: XML, Parquet, SQL databases

All formats are automatically:
- Detected and validated
- Parsed into unified `NormalizedData` structure
- Sanitized for safe processing
- Chunked according to chosen strategy
- Enhanced with extracted metadata and numerical features

For integration with external systems, ensure your data conforms to the specifications in this document for optimal results.

---

**Next Steps**:
- See [05_DATA_INGESTION.md](05_DATA_INGESTION.md) for usage examples and API reference
- See [06_ARCHITECTURE.md](06_ARCHITECTURE.md) for technical implementation details
- See [07_API_REFERENCE.md](07_API_REFERENCE.md) for complete API documentation
