#include <napi.h>
#include "database.h"
#include "search.h"
#include "collections.h"
#include "ingestion.h"
#include "index_mgmt.h"
#include "quantization.h"
#include "async_operations.h"

namespace hektor_native {

// Initialize the native addon
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  // Core classes
  Database::Init(env, exports);
  Search::Init(env, exports);
  Collections::Init(env, exports);
  Ingestion::Init(env, exports);
  IndexManagement::Init(env, exports);
  Quantization::Init(env, exports);
  
  // Async operations (legacy compatibility)
  exports.Set("queryVectorsAsync", Napi::Function::New(env, AsyncOperations::QueryVectorsAsync));
  
  // Metadata
  exports.Set("getVersion", Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
    Napi::Env env = info.Env();
    Napi::Object version = Napi::Object::New(env);
    version.Set("version", Napi::String::New(env, "4.0.0"));
    version.Set("native", Napi::String::New(env, "hektor_native"));
    version.Set("api", Napi::String::New(env, "1.0.0"));
    return version;
  }));
  
  // System info
  exports.Set("getSystemInfo", Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
    Napi::Env env = info.Env();
    Napi::Object sysinfo = Napi::Object::New(env);
    
    #if defined(__AVX512F__)
      sysinfo.Set("simd", Napi::String::New(env, "AVX512"));
    #elif defined(__AVX2__)
      sysinfo.Set("simd", Napi::String::New(env, "AVX2"));
    #elif defined(__SSE4_1__)
      sysinfo.Set("simd", Napi::String::New(env, "SSE4"));
    #else
      sysinfo.Set("simd", Napi::String::New(env, "None"));
    #endif
    
    sysinfo.Set("cppStandard", Napi::String::New(env, "C++23"));
    sysinfo.Set("platform", Napi::String::New(env, 
      #ifdef _WIN32
        "Windows"
      #elif __APPLE__
        "macOS"
      #else
        "Linux"
      #endif
    ));
    
    return sysinfo;
  }));
  
  return exports;
}

NODE_API_MODULE(hektor_native, Init)

} // namespace hektor_native
