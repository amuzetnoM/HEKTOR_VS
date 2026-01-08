// ============================================================================
// VectorDB - OpenTelemetry Implementation
// ============================================================================

#include "vdb/telemetry.hpp"
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>

#ifdef VDB_USE_OPENTELEMETRY
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/resource/semantic_conventions.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h>
#include <opentelemetry/exporters/prometheus/exporter.h>
#include <opentelemetry/metrics/provider.h>

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace otlp = opentelemetry::exporter::otlp;
namespace resource = opentelemetry::sdk::resource;
namespace metrics_api = opentelemetry::metrics;
namespace metrics_sdk = opentelemetry::sdk::metrics;
namespace prometheus = opentelemetry::exporter::metrics;
#endif

namespace vdb::telemetry {

// ============================================================================
// Utility Functions
// ============================================================================

namespace {

std::string generate_trace_id() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << dis(gen) 
       << std::setw(16) << dis(gen);
    return ss.str();
}

std::string generate_span_id() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << dis(gen);
    return ss.str();
}

} // anonymous namespace

// ============================================================================
// TraceContext Implementation
// ============================================================================

std::string TraceContext::to_w3c_traceparent() const {
    std::stringstream ss;
    ss << "00-" << trace_id << "-" << span_id << "-" << (sampled ? "01" : "00");
    return ss.str();
}

TraceContext TraceContext::from_w3c_traceparent(const std::string& header) {
    TraceContext ctx;
    
    // Parse W3C Trace Context: "00-{trace_id}-{span_id}-{flags}"
    size_t pos1 = header.find('-');
    size_t pos2 = header.find('-', pos1 + 1);
    size_t pos3 = header.find('-', pos2 + 1);
    
    if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
        ctx.trace_id = header.substr(pos1 + 1, pos2 - pos1 - 1);
        ctx.span_id = header.substr(pos2 + 1, pos3 - pos2 - 1);
        std::string flags = header.substr(pos3 + 1);
        ctx.sampled = (flags == "01");
    }
    
    return ctx;
}

// ============================================================================
// TelemetrySpan Implementation
// ============================================================================

struct TelemetrySpan::Impl {
#ifdef VDB_USE_OPENTELEMETRY
    std::shared_ptr<trace_api::Span> span;
#endif
    TraceContext context;
};

TelemetrySpan::TelemetrySpan(const std::string& operation_name,
                             const std::optional<TraceContext>& parent_context)
    : impl_(std::make_unique<Impl>())
    , operation_name_(operation_name)
    , start_time_(std::chrono::steady_clock::now()) {
    
    // Generate trace context
    if (parent_context) {
        impl_->context = *parent_context;
        impl_->context.parent_span_id = impl_->context.span_id;
        impl_->context.span_id = generate_span_id();
    } else {
        impl_->context.trace_id = generate_trace_id();
        impl_->context.span_id = generate_span_id();
        impl_->context.sampled = true;
    }
    
#ifdef VDB_USE_OPENTELEMETRY
    auto& manager = TelemetryManager::instance();
    if (manager.is_enabled()) {
        auto tracer = trace_api::Provider::GetTracerProvider()->GetTracer(
            manager.config().service_name, manager.config().service_version);
        
        trace_api::StartSpanOptions options;
        impl_->span = tracer->StartSpan(operation_name, options);
    }
#endif
    
    // Log span start (only if debug logging enabled)
    if (Logger::instance().config().min_level <= LogLevel::DEBUG) {
        LOG_DEBUG("Starting trace span: " + operation_name + 
                  " [trace_id=" + impl_->context.trace_id + 
                  ", span_id=" + impl_->context.span_id + "]");
    }
}

TelemetrySpan::~TelemetrySpan() {
    if (impl_) {
        auto duration = std::chrono::steady_clock::now() - start_time_;
        auto duration_ms = std::chrono::duration<double, std::milli>(duration).count();
        
#ifdef VDB_USE_OPENTELEMETRY
        if (impl_->span) {
            impl_->span->SetAttribute("duration_ms", duration_ms);
            impl_->span->End();
        }
#endif
        
        // Log span completion (only if debug logging enabled)
        if (Logger::instance().config().min_level <= LogLevel::DEBUG) {
            std::ostringstream oss;
            oss << "Completed trace span: " << operation_name_ 
                << " (duration: " << duration_ms << "ms)";
            LOG_DEBUG(oss.str());
        }
    }
}

TelemetrySpan::TelemetrySpan(TelemetrySpan&& other) noexcept
    : impl_(std::move(other.impl_))
    , operation_name_(std::move(other.operation_name_))
    , start_time_(other.start_time_) {
}

TelemetrySpan& TelemetrySpan::operator=(TelemetrySpan&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
        operation_name_ = std::move(other.operation_name_);
        start_time_ = other.start_time_;
    }
    return *this;
}

void TelemetrySpan::set_attribute(const std::string& key, const std::string& value) {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->span) {
        impl_->span->SetAttribute(key, value);
    }
