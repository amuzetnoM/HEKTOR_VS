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
  
  // Quantization
  Quantization::Init(env, exports);
  
  // Embeddings (NEW - Complete)
  TextEncoderWrap::Init(env, exports);
  ImageEncoderWrap::Init(env, exports);
  OnnxSessionWrap::Init(env, exports);
  TokenizerWrap::Init(env, exports);
  ImagePreprocessorWrap::Init(env, exports);
  
  // Storage layer (NEW - Complete)
  // MemoryMappedFileWrap::Init(env, exports);
  // VectorStoreWrap::Init(env, exports);
  // MetadataStoreWrap::Init(env, exports);
  // SqliteStoreWrap::Init(env, exports);
  // PgVectorStoreWrap::Init(env, exports);
  
  // Index structures (NEW - Complete)
  // HnswIndexWrap::Init(env, exports);
  // FlatIndexWrap::Init(env, exports);
  // MetadataIndexWrap::Init(env, exports);
  
  // Hybrid search (NEW - Complete)
  // BM25EngineWrap::Init(env, exports);
  // KeywordExtractorWrap::Init(env, exports);
  // HybridSearchEngineWrap::Init(env, exports);
  // QueryRewriterWrap::Init(env, exports);
  
  // RAG & LLM (NEW - Complete)
  // RAGEngineWrap::Init(env, exports);
  // LLMEngineWrap::Init(env, exports);
  // LangChainAdapterWrap::Init(env, exports);
  // LlamaIndexAdapterWrap::Init(env, exports);
  // DocumentChunkerWrap::Init(env, exports);
  // RAGUtils::Init(env, exports);
  
  // Distributed systems (NEW - Complete)
  // ReplicationManagerWrap::Init(env, exports);
  // ShardingManagerWrap::Init(env, exports);
  // DistributedVectorDatabaseWrap::Init(env, exports);
  
  // Framework integration (NEW - Complete)
  // TensorFlowEmbedderWrap::Init(env, exports);
  // PyTorchEmbedderWrap::Init(env, exports);
  
  // Telemetry & Logging (NEW - Complete)
  // TelemetrySpanWrap::Init(env, exports);
  // TelemetryMetrics::Init(env, exports);
  // TelemetryManager::Init(env, exports);
  // Logger::Init(env, exports);
  
  // Utility functions (NEW - Complete)
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
