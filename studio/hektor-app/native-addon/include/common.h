#pragma once

#include <napi.h>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <functional>

// Include HEKTOR core types directly instead of forward declarations
// This ensures enum underlying types match
#include "vdb/core.hpp"

namespace hektor_native {

// Helper to convert C++ Result<T> to N-API value or throw
template<typename T>
inline T UnwrapResult(const vdb::Result<T>& result, Napi::Env env) {
    if (!result.has_value()) {
        Napi::Error::New(env, result.error().message).ThrowAsJavaScriptException();
        return T{}; // Never reached due to exception
    }
    return result.value();
}

// Specialization for Result<void>
template<>
inline void UnwrapResult<void>(const vdb::Result<void>& result, Napi::Env env) {
    if (!result.has_value()) {
        Napi::Error::New(env, result.error().message).ThrowAsJavaScriptException();
    }
}

// Helper to convert optional to N-API value or undefined
template<typename T, typename Converter>
inline Napi::Value OptionalToNapi(const std::optional<T>& opt, Napi::Env env, Converter&& converter) {
    if (opt.has_value()) {
        return converter(opt.value(), env);
    }
    return env.Undefined();
}

// Helper to convert vector to N-API array
template<typename T, typename Converter>
inline Napi::Array VectorToNapiArray(const std::vector<T>& vec, Napi::Env env, Converter&& converter) {
    Napi::Array arr = Napi::Array::New(env, vec.size());
    for (size_t i = 0; i < vec.size(); ++i) {
        arr[i] = converter(vec[i], env);
    }
    return arr;
}

// Helper to convert N-API array to vector
template<typename T, typename Converter>
inline std::vector<T> NapiArrayToVector(const Napi::Array& arr, Converter&& converter) {
    std::vector<T> vec;
    vec.reserve(arr.Length());
    for (uint32_t i = 0; i < arr.Length(); ++i) {
        vec.push_back(converter(arr[i]));
    }
    return vec;
}

// Helper to safely get string from N-API value
inline std::string GetString(const Napi::Value& val, const std::string& defaultValue = "") {
    if (val.IsString()) {
        return val.As<Napi::String>().Utf8Value();
    }
    return defaultValue;
}

// Helper to safely get number from N-API value
inline double GetNumber(const Napi::Value& val, double defaultValue = 0.0) {
    if (val.IsNumber()) {
        return val.As<Napi::Number>().DoubleValue();
    }
    return defaultValue;
}

// Helper to safely get boolean from N-API value
inline bool GetBool(const Napi::Value& val, bool defaultValue = false) {
    if (val.IsBoolean()) {
        return val.As<Napi::Boolean>().Value();
    }
    return defaultValue;
}

// Helper to check if value is null or undefined
inline bool IsNullOrUndefined(const Napi::Value& val) {
    return val.IsNull() || val.IsUndefined();
}

// Async worker base class for long-running operations
class AsyncWorker : public Napi::AsyncWorker {
public:
    AsyncWorker(Napi::Function& callback) 
        : Napi::AsyncWorker(callback), deferred(Napi::Promise::Deferred::New(callback.Env())) {}
    
    Napi::Promise::Deferred GetPromise() { return deferred; }

protected:
    Napi::Promise::Deferred deferred;
};

} // namespace hektor_native
