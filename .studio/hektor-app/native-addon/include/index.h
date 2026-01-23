#pragma once

#include <napi.h>
#include "common.h"
#include <memory>

// Forward declarations
namespace vdb {
    class HnswIndex;
    class FlatIndex;
    namespace index {
        class MetadataIndex;
    }
}

namespace hektor_native {

// HnswIndex wrapper
class HnswIndexWrap : public Napi::ObjectWrap<HnswIndexWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    HnswIndexWrap(const Napi::CallbackInfo& info);
    
private:
    // Core operations
    Napi::Value Add(const Napi::CallbackInfo& info);
    Napi::Value AddBatch(const Napi::CallbackInfo& info);
    Napi::Value Search(const Napi::CallbackInfo& info);
    Napi::Value SearchFiltered(const Napi::CallbackInfo& info);
    Napi::Value Remove(const Napi::CallbackInfo& info);
    Napi::Value Contains(const Napi::CallbackInfo& info);
    Napi::Value GetVector(const Napi::CallbackInfo& info);
    
    // Management
    Napi::Value Size(const Napi::CallbackInfo& info);
    Napi::Value Capacity(const Napi::CallbackInfo& info);
    Napi::Value Empty(const Napi::CallbackInfo& info);
    Napi::Value Dimension(const Napi::CallbackInfo& info);
    Napi::Value Config(const Napi::CallbackInfo& info);
    Napi::Value Stats(const Napi::CallbackInfo& info);
    Napi::Value SetEfSearch(const Napi::CallbackInfo& info);
    Napi::Value Resize(const Napi::CallbackInfo& info);
    Napi::Value Optimize(const Napi::CallbackInfo& info);
    
    // Persistence
    Napi::Value Save(const Napi::CallbackInfo& info);
    static Napi::Value Load(const Napi::CallbackInfo& info);
    Napi::Value Serialize(const Napi::CallbackInfo& info);
    static Napi::Value Deserialize(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::HnswIndex> index_;
};

// FlatIndex wrapper
class FlatIndexWrap : public Napi::ObjectWrap<FlatIndexWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    FlatIndexWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Add(const Napi::CallbackInfo& info);
    Napi::Value Search(const Napi::CallbackInfo& info);
    Napi::Value Contains(const Napi::CallbackInfo& info);
    Napi::Value GetVector(const Napi::CallbackInfo& info);
    Napi::Value Size(const Napi::CallbackInfo& info);
    Napi::Value Dimension(const Napi::CallbackInfo& info);
    Napi::Value Save(const Napi::CallbackInfo& info);
    static Napi::Value Load(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::FlatIndex> index_;
};

// MetadataIndex wrapper
class MetadataIndexWrap : public Napi::ObjectWrap<MetadataIndexWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    MetadataIndexWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Add(const Napi::CallbackInfo& info);
    Napi::Value Remove(const Napi::CallbackInfo& info);
    Napi::Value Search(const Napi::CallbackInfo& info);
    Napi::Value Size(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::index::MetadataIndex> index_;
};

} // namespace hektor_native
