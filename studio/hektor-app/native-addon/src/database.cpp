#include "database.h"
#include <iostream>
#include <sstream>

namespace hektor_native {

// Static database manager
std::map<std::string, std::shared_ptr<vdb::VectorDatabase>> DatabaseManager::databases_;

std::shared_ptr<vdb::VectorDatabase> DatabaseManager::get(const std::string& id) {
  auto it = databases_.find(id);
  return (it != databases_.end()) ? it->second : nullptr;
}

void DatabaseManager::set(const std::string& id, std::shared_ptr<vdb::VectorDatabase> db) {
  databases_[id] = db;
}

void DatabaseManager::remove(const std::string& id) {
  databases_.erase(id);
}

// Helper: Convert metadata to JS object
Napi::Object MetadataToJS(Napi::Env env, const vdb::Metadata& meta) {
  Napi::Object obj = Napi::Object::New(env);
  obj.Set("id", Napi::Number::New(env, meta.id));
  obj.Set("date", Napi::String::New(env, meta.date));
  obj.Set("type", Napi::Number::New(env, static_cast<int>(meta.type)));
  obj.Set("sourceFile", Napi::String::New(env, meta.source_file));
  obj.Set("asset", Napi::String::New(env, meta.asset));
  obj.Set("bias", Napi::String::New(env, meta.bias));
  
  if (meta.gold_price) obj.Set("goldPrice", Napi::Number::New(env, *meta.gold_price));
  if (meta.silver_price) obj.Set("silverPrice", Napi::Number::New(env, *meta.silver_price));
  if (meta.gsr) obj.Set("gsr", Napi::Number::New(env, *meta.gsr));
  if (meta.dxy) obj.Set("dxy", Napi::Number::New(env, *meta.dxy));
  if (meta.vix) obj.Set("vix", Napi::Number::New(env, *meta.vix));
  if (meta.yield_10y) obj.Set("yield10y", Napi::Number::New(env, *meta.yield_10y));
  
  return obj;
}

// Helper: Convert JS object to metadata
vdb::Metadata JSToMetadata(const Napi::Object& obj) {
  vdb::Metadata meta;
  
  if (obj.Has("id")) meta.id = obj.Get("id").As<Napi::Number>().Uint32Value();
  if (obj.Has("date")) meta.date = obj.Get("date").As<Napi::String>().Utf8Value();
  if (obj.Has("type")) meta.type = static_cast<vdb::DocumentType>(obj.Get("type").As<Napi::Number>().Uint32Value());
  if (obj.Has("sourceFile")) meta.source_file = obj.Get("sourceFile").As<Napi::String>().Utf8Value();
  if (obj.Has("asset")) meta.asset = obj.Get("asset").As<Napi::String>().Utf8Value();
  if (obj.Has("bias")) meta.bias = obj.Get("bias").As<Napi::String>().Utf8Value();
  
  if (obj.Has("goldPrice")) meta.gold_price = obj.Get("goldPrice").As<Napi::Number>().FloatValue();
  if (obj.Has("silverPrice")) meta.silver_price = obj.Get("silverPrice").As<Napi::Number>().FloatValue();
  if (obj.Has("gsr")) meta.gsr = obj.Get("gsr").As<Napi::Number>().FloatValue();
  if (obj.Has("dxy")) meta.dxy = obj.Get("dxy").As<Napi::Number>().FloatValue();
  if (obj.Has("vix")) meta.vix = obj.Get("vix").As<Napi::Number>().FloatValue();
  if (obj.Has("yield10y")) meta.yield_10y = obj.Get("yield10y").As<Napi::Number>().FloatValue();
  
  return meta;
}

// Helper: Convert vector to JS array (overload for vdb::Vector)
Napi::Array VectorToJS(Napi::Env env, const vdb::Vector& vec) {
  Napi::Array arr = Napi::Array::New(env, vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    arr.Set(static_cast<uint32_t>(i), Napi::Number::New(env, vec[i]));
  }
  return arr;
}

// Helper: Convert vector to JS array
Napi::Array VectorToJS(Napi::Env env, const std::vector<float>& vec) {
  Napi::Array arr = Napi::Array::New(env, vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    arr.Set(static_cast<uint32_t>(i), Napi::Number::New(env, vec[i]));
  }
  return arr;
}

// Helper: Convert JS array to vector
std::vector<float> JSToVector(const Napi::Array& arr) {
  std::vector<float> vec;
  vec.reserve(arr.Length());
  for (uint32_t i = 0; i < arr.Length(); i++) {
    vec.push_back(arr.Get(i).As<Napi::Number>().FloatValue());
  }
  return vec;
}

// Database class implementation
Napi::Object Database::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Database", {
    InstanceMethod("initialize", &Database::Initialize),
    InstanceMethod("close", &Database::Close),
    InstanceMethod("getConfig", &Database::GetConfig),
    InstanceMethod("isReady", &Database::IsReady),
    InstanceMethod("createCollection", &Database::CreateCollection),
    InstanceMethod("listCollections", &Database::ListCollections),
    InstanceMethod("deleteCollection", &Database::DeleteCollection),
    InstanceMethod("addDocument", &Database::AddDocument),
    InstanceMethod("addDocuments", &Database::AddDocuments),
    InstanceMethod("getDocument", &Database::GetDocument),
    InstanceMethod("updateDocument", &Database::UpdateDocument),
    InstanceMethod("deleteDocument", &Database::DeleteDocument),
    InstanceMethod("addVector", &Database::AddVector),
    InstanceMethod("addVectors", &Database::AddVectors),
    InstanceMethod("getVector", &Database::GetVector),
    InstanceMethod("getSize", &Database::GetSize),
    InstanceMethod("getStats", &Database::GetStats),
  });
  
  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);
  
  exports.Set("Database", func);
  return exports;
}

