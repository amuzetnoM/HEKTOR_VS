# Vector Studio Examples

This directory contains example data files demonstrating the various data formats supported by Vector Studio.

## Sample Files

### XML Documents
- `sample_data.xml` - Example XML document with book catalog data

### Usage

These example files can be used to test the data ingestion adapters:

```cpp
#include "vdb/adapters/data_adapter.hpp"

using namespace vdb::adapters;

// Auto-parse
auto data = manager.auto_parse("examples/sample_data.xml");

if (data) {
    std::cout << "Format: " << format_to_string(data->format) << "\n";
    std::cout << "Chunks: " << data->chunks.size() << "\n";
    
    for (const auto& chunk : data->chunks) {
        std::cout << "Content: " << chunk.content << "\n";
    }
}
```

## Creating Your Own Test Files

### XML Files
Create XML documents with structured data. The adapter will extract all text content and attributes.

### SQLite Databases
Create SQLite databases with the `.db` extension:

```sql
CREATE TABLE documents (
    id INTEGER PRIMARY KEY,
    title TEXT,
    content TEXT,
    author TEXT,
    date DATE
);

INSERT INTO documents VALUES (1, 'Sample', 'Content here', 'Author', '2025-01-01');
```

### pgvector Databases
Set up PostgreSQL with pgvector extension:

```sql
CREATE EXTENSION vector;

CREATE TABLE embeddings (
    id SERIAL PRIMARY KEY,
    content TEXT,
    embedding vector(384),
    author TEXT,
    date DATE
);

CREATE INDEX ON embeddings USING ivfflat (embedding vector_cosine_ops);
```

## Testing

Use these files to verify that the data ingestion pipeline works correctly before processing your production data.

