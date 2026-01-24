#pragma once
#include <napi.h>

namespace hektor_native {

class AsyncOperations {
public:
  // Async query vectors (non-blocking)
  static Napi::Value QueryVectorsAsync(const Napi::CallbackInfo& info);
};

// Worker class for async operations
class QueryVectorsWorker : public Napi::AsyncWorker {
public:
  QueryVectorsWorker(Napi::Function& callback, const std::vector<float>& queryVector, int topK);
  virtual ~QueryVectorsWorker() = default;
  
  void Execute() override;
  void OnOK() override;
  
private:
  std::vector<float> queryVector_;
  int topK_;
  std::vector<std::pair<std::string, float>> results_;
};

} // namespace hektor_native
