#pragma once
#include <napi.h>
#include <memory>
#include <string>
#include <vector>

namespace hektor_native {

// Collection manager - simplified for HEKTOR's file-based approach
class Collections : public Napi::ObjectWrap<Collections> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Collections(const Napi::CallbackInfo& info);
  
private:
  Napi::Value Create(const Napi::CallbackInfo& info);
  Napi::Value List(const Napi::CallbackInfo& info);
  Napi::Value Get(const Napi::CallbackInfo& info);
  Napi::Value Delete(const Napi::CallbackInfo& info);
  Napi::Value Count(const Napi::CallbackInfo& info);
  
  std::string base_path_;
};

} // namespace hektor_native
