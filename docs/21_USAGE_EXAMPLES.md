---
title: "Usage Examples"
description: "Practical code examples and common patterns"
version: "3.0.0"
last_updated: "2026-01-08"
sidebar_position: 21
category: "Tutorial"
---

![Version](https://img.shields.io/badge/version-3.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--08-green?style=flat-square)
![Examples](https://img.shields.io/badge/examples-practical-brightgreen?style=flat-square)

# Usage Examples

## Table of Contents

- [Auto-Parse Any Format](#auto-parse-any-format)
- [Write to Any Format](#write-to-any-format)
- [Specific Adapters](#specific-adapters)
  - [XML](#xml)
  - [PDF](#pdf)
  - [SQLite](#sqlite)
  - [Parquet](#parquet)
- [Logging](#logging)
- [Configuration](#configuration)
- [Version Check](#version-check)

---

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
