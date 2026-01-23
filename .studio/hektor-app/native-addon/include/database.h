#pragma once
#include <napi.h>
#include <string>
#include <memory>
#include <map>
#include "vdb/database.hpp"

namespace hektor_native {

// Global database instance manager
class DatabaseManager {
public:
  static std::shared_ptr<vdb::VectorDatabase> get(const std::string& id);
  static void set(const std::string& id, std::shared_ptr<vdb::VectorDatabase> db);
  static void remove(const std::string& id);
  
private:
  static std::map<std::string, std::shared_ptr<vdb::VectorDatabase>> databases_;
};

class Database : public Napi::ObjectWrap<Database> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Database(const Napi::CallbackInfo& info);
  
private:
  // Core database operations
  Napi::Value Initialize(const Napi::CallbackInfo& info);
  Napi::Value Close(const Napi::CallbackInfo& info);
  Napi::Value GetConfig(const Napi::CallbackInfo& info);
  Napi::Value IsReady(const Napi::CallbackInfo& info);
  
  // Collection management
  Napi::Value CreateCollection(const Napi::CallbackInfo& info);
  Napi::Value ListCollections(const Napi::CallbackInfo& info);
  Napi::Value DeleteCollection(const Napi::CallbackInfo& info);
  
  // Document operations
  Napi::Value AddDocument(const Napi::CallbackInfo& info);
  Napi::Value AddDocuments(const Napi::CallbackInfo& info);
  Napi::Value GetDocument(const Napi::CallbackInfo& info);
  Napi::Value UpdateDocument(const Napi::CallbackInfo& info);
  Napi::Value DeleteDocument(const Napi::CallbackInfo& info);
  
  // Vector operations
  Napi::Value AddVector(const Napi::CallbackInfo& info);
  Napi::Value AddVectors(const Napi::CallbackInfo& info);
  Napi::Value GetVector(const Napi::CallbackInfo& info);
  
  // Statistics
  Napi::Value GetSize(const Napi::CallbackInfo& info);
  Napi::Value GetStats(const Napi::CallbackInfo& info);
  
  std::shared_ptr<vdb::VectorDatabase> db_;
  std::string db_id_;
};

// Helper functions
Napi::Object MetadataToJS(Napi::Env env, const vdb::Metadata& meta);
vdb::Metadata JSToMetadata(const Napi::Object& obj);
Napi::Array VectorToJS(Napi::Env env, const std::vector<float>& vec);
std::vector<float> JSToVector(const Napi::Array& arr);

} // namespace hektor_native
