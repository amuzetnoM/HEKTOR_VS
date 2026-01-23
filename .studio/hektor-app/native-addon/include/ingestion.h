#pragma once
#include <napi.h>
#include <memory>
#include <string>
#include "vdb/database.hpp"

namespace hektor_native {

class Ingestion : public Napi::ObjectWrap<Ingestion> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Ingestion(const Napi::CallbackInfo& info);
  
private:
  // CSV ingestion
  Napi::Value IngestCSV(const Napi::CallbackInfo& info);
  Napi::Value IngestCSVAsync(const Napi::CallbackInfo& info);
  
  // JSON ingestion
  Napi::Value IngestJSON(const Napi::CallbackInfo& info);
  Napi::Value IngestJSONAsync(const Napi::CallbackInfo& info);
  
  // Parquet ingestion
  Napi::Value IngestParquet(const Napi::CallbackInfo& info);
  Napi::Value IngestParquetAsync(const Napi::CallbackInfo& info);
  
  // Batch operations
  Napi::Value BatchIngest(const Napi::CallbackInfo& info);
  Napi::Value GetProgress(const Napi::CallbackInfo& info);
  
  std::shared_ptr<vdb::VectorDatabase> db_;
  std::string db_id_;
  
  // Progress tracking
  size_t total_items_;
  size_t processed_items_;
  bool is_running_;
};

} // namespace hektor_native
