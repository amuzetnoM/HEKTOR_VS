#pragma once
#include <napi.h>
#include <memory>
#include "vdb/database.hpp"

namespace hektor_native {

class Quantization : public Napi::ObjectWrap<Quantization> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Quantization(const Napi::CallbackInfo& info);
  
private:
  // Product Quantization
  Napi::Value ApplyPQ(const Napi::CallbackInfo& info);
  
  // Scalar Quantization
  Napi::Value ApplySQ(const Napi::CallbackInfo& info);
  
  // Get quantization info
  Napi::Value GetQuantizationInfo(const Napi::CallbackInfo& info);
  
  std::shared_ptr<vdb::VectorDatabase> db_;
  std::string db_id_;
};

} // namespace hektor_native
