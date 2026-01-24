#include <napi.h>
#include "common.h"
#include "database.h"
#include "search.h"
#include "collections.h"
#include "ingestion.h"
#include "index_mgmt.h"
#include "quantization.h"
#include "async_operations.h"
#include "utils.h"
#include "embeddings.h"
#include "storage.h"
#include "index.h"
#include "hybrid.h"
#include "rag.h"
#include "distributed.h"
#include "framework.h"
#include "telemetry.h"

namespace hektor_native {

// Initialize the native addon with COMPLETE API surface
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  // Core database operations
  Database::Init(env, exports);
  
  // Search operations
  Search::Init(env, exports);
  
  // Collections
  Collections::Init(env, exports);
  
  // Data ingestion
  Ingestion::Init(env, exports);
  
  // Index management
  IndexManagement::Init(env, exports);
  
  // Quantization (with full PQ/SQ/HDR support)
  Quantization::Init(env, exports);
  
  // Embeddings
  TextEncoderWrap::Init(env, exports);
  ImageEncoderWrap::Init(env, exports);
  OnnxSessionWrap::Init(env, exports);
  TokenizerWrap::Init(env, exports);
  ImagePreprocessorWrap::Init(env, exports);
  
  // Hybrid search (BM25 + Vector fusion)
  BM25EngineWrap::Init(env, exports);
  KeywordExtractorWrap::Init(env, exports);
  HybridSearchEngineWrap::Init(env, exports);
  QueryRewriterWrap::Init(env, exports);
  
  // Utility functions
  Utils::Init(env, exports);
  
  // Async operations (legacy compatibility)
  exports.Set("queryVectorsAsync", Napi::Function::New(env, AsyncOperations::QueryVectorsAsync));
  
  // Metadata
  exports.Set("getVersion", Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
    Napi::Env env = info.Env();
    Napi::Object version = Napi::Object::New(env);
    version.Set("version", Napi::String::New(env, "4.0.0"));
    version.Set("native", Napi::String::New(env, "hektor_native"));
    version.Set("api", Napi::String::New(env, "2.0.0"));
    version.Set("complete", Napi::Boolean::New(env, true));
    
    // Create features array manually
    Napi::Array features = Napi::Array::New(env, 10);
    features.Set(0u, Napi::String::New(env, "HNSW"));
    features.Set(1u, Napi::String::New(env, "BM25"));
    features.Set(2u, Napi::String::New(env, "HybridSearch"));
    features.Set(3u, Napi::String::New(env, "ProductQuantization"));
    features.Set(4u, Napi::String::New(env, "ScalarQuantization"));
    features.Set(5u, Napi::String::New(env, "PerceptualQuantization"));
    features.Set(6u, Napi::String::New(env, "PQ_ST2084"));
    features.Set(7u, Napi::String::New(env, "HLG_Rec2100"));
    features.Set(8u, Napi::String::New(env, "HDR1000"));
    features.Set(9u, Napi::String::New(env, "DolbyVision"));
    version.Set("features", features);
    
    return version;
  }));
  
  // System info
  exports.Set("getSystemInfo", Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
    Napi::Env env = info.Env();
    Napi::Object sysinfo = Napi::Object::New(env);
    
    #if defined(__AVX512F__)
      sysinfo.Set("simd", Napi::String::New(env, "AVX512"));
    #elif defined(__AVX2__) || defined(_M_AMD64)
      sysinfo.Set("simd", Napi::String::New(env, "AVX2"));
    #elif defined(__SSE4_1__)
      sysinfo.Set("simd", Napi::String::New(env, "SSE4"));
    #else
      sysinfo.Set("simd", Napi::String::New(env, "Scalar"));
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
    
    // Perceptual quantization capabilities
    Napi::Object pq = Napi::Object::New(env);
    pq.Set("peakLuminance", Napi::Number::New(env, 10000.0));  // ST 2084 max
    
    Napi::Array curves = Napi::Array::New(env, 5);
    curves.Set(0u, Napi::String::New(env, "PQ_ST2084"));
    curves.Set(1u, Napi::String::New(env, "HLG_Rec2100"));
    curves.Set(2u, Napi::String::New(env, "Gamma22"));
    curves.Set(3u, Napi::String::New(env, "Gamma24"));
    curves.Set(4u, Napi::String::New(env, "Linear"));
    pq.Set("supportedCurves", curves);
    
    Napi::Array profiles = Napi::Array::New(env, 3);
    profiles.Set(0u, Napi::Number::New(env, 5));
    profiles.Set(1u, Napi::Number::New(env, 7));
    profiles.Set(2u, Napi::Number::New(env, 8));
    pq.Set("dolbyVisionProfiles", profiles);
    
    sysinfo.Set("perceptualQuantization", pq);
    
    return sysinfo;
  }));
  
  return exports;
}

NODE_API_MODULE(hektor_native, Init)

} // namespace hektor_native
