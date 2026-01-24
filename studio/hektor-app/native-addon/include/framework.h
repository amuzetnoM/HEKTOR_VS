#pragma once

#include <napi.h>
#include "common.h"
#include <memory>

// Forward declarations
namespace vdb {
    namespace framework {
        class TensorFlowEmbedder;
        class PyTorchEmbedder;
    }
}

namespace hektor_native {

// TensorFlowEmbedder wrapper
class TensorFlowEmbedderWrap : public Napi::ObjectWrap<TensorFlowEmbedderWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    TensorFlowEmbedderWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Embed(const Napi::CallbackInfo& info);
    Napi::Value EmbedBatch(const Napi::CallbackInfo& info);
    Napi::Value Dimension(const Napi::CallbackInfo& info);
    Napi::Value IsLoaded(const Napi::CallbackInfo& info);
    
    // Static methods
    static Napi::Value ExportForTraining(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::framework::TensorFlowEmbedder> embedder_;
};

// PyTorchEmbedder wrapper
class PyTorchEmbedderWrap : public Napi::ObjectWrap<PyTorchEmbedderWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    PyTorchEmbedderWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Embed(const Napi::CallbackInfo& info);
    Napi::Value EmbedBatch(const Napi::CallbackInfo& info);
    Napi::Value Dimension(const Napi::CallbackInfo& info);
    Napi::Value IsLoaded(const Napi::CallbackInfo& info);
    Napi::Value Device(const Napi::CallbackInfo& info);
    
    // Static methods
    static Napi::Value ExportForTraining(const Napi::CallbackInfo& info);
    static Napi::Value FromTrained(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::framework::PyTorchEmbedder> embedder_;
};

} // namespace hektor_native