#endif
}

void TelemetrySpan::set_attribute(const std::string& key, int64_t value) {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->span) {
        impl_->span->SetAttribute(key, value);
    }
#endif
}

void TelemetrySpan::set_attribute(const std::string& key, double value) {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->span) {
        impl_->span->SetAttribute(key, value);
    }
#endif
}

void TelemetrySpan::set_attribute(const std::string& key, bool value) {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->span) {
        impl_->span->SetAttribute(key, value);
    }
#endif
}

void TelemetrySpan::set_attributes(const SpanAttributes& attrs) {
    set_attribute(semantic::DB_OPERATION, attrs.operation_type);
    set_attribute(semantic::DB_NAME, attrs.db_name);
    
    if (attrs.vector_dimension > 0) {
        set_attribute(semantic::VECTOR_DIMENSION, static_cast<int64_t>(attrs.vector_dimension));
    }
    if (attrs.vector_count > 0) {
        set_attribute(semantic::VECTOR_COUNT, static_cast<int64_t>(attrs.vector_count));
    }
    if (attrs.result_count > 0) {
        set_attribute(semantic::VECTOR_RESULT_COUNT, static_cast<int64_t>(attrs.result_count));
    }
    if (attrs.query_latency_ms > 0.0) {
        set_attribute(semantic::QUERY_DURATION_MS, attrs.query_latency_ms);
    }
    
    for (const auto& [key, value] : attrs.custom) {
        set_attribute(key, value);
    }
}

void TelemetrySpan::add_event(const std::string& name,
                              const std::unordered_map<std::string, std::string>& attributes) {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->span) {
        impl_->span->AddEvent(name);
    }
#endif
    if (Logger::instance().config().min_level <= LogLevel::DEBUG) {
        LOG_DEBUG("Trace event: " + name);
    }
}

void TelemetrySpan::record_error(const std::string& error_message) {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->span) {
        impl_->span->SetStatus(trace_api::StatusCode::kError, error_message);
        impl_->span->SetAttribute("error", true);
        impl_->span->SetAttribute("error.message", error_message);
    }
#endif
    LOG_ERROR("Trace span error: " + error_message);
}

void TelemetrySpan::set_status(bool success, const std::string& description) {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->span) {
        auto status = success ? trace_api::StatusCode::kOk : trace_api::StatusCode::kError;
        impl_->span->SetStatus(status, description);
    }
#endif
}

TraceContext TelemetrySpan::get_context() const {
    return impl_->context;
}

// ============================================================================
// TelemetryMetrics Implementation
// ============================================================================

struct TelemetryMetrics::Impl {
#ifdef VDB_USE_OPENTELEMETRY
    std::shared_ptr<metrics_api::Meter> meter;
#endif
};

TelemetryMetrics::TelemetryMetrics() : impl_(std::make_unique<Impl>()) {
#ifdef VDB_USE_OPENTELEMETRY
    auto& manager = TelemetryManager::instance();
    if (manager.is_enabled()) {
        impl_->meter = metrics_api::Provider::GetMeterProvider()->GetMeter(
            manager.config().service_name, manager.config().service_version);
    }
#endif
}

TelemetryMetrics::~TelemetryMetrics() = default;

TelemetryMetrics& TelemetryMetrics::instance() {
    static TelemetryMetrics instance;
    return instance;
}

void TelemetryMetrics::increment_counter(const std::string& name, int64_t value,
                                        const std::unordered_map<std::string, std::string>& labels) {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->meter) {
        auto counter = impl_->meter->CreateInt64Counter(name);
        counter->Add(value);
    }
#endif
}

void TelemetryMetrics::record_histogram(const std::string& name, double value,
                                       const std::unordered_map<std::string, std::string>& labels) {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->meter) {
        auto histogram = impl_->meter->CreateDoubleHistogram(name);
        histogram->Record(value);
    }
#endif
}

void TelemetryMetrics::set_gauge(const std::string& name, double value,
                                const std::unordered_map<std::string, std::string>& labels) {
#ifdef VDB_USE_OPENTELEMETRY
    // Note: OpenTelemetry doesn't have direct gauge support in all versions
    // Use observable gauge or histogram as fallback
#endif
}

void TelemetryMetrics::record_search_latency(double latency_ms, size_t result_count) {
    record_histogram("vector.search.latency_ms", latency_ms);
    increment_counter("vector.search.count", 1);
    record_histogram("vector.search.results", static_cast<double>(result_count));
}

void TelemetryMetrics::record_insert_operation(size_t vector_count, double latency_ms) {
    increment_counter("vector.insert.count", static_cast<int64_t>(vector_count));
    record_histogram("vector.insert.latency_ms", latency_ms);
}

void TelemetryMetrics::record_index_build_time(double duration_seconds) {
    record_histogram("vector.index.build_duration_seconds", duration_seconds);
}

void TelemetryMetrics::record_memory_usage(size_t bytes) {
    record_histogram("vector.memory.bytes", static_cast<double>(bytes));
}

