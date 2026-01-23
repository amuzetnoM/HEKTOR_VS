#include "search.h"
#include "database.h"
#include <iostream>

namespace hektor_native {

// Helper: Convert QueryResult to JS
Napi::Object QueryResultToJS(Napi::Env env, const vdb::QueryResult& result) {
  Napi::Object obj = Napi::Object::New(env);
  obj.Set("id", Napi::Number::New(env, result.id));
  obj.Set("distance", Napi::Number::New(env, result.distance));
  obj.Set("score", Napi::Number::New(env, result.score));
  
  if (result.metadata) {
    obj.Set("metadata", MetadataToJS(env, *result.metadata));
  }
  
  return obj;
}

// Helper: Convert array of QueryResults to JS
Napi::Array QueryResultsToJS(Napi::Env env, const std::vector<vdb::QueryResult>& results) {
  Napi::Array arr = Napi::Array::New(env, results.size());
  for (size_t i = 0; i < results.size(); i++) {
    arr.Set(static_cast<uint32_t>(i), QueryResultToJS(env, results[i]));
  }
  return arr;
}

// Helper: Convert JS to QueryOptions
vdb::QueryOptions JSToQueryOptions(const Napi::Object& obj) {
  vdb::QueryOptions opts;
  
  if (obj.Has("k")) {
    opts.k = obj.Get("k").As<Napi::Number>().Uint32Value();
  }
  
  if (obj.Has("efSearch")) {
    opts.ef_search = obj.Get("efSearch").As<Napi::Number>().Uint32Value();
  }
  
  if (obj.Has("includeMetadata")) {
    opts.include_metadata = obj.Get("includeMetadata").As<Napi::Boolean>().Value();
  }
  
  if (obj.Has("typeFilter")) {
    opts.type_filter = static_cast<vdb::DocumentType>(
      obj.Get("typeFilter").As<Napi::Number>().Uint32Value()
    );
  }
  
  if (obj.Has("dateFilter")) {
    opts.date_filter = obj.Get("dateFilter").As<Napi::String>().Utf8Value();
  }
  
  if (obj.Has("dateFrom")) {
    opts.date_from = obj.Get("dateFrom").As<Napi::String>().Utf8Value();
  }
  
  if (obj.Has("dateTo")) {
    opts.date_to = obj.Get("dateTo").As<Napi::String>().Utf8Value();
  }
  
  if (obj.Has("assetFilter")) {
    opts.asset_filter = obj.Get("assetFilter").As<Napi::String>().Utf8Value();
  }
  
  if (obj.Has("biasFilter")) {
    opts.bias_filter = obj.Get("biasFilter").As<Napi::String>().Utf8Value();
  }
  
  return opts;
}

Napi::Object Search::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Search", {
    InstanceMethod("vectorSearch", &Search::VectorSearch),
    InstanceMethod("vectorSearchAsync", &Search::VectorSearchAsync),
    InstanceMethod("textSearch", &Search::TextSearch),
    InstanceMethod("textSearchAsync", &Search::TextSearchAsync),
    InstanceMethod("hybridSearch", &Search::HybridSearch),
    InstanceMethod("hybridSearchAsync", &Search::HybridSearchAsync),
    InstanceMethod("filterByMetadata", &Search::FilterByMetadata),
    InstanceMethod("filterByDate", &Search::FilterByDate),
    InstanceMethod("filterByType", &Search::FilterByType),
  });
  
  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  
  exports.Set("Search", func);
  return exports;
}

Search::Search(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Search>(info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected database ID").ThrowAsJavaScriptException();
    return;
  }
  
  db_id_ = info[0].As<Napi::String>().Utf8Value();
  db_ = DatabaseManager::get(db_id_);
  
  if (!db_) {
    Napi::Error::New(env, "Database not found").ThrowAsJavaScriptException();
  }
}

Napi::Value Search::VectorSearch(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_ || !db_->is_ready()) {
    Napi::Error::New(env, "Database not ready").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "Expected query vector").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::vector<float> query = JSToVector(info[0].As<Napi::Array>());
  
  vdb::QueryOptions opts;
  if (info.Length() >= 2 && info[1].IsObject()) {
    opts = JSToQueryOptions(info[1].As<Napi::Object>());
  }
  
  auto result = db_->query_vector(query, opts);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    response.Set("results", QueryResultsToJS(env, result.value()));
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Search::VectorSearchAsync(const Napi::CallbackInfo& info) {
  // TODO: Implement async version using AsyncWorker
  return VectorSearch(info);
}

Napi::Value Search::TextSearch(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_ || !db_->is_ready()) {
    Napi::Error::New(env, "Database not ready").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected query text").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string query = info[0].As<Napi::String>().Utf8Value();
  
  vdb::QueryOptions opts;
  if (info.Length() >= 2 && info[1].IsObject()) {
    opts = JSToQueryOptions(info[1].As<Napi::Object>());
  }
  
  auto result = db_->query_text(query, opts);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    response.Set("results", QueryResultsToJS(env, result.value()));
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Search::TextSearchAsync(const Napi::CallbackInfo& info) {
  // TODO: Implement async version
  return TextSearch(info);
}

Napi::Value Search::HybridSearch(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_ || !db_->is_ready()) {
    Napi::Error::New(env, "Database not ready").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected query text").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string query = info[0].As<Napi::String>().Utf8Value();
  
  vdb::QueryOptions opts;
  if (info.Length() >= 2 && info[1].IsObject()) {
    opts = JSToQueryOptions(info[1].As<Napi::Object>());
  }
  
  // Use text search which internally uses hybrid approach in HEKTOR
  auto result = db_->query_text(query, opts);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    response.Set("results", QueryResultsToJS(env, result.value()));
    response.Set("searchType", Napi::String::New(env, "hybrid"));
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Search::HybridSearchAsync(const Napi::CallbackInfo& info) {
  // TODO: Implement async version
  return HybridSearch(info);
}

Napi::Value Search::FilterByMetadata(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  // Metadata filtering is integrated into query options
  Napi::Object result = Napi::Object::New(env);
  result.Set("success", true);
  result.Set("message", "Use query options for metadata filtering");
  
  return result;
}

Napi::Value Search::FilterByDate(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected date string").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string date = info[0].As<Napi::String>().Utf8Value();
  auto results = db_->find_by_date(date);
  
  Napi::Array arr = Napi::Array::New(env, results.size());
  for (size_t i = 0; i < results.size(); i++) {
    arr.Set(static_cast<uint32_t>(i), MetadataToJS(env, results[i]));
  }
  
  return arr;
}

Napi::Value Search::FilterByType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Expected document type").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  auto type = static_cast<vdb::DocumentType>(info[0].As<Napi::Number>().Uint32Value());
  auto results = db_->find_by_type(type);
  
  Napi::Array arr = Napi::Array::New(env, results.size());
  for (size_t i = 0; i < results.size(); i++) {
    arr.Set(static_cast<uint32_t>(i), MetadataToJS(env, results[i]));
  }
  
  return arr;
}

} // namespace hektor_native
