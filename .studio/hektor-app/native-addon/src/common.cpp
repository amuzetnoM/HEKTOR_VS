#include "common.h"
#include "vdb/types.hpp"

namespace hektor_native {

// DistanceMetric conversions
Napi::Value DistanceMetricToNapi(vdb::DistanceMetric value, Napi::Env env) {
    switch (value) {
        case vdb::DistanceMetric::Cosine: return Napi::String::New(env, "cosine");
        case vdb::DistanceMetric::L2: return Napi::String::New(env, "l2");
        case vdb::DistanceMetric::DotProduct: return Napi::String::New(env, "dot");
        case vdb::DistanceMetric::L2Squared: return Napi::String::New(env, "l2squared");
        default: return env.Undefined();
    }
}

vdb::DistanceMetric NapiToDistanceMetric(const Napi::Value& val) {
    std::string str = val.As<Napi::String>().Utf8Value();
    if (str == "cosine") return vdb::DistanceMetric::Cosine;
    if (str == "l2") return vdb::DistanceMetric::L2;
    if (str == "dot") return vdb::DistanceMetric::DotProduct;
    if (str == "l2squared") return vdb::DistanceMetric::L2Squared;
    return vdb::DistanceMetric::Cosine; // default
}

// DocumentType conversions
Napi::Value DocumentTypeToNapi(vdb::DocumentType value, Napi::Env env) {
    switch (value) {
        case vdb::DocumentType::Journal: return Napi::String::New(env, "journal");
        case vdb::DocumentType::Chart: return Napi::String::New(env, "chart");
        case vdb::DocumentType::CatalystWatchlist: return Napi::String::New(env, "catalyst_watchlist");
        case vdb::DocumentType::InstitutionalMatrix: return Napi::String::New(env, "institutional_matrix");
        case vdb::DocumentType::EconomicCalendar: return Napi::String::New(env, "economic_calendar");
        case vdb::DocumentType::WeeklyRundown: return Napi::String::New(env, "weekly_rundown");
        case vdb::DocumentType::ThreeMonthReport: return Napi::String::New(env, "three_month_report");
        case vdb::DocumentType::OneYearReport: return Napi::String::New(env, "one_year_report");
        case vdb::DocumentType::MonthlyReport: return Napi::String::New(env, "monthly_report");
        case vdb::DocumentType::YearlyReport: return Napi::String::New(env, "yearly_report");
        case vdb::DocumentType::PreMarket: return Napi::String::New(env, "pre_market");
        default: return Napi::String::New(env, "unknown");
    }
}

vdb::DocumentType NapiToDocumentType(const Napi::Value& val) {
    std::string str = val.As<Napi::String>().Utf8Value();
    if (str == "journal") return vdb::DocumentType::Journal;
    if (str == "chart") return vdb::DocumentType::Chart;
    if (str == "catalyst_watchlist") return vdb::DocumentType::CatalystWatchlist;
    if (str == "institutional_matrix") return vdb::DocumentType::InstitutionalMatrix;
    if (str == "economic_calendar") return vdb::DocumentType::EconomicCalendar;
    if (str == "weekly_rundown") return vdb::DocumentType::WeeklyRundown;
    if (str == "three_month_report") return vdb::DocumentType::ThreeMonthReport;
    if (str == "one_year_report") return vdb::DocumentType::OneYearReport;
    if (str == "monthly_report") return vdb::DocumentType::MonthlyReport;
    if (str == "yearly_report") return vdb::DocumentType::YearlyReport;
    if (str == "pre_market") return vdb::DocumentType::PreMarket;
    return vdb::DocumentType::Unknown;
}

// Device conversions
Napi::Value DeviceToNapi(vdb::Device value, Napi::Env env) {
    switch (value) {
        case vdb::Device::CPU: return Napi::String::New(env, "cpu");
        case vdb::Device::CUDA: return Napi::String::New(env, "cuda");
        case vdb::Device::DirectML: return Napi::String::New(env, "directml");
        default: return Napi::String::New(env, "cpu");
    }
}

vdb::Device NapiToDevice(const Napi::Value& val) {
    std::string str = val.As<Napi::String>().Utf8Value();
    if (str == "cuda") return vdb::Device::CUDA;
    if (str == "directml") return vdb::Device::DirectML;
    return vdb::Device::CPU;
}

// LogLevel conversions
Napi::Value LogLevelToNapi(vdb::LogLevel value, Napi::Env env) {
    switch (value) {
        case vdb::LogLevel::DEBUG: return Napi::Number::New(env, 0);
        case vdb::LogLevel::INFO: return Napi::Number::New(env, 1);
        case vdb::LogLevel::WARN: return Napi::Number::New(env, 2);
        case vdb::LogLevel::ERROR: return Napi::Number::New(env, 3);
        case vdb::LogLevel::CRITICAL: return Napi::Number::New(env, 4);
        case vdb::LogLevel::ANOMALY: return Napi::Number::New(env, 5);
        default: return Napi::Number::New(env, 1);
    }
}

vdb::LogLevel NapiToLogLevel(const Napi::Value& val) {
    int level = val.As<Napi::Number>().Int32Value();
    switch (level) {
        case 0: return vdb::LogLevel::DEBUG;
        case 1: return vdb::LogLevel::INFO;
        case 2: return vdb::LogLevel::WARN;
        case 3: return vdb::LogLevel::ERROR;
        case 4: return vdb::LogLevel::CRITICAL;
        case 5: return vdb::LogLevel::ANOMALY;
        default: return vdb::LogLevel::INFO;
    }
}

// Add stub implementations for remaining enums
Napi::Value AnomalyTypeToNapi(vdb::AnomalyType value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::AnomalyType NapiToAnomalyType(const Napi::Value& val) {
    return static_cast<vdb::AnomalyType>(val.As<Napi::Number>().Int32Value());
}

Napi::Value ChunkStrategyToNapi(vdb::ChunkStrategy value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::ChunkStrategy NapiToChunkStrategy(const Napi::Value& val) {
    return static_cast<vdb::ChunkStrategy>(val.As<Napi::Number>().Int32Value());
}

Napi::Value FusionMethodToNapi(vdb::FusionMethod value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::FusionMethod NapiToFusionMethod(const Napi::Value& val) {
    return static_cast<vdb::FusionMethod>(val.As<Napi::Number>().Int32Value());
}

Napi::Value ReplicationModeToNapi(vdb::ReplicationMode value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::ReplicationMode NapiToReplicationMode(const Napi::Value& val) {
    return static_cast<vdb::ReplicationMode>(val.As<Napi::Number>().Int32Value());
}

Napi::Value ShardingStrategyToNapi(vdb::ShardingStrategy value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::ShardingStrategy NapiToShardingStrategy(const Napi::Value& val) {
    return static_cast<vdb::ShardingStrategy>(val.As<Napi::Number>().Int32Value());
}

Napi::Value DataFormatToNapi(vdb::DataFormat value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::DataFormat NapiToDataFormat(const Napi::Value& val) {
    return static_cast<vdb::DataFormat>(val.As<Napi::Number>().Int32Value());
}

Napi::Value PerceptualCurveToNapi(vdb::PerceptualCurve value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::PerceptualCurve NapiToPerceptualCurve(const Napi::Value& val) {
    return static_cast<vdb::PerceptualCurve>(val.As<Napi::Number>().Int32Value());
}

Napi::Value DisplayTypeToNapi(vdb::DisplayType value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::DisplayType NapiToDisplayType(const Napi::Value& val) {
    return static_cast<vdb::DisplayType>(val.As<Napi::Number>().Int32Value());
}

Napi::Value ColorGamutToNapi(vdb::ColorGamut value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::ColorGamut NapiToColorGamut(const Napi::Value& val) {
    return static_cast<vdb::ColorGamut>(val.As<Napi::Number>().Int32Value());
}

Napi::Value HttpMethodToNapi(vdb::HttpMethod value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::HttpMethod NapiToHttpMethod(const Napi::Value& val) {
    return static_cast<vdb::HttpMethod>(val.As<Napi::Number>().Int32Value());
}

Napi::Value RoleToNapi(vdb::Role value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::Role NapiToRole(const Napi::Value& val) {
    return static_cast<vdb::Role>(val.As<Napi::Number>().Int32Value());
}

Napi::Value SimdLevelToNapi(vdb::SimdLevel value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::SimdLevel NapiToSimdLevel(const Napi::Value& val) {
    return static_cast<vdb::SimdLevel>(val.As<Napi::Number>().Int32Value());
}

Napi::Value ErrorCodeToNapi(vdb::ErrorCode value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::ErrorCode NapiToErrorCode(const Napi::Value& val) {
    return static_cast<vdb::ErrorCode>(val.As<Napi::Number>().Int32Value());
}

} // namespace hektor_native
