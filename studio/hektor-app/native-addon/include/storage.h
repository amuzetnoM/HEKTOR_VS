#pragma once

#include <napi.h>
#include "common.h"
#include <memory>

// Forward declarations
namespace vdb {
    namespace storage {
        class MemoryMappedFile;
        class VectorStore;
        class MetadataStore;
        class SqliteStore;
        class PgVectorStore;
    }
}

namespace hektor_native {

// MemoryMappedFile wrapper
class MemoryMappedFileWrap : public Napi::ObjectWrap<MemoryMappedFileWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    MemoryMappedFileWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value OpenRead(const Napi::CallbackInfo& info);
    Napi::Value OpenWrite(const Napi::CallbackInfo& info);
    Napi::Value Close(const Napi::CallbackInfo& info);
    Napi::Value IsOpen(const Napi::CallbackInfo& info);
    Napi::Value Size(const Napi::CallbackInfo& info);
    Napi::Value Resize(const Napi::CallbackInfo& info);
    Napi::Value Sync(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::storage::MemoryMappedFile> mmap_;
};

// VectorStore wrapper
class VectorStoreWrap : public Napi::ObjectWrap<VectorStoreWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    VectorStoreWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Init(const Napi::CallbackInfo& info);
    Napi::Value Add(const Napi::CallbackInfo& info);
    Napi::Value Get(const Napi::CallbackInfo& info);
    Napi::Value Contains(const Napi::CallbackInfo& info);
    Napi::Value Remove(const Napi::CallbackInfo& info);
    Napi::Value AllIds(const Napi::CallbackInfo& info);
    Napi::Value Size(const Napi::CallbackInfo& info);
    Napi::Value Capacity(const Napi::CallbackInfo& info);
    Napi::Value Sync(const Napi::CallbackInfo& info);
    Napi::Value Compact(const Napi::CallbackInfo& info);
    Napi::Value MemoryUsage(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::storage::VectorStore> store_;
};

// MetadataStore wrapper
class MetadataStoreWrap : public Napi::ObjectWrap<MetadataStoreWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    MetadataStoreWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Init(const Napi::CallbackInfo& info);
    Napi::Value Add(const Napi::CallbackInfo& info);
    Napi::Value Update(const Napi::CallbackInfo& info);
    Napi::Value Get(const Napi::CallbackInfo& info);
    Napi::Value All(const Napi::CallbackInfo& info);
    Napi::Value FindByDate(const Napi::CallbackInfo& info);
    Napi::Value FindByType(const Napi::CallbackInfo& info);
    Napi::Value FindByAsset(const Napi::CallbackInfo& info);
    Napi::Value Remove(const Napi::CallbackInfo& info);
    Napi::Value Size(const Napi::CallbackInfo& info);
    Napi::Value Sync(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::storage::MetadataStore> store_;
};

// SqliteStore wrapper
class SqliteStoreWrap : public Napi::ObjectWrap<SqliteStoreWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    SqliteStoreWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Init(const Napi::CallbackInfo& info);
    Napi::Value Close(const Napi::CallbackInfo& info);
    
    // Metadata operations
    Napi::Value StoreMetadata(const Napi::CallbackInfo& info);
    Napi::Value GetMetadata(const Napi::CallbackInfo& info);
    Napi::Value UpdateMetadata(const Napi::CallbackInfo& info);
    Napi::Value DeleteMetadata(const Napi::CallbackInfo& info);
    Napi::Value ListMetadata(const Napi::CallbackInfo& info);
    
    // Cache operations
    Napi::Value CachePut(const Napi::CallbackInfo& info);
    Napi::Value CacheGet(const Napi::CallbackInfo& info);
    Napi::Value CacheDelete(const Napi::CallbackInfo& info);
    Napi::Value CacheClear(const Napi::CallbackInfo& info);
    Napi::Value CacheSize(const Napi::CallbackInfo& info);
    
    // Config operations
    Napi::Value ConfigSet(const Napi::CallbackInfo& info);
    Napi::Value ConfigGet(const Napi::CallbackInfo& info);
    Napi::Value ConfigDelete(const Napi::CallbackInfo& info);
    Napi::Value ConfigList(const Napi::CallbackInfo& info);
    
    // Maintenance
    Napi::Value Vacuum(const Napi::CallbackInfo& info);
    Napi::Value EvictExpiredCache(const Napi::CallbackInfo& info);
    Napi::Value GetStats(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::storage::SqliteStore> store_;
};

// PgVectorStore wrapper
class PgVectorStoreWrap : public Napi::ObjectWrap<PgVectorStoreWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    PgVectorStoreWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Init(const Napi::CallbackInfo& info);
    Napi::Value Close(const Napi::CallbackInfo& info);
    Napi::Value Add(const Napi::CallbackInfo& info);
    Napi::Value AddBatch(const Napi::CallbackInfo& info);
    Napi::Value Remove(const Napi::CallbackInfo& info);
    Napi::Value RemoveBatch(const Napi::CallbackInfo& info);
    Napi::Value Get(const Napi::CallbackInfo& info);
    Napi::Value GetMetadata(const Napi::CallbackInfo& info);
    Napi::Value UpdateMetadata(const Napi::CallbackInfo& info);
    Napi::Value Search(const Napi::CallbackInfo& info);
    Napi::Value Count(const Napi::CallbackInfo& info);
    Napi::Value SizeBytes(const Napi::CallbackInfo& info);
    Napi::Value Sync(const Napi::CallbackInfo& info);
    Napi::Value Vacuum(const Napi::CallbackInfo& info);
    Napi::Value CreateIndex(const Napi::CallbackInfo& info);
    Napi::Value DropIndex(const Napi::CallbackInfo& info);
    Napi::Value Reindex(const Napi::CallbackInfo& info);
    Napi::Value IsConnected(const Napi::CallbackInfo& info);
    Napi::Value Reconnect(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::storage::PgVectorStore> store_;
};

} // namespace hektor_native
