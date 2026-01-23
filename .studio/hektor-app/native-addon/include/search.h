#pragma once
#include <napi.h>
#include <memory>
#include "vdb/database.hpp"

namespace hektor_native {

class Search : public Napi::ObjectWrap<Search> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Search(const Napi::CallbackInfo& info);
  
private:
  // Vector similarity search
  Napi::Value VectorSearch(const Napi::CallbackInfo& info);
  Napi::Value VectorSearchAsync(const Napi::CallbackInfo& info);
  
  // Text search
  Napi::Value TextSearch(const Napi::CallbackInfo& info);
  Napi::Value TextSearchAsync(const Napi::CallbackInfo& info);
  
  // Hybrid BM25 search
  Napi::Value HybridSearch(const Napi::CallbackInfo& info);
  Napi::Value HybridSearchAsync(const Napi::CallbackInfo& info);
  
  // Metadata filtering
  Napi::Value FilterByMetadata(const Napi::CallbackInfo& info);
  Napi::Value FilterByDate(const Napi::CallbackInfo& info);
  Napi::Value FilterByType(const Napi::CallbackInfo& info);
  
  std::shared_ptr<vdb::VectorDatabase> db_;
  std::string db_id_;
};

// Helper to convert QueryResult to JS
Napi::Object QueryResultToJS(Napi::Env env, const vdb::QueryResult& result);
Napi::Array QueryResultsToJS(Napi::Env env, const std::vector<vdb::QueryResult>& results);
vdb::QueryOptions JSToQueryOptions(const Napi::Object& obj);

} // namespace hektor_native
