#pragma once
#include <napi.h>
#include <string>
#include <memory>

namespace hektor_native {

class Database {
public:
  // Open database
  static Napi::Value Open(const Napi::CallbackInfo& info);
  
  // Close database
  static Napi::Value Close(const Napi::CallbackInfo& info);
  
  // Add vector to database
  static Napi::Value AddVector(const Napi::CallbackInfo& info);
  
  // Query vectors (synchronous)
  static Napi::Value QueryVectors(const Napi::CallbackInfo& info);
};

} // namespace hektor_native
