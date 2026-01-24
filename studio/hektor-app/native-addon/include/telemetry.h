#pragma once

#include <napi.h>
#include "common.h"
#include <memory>

// Forward declarations
namespace vdb {
    namespace telemetry {
        class TelemetrySpan;
        class TelemetryMetrics;
        class TelemetryManager;
    }
    namespace logging {
        class Logger;
    }
}

namespace hektor_native {

// TelemetrySpan wrapper
class TelemetrySpanWrap : public Napi::ObjectWrap<TelemetrySpanWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    TelemetrySpanWrap(const Napi::CallbackInfo& info);
    ~TelemetrySpanWrap();
    
private:
    Napi::Value SetAttribute(const Napi::CallbackInfo& info);
    Napi::Value SetAttributes(const Napi::CallbackInfo& info);
    Napi::Value AddEvent(const Napi::CallbackInfo& info);
    Napi::Value RecordError(const Napi::CallbackInfo& info);
    Napi::Value SetStatus(const Napi::CallbackInfo& info);
    Napi::Value GetContext(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::telemetry::TelemetrySpan> span_;
};

// TelemetryMetrics wrapper (singleton)
class TelemetryMetrics {
public:
    static void Init(Napi::Env env, Napi::Object& exports);
    
    static Napi::Value IncrementCounter(const Napi::CallbackInfo& info);
    static Napi::Value RecordHistogram(const Napi::CallbackInfo& info);
    static Napi::Value SetGauge(const Napi::CallbackInfo& info);
    static Napi::Value RecordSearchLatency(const Napi::CallbackInfo& info);
    static Napi::Value RecordInsertOperation(const Napi::CallbackInfo& info);
    static Napi::Value RecordIndexBuildTime(const Napi::CallbackInfo& info);
    static Napi::Value RecordMemoryUsage(const Napi::CallbackInfo& info);
};

// TelemetryManager wrapper (singleton)
class TelemetryManager {
public:
    static void Init(Napi::Env env, Napi::Object& exports);
    
    static Napi::Value Initialize(const Napi::CallbackInfo& info);
    static Napi::Value Shutdown(const Napi::CallbackInfo& info);
    static Napi::Value IsEnabled(const Napi::CallbackInfo& info);
};

// Logger wrapper (singleton)
class Logger {
public:
    static void Init(Napi::Env env, Napi::Object& exports);
    
    static Napi::Value Initialize(const Napi::CallbackInfo& info);
    static Napi::Value Log(const Napi::CallbackInfo& info);
    static Napi::Value LogAnomaly(const Napi::CallbackInfo& info);
    static Napi::Value Debug(const Napi::CallbackInfo& info);
    static Napi::Value Info(const Napi::CallbackInfo& info);
    static Napi::Value Warn(const Napi::CallbackInfo& info);
    static Napi::Value Error(const Napi::CallbackInfo& info);
    static Napi::Value Critical(const Napi::CallbackInfo& info);
};

} // namespace hektor_native