// ============================================================================
// TelemetryManager Implementation
// ============================================================================

struct TelemetryManager::Impl {
#ifdef VDB_USE_OPENTELEMETRY
    std::shared_ptr<trace_api::TracerProvider> tracer_provider;
    std::shared_ptr<metrics_api::MeterProvider> meter_provider;
#endif
};

TelemetryManager::TelemetryManager() : impl_(std::make_unique<Impl>()) {
}

TelemetryManager::~TelemetryManager() {
    if (initialized_) {
        cleanup();
    }
}

TelemetryManager& TelemetryManager::instance() {
    static TelemetryManager instance;
    return instance;
}

void TelemetryManager::initialize(const TelemetryConfig& config) {
    if (initialized_) {
        LOG_WARN("TelemetryManager already initialized");
        return;
    }
    
    config_ = config;
    enabled_ = config.enable_tracing;
    
    if (!enabled_) {
        LOG_INFO("Telemetry disabled by configuration");
        return;
    }
    
#ifdef VDB_USE_OPENTELEMETRY
    LOG_INFO("Initializing OpenTelemetry with service: " + config.service_name);
    
    initialize_tracing();
    initialize_metrics();
    
    initialized_ = true;
    LOG_INFO("OpenTelemetry initialized successfully");
#else
    LOG_WARN("OpenTelemetry not available (VDB_USE_OPENTELEMETRY not defined)");
    LOG_INFO("Telemetry will use fallback logging-based tracing");
    enabled_ = true;  // Enable fallback mode
    initialized_ = true;
#endif
}

void TelemetryManager::initialize_tracing() {
#ifdef VDB_USE_OPENTELEMETRY
    // Create resource with service information
    auto resource_attributes = resource::ResourceAttributes{
        {resource::SemanticConventions::kServiceName, config_.service_name},
        {resource::SemanticConventions::kServiceVersion, config_.service_version},
        {resource::SemanticConventions::kDeploymentEnvironment, config_.deployment_environment}
    };
    
    // Add custom resource attributes
    for (const auto& [key, value] : config_.resource_attributes) {
        resource_attributes[key] = value;
    }
    
    auto resource = resource::Resource::Create(resource_attributes);
    
    // Create exporter based on configuration
    std::unique_ptr<trace_sdk::SpanExporter> exporter;
    
    if (config_.trace_exporter == "otlp") {
        otlp::OtlpGrpcExporterOptions opts;
        opts.endpoint = config_.otlp_endpoint;
        exporter = std::make_unique<otlp::OtlpGrpcExporter>(opts);
        LOG_INFO("Using OTLP trace exporter: " + config_.otlp_endpoint);
    } else {
        // Fallback to console exporter
        exporter = std::make_unique<opentelemetry::exporter::trace::OStreamSpanExporter>();
        LOG_INFO("Using console trace exporter");
    }
    
    // Create span processor
    auto processor = std::make_unique<trace_sdk::BatchSpanProcessor>(
        std::move(exporter),
        trace_sdk::BatchSpanProcessorOptions{}
    );
    
    // Create tracer provider
    impl_->tracer_provider = std::make_shared<trace_sdk::TracerProvider>(
        std::move(processor), resource);
    
    // Set global tracer provider
    trace_api::Provider::SetTracerProvider(impl_->tracer_provider);
#endif
}

void TelemetryManager::initialize_metrics() {
#ifdef VDB_USE_OPENTELEMETRY
    // Create metrics exporter
    if (config_.metrics_exporter == "prometheus") {
        prometheus::PrometheusExporterOptions opts;
        opts.url = config_.prometheus_host + ":" + std::to_string(config_.prometheus_port);
        
        auto exporter = std::make_unique<prometheus::PrometheusExporter>(opts);
        
        auto reader = std::make_unique<metrics_sdk::PeriodicExportingMetricReader>(
            std::move(exporter),
            metrics_sdk::PeriodicExportingMetricReaderOptions{}
        );
        
        impl_->meter_provider = std::make_shared<metrics_sdk::MeterProvider>(
            std::move(reader));
        
        metrics_api::Provider::SetMeterProvider(impl_->meter_provider);
        
        LOG_INFO("Prometheus metrics exporter initialized on " + 
                 config_.prometheus_host + ":" + std::to_string(config_.prometheus_port));
    }
#endif
}

void TelemetryManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    LOG_INFO("Shutting down telemetry");
    cleanup();
    initialized_ = false;
    enabled_ = false;
}

void TelemetryManager::cleanup() {
#ifdef VDB_USE_OPENTELEMETRY
    if (impl_->tracer_provider) {
        // Flush any pending spans
        impl_->tracer_provider.reset();
    }
    if (impl_->meter_provider) {
        impl_->meter_provider.reset();
    }
#endif
}

TelemetrySpan TelemetryManager::start_span(const std::string& operation_name,
                                          const std::optional<TraceContext>& parent_context) {
    return TelemetrySpan(operation_name, parent_context);
}

} // namespace vdb::telemetry
