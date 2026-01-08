#pragma once
// ============================================================================
// VectorDB - OpenTelemetry Distributed Tracing and Observability
// ============================================================================

#include "core.hpp"
#include "logging.hpp"
#include <string>
#include <memory>
#include <optional>
#include <chrono>
#include <unordered_map>

// Forward declarations for OpenTelemetry types (to avoid hard dependency)
// When VDB_USE_OPENTELEMETRY is enabled, these are defined in the .cpp file
namespace opentelemetry {
namespace trace {
    class Tracer;
    class Span;
    class TracerProvider;
}
namespace metrics {
    class Meter;
    class MeterProvider;
}
namespace context {
    class Context;
}
}

namespace vdb::telemetry {

// ============================================================================
// Trace Context - Lightweight wrapper for span context
// ============================================================================

struct TraceContext {
    std::string trace_id;
    std::string span_id;
    std::string parent_span_id;
    bool sampled = true;
    
    // W3C Trace Context format: "00-{trace_id}-{span_id}-{trace_flags}"
    std::string to_w3c_traceparent() const;
    static TraceContext from_w3c_traceparent(const std::string& header);
};

// ============================================================================
// Span Attributes - Common attributes for vector operations
// ============================================================================

struct SpanAttributes {
    // Operation attributes
    std::string operation_name;
    std::string operation_type;  // "search", "insert", "update", "delete"
    
    // Vector operation attributes
    size_t vector_dimension = 0;
    size_t vector_count = 0;
    size_t result_count = 0;
    
    // Database attributes
    std::string db_system = "vectordb";
    std::string db_name;
    std::string collection_name;
    
    // Performance attributes
    double query_latency_ms = 0.0;
    size_t memory_bytes = 0;
    
    // Custom attributes
    std::unordered_map<std::string, std::string> custom;
};

// ============================================================================
// Telemetry Span - RAII wrapper for distributed tracing
// ============================================================================

class TelemetrySpan {
public:
    TelemetrySpan(const std::string& operation_name, 
                  const std::optional<TraceContext>& parent_context = std::nullopt);
    ~TelemetrySpan();
    
    // Move-only semantics
    TelemetrySpan(const TelemetrySpan&) = delete;
    TelemetrySpan& operator=(const TelemetrySpan&) = delete;
    TelemetrySpan(TelemetrySpan&&) noexcept;
    TelemetrySpan& operator=(TelemetrySpan&&) noexcept;
    
    // Add attributes
    void set_attribute(const std::string& key, const std::string& value);
    void set_attribute(const std::string& key, int64_t value);
    void set_attribute(const std::string& key, double value);
    void set_attribute(const std::string& key, bool value);
    void set_attributes(const SpanAttributes& attrs);
    
    // Add events
    void add_event(const std::string& name, 
                   const std::unordered_map<std::string, std::string>& attributes = {});
    
    // Error handling
    void record_error(const std::string& error_message);
    void set_status(bool success, const std::string& description = "");
    
    // Get trace context for propagation
    TraceContext get_context() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string operation_name_;
    std::chrono::steady_clock::time_point start_time_;
};

// ============================================================================
// Telemetry Metrics - Structured metrics for observability
// ============================================================================

class TelemetryMetrics {
public:
    static TelemetryMetrics& instance();
    
    // Counter metrics
    void increment_counter(const std::string& name, int64_t value = 1,
                          const std::unordered_map<std::string, std::string>& labels = {});
    
    // Histogram metrics (for latencies, sizes, etc.)
    void record_histogram(const std::string& name, double value,
                         const std::unordered_map<std::string, std::string>& labels = {});
    
    // Gauge metrics (for current values)
    void set_gauge(const std::string& name, double value,
                   const std::unordered_map<std::string, std::string>& labels = {});
    
    // Common vector database metrics
    void record_search_latency(double latency_ms, size_t result_count);
    void record_insert_operation(size_t vector_count, double latency_ms);
    void record_index_build_time(double duration_seconds);
    void record_memory_usage(size_t bytes);
    
private:
    TelemetryMetrics();
    ~TelemetryMetrics();
    TelemetryMetrics(const TelemetryMetrics&) = delete;
    TelemetryMetrics& operator=(const TelemetryMetrics&) = delete;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Telemetry Configuration
// ============================================================================

struct TelemetryConfig {
    // Tracing
    bool enable_tracing = true;
    std::string service_name = "vector-studio";
    std::string service_version = "3.0.0";
    std::string deployment_environment = "production";
    
