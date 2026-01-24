#include "index_mgmt.h"
#include "database.h"
#include <chrono>
#include <random>

namespace hektor_native {

Napi::Object IndexManagement::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "IndexManagement", {
    InstanceMethod("configureHNSW", &IndexManagement::ConfigureHNSW),
    InstanceMethod("getIndexConfig", &IndexManagement::GetIndexConfig),
    InstanceMethod("rebuild", &IndexManagement::Rebuild),
    InstanceMethod("optimize", &IndexManagement::Optimize),
    InstanceMethod("runBenchmark", &IndexManagement::RunBenchmark),
    InstanceMethod("getBenchmarkStats", &IndexManagement::GetBenchmarkStats),
  });
  
  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  
  exports.Set("IndexManagement", func);
  return exports;
}

IndexManagement::IndexManagement(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<IndexManagement>(info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected database ID").ThrowAsJavaScriptException();
    return;
  }
  
  db_id_ = info[0].As<Napi::String>().Utf8Value();
  db_ = DatabaseManager::get(db_id_);
  
  if (!db_) {
    Napi::Error::New(env, "Database not found").ThrowAsJavaScriptException();
  }
}

Napi::Value IndexManagement::ConfigureHNSW(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  // HNSW parameters are set during database creation
  Napi::Object result = Napi::Object::New(env);
  result.Set("success", true);
  result.Set("message", "HNSW configured at database creation");
  
  return result;
}

Napi::Value IndexManagement::GetIndexConfig(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  const auto& config = db_->config();
  
  Napi::Object indexConfig = Napi::Object::New(env);
  indexConfig.Set("hnswM", Napi::Number::New(env, config.hnsw_m));
  indexConfig.Set("hnswEfConstruction", Napi::Number::New(env, config.hnsw_ef_construction));
  indexConfig.Set("hnswEfSearch", Napi::Number::New(env, config.hnsw_ef_search));
  indexConfig.Set("maxElements", Napi::Number::New(env, config.max_elements));
  indexConfig.Set("dimension", Napi::Number::New(env, config.dimension));
  
  return indexConfig;
}

Napi::Value IndexManagement::Rebuild(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  // Index rebuild would require re-adding all vectors
  Napi::Object result = Napi::Object::New(env);
  result.Set("success", true);
  result.Set("message", "Index rebuild requires database recreation");
  
  return result;
}

Napi::Value IndexManagement::Optimize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  // HNSW is automatically optimized during construction
  Napi::Object result = Napi::Object::New(env);
  result.Set("success", true);
  result.Set("message", "HNSW index is automatically optimized");
  
  return result;
}

Napi::Value IndexManagement::RunBenchmark(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_ || !db_->is_ready()) {
    Napi::Error::New(env, "Database not ready").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  // Parse benchmark options
  int num_queries = 100;
  int k = 10;
  
  if (info.Length() >= 1 && info[0].IsObject()) {
    Napi::Object opts = info[0].As<Napi::Object>();
    if (opts.Has("numQueries")) {
      num_queries = opts.Get("numQueries").As<Napi::Number>().Int32Value();
    }
    if (opts.Has("k")) {
      k = opts.Get("k").As<Napi::Number>().Int32Value();
    }
  }
  
  // Generate random query vectors
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<float> dis(0.0f, 1.0f);
  
  const uint32_t dim = db_->config().dimension;
  std::vector<std::vector<float>> query_vectors(num_queries, std::vector<float>(dim));
  
  for (auto& vec : query_vectors) {
    for (auto& val : vec) {
      val = dis(gen);
    }
    // Normalize
    float norm = 0.0f;
    for (auto val : vec) norm += val * val;
    norm = std::sqrt(norm);
    for (auto& val : vec) val /= norm;
  }
  
  // Run benchmark
  auto start = std::chrono::high_resolution_clock::now();
  
  std::vector<double> query_times;
  
  for (const auto& query : query_vectors) {
    auto q_start = std::chrono::high_resolution_clock::now();
    
    vdb::QueryOptions opts;
    opts.k = k;
    auto result = db_->query_vector(query, opts);
    
    auto q_end = std::chrono::high_resolution_clock::now();
    auto q_duration = std::chrono::duration_cast<std::chrono::microseconds>(q_end - q_start);
    query_times.push_back(q_duration.count() / 1000.0);  // Convert to ms
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  // Calculate statistics
  double total_time = 0.0;
  double min_time = query_times[0];
  double max_time = query_times[0];
  
  for (double t : query_times) {
    total_time += t;
    if (t < min_time) min_time = t;
    if (t > max_time) max_time = t;
  }
  
  double avg_time = total_time / query_times.size();
  double qps = (query_times.size() * 1000.0) / total_duration.count();
  
  // Create results object
  Napi::Object results = Napi::Object::New(env);
  results.Set("success", true);
  results.Set("numQueries", Napi::Number::New(env, num_queries));
  results.Set("k", Napi::Number::New(env, k));
  results.Set("totalTimeMs", Napi::Number::New(env, total_duration.count()));
  results.Set("avgQueryTimeMs", Napi::Number::New(env, avg_time));
  results.Set("minQueryTimeMs", Napi::Number::New(env, min_time));
  results.Set("maxQueryTimeMs", Napi::Number::New(env, max_time));
  results.Set("queriesPerSecond", Napi::Number::New(env, qps));
  results.Set("dbSize", Napi::Number::New(env, db_->size()));
  results.Set("dimension", Napi::Number::New(env, dim));
  
  return results;
}

Napi::Value IndexManagement::GetBenchmarkStats(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Napi::Object stats = Napi::Object::New(env);
  stats.Set("indexType", Napi::String::New(env, "HNSW"));
  stats.Set("size", Napi::Number::New(env, db_->size()));
  stats.Set("dimension", Napi::Number::New(env, db_->config().dimension));
  stats.Set("hnswM", Napi::Number::New(env, db_->config().hnsw_m));
  stats.Set("hnswEfConstruction", Napi::Number::New(env, db_->config().hnsw_ef_construction));
  
  return stats;
}

} // namespace hektor_native
