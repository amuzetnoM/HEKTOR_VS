---
title: "HTTP Adapter"
description: "Generic HTTP adapter for fetching and parsing data from web APIs"
version: "4.0.0"
last_updated: "2026-01-22"
sidebar_position: 17
category: "Data Ingestion"
---

![Version](https://img.shields.io/badge/version-4.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--22-green?style=flat-square)
![Type](https://img.shields.io/badge/type-adapter-red?style=flat-square)

# HTTP Adapter

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
  - [C++ Usage](#c-usage)
  - [Python Usage](#python-usage)
- [Configuration](#configuration)
  - [HTTPConfig](#httpconfig)
  - [Custom Headers](#custom-headers)
  - [Timeout and Retry](#timeout-and-retry)
- [Format Detection](#format-detection)
- [Supported Formats](#supported-formats)
- [API Reference](#api-reference)
  - [HTTPAdapter Class](#httpadapter-class)
  - [Methods](#methods)
- [Response Metadata](#response-metadata)
- [Error Handling](#error-handling)
- [Advanced Usage](#advanced-usage)
  - [Custom Authentication](#custom-authentication)
  - [Rate Limiting](#rate-limiting)
  - [Response Caching](#response-caching)
- [Use Cases](#use-cases)

---

## Overview

The HTTP Adapter provides a generic way to fetch and parse data from any HTTP/HTTPS endpoint, with automatic format detection based on Content-Type headers.

The HTTP Adapter is part of the Universal Data Adapter System and implements the `IDataAdapter` interface. It enables:

- Fetching data from any HTTP/HTTPS URL
- Automatic format detection (JSON, XML, CSV, HTML, plain text)
- Integration with the existing `HttpClient` for retry, caching, and rate limiting
- Custom headers for authentication and API keys

## Quick Start

### C++ Usage

```cpp
#include "vdb/adapters/http_adapter.hpp"

using namespace vdb::adapters;

// Create adapter with optional configuration
HTTPConfig config;
config.default_headers["Authorization"] = "Bearer YOUR_API_KEY";
config.timeout = std::chrono::seconds(30);

HTTPAdapter adapter(config);

// Fetch and parse data from a URL
auto result = adapter.parse("https://api.example.com/data.json");
if (result) {
    for (const auto& chunk : result->chunks) {
        std::cout << chunk.content << std::endl;
    }
}
```

### Using DataAdapterManager

```cpp
#include "vdb/adapters/data_adapter.hpp"

DataAdapterManager manager;

// Auto-detects HTTP URL and uses HTTPAdapter
auto result = manager.auto_parse("https://api.example.com/data.json");
```

## Configuration

### HTTPConfig

```cpp
struct HTTPConfig {
    std::string base_url;                               // Optional base URL for relative paths
    std::map<std::string, std::string> default_headers; // Default headers for all requests
    std::chrono::seconds timeout{30};                   // Request timeout
    bool verify_ssl{true};                              // SSL certificate verification
    bool follow_redirects{true};                        // Follow HTTP redirects
    int max_redirects{5};                              // Maximum redirect count
};
```

### Example: API with Authentication

```cpp
HTTPConfig config;
config.default_headers["Authorization"] = "Bearer sk-xxx";
config.default_headers["Accept"] = "application/json";

HTTPAdapter adapter(config);
auto result = adapter.parse("https://api.openai.com/v1/models");
```

### Example: Base URL with Relative Paths

```cpp
HTTPConfig config;
config.base_url = "https://api.example.com/v2";
config.default_headers["X-API-Key"] = "your-key";

HTTPAdapter adapter(config);

// Fetches https://api.example.com/v2/users
auto users = adapter.parse("/users");

// Fetches https://api.example.com/v2/data
auto data = adapter.parse("/data");
```

## Format Detection

The adapter automatically detects the response format using:

1. **Content-Type Header** (primary method)
   - `application/json` → JSON/API_JSON
   - `application/xml`, `text/xml` → XML
   - `text/csv` → CSV
   - `text/html` → HTML
   - `text/plain` → PlainText

2. **URL Extension** (fallback)
   - `.json` → JSON
   - `.xml` → XML
   - `.csv` → CSV

3. **Content Sniffing** (last resort)
   - Starts with `{` or `[` → JSON
   - Starts with `<` → XML or HTML
   - Comma-separated patterns → CSV

## Supported Formats

| Format | Content-Type | Auto-Parsed |
|--------|-------------|-------------|
| JSON | `application/json`, `text/json` | ✅ Delegated to JSONAdapter |
| XML | `application/xml`, `text/xml` | ✅ Delegated to XMLAdapter |
| CSV | `text/csv` | ✅ Delegated to CSVAdapter |
| HTML | `text/html` | ⚠️ Parsed as plain text |
| Plain Text | `text/plain` | ✅ Chunked text |

## API Reference

### HTTPAdapter Class

```cpp
class HTTPAdapter : public IDataAdapter {
public:
    explicit HTTPAdapter(HTTPConfig config = {});
    
    // IDataAdapter interface
    bool can_handle(const fs::path& path) const override;
    bool can_handle(std::string_view content) const override;
    
    Result<NormalizedData> parse(const fs::path& path, const ChunkConfig& config = {}) override;
    Result<NormalizedData> parse_content(std::string_view content, const ChunkConfig& config = {},
                                         std::string_view source_hint = "") override;
    Result<void> sanitize(NormalizedData& data) override;
    
    std::string name() const override { return "HTTP"; }
    std::vector<DataFormat> supported_formats() const override;
    
    // HTTP-specific methods
    void set_header(const std::string& key, const std::string& value);
    void remove_header(const std::string& key);
    void set_base_url(const std::string& url);
    HttpClient& get_client();
};
```

### Methods

#### `parse(path, config)`

Fetches data from a URL and parses it based on the detected format.

**Parameters:**
- `path`: URL to fetch (e.g., `https://api.example.com/data.json`)
- `config`: Optional chunking configuration

**Returns:** `Result<NormalizedData>` with parsed data or error

#### `can_handle(path)`

Checks if this adapter can handle the given path (returns `true` for `http://` and `https://` URLs).

#### `set_header(key, value)`

Sets a default header for all requests.

```cpp
adapter.set_header("Authorization", "Bearer token");
adapter.set_header("User-Agent", "MyApp/1.0");
```

#### `get_client()`

Returns the underlying `HttpClient` for advanced configuration.

```cpp
auto& client = adapter.get_client();
client.set_rate_limit("api.example.com", RateLimitConfig{.requests_per_second = 5});
```

## Response Metadata

The adapter captures HTTP response metadata in `global_metadata`:

| Key | Description |
|-----|-------------|
| `http_status` | HTTP status code (e.g., "200") |
| `elapsed_ms` | Request duration in milliseconds |
| `header_*` | Response headers (e.g., `header_Content-Type`) |

## Error Handling

```cpp
auto result = adapter.parse("https://api.example.com/data");

if (!result) {
    const auto& error = result.error();
    
    if (error.code == ErrorCode::IoError) {
        // Network error, timeout, or HTTP error
        std::cerr << "HTTP Error: " << error.message << std::endl;
    }
}
```

Common errors:
- **IoError**: Network timeout, connection refused, HTTP 4xx/5xx
- **InvalidData**: Failed to parse response body

## Advanced Usage

### Rate Limiting

```cpp
HTTPAdapter adapter;
auto& client = adapter.get_client();

// Limit to 10 requests per second for specific endpoint
client.set_rate_limit("api.example.com", RateLimitConfig{
    .requests_per_second = 10,
    .requests_per_minute = 100
});
```

### Custom Retry Configuration

The underlying `HttpClient` handles retries automatically for transient errors (408, 429, 500, 502, 503, 504).

### Caching

Responses are cached by default. To clear the cache:

```cpp
adapter.get_client().clear_cache();
```

## Use Cases

### Fetching Financial Data

```cpp
HTTPConfig config;
config.default_headers["X-RapidAPI-Key"] = "your-key";

HTTPAdapter adapter(config);
auto result = adapter.parse("https://api.example.com/stock/AAPL/quote");
```

### Consuming REST APIs

```cpp
HTTPAdapter adapter;
adapter.set_header("Accept", "application/json");

auto users = adapter.parse("https://jsonplaceholder.typicode.com/users");
if (users) {
    for (const auto& chunk : users->chunks) {
        // Process user data
    }
}
```

### RSS/Atom Feeds

```cpp
HTTPAdapter adapter;
auto feed = adapter.parse("https://news.ycombinator.com/rss");
// Parsed as XML, chunks contain feed items
```

---

**See Also:**
- [Data Ingestion Guide](07_DATA_INGESTION.md)
- [Data Formats](06_DATA_FORMATS.md)
- [API Reference](20_API_REFERENCE.md)

---

**Last Updated:** 2026-01-07  
**Contributors:** Vector Studio Team