Database::Database(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Database>(info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::TypeError::New(env, "Expected config object").ThrowAsJavaScriptException();
    return;
  }
  
  Napi::Object config = info[0].As<Napi::Object>();
  
  // Parse configuration
  vdb::DatabaseConfig dbConfig;
  
  if (config.Has("path")) {
    dbConfig.path = config.Get("path").As<Napi::String>().Utf8Value();
  }
  
  if (config.Has("dimension")) {
    dbConfig.dimension = config.Get("dimension").As<Napi::Number>().Uint32Value();
  }
  
  if (config.Has("metric")) {
    std::string metric = config.Get("metric").As<Napi::String>().Utf8Value();
    if (metric == "cosine") dbConfig.metric = vdb::DistanceMetric::Cosine;
    else if (metric == "euclidean" || metric == "l2") dbConfig.metric = vdb::DistanceMetric::L2;
    else if (metric == "dot") dbConfig.metric = vdb::DistanceMetric::DotProduct;
  }
  
  if (config.Has("hnswM")) {
    dbConfig.hnsw_m = config.Get("hnswM").As<Napi::Number>().Uint32Value();
  }
  
  if (config.Has("hnswEfConstruction")) {
    dbConfig.hnsw_ef_construction = config.Get("hnswEfConstruction").As<Napi::Number>().Uint32Value();
  }
  
  if (config.Has("hnswEfSearch")) {
    dbConfig.hnsw_ef_search = config.Get("hnswEfSearch").As<Napi::Number>().Uint32Value();
  }
  
  if (config.Has("maxElements")) {
    dbConfig.max_elements = config.Get("maxElements").As<Napi::Number>().Uint32Value();
  }
  
  // Create database
  try {
    db_ = std::make_shared<vdb::VectorDatabase>(dbConfig);
    
    // Generate unique ID
    static size_t id_counter = 0;
    db_id_ = "db_" + std::to_string(id_counter++);
    DatabaseManager::set(db_id_, db_);
    
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to create database: ") + e.what()).ThrowAsJavaScriptException();
  }
}

Napi::Value Database::Initialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not created").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  auto result = db_->init();
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    response.Set("message", "Database initialized successfully");
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Database::Close(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  DatabaseManager::remove(db_id_);
  db_.reset();
  
  Napi::Object result = Napi::Object::New(env);
  result.Set("success", true);
  result.Set("message", "Database closed successfully");
  
  return result;
}

Napi::Value Database::GetConfig(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  const auto& config = db_->config();
  Napi::Object obj = Napi::Object::New(env);
  
  obj.Set("path", Napi::String::New(env, config.path.string()));
  obj.Set("dimension", Napi::Number::New(env, config.dimension));
  obj.Set("hnswM", Napi::Number::New(env, config.hnsw_m));
  obj.Set("hnswEfConstruction", Napi::Number::New(env, config.hnsw_ef_construction));
  obj.Set("hnswEfSearch", Napi::Number::New(env, config.hnsw_ef_search));
  obj.Set("maxElements", Napi::Number::New(env, config.max_elements));
  
  return obj;
}

Napi::Value Database::IsReady(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, db_ && db_->is_ready());
}

Napi::Value Database::CreateCollection(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  // Collections are file-system based in HEKTOR
  Napi::Object result = Napi::Object::New(env);
  result.Set("success", true);
  result.Set("message", "Collection management is automatic in HEKTOR");
  
  return result;
}

Napi::Value Database::ListCollections(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  Napi::Array collections = Napi::Array::New(env);
  collections.Set(0u, Napi::String::New(env, "default"));
  
  return collections;
}

Napi::Value Database::DeleteCollection(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  Napi::Object result = Napi::Object::New(env);
  result.Set("success", true);
  
  return result;
}

