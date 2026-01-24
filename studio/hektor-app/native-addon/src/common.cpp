#include "common.h"

// common.h already includes vdb/core.hpp which has all the types we need

namespace hektor_native {

// DistanceMetric conversions
Napi::Value DistanceMetricToNapi(vdb::DistanceMetric value, Napi::Env env) {
    switch (value) {
        case vdb::DistanceMetric::Cosine: return Napi::String::New(env, "cosine");
        case vdb::DistanceMetric::L2: return Napi::String::New(env, "l2");
        case vdb::DistanceMetric::DotProduct: return Napi::String::New(env, "dot");
        default: return env.Undefined();
    }
}

vdb::DistanceMetric NapiToDistanceMetric(const Napi::Value& val) {
    std::string str = val.As<Napi::String>().Utf8Value();
    if (str == "cosine") return vdb::DistanceMetric::Cosine;
    if (str == "l2") return vdb::DistanceMetric::L2;
    if (str == "dot") return vdb::DistanceMetric::DotProduct;
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

// SimdLevel conversions
Napi::Value SimdLevelToNapi(vdb::SimdLevel value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::SimdLevel NapiToSimdLevel(const Napi::Value& val) {
    return static_cast<vdb::SimdLevel>(val.As<Napi::Number>().Int32Value());
}

// ErrorCode conversions
Napi::Value ErrorCodeToNapi(vdb::ErrorCode value, Napi::Env env) {
    return Napi::Number::New(env, static_cast<int>(value));
}

vdb::ErrorCode NapiToErrorCode(const Napi::Value& val) {
    return static_cast<vdb::ErrorCode>(val.As<Napi::Number>().Int32Value());
}

} // namespace hektor_native
