#include <napi.h>
#include "database.h"
#include "async_operations.h"

namespace hektor_native {

// Initialize the native addon
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  // Database operations
  exports.Set("openDatabase", Napi::Function::New(env, Database::Open));
  exports.Set("closeDatabase", Napi::Function::New(env, Database::Close));
  exports.Set("addVector", Napi::Function::New(env, Database::AddVector));
  exports.Set("queryVectors", Napi::Function::New(env, Database::QueryVectors));
  exports.Set("queryVectorsAsync", Napi::Function::New(env, AsyncOperations::QueryVectorsAsync));
  
  // Metadata
  exports.Set("getVersion", Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
    return Napi::String::New(info.Env(), "4.0.0");
  }));
  
  return exports;
}

NODE_API_MODULE(hektor_native, Init)

} // namespace hektor_native
