#include "async_operations.h"
#include <thread>
#include <chrono>

namespace hektor_native {

QueryVectorsWorker::QueryVectorsWorker(Napi::Function& callback, const std::vector<float>& queryVector, int topK)
    : Napi::AsyncWorker(callback), queryVector_(queryVector), topK_(topK) {}

void QueryVectorsWorker::Execute() {
  // TODO: Integrate with actual HEKTOR VDB
  // Simulate async query operation
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  // Mock results
  for (int i = 0; i < topK_; i++) {
    results_.push_back({"vec_" + std::to_string(i), 0.1f * (i + 1)});
  }
}

void QueryVectorsWorker::OnOK() {
  Napi::Env env = Env();
  Napi::Array results = Napi::Array::New(env, results_.size());
  
  for (size_t i = 0; i < results_.size(); i++) {
    Napi::Object result = Napi::Object::New(env);
    result.Set("id", results_[i].first);
    result.Set("distance", results_[i].second);
    results[i] = result;
  }
  
  Callback().Call({env.Null(), results});
}

Napi::Value AsyncOperations::QueryVectorsAsync(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 3 || !info[0].IsArray() || !info[1].IsNumber() || !info[2].IsFunction()) {
    Napi::TypeError::New(env, "Expected (vector: Array, topK: Number, callback: Function)").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  // Parse query vector
  Napi::Array vectorArray = info[0].As<Napi::Array>();
  std::vector<float> queryVector;
  for (uint32_t i = 0; i < vectorArray.Length(); i++) {
    queryVector.push_back(vectorArray.Get(i).As<Napi::Number>().FloatValue());
  }
  
  int topK = info[1].As<Napi::Number>().Int32Value();
  Napi::Function callback = info[2].As<Napi::Function>();
  
  QueryVectorsWorker* worker = new QueryVectorsWorker(callback, queryVector, topK);
  worker->Queue();
  
  return env.Undefined();
}

} // namespace hektor_native
