#pragma once
#include <napi.h>
#include <memory>
#include "vdb/database.hpp"

namespace hektor_native {

class IndexManagement : public Napi::ObjectWrap<IndexManagement> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  IndexManagement(const Napi::CallbackInfo& info);
  
private:
  // Index configuration
  Napi::Value ConfigureHNSW(const Napi::CallbackInfo& info);
  Napi::Value GetIndexConfig(const Napi::CallbackInfo& info);
  
  // Index operations
  Napi::Value Rebuild(const Napi::CallbackInfo& info);
  Napi::Value Optimize(const Napi::CallbackInfo& info);
  
  // Benchmarking
  Napi::Value RunBenchmark(const Napi::CallbackInfo& info);
  Napi::Value GetBenchmarkStats(const Napi::CallbackInfo& info);
  
  std::shared_ptr<vdb::VectorDatabase> db_;
  std::string db_id_;
};

} // namespace hektor_native