    // Sampling
    double sampling_ratio = 1.0;  // 1.0 = trace everything, 0.0 = trace nothing
    bool use_tail_based_sampling = false;
    
    // Exporters
    std::string trace_exporter = "otlp";  // "otlp", "jaeger", "zipkin", "console"
    std::string metrics_exporter = "prometheus";  // "prometheus", "otlp", "console"
    
    // OTLP endpoints
    std::string otlp_endpoint = "http://localhost:4317";
    std::string otlp_headers;  // "key1=value1,key2=value2"
    
    // Prometheus
    std::string prometheus_host = "0.0.0.0";
    int prometheus_port = 8888;  // Standard Prometheus exporter port
    
    // Resource attributes
    std::unordered_map<std::string, std::string> resource_attributes;
    
    // Performance
    size_t max_queue_size = 2048;
    size_t batch_size = 512;
    std::chrono::milliseconds export_interval{5000};
};

// ============================================================================
// Telemetry Manager - Singleton for lifecycle management
// ============================================================================

class TelemetryManager {
public:
    static TelemetryManager& instance();
    
    // Initialize telemetry with configuration
    void initialize(const TelemetryConfig& config);
    void shutdown();
    
    // Check if telemetry is enabled
    bool is_enabled() const { return enabled_; }
    
    // Get current configuration
    const TelemetryConfig& config() const { return config_; }
    
    // Create a new span (convenience method)
    TelemetrySpan start_span(const std::string& operation_name,
                            const std::optional<TraceContext>& parent_context = std::nullopt);
    
    // Metrics access
    TelemetryMetrics& metrics() { return TelemetryMetrics::instance(); }
    
private:
    TelemetryManager();
    ~TelemetryManager();
    TelemetryManager(const TelemetryManager&) = delete;
    TelemetryManager& operator=(const TelemetryManager&) = delete;
    
    void initialize_tracing();
    void initialize_metrics();
    void cleanup();
    
    bool enabled_ = false;
    bool initialized_ = false;
    TelemetryConfig config_;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Convenience Macros for Tracing
// ============================================================================

#define TRACE_SPAN(operation_name) \
    auto _trace_span_ = vdb::telemetry::TelemetryManager::instance().start_span(operation_name)

#define TRACE_SPAN_WITH_PARENT(operation_name, parent_ctx) \
    auto _trace_span_ = vdb::telemetry::TelemetryManager::instance().start_span(operation_name, parent_ctx)

#define TRACE_ADD_ATTRIBUTE(key, value) \
    _trace_span_.set_attribute(key, value)

#define TRACE_ADD_EVENT(name, ...) \
    _trace_span_.add_event(name, ##__VA_ARGS__)

#define TRACE_RECORD_ERROR(message) \
    _trace_span_.record_error(message)

// ============================================================================
// Semantic Conventions for Vector Database Operations
// ============================================================================

namespace semantic {

// Database semantic conventions (following OpenTelemetry standards)
constexpr const char* DB_SYSTEM = "db.system";
constexpr const char* DB_NAME = "db.name";
constexpr const char* DB_OPERATION = "db.operation";
constexpr const char* DB_STATEMENT = "db.statement";

// Vector-specific semantic conventions
constexpr const char* VECTOR_DIMENSION = "vector.dimension";
constexpr const char* VECTOR_COUNT = "vector.count";
constexpr const char* VECTOR_INDEX_TYPE = "vector.index.type";
constexpr const char* VECTOR_DISTANCE_METRIC = "vector.distance.metric";
constexpr const char* VECTOR_SEARCH_K = "vector.search.k";
constexpr const char* VECTOR_SEARCH_EF = "vector.search.ef";
constexpr const char* VECTOR_RESULT_COUNT = "vector.result.count";

// Performance conventions
constexpr const char* QUERY_DURATION_MS = "query.duration_ms";
constexpr const char* MEMORY_BYTES = "memory.bytes";
constexpr const char* CPU_USAGE_PERCENT = "cpu.usage.percent";

// Operation types
constexpr const char* OP_SEARCH = "search";
constexpr const char* OP_INSERT = "insert";
constexpr const char* OP_UPDATE = "update";
constexpr const char* OP_DELETE = "delete";
constexpr const char* OP_INDEX_BUILD = "index.build";
constexpr const char* OP_INDEX_OPTIMIZE = "index.optimize";

} // namespace semantic

} // namespace vdb::telemetry