Napi::Value Database::AddDocument(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_ || !db_->is_ready()) {
    Napi::Error::New(env, "Database not ready").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::TypeError::New(env, "Expected document object").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Napi::Object doc = info[0].As<Napi::Object>();
  
  // Extract text and metadata
  std::string text = doc.Get("text").As<Napi::String>().Utf8Value();
  vdb::Metadata metadata = JSToMetadata(doc.Has("metadata") ? 
    doc.Get("metadata").As<Napi::Object>() : Napi::Object::New(env));
  
  // Add to database
  auto result = db_->add_text(text, metadata);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    response.Set("id", Napi::Number::New(env, result.value()));
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Database::AddDocuments(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_ || !db_->is_ready()) {
    Napi::Error::New(env, "Database not ready").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "Expected array of documents").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Napi::Array docs = info[0].As<Napi::Array>();
  std::vector<std::string> texts;
  std::vector<vdb::Metadata> metadata_list;
  
  for (uint32_t i = 0; i < docs.Length(); i++) {
    Napi::Object doc = docs.Get(i).As<Napi::Object>();
    texts.push_back(doc.Get("text").As<Napi::String>().Utf8Value());
    metadata_list.push_back(JSToMetadata(doc.Has("metadata") ? 
      doc.Get("metadata").As<Napi::Object>() : Napi::Object::New(env)));
  }
  
  // Batch add
  auto result = db_->add_texts(texts, metadata_list);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    Napi::Array ids = Napi::Array::New(env, result.value().size());
    for (size_t i = 0; i < result.value().size(); i++) {
      ids.Set(static_cast<uint32_t>(i), Napi::Number::New(env, result.value()[i]));
    }
    response.Set("success", true);
    response.Set("ids", ids);
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Database::GetDocument(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Expected document ID").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  uint64_t id = info[0].As<Napi::Number>().Int64Value();
  auto metadata = db_->get_metadata(id);
  
  if (metadata) {
    return MetadataToJS(env, *metadata);
  }
  
  return env.Null();
}

Napi::Value Database::UpdateDocument(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Expected ID and metadata").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  uint64_t id = info[0].As<Napi::Number>().Int64Value();
  vdb::Metadata metadata = JSToMetadata(info[1].As<Napi::Object>());
  
  auto result = db_->update_metadata(id, metadata);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Database::DeleteDocument(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Expected document ID").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  uint64_t id = info[0].As<Napi::Number>().Int64Value();
  auto result = db_->remove(id);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Database::AddVector(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_ || !db_->is_ready()) {
    Napi::Error::New(env, "Database not ready").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 2 || !info[0].IsArray() || !info[1].IsObject()) {
    Napi::TypeError::New(env, "Expected vector array and metadata object").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::vector<float> vector = JSToVector(info[0].As<Napi::Array>());
  vdb::Metadata metadata = JSToMetadata(info[1].As<Napi::Object>());
  
  auto result = db_->add_vector(vector, metadata);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    response.Set("id", Napi::Number::New(env, result.value()));
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Database::AddVectors(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_ || !db_->is_ready()) {
    Napi::Error::New(env, "Database not ready").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 2 || !info[0].IsArray() || !info[1].IsArray()) {
    Napi::TypeError::New(env, "Expected arrays of vectors and metadata").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Napi::Array vectors_arr = info[0].As<Napi::Array>();
  Napi::Array metadata_arr = info[1].As<Napi::Array>();
  
  std::vector<uint64_t> ids;
  
  for (uint32_t i = 0; i < vectors_arr.Length(); i++) {
    std::vector<float> vector = JSToVector(vectors_arr.Get(i).As<Napi::Array>());
    vdb::Metadata metadata = JSToMetadata(metadata_arr.Get(i).As<Napi::Object>());
    
    auto result = db_->add_vector(vector, metadata);
    if (result.has_value()) {
      ids.push_back(result.value());
    }
  }
  
  Napi::Array ids_arr = Napi::Array::New(env, ids.size());
  for (size_t i = 0; i < ids.size(); i++) {
    ids_arr.Set(static_cast<uint32_t>(i), Napi::Number::New(env, ids[i]));
  }
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", true);
  response.Set("ids", ids_arr);
  
  return response;
}

Napi::Value Database::GetVector(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    Napi::Error::New(env, "Database not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Expected vector ID").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  uint64_t id = info[0].As<Napi::Number>().Int64Value();
  auto vector = db_->get_vector(id);
  
  if (vector) {
    return VectorToJS(env, *vector);
  }
  
  return env.Null();
}

Napi::Value Database::GetSize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!db_) {
    return Napi::Number::New(env, 0);
  }
  
  return Napi::Number::New(env, db_->size());
}

Napi::Value Database::GetStats(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  Napi::Object stats = Napi::Object::New(env);
  
  if (db_) {
    stats.Set("size", Napi::Number::New(env, db_->size()));
    stats.Set("dimension", Napi::Number::New(env, db_->config().dimension));
    stats.Set("ready", Napi::Boolean::New(env, db_->is_ready()));
  } else {
    stats.Set("size", Napi::Number::New(env, 0));
    stats.Set("ready", Napi::Boolean::New(env, false));
  }
  
  return stats;
}

} // namespace hektor_native
