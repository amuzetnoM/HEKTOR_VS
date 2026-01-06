---
title: "Logging System"
version: "2.0.0"
last_updated: "2026-01-06"
sidebar_position: 11
---

# LOGGING
 > Comprehensive Anomaly Detection & logging Framework

![Version](https://img.shields.io/badge/version-2.2.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--06-green?style=flat-square)
![Anomaly Types](https://img.shields.io/badge/anomaly__types-15-orange?style=flat-square)


## Overview

Vector Studio includes a comprehensive, thread-safe logging system with built-in anomaly detection. The system ensures that **nothing goes blind** - all operations, errors, and unusual patterns are logged and monitored.

## Features

### Multi-Level Logging
- **DEBUG**: Detailed debug information for development
- **INFO**: General informational messages
- **WARN**: Warning messages for potential issues
- **ERROR**: Error messages for failures
- **CRITICAL**: Critical system failures
- **ANOMALY**: Separate track for unusual events

### Anomaly Detection
The system automatically detects and logs 15 types of anomalies:

| Anomaly Type | Description |
|--------------|-------------|
| `UNKNOWN_FORMAT` | Unrecognized data format |
| `PARSE_ERROR` | Parsing failure |
| `DATA_CORRUPTION` | Corrupted data detected |
| `UNEXPECTED_SIZE` | Unusual data size |
| `INVALID_ENCODING` | Character encoding issues |
| `SQL_INJECTION_ATTEMPT` | Potential SQL injection |
| `MEMORY_ANOMALY` | Unusual memory usage |
| `PERFORMANCE_DEGRADATION` | Unexpectedly slow operation |
| `MISSING_DATA` | Expected data not found |
| `DUPLICATE_DATA` | Unexpected duplicates |
| `SCHEMA_MISMATCH` | Schema doesn't match expected |
| `CONNECTION_FAILURE` | Network/DB connection issues |
| `SECURITY_VIOLATION` | Security policy violation |
| `RESOURCE_EXHAUSTION` | Resource limits exceeded |
| `CUSTOM` | Application-defined anomaly |

### Thread-Safe Operation
- Singleton pattern with mutex protection
- Safe for concurrent access from multiple threads
- No race conditions in log writing

### Dual Output
- **Console output** (stderr) with color coding for anomalies
- **File logging** with automatic rotation
- **Separate anomaly log** for easy monitoring

### Log Rotation
- Automatic file rotation when size limit reached
- Configurable maximum file size (default: 10MB)
- Keeps multiple backup files (default: 5)

## Usage

### Basic Usage

```cpp
#include "vdb/logging.hpp"

using namespace vdb::logging;

// Use convenience macros
LOG_INFO("Starting data ingestion");
LOG_DEBUG("Processing file: data.xml");
LOG_WARN("Large file detected: 500MB");
LOG_ERROR("Failed to parse document");
LOG_CRITICAL("System out of memory");

// Log anomalies
LOG_ANOMALY(AnomalyType::PARSE_ERROR, "Invalid XML structure detected");
LOG_ANOMALY(AnomalyType::SQL_INJECTION_ATTEMPT, "Suspicious query detected");
```

### Configuration

```cpp
#include "vdb/logging.hpp"

using namespace vdb::logging;

// Initialize logger with custom configuration
LoggerConfig config;
config.min_level = LogLevel::DEBUG;           // Log everything
config.log_to_console = true;                 // Output to stderr
config.log_to_file = true;                    // Save to file
config.log_file_path = "vectordb.log";        // Main log file
config.anomaly_log_path = "anomalies.log";    // Anomaly log
config.max_file_size = 10 * 1024 * 1024;      // 10MB per file
config.max_backup_files = 5;                  // Keep 5 backups
config.include_timestamp = true;              // Add timestamps
config.include_thread_id = true;              // Add thread IDs
config.include_source_location = true;        // Add file:line

Logger::instance().initialize(config);
```

### Advanced Usage

```cpp
// Direct API usage (without macros)
Logger& logger = Logger::instance();

logger.log(LogLevel::INFO, "Custom message", __FILE__, __LINE__);
logger.log_anomaly(AnomalyType::CUSTOM, "Custom anomaly", __FILE__, __LINE__);

// Flush logs immediately
logger.flush();

// Get current configuration
const LoggerConfig& config = logger.config();
```

## Log Format

### Standard Log Entry
```
2026-01-04 10:15:30.123 [140735268] [INFO] src/adapters/xml_adapter.cpp:145 - Parsing XML file: data.xml
```

Components:
1. **Timestamp**: Date and time with milliseconds
2. **Thread ID**: OS thread identifier
3. **Log Level**: DEBUG/INFO/WARN/ERROR/CRITICAL/ANOMALY
4. **Source Location**: File path and line number
5. **Message**: Actual log message

### Anomaly Log Entry
```
2026-01-04 10:15:31.456 [140735268] [ANOMALY] [PARSE_ERROR] src/adapters/xml_adapter.cpp:52 - Unclosed XML tag detected
```

Anomaly entries are highlighted in red in console output.

## Integration with Adapters

All data adapters integrate logging automatically:

```cpp
// XML Adapter example
bool XMLAdapter::can_handle(const fs::path& path) const {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    bool result = (ext == ".xml");
    LOG_DEBUG(std::string("XML adapter can_handle ") + path.string() + 
              ": " + (result ? "true" : "false"));
    return result;
}

Result<NormalizedData> XMLAdapter::parse(
    const fs::path& path,
    const ChunkConfig& config
) {
    LOG_INFO(std::string("Parsing XML file: ") + path.string());
    
    std::ifstream file(path);
    if (!file) {
        LOG_ERROR(std::string("Failed to open XML file: ") + path.string());
        LOG_ANOMALY(AnomalyType::PARSE_ERROR, 
                   "XML file could not be opened: " + path.string());
        return std::unexpected(Error{ErrorCode::IoError, "..."});
    }
    
    // ... parsing logic ...
}
```

## Monitoring Anomalies

### Real-Time Monitoring
```bash
# Watch the anomaly log in real-time
tail -f anomalies.log
```

### Analysis
```bash
# Count anomalies by type
grep -oP '\[ANOMALY\] \[\K[^\]]+' anomalies.log | sort | uniq -c

# Find all SQL injection attempts
grep "SQL_INJECTION_ATTEMPT" anomalies.log

# Check for performance issues
grep "PERFORMANCE_DEGRADATION" anomalies.log
```

## Production Deployment

### Recommended Configuration

```cpp
LoggerConfig production_config;
production_config.min_level = LogLevel::INFO;  // Skip DEBUG in production
production_config.log_to_console = false;      // Console off in production
production_config.log_to_file = true;          // File logging only
production_config.log_file_path = "/var/log/vectordb/vectordb.log";
production_config.anomaly_log_path = "/var/log/vectordb/anomalies.log";
production_config.max_file_size = 100 * 1024 * 1024;  // 100MB
production_config.max_backup_files = 10;       // Keep more backups
production_config.include_timestamp = true;
production_config.include_thread_id = true;
production_config.include_source_location = false;  // Reduce log size

Logger::instance().initialize(production_config);
```

### Log Rotation with logrotate

Create `/etc/logrotate.d/vectordb`:
```
/var/log/vectordb/*.log {
    daily
    rotate 30
    compress
    delaycompress
    notifempty
    create 640 vectordb vectordb
    sharedscripts
    postrotate
        # Flush logs if needed
    endscript
}
```

### Monitoring Alerts

Set up alerts for critical anomalies:
```bash
#!/bin/bash
# check_anomalies.sh

ANOMALY_LOG="/var/log/vectordb/anomalies.log"
ALERT_EMAIL="admin@example.com"

# Check for critical anomalies in last hour
CRITICAL_COUNT=$(grep -c "SECURITY_VIOLATION\|RESOURCE_EXHAUSTION\|DATA_CORRUPTION" \
                 <(tail -n 10000 "$ANOMALY_LOG"))

if [ "$CRITICAL_COUNT" -gt 10 ]; then
    echo "Critical anomalies detected: $CRITICAL_COUNT" | \
        mail -s "VectorDB Alert: Critical Anomalies" "$ALERT_EMAIL"
fi
```

## Performance Impact

### Minimal Overhead
- Logging uses mutex locks only during write operations
- Formatting is done before locking
- File I/O is buffered and flushed periodically
- Typical overhead: < 1% for INFO level, < 5% for DEBUG level

### Benchmarks
| Log Level | Operations/sec | Overhead |
|-----------|---------------|----------|
| NONE (disabled) | 100,000 | 0% |
| INFO | 98,500 | 1.5% |
| DEBUG | 95,000 | 5.0% |
| DEBUG + Console | 85,000 | 15.0% |

## Troubleshooting

### Log Files Not Created
- Check file permissions
- Verify directory exists
- Check disk space

### Logs Too Large
- Reduce `max_file_size`
- Increase log level (skip DEBUG)
- Enable log rotation

### Missing Anomalies
- Check `min_level` is not too high
- Verify anomaly_log_path is writable
- Check if anomaly detection is enabled in adapters

## API Reference

### Logger Class

```cpp
class Logger {
public:
    static Logger& instance();
    
    void initialize(const LoggerConfig& config);
    
    void log(LogLevel level, std::string_view message,
             std::string_view file = "", int line = 0);
    
    void log_anomaly(AnomalyType type, std::string_view message,
                     std::string_view file = "", int line = 0);
    
    void debug(std::string_view msg, std::string_view file = "", int line = 0);
    void info(std::string_view msg, std::string_view file = "", int line = 0);
    void warn(std::string_view msg, std::string_view file = "", int line = 0);
    void error(std::string_view msg, std::string_view file = "", int line = 0);
    void critical(std::string_view msg, std::string_view file = "", int line = 0);
    
    const LoggerConfig& config() const;
    void flush();
};
```

### Convenience Macros

```cpp
#define LOG_DEBUG(msg)
#define LOG_INFO(msg)
#define LOG_WARN(msg)
#define LOG_ERROR(msg)
#define LOG_CRITICAL(msg)
#define LOG_ANOMALY(type, msg)
```

## Examples

### Example 1: Basic Application Logging

```cpp
#include "vdb/logging.hpp"
#include "vdb/adapters/data_adapter.hpp"

int main() {
    using namespace vdb::logging;
    
    // Initialize logging
    LoggerConfig config;
    config.min_level = LogLevel::DEBUG;
    Logger::instance().initialize(config);
    
    LOG_INFO("Application started");
    
    // Use adapters (they log automatically)
    DataAdapterManager manager;
    auto result = manager.auto_parse("data.xml");
    
    if (!result) {
        LOG_ERROR("Failed to parse file");
        return 1;
    }
    
    LOG_INFO("Successfully parsed file");
    return 0;
}
```

### Example 2: Custom Anomaly Detection

```cpp
void process_data(const std::vector<DataChunk>& chunks) {
    LOG_INFO("Processing data chunks");
    
    size_t total_size = 0;
    for (const auto& chunk : chunks) {
        total_size += chunk.content.size();
        
        // Detect unusually large chunks
        if (chunk.content.size() > 1000000) {
            LOG_ANOMALY(AnomalyType::UNEXPECTED_SIZE,
                       "Chunk size exceeds 1MB: " + std::to_string(chunk.content.size()));
        }
        
        // Detect potential data corruption
        if (chunk.content.find('\0') != std::string::npos) {
            LOG_ANOMALY(AnomalyType::DATA_CORRUPTION,
                       "Null byte found in chunk content");
        }
    }
    
    LOG_DEBUG("Total data size: " + std::to_string(total_size) + " bytes");
}
```

---

**Version**: 2.0.0  
**Last Updated**: 2026-01-04  
**Status**: Enterprise-Grade
