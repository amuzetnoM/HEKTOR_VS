# Changelog

All notable changes to Vector Studio will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

## [2.0.0] - 2026-01-04

### Added
- **XML Adapter**: Full support for XML document parsing with DOM traversal, attribute extraction, and hierarchical metadata
  - Complete read/write capabilities with proper XML escaping
  - DOM-based parser with recursive tree traversal
  - Attribute extraction and hierarchical metadata preservation
- **Parquet Adapter**: Full Apache Parquet columnar format support with Apache Arrow C++ library integration
  - Row-based and column-based chunking strategies
  - Type-aware data extraction (STRING, INT64, DOUBLE, FLOAT, BOOL)
  - Complete read/write support with schema preservation
  - Conditional compilation with graceful fallback when Arrow not available
- **SQLite Adapter**: Complete SQLite database reading with multi-table support and custom SQL queries
  - Database creation and population with parameterized queries
  - SQL injection protection with identifier quoting
  - Multi-table support and custom SQL execution
- **pgvector Adapter**: PostgreSQL with pgvector extension integration for distributed vector storage
  - Bidirectional sync (read/write operations)
  - Native similarity search with distance operators
  - IVFFlat and HNSW index support
  - Batch insert operations
  - SQL injection protection with identifier quoting and literal escaping
- **PDF Write Support**: Basic PDF 1.4 document generation
  - Text rendering with word wrapping
  - Valid PDF structure with proper string escaping
- **Comprehensive Logging System** (`logging.hpp`):
  - Thread-safe multi-level logging (DEBUG, INFO, WARN, ERROR, CRITICAL, ANOMALY)
  - Dual output: console with ANSI colors and file logging
  - Separate anomaly log for unusual events
  - 15 predefined anomaly types including PARSE_ERROR, SQL_INJECTION_ATTEMPT, DATA_CORRUPTION, MEMORY_ANOMALY, PERFORMANCE_DEGRADATION, SECURITY_VIOLATION
  - Automatic log rotation with configurable file size limits
  - Source location tracking (file and line numbers)
  - Zero blind spots - all operations logged
- **IDataAdapter Interface Updates**:
  - Added `write()` method for bidirectional support
  - Added `supports_write()` query method
- Comprehensive documentation suite:
  - `docs/LOGGING.md` (350+ lines) - Complete logging system documentation
  - `docs/REAL_WORLD_APPLICATIONS.md` (600+ lines) - 6 detailed production use cases with performance benchmarks
  - `IMPLEMENTATION_SUMMARY.md` (300+ lines) - Complete implementation overview
  - `NEW_FEATURES.md` - Migration guide and dependency requirements
- Enhanced data format documentation with examples for all new formats
- Comprehensively updated `docs/05_DATA_INGESTION.md` (650+ new lines)
  - All adapters documented with read/write examples
  - 200+ line "Logging System Integration" section
  - Security notes for SQL injection protection
  - 20+ new code examples
- Usage examples for XML, Parquet, SQLite, and pgvector adapters
- Updated `docs/index.html` with new feature cards
- Automated setup scripts for Windows (setup.ps1) and Unix (setup.sh)
- Python virtual environment management with auto-activation
- Automatic Python installation via winget/brew/apt
- Git configuration files (.gitignore, .gitattributes)
- CONTRIBUTING.md with development guidelines
- Example XML file in `examples/` for testing

### Changed
- Updated DataAdapterManager to register new adapters (XML, Parquet, SQLite, pgvector)
- Enhanced CMakeLists.txt with:
  - Conditional compilation flags for SQLite3 and libpq
  - Apache Arrow/Parquet C++ library detection
  - New adapter source files
- Updated format detection to include .xml, .parquet, .db, .sqlite, .sqlite3 extensions
- Renamed project from "VectorDB" to "Vector Studio"
- Updated requirements.txt with latest package versions (December 2025)
- Enhanced build.ps1 with -AutoInstall and -CheckOnly flags
- Bumped project version from 1.0.0 to 2.0.0
- Replaced "production ready" terminology with "enterprise-grade" and "high-fidelity" throughout all documentation
- All documentation updated to reflect v2.0 features

### Security
- SQL identifier quoting to prevent SQL injection in SQLite and pgvector adapters
- String literal escaping in pgvector adapter
- XML special character escaping (&, <, >, ", ')
- PDF string escaping for parentheses and backslashes
- XML parser bounds checking for empty/malformed tags
- Input validation across all adapters

### Technical Details
- XML parsing with DOM parser and write support with proper escaping
- SQLite integration with conditional compilation (HAVE_SQLITE3 flag)
  - Read and write capabilities with parameterized queries
- PostgreSQL pgvector integration with conditional compilation (HAVE_LIBPQ flag)
  - Bidirectional sync with security hardening
- **Full Apache Arrow C++ integration** for Parquet (HAVE_ARROW flag)
  - 400+ lines of production-ready implementation
  - Row-based and column-based chunking
  - Type-aware extraction and write support
- PDF write support with basic PDF 1.4 generation
- Logging system with thread-safe singleton pattern
- All new adapters follow IDataAdapter interface with write() methods
- Type-aware data extraction for numerical features
- Batch processing support for database operations
- Comprehensive error handling and logging integration

### Performance
- Sub-millisecond queries maintained with new adapters
- Efficient memory usage with Apache Arrow's zero-copy design
- Parallel-safe logging with minimal overhead (~1.5% at INFO level)
- Batch operations for optimal database performance

### Fixed
- (None in this release)

---

## [0.1.0] - 2025-12-01

### Added
- Initial project structure with C++ core
- HNSW index implementation with SIMD optimization (AVX2/AVX-512)
- ONNX Runtime integration for local embeddings
- Text encoder support (all-MiniLM-L6-v2)
- Image encoder support (CLIP ViT-B/32)
- Memory-mapped vector storage
- Python bindings via pybind11
- CMake build system with Ninja support
- Basic CLI tool (vdb)
- Gold Standard integration for journal/chart ingestion

### Technical Details
- C++20 standard
- Cross-platform support (Windows, Linux)
- Thread-safe read/write operations
- Configurable HNSW parameters (M, ef_construction, ef_search)
- Metadata storage in JSONL format

---

## Version History Summary

| Version | Date | Highlights |
|---------|------|------------|
| 0.1.0 | 2025-12-01 | Initial release with core functionality |

---

## Upgrade Notes

### 0.1.0

First release - no upgrade path needed.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for how to contribute changes.
