# Vector Studio v2.0.0

**Release Date**: January 4, 2026

## New Features

- **4 New Adapters**: XML, Parquet (Apache Arrow), SQLite, PostgreSQL pgvector
- **Bidirectional Support**: Read and write for all formats
- **Logging System**: Thread-safe with 15 anomaly types
- **Security**: SQL injection protection, input validation

## Breaking Changes

- `IDataAdapter` interface: Added `write()` and `supports_write()` methods
- Custom adapters must implement new interface methods

## Installation

```bash
# Optional dependencies
sudo apt-get install libsqlite3-dev libpq-dev libarrow-dev libparquet-dev
```

## Quick Start

```cpp
// Auto-detect and parse
auto data = manager.auto_parse("file.xml");

// Write to any format
adapter.write(data, "output.db");

// Enable logging
LOG_INFO("Operation complete");
```

## Documentation

- Updated all documentation to v2.0
- Added real-world application examples
- Comprehensive logging guide

## Performance

- P99 latency: 8-45ms
- Throughput: 5,500-22,000 QPS
- Parquet: Zero-copy with Arrow

## Links

- [Full Changelog](CHANGELOG.md)
- [Implementation Summary](IMPLEMENTATION_SUMMARY.md)
- [Real-World Applications](docs/REAL_WORLD_APPLICATIONS.md)
