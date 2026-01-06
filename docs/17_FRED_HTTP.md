---
title: "FRED Adapter & HTTP Client"
version: "2.0.0"
last_updated: "2026-01-06"
sidebar_position: 17
---

# FRED Adapter & HTTP Client

![Version](https://img.shields.io/badge/version-2.2.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--06-green?style=flat-square)
![Status](https://img.shields.io/badge/status-production-success?style=flat-square)

**Version**: 2.0.0  
**Status**: Production Ready  
**Category**: Data Adapters & Utilities

---

## Table of Contents

1. [Overview](#overview)
2. [FRED Adapter](#fred-adapter)
3. [HTTP Client](#http-client)
4. [API Reference](#api-reference)
5. [Usage Examples](#usage-examples)
6. [Configuration](#configuration)
7. [Best Practices](#best-practices)

---

## Overview

This document covers two important components for external data integration:

1. **FRED Adapter**: Federal Reserve Economic Data API integration
2. **HTTP Client**: Production-ready HTTP client with retry, rate limiting, and caching

Both components work together to provide reliable data ingestion from web APIs and services.

---

## FRED Adapter

### Introduction

The FRED Adapter enables seamless integration with the **Federal Reserve Economic Data (FRED)** API, providing access to thousands of economic time series maintained by the Federal Reserve Bank of St. Louis.

**Features**:
- 📊 Access to 800,000+ economic data series
- 📈 Time series data with date range filtering
- 🔍 Series search functionality
- 📋 Popular series quick access
- 🔄 Automatic retry and rate limiting
- 📦 Built-in data normalization

### Quick Start

```cpp
#include "vdb/adapters/fred_adapter.hpp"

using namespace vdb::adapters;

// Configure FRED adapter
FREDConfig config;
config.api_key = "your_fred_api_key_here";  // Get from https://fred.stlouisfed.org/docs/api/api_key.html
config.base_url = "https://api.stlouisfed.org/fred";  // Default
config.timeout = std::chrono::seconds(30);

// Create adapter
FREDAdapter adapter(config);

// Fetch a series (e.g., GDP)
auto result = adapter.fetch_series("GDP", 
    "2020-01-01",  // Start date (optional)
    "2023-12-31"   // End date (optional)
);

if (result) {
    FREDSeries series = *result;
    std::cout << "Series: " << series.title << "\n";
    std::cout << "Units: " << series.units << "\n";
    std::cout << "Frequency: " << series.frequency << "\n";
    
    for (const auto& obs : series.observations) {
        std::cout << obs.date << ": " << obs.value << "\n";
    }
}
```

### Configuration

```cpp
struct FREDConfig {
    std::string api_key;                    // Required: FRED API key
    std::string base_url;                   // API endpoint (default: fred.stlouisfed.org)
    std::chrono::seconds timeout{30};       // Request timeout
    size_t max_observations{10000};         // Max observations per request
};
```

### FRED Series Structure

```cpp
struct FREDSeries {
    std::string id;                         // Series ID (e.g., "GDP")
    std::string title;                      // Human-readable title
    std::string units;                      // Units of measurement
    std::string frequency;                  // Daily, Monthly, Quarterly, Annual
    std::string seasonal_adjustment;        // Seasonally Adjusted, Not Seasonally Adjusted
    std::chrono::system_clock::time_point last_updated;
    
    struct Observation {
        std::string date;                   // YYYY-MM-DD format
        float value;                        // Numeric value
        std::optional<std::string> notes;   // Optional notes
    };
    std::vector<Observation> observations;
};
```

### API Methods

#### `fetch_series()`

```cpp
[[nodiscard]] Result<FREDSeries> fetch_series(
    const std::string& series_id,
    std::optional<std::string> start_date = std::nullopt,
    std::optional<std::string> end_date = std::nullopt
);
```

Fetch a specific economic data series.

**Parameters**:
- `series_id`: FRED series identifier (e.g., "GDP", "UNRATE", "DGS10")
- `start_date`: Optional start date (YYYY-MM-DD)
- `end_date`: Optional end date (YYYY-MM-DD)

**Returns**: `Result<FREDSeries>` with series data or error

---

#### `search_series()`

```cpp
[[nodiscard]] Result<std::vector<std::string>> search_series(
    const std::string& query,
    size_t limit = 100
);
```

Search for series by keywords.

**Parameters**:
- `query`: Search query (e.g., "unemployment", "inflation")
- `limit`: Maximum results (default: 100)

**Returns**: Vector of series IDs matching the query

---

#### `get_popular_series()`

```cpp
[[nodiscard]] Result<std::vector<std::string>> get_popular_series() const;
```

Get list of popular/frequently accessed series.

**Returns**: Vector of popular series IDs

---

### Usage Examples

#### Example 1: GDP Analysis

```cpp
#include "vdb/adapters/fred_adapter.hpp"
#include "vdb/database.hpp"

FREDAdapter fred(config);

// Fetch GDP data
auto gdp = fred.fetch_series("GDP", "2015-01-01", "2023-12-31");

if (gdp) {
    // Process each observation
    for (const auto& obs : gdp->observations) {
        // Create content for vectorization
        std::string content = "US GDP on " + obs.date + " was $" + 
                            std::to_string(obs.value) + " billion.";
        
        // Add to vector database
        Metadata meta;
        meta.type = "economic_indicator";
        meta.series_id = "GDP";
        meta.date = obs.date;
        meta.value = obs.value;
        
        db.add_text(content, meta);
    }
}
```

---

#### Example 2: Multiple Indicators

```cpp
FREDAdapter fred(config);

// Fetch multiple indicators
std::vector<std::string> indicators = {
    "GDP",      // Gross Domestic Product
    "UNRATE",   // Unemployment Rate
    "CPIAUCSL", // Consumer Price Index
    "DGS10"     // 10-Year Treasury Rate
};

for (const auto& indicator : indicators) {
    auto series = fred.fetch_series(indicator, "2023-01-01");
    
    if (series) {
        std::cout << series->title << ":\n";
        // Latest observation
        if (!series->observations.empty()) {
            const auto& latest = series->observations.back();
            std::cout << "  Latest: " << latest.date 
                     << " = " << latest.value << "\n";
        }
    }
}
```

---

#### Example 3: Search and Ingest

```cpp
FREDAdapter fred(config);

// Search for inflation-related series
auto search_results = fred.search_series("inflation", 10);

if (search_results) {
    for (const auto& series_id : *search_results) {
        // Fetch each series
        auto series = fred.fetch_series(series_id);
        
        if (series) {
            // Process and add to database
            std::cout << "Processing: " << series->title << "\n";
            // ... add to vector database
        }
    }
}
```

---

### Popular FRED Series

| Series ID | Description | Frequency |
|-----------|-------------|-----------|
| **GDP** | Gross Domestic Product | Quarterly |
| **UNRATE** | Unemployment Rate | Monthly |
| **CPIAUCSL** | Consumer Price Index | Monthly |
| **DGS10** | 10-Year Treasury Rate | Daily |
| **FEDFUNDS** | Federal Funds Rate | Monthly |
| **INDPRO** | Industrial Production Index | Monthly |
| **HOUST** | Housing Starts | Monthly |
| **PAYEMS** | Nonfarm Payrolls | Monthly |

---

## HTTP Client

### Introduction

The HTTP Client is a production-ready utility for making HTTP requests with enterprise features:

**Features**:
- 🔄 Automatic retry with exponential backoff
- ⏱️ Rate limiting (per-second, per-minute, per-hour)
- 💾 Response caching with TTL
- 🔗 Connection pooling and keepalive
- 🔒 SSL/TLS support
- 📦 Compression (gzip, deflate)
- 📊 Progress callbacks for large downloads
- 📈 Request statistics

### Quick Start

```cpp
#include "vdb/adapters/http_client.hpp"

using namespace vdb::adapters;

// Create HTTP client
HttpClientConfig config;
config.user_agent = "VectorStudio/2.0";
config.rate_limit.requests_per_second = 10;
config.retry.max_retries = 3;

HttpClient client(config);

// Make GET request
auto response = client.get("https://api.example.com/data");

if (response && response->is_success()) {
    std::cout << "Status: " << response->status_code << "\n";
    std::cout << "Body: " << response->body << "\n";
}
```

### Configuration

```cpp
struct HttpClientConfig {
    std::string user_agent{"VectorDB/1.0"};
    
    RateLimitConfig rate_limit;
    RetryConfig retry;
    CacheConfig cache;
    
    bool enable_compression{true};
    bool enable_keepalive{true};
    std::chrono::seconds keepalive_timeout{60};
    size_t max_concurrent_requests{10};
};
```

### Rate Limiting

```cpp
struct RateLimitConfig {
    size_t requests_per_second{10};
    size_t requests_per_minute{100};
    size_t requests_per_hour{1000};
    size_t burst_size{20};              // Max burst requests
    bool enable{true};
};
```

### Retry Configuration

```cpp
struct RetryConfig {
    size_t max_retries{3};
    std::chrono::milliseconds initial_backoff{1000};
    double backoff_multiplier{2.0};
    std::chrono::milliseconds max_backoff{30000};
    
    bool retry_on_timeout{true};
    bool retry_on_connection_error{true};
    bool retry_on_rate_limit{true};
    
    std::vector<int> retryable_status_codes{408, 429, 500, 502, 503, 504};
    bool enable{true};
};
```

### HTTP Methods

```cpp
// GET request
auto response = client.get("https://api.example.com/data");

// POST request
auto response = client.post(
    "https://api.example.com/submit",
    "{\"key\": \"value\"}",
    {{"Content-Type", "application/json"}}
);

// PUT request
auto response = client.put(url, body, headers);

// DELETE request
auto response = client.del(url, headers);

// Generic request
HttpRequest req;
req.method = HttpMethod::POST;
req.url = "https://api.example.com/endpoint";
req.body = data;
req.headers = {{"Authorization", "Bearer token"}};
auto response = client.request(req);
```

### Response Structure

```cpp
struct HttpResponse {
    int status_code{0};
    std::string body;
    std::map<std::string, std::string> headers;
    std::chrono::milliseconds elapsed{0};
    std::string error_message;
    
    bool is_success() const;        // 2xx status
    bool is_client_error() const;   // 4xx status
    bool is_server_error() const;   // 5xx status
    bool is_rate_limited() const;   // 429 status
};
```

### URL Builder

```cpp
#include "vdb/adapters/http_client.hpp"

UrlBuilder builder("https://api.fred.stlouisfed.org/fred/series/observations");
builder.add_param("series_id", "GDP")
       .add_param("api_key", api_key)
       .add_param("file_type", "json")
       .add_param("observation_start", "2020-01-01");

std::string url = builder.build();
// Result: https://api.fred.stlouisfed.org/fred/series/observations?series_id=GDP&api_key=...&file_type=json&observation_start=2020-01-01
```

### JSON Helper

```cpp
#include "vdb/adapters/http_client.hpp"
#include <nlohmann/json.hpp>

// Parse JSON response
auto response = client.get(url);
auto json_result = HttpJsonHelper::parse_json(*response);

if (json_result) {
    nlohmann::json data = *json_result;
    // Process JSON...
}

// Create JSON request
nlohmann::json body = {
    {"name", "example"},
    {"value", 42}
};

HttpRequest req = HttpJsonHelper::create_json_request(
    HttpMethod::POST,
    url,
    body
);

auto response = client.request(req);
```

### Statistics

```cpp
auto stats = client.get_statistics();

std::cout << "Total requests: " << stats.total_requests << "\n";
std::cout << "Successful: " << stats.successful_requests << "\n";
std::cout << "Failed: " << stats.failed_requests << "\n";
std::cout << "Cached: " << stats.cached_responses << "\n";
std::cout << "Retried: " << stats.retried_requests << "\n";
std::cout << "Rate limited: " << stats.rate_limited_requests << "\n";
std::cout << "Average time: " << stats.average_time.count() << " ms\n";

// Reset statistics
client.reset_statistics();
```

---

## Best Practices

### FRED Adapter

1. **API Key Management**:
```cpp
// Don't hardcode API keys
config.api_key = std::getenv("FRED_API_KEY");
```

2. **Rate Limiting**:
```cpp
// FRED allows 120 requests/minute
config.rate_limit.requests_per_minute = 100;  // Stay under limit
```

3. **Data Caching**:
```cpp
// Enable caching to avoid redundant requests
config.cache.enable = true;
config.cache.default_ttl = std::chrono::hours(24);
```

4. **Error Handling**:
```cpp
auto result = fred.fetch_series("GDP");
if (!result) {
    LOG_ERROR("FRED API error: " + result.error().message);
    // Handle error...
}
```

### HTTP Client

1. **Connection Pooling**:
```cpp
// Reuse client for multiple requests
HttpClient client(config);  // Create once
// Use for many requests
```

2. **Timeout Configuration**:
```cpp
HttpRequest req;
req.timeout = std::chrono::seconds(10);  // Per-request timeout
```

3. **Progress Tracking**:
```cpp
client.download(
    "https://example.com/large_file.zip",
    "output.zip",
    [](size_t current, size_t total) {
        std::cout << "Progress: " << (current * 100 / total) << "%\r";
    }
);
```

---

## See Also

- [Data Ingestion](05_DATA_INGESTION.md) - Complete data adapter guide
- [API Reference](07_API_REFERENCE.md) - Full API documentation
- [Usage Examples](12_USAGE_EXAMPLES.md) - More examples

---

**Last Updated**: 2026-01-06  
**Version**: 2.0.0  
**Status**: Production Ready ✅
