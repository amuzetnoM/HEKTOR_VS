# Vector Studio v2.0 - Quick Usage Examples

## Auto-Parse Any Format

```cpp
auto data = manager.auto_parse("file.xml");
```

## Write to Any Format

```cpp
adapter.write(data, "output.db");
```

## Specific Adapters

### XML
```cpp
XMLAdapter xml;
auto data = xml.parse("input.xml");
xml.write(data, "output.xml");
```

### PDF
```cpp
PDFAdapter pdf;
auto data = pdf.parse("input.pdf");
pdf.write(data, "report.pdf");
```

### SQLite
```cpp
SQLiteAdapter db;
auto data = db.parse("input.db");
db.write(data, "output.db");
```

### Parquet
```cpp
ParquetAdapter parquet;
auto data = parquet.parse("data.parquet");
parquet.write(data, "output.parquet");
```

### pgvector
```cpp
PgvectorAdapter pg("host=localhost dbname=vectors");
pg.connect();
auto results = pg.query_similar(vector, 10);
```

## Logging

```cpp
LOG_INFO("Processing started");
LOG_ERROR("Failed to parse file");
LOG_ANOMALY(PARSE_ERROR, "Invalid structure");
```

## Configuration

```cpp
// SQLite with custom tables
SQLiteAdapter db({.tables = {"users", "orders"}});

// pgvector with connection
PgvectorAdapter pg({
    .host = "localhost",
    .database = "vectors",
    .table = "embeddings"
});
```

## Version Check

```cpp
if (VDB_VERSION_MAJOR >= 2) {
    // Use v2.0 features
}
```
