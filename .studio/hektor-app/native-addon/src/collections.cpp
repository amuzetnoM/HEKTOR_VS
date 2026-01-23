#include "collections.h"
#include <filesystem>
#include <iostream>

namespace hektor_native {

namespace fs = std::filesystem;

Napi::Object Collections::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Collections", {
    InstanceMethod("create", &Collections::Create),
    InstanceMethod("list", &Collections::List),
    InstanceMethod("get", &Collections::Get),
    InstanceMethod("delete", &Collections::Delete),
    InstanceMethod("count", &Collections::Count),
  });
  
  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  
  exports.Set("Collections", func);
  return exports;
}

Collections::Collections(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Collections>(info) {
  Napi::Env env = info.Env();
  
  if (info.Length() >= 1 && info[0].IsString()) {
    base_path_ = info[0].As<Napi::String>().Utf8Value();
  } else {
    base_path_ = "./hektor_collections";
  }
  
  // Create base directory
  try {
    fs::create_directories(base_path_);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to create collections directory: ") + e.what())
      .ThrowAsJavaScriptException();
  }
}

Napi::Value Collections::Create(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected collection name").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string name = info[0].As<Napi::String>().Utf8Value();
  fs::path collection_path = fs::path(base_path_) / name;
  
  try {
    if (fs::exists(collection_path)) {
      Napi::Object result = Napi::Object::New(env);
      result.Set("success", false);
      result.Set("error", "Collection already exists");
      return result;
    }
    
    fs::create_directories(collection_path);
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("success", true);
    result.Set("name", name);
    result.Set("path", collection_path.string());
    
    return result;
    
  } catch (const std::exception& e) {
    Napi::Object result = Napi::Object::New(env);
    result.Set("success", false);
    result.Set("error", e.what());
    return result;
  }
}

Napi::Value Collections::List(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  try {
    Napi::Array collections = Napi::Array::New(env);
    uint32_t idx = 0;
    
    for (const auto& entry : fs::directory_iterator(base_path_)) {
      if (entry.is_directory()) {
        Napi::Object coll = Napi::Object::New(env);
        coll.Set("name", entry.path().filename().string());
        coll.Set("path", entry.path().string());
        
        // Count files in collection
        size_t file_count = 0;
        for (const auto& file : fs::recursive_directory_iterator(entry.path())) {
          if (file.is_regular_file()) file_count++;
        }
        coll.Set("fileCount", Napi::Number::New(env, file_count));
        
        collections.Set(idx++, coll);
      }
    }
    
    return collections;
    
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to list collections: ") + e.what())
      .ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value Collections::Get(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected collection name").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string name = info[0].As<Napi::String>().Utf8Value();
  fs::path collection_path = fs::path(base_path_) / name;
  
  if (!fs::exists(collection_path)) {
    return env.Null();
  }
  
  Napi::Object coll = Napi::Object::New(env);
  coll.Set("name", name);
  coll.Set("path", collection_path.string());
  coll.Set("exists", true);
  
  return coll;
}

Napi::Value Collections::Delete(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected collection name").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string name = info[0].As<Napi::String>().Utf8Value();
  fs::path collection_path = fs::path(base_path_) / name;
  
  try {
    if (!fs::exists(collection_path)) {
      Napi::Object result = Napi::Object::New(env);
      result.Set("success", false);
      result.Set("error", "Collection not found");
      return result;
    }
    
    fs::remove_all(collection_path);
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("success", true);
    result.Set("message", "Collection deleted");
    
    return result;
    
  } catch (const std::exception& e) {
    Napi::Object result = Napi::Object::New(env);
    result.Set("success", false);
    result.Set("error", e.what());
    return result;
  }
}

Napi::Value Collections::Count(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  try {
    size_t count = 0;
    for (const auto& entry : fs::directory_iterator(base_path_)) {
      if (entry.is_directory()) count++;
    }
    
    return Napi::Number::New(env, count);
    
  } catch (const std::exception& e) {
    return Napi::Number::New(env, 0);
  }
}

} // namespace hektor_native
