#include "quantization.h"
#include "database.h"
#include "vdb/quantization/product_quantizer.hpp"
#include "vdb/quantization/scalar_quantizer.hpp"
#include "vdb/quantization/perceptual_curves.hpp"
#include "vdb/quantization/adaptive_quantizer.hpp"

namespace hektor_native {

// Helper to convert JS array to vdb::Vector
static vdb::Vector JSArrayToVector(const Napi::Array& arr) {
  vdb::Vector vec(static_cast<vdb::Dim>(arr.Length()));
  for (uint32_t i = 0; i < arr.Length(); i++) {
    vec[i] = arr.Get(i).As<Napi::Number>().FloatValue();
  }
  return vec;
}

// Helper to convert vdb::Vector to JS array
static Napi::Array VectorToJSArray(Napi::Env env, const vdb::Vector& vec) {
  Napi::Array arr = Napi::Array::New(env, vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    arr.Set(static_cast<uint32_t>(i), Napi::Number::New(env, vec[i]));
  }
  return arr;
}

Napi::Object Quantization::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Quantization", {
    InstanceMethod("applyPQ", &Quantization::ApplyPQ),
    InstanceMethod("applySQ", &Quantization::ApplySQ),
    InstanceMethod("getQuantizationInfo", &Quantization::GetQuantizationInfo),
    // Perceptual curve methods
    InstanceMethod("encodePQ", &Quantization::EncodePQ),
    InstanceMethod("decodePQ", &Quantization::DecodePQ),
    InstanceMethod("encodeHLG", &Quantization::EncodeHLG),
    InstanceMethod("decodeHLG", &Quantization::DecodeHLG),
    InstanceMethod("encodeGamma", &Quantization::EncodeGamma),
    InstanceMethod("decodeGamma", &Quantization::DecodeGamma),
    // Display-aware quantization
    InstanceMethod("createDisplayProfile", &Quantization::CreateDisplayProfile),
    InstanceMethod("quantizeForDisplay", &Quantization::QuantizeForDisplay),
    // HDR metadata
    InstanceMethod("getHDRMetadata", &Quantization::GetHDRMetadata),
    InstanceMethod("getST2084Constants", &Quantization::GetST2084Constants),
  });
  
  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  
  exports.Set("Quantization", func);
  return exports;
}

Quantization::Quantization(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<Quantization>(info) {
  Napi::Env env = info.Env();
  
  if (info.Length() >= 1 && info[0].IsString()) {
    db_id_ = info[0].As<Napi::String>().Utf8Value();
    db_ = DatabaseManager::get(db_id_);
  }
  // Allow construction without database for standalone quantization ops
}

Napi::Value Quantization::ApplyPQ(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 2 || !info[0].IsArray() || !info[1].IsObject()) {
    Napi::TypeError::New(env, "Expected training vectors array and config object").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Napi::Array trainingData = info[0].As<Napi::Array>();
  Napi::Object config = info[1].As<Napi::Object>();
  
  // Parse config
  vdb::quantization::ProductQuantizerConfig pqConfig;
  if (config.Has("dimension")) {
    pqConfig.dimension = config.Get("dimension").As<Napi::Number>().Uint32Value();
  }
  if (config.Has("numSubquantizers")) {
    pqConfig.num_subquantizers = config.Get("numSubquantizers").As<Napi::Number>().Uint32Value();
  }
  if (config.Has("numCentroids")) {
    pqConfig.num_centroids = config.Get("numCentroids").As<Napi::Number>().Uint32Value();
  }
  if (config.Has("numIterations")) {
    pqConfig.num_iterations = config.Get("numIterations").As<Napi::Number>().Uint32Value();
  }
  
  // Convert training data
  std::vector<vdb::Vector> vectors;
  vectors.reserve(trainingData.Length());
  for (uint32_t i = 0; i < trainingData.Length(); i++) {
    vectors.push_back(JSArrayToVector(trainingData.Get(i).As<Napi::Array>()));
  }
  
  // Create and train quantizer
  vdb::quantization::ProductQuantizer pq(pqConfig);
  auto result = pq.train(vectors);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    response.Set("message", "Product Quantization trained successfully");
    response.Set("compressionRatio", Napi::Number::New(env, pq.compression_ratio()));
    response.Set("codeSize", Napi::Number::New(env, pq.code_size()));
    response.Set("dimension", Napi::Number::New(env, pq.dimension()));
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Quantization::ApplySQ(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 2 || !info[0].IsArray() || !info[1].IsObject()) {
    Napi::TypeError::New(env, "Expected training vectors array and config object").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Napi::Array trainingData = info[0].As<Napi::Array>();
  Napi::Object config = info[1].As<Napi::Object>();
  
  // Parse config
  vdb::quantization::ScalarQuantizerConfig sqConfig;
  if (config.Has("dimension")) {
    sqConfig.dimension = config.Get("dimension").As<Napi::Number>().Uint32Value();
  }
  if (config.Has("perDimension")) {
    sqConfig.per_dimension = config.Get("perDimension").As<Napi::Boolean>().Value();
  }
  
  // Convert training data
  std::vector<vdb::Vector> vectors;
  vectors.reserve(trainingData.Length());
  for (uint32_t i = 0; i < trainingData.Length(); i++) {
    vectors.push_back(JSArrayToVector(trainingData.Get(i).As<Napi::Array>()));
  }
  
  // Create and train quantizer
  vdb::quantization::ScalarQuantizer sq(sqConfig);
  auto result = sq.train(vectors);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    response.Set("message", "Scalar Quantization trained successfully");
    response.Set("compressionRatio", Napi::Number::New(env, sq.compression_ratio()));
    response.Set("codeSize", Napi::Number::New(env, static_cast<double>(sq.code_size())));
    response.Set("perDimension", Napi::Boolean::New(env, sqConfig.per_dimension));
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value Quantization::GetQuantizationInfo(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  Napi::Object qinfo = Napi::Object::New(env);
  qinfo.Set("enabled", Napi::Boolean::New(env, true));
  
  Napi::Array supportedTypes = Napi::Array::New(env, 4);
  supportedTypes.Set(0u, Napi::String::New(env, "ProductQuantization"));
  supportedTypes.Set(1u, Napi::String::New(env, "ScalarQuantization"));
  supportedTypes.Set(2u, Napi::String::New(env, "PerceptualQuantization"));
  supportedTypes.Set(3u, Napi::String::New(env, "DisplayAwareQuantization"));
  qinfo.Set("supportedTypes", supportedTypes);
  
  // Perceptual curves info
  Napi::Object curves = Napi::Object::New(env);
  curves.Set("PQ_ST2084", Napi::String::New(env, "SMPTE ST 2084 (Dolby Vision, HDR10)"));
  curves.Set("HLG_Rec2100", Napi::String::New(env, "Hybrid Log-Gamma (BBC/NHK HDR)"));
  curves.Set("Gamma22", Napi::String::New(env, "sRGB/Rec.709 gamma 2.2"));
  curves.Set("Gamma24", Napi::String::New(env, "DCI-P3 gamma 2.4"));
  qinfo.Set("perceptualCurves", curves);
  
  return qinfo;
}

// ============================================================================
// Perceptual Quantization Methods - SMPTE ST 2084 (PQ)
// ============================================================================

Napi::Value Quantization::EncodePQ(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected luminance value or array").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info[0].IsNumber()) {
    // Single value
    float luminance = info[0].As<Napi::Number>().FloatValue();
    float encoded = vdb::quantization::PQCurve::encode(luminance);
    return Napi::Number::New(env, encoded);
  } else if (info[0].IsArray()) {
    // Batch encode
    Napi::Array input = info[0].As<Napi::Array>();
    std::vector<float> values;
    values.reserve(input.Length());
    for (uint32_t i = 0; i < input.Length(); i++) {
      values.push_back(input.Get(i).As<Napi::Number>().FloatValue());
    }
    
    auto encoded = vdb::quantization::PQCurve::encode_batch(values);
    
    Napi::Array result = Napi::Array::New(env, encoded.size());
    for (size_t i = 0; i < encoded.size(); i++) {
      result.Set(static_cast<uint32_t>(i), Napi::Number::New(env, encoded[i]));
    }
    return result;
  }
  
  Napi::TypeError::New(env, "Expected number or array").ThrowAsJavaScriptException();
  return env.Null();
}

Napi::Value Quantization::DecodePQ(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected PQ-encoded value or array").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info[0].IsNumber()) {
    float pqValue = info[0].As<Napi::Number>().FloatValue();
    float decoded = vdb::quantization::PQCurve::decode(pqValue);
    return Napi::Number::New(env, decoded);
  } else if (info[0].IsArray()) {
    Napi::Array input = info[0].As<Napi::Array>();
    std::vector<float> values;
    values.reserve(input.Length());
    for (uint32_t i = 0; i < input.Length(); i++) {
      values.push_back(input.Get(i).As<Napi::Number>().FloatValue());
    }
    
    auto decoded = vdb::quantization::PQCurve::decode_batch(values);
    
    Napi::Array result = Napi::Array::New(env, decoded.size());
    for (size_t i = 0; i < decoded.size(); i++) {
      result.Set(static_cast<uint32_t>(i), Napi::Number::New(env, decoded[i]));
    }
    return result;
  }
  
  Napi::TypeError::New(env, "Expected number or array").ThrowAsJavaScriptException();
  return env.Null();
}

// ============================================================================
// Perceptual Quantization Methods - HLG (Rec. 2100)
// ============================================================================

Napi::Value Quantization::EncodeHLG(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected linear value or array").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info[0].IsNumber()) {
    float linear = info[0].As<Napi::Number>().FloatValue();
    float encoded = vdb::quantization::HLGCurve::encode(linear);
    return Napi::Number::New(env, encoded);
  } else if (info[0].IsArray()) {
    Napi::Array input = info[0].As<Napi::Array>();
    std::vector<float> values;
    values.reserve(input.Length());
    for (uint32_t i = 0; i < input.Length(); i++) {
      values.push_back(input.Get(i).As<Napi::Number>().FloatValue());
    }
    
    auto encoded = vdb::quantization::HLGCurve::encode_batch(values);
    
    Napi::Array result = Napi::Array::New(env, encoded.size());
    for (size_t i = 0; i < encoded.size(); i++) {
      result.Set(static_cast<uint32_t>(i), Napi::Number::New(env, encoded[i]));
    }
    return result;
  }
  
  Napi::TypeError::New(env, "Expected number or array").ThrowAsJavaScriptException();
  return env.Null();
}

Napi::Value Quantization::DecodeHLG(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected HLG-encoded value or array").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info[0].IsNumber()) {
    float hlgValue = info[0].As<Napi::Number>().FloatValue();
    float decoded = vdb::quantization::HLGCurve::decode(hlgValue);
    return Napi::Number::New(env, decoded);
  } else if (info[0].IsArray()) {
    Napi::Array input = info[0].As<Napi::Array>();
    std::vector<float> values;
    values.reserve(input.Length());
    for (uint32_t i = 0; i < input.Length(); i++) {
      values.push_back(input.Get(i).As<Napi::Number>().FloatValue());
    }
    
    auto decoded = vdb::quantization::HLGCurve::decode_batch(values);
    
    Napi::Array result = Napi::Array::New(env, decoded.size());
    for (size_t i = 0; i < decoded.size(); i++) {
      result.Set(static_cast<uint32_t>(i), Napi::Number::New(env, decoded[i]));
    }
    return result;
  }
  
  Napi::TypeError::New(env, "Expected number or array").ThrowAsJavaScriptException();
  return env.Null();
}

// ============================================================================
// Perceptual Quantization Methods - Gamma Curves
// ============================================================================

Napi::Value Quantization::EncodeGamma(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected linear value or array").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  float gamma = 2.2f;  // Default sRGB
  if (info.Length() >= 2 && info[1].IsNumber()) {
    gamma = info[1].As<Napi::Number>().FloatValue();
  }
  
  vdb::quantization::GammaCurve curve(gamma);
  
  if (info[0].IsNumber()) {
    float linear = info[0].As<Napi::Number>().FloatValue();
    float encoded = curve.encode(linear);
    return Napi::Number::New(env, encoded);
  } else if (info[0].IsArray()) {
    Napi::Array input = info[0].As<Napi::Array>();
    std::vector<float> values;
    values.reserve(input.Length());
    for (uint32_t i = 0; i < input.Length(); i++) {
      values.push_back(input.Get(i).As<Napi::Number>().FloatValue());
    }
    
    auto encoded = curve.encode_batch(values);
    
    Napi::Array result = Napi::Array::New(env, encoded.size());
    for (size_t i = 0; i < encoded.size(); i++) {
      result.Set(static_cast<uint32_t>(i), Napi::Number::New(env, encoded[i]));
    }
    return result;
  }
  
  Napi::TypeError::New(env, "Expected number or array").ThrowAsJavaScriptException();
  return env.Null();
}

Napi::Value Quantization::DecodeGamma(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected gamma-encoded value or array").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  float gamma = 2.2f;
  if (info.Length() >= 2 && info[1].IsNumber()) {
    gamma = info[1].As<Napi::Number>().FloatValue();
  }
  
  vdb::quantization::GammaCurve curve(gamma);
  
  if (info[0].IsNumber()) {
    float gammaValue = info[0].As<Napi::Number>().FloatValue();
    float decoded = curve.decode(gammaValue);
    return Napi::Number::New(env, decoded);
  } else if (info[0].IsArray()) {
    Napi::Array input = info[0].As<Napi::Array>();
    std::vector<float> values;
    values.reserve(input.Length());
    for (uint32_t i = 0; i < input.Length(); i++) {
      values.push_back(input.Get(i).As<Napi::Number>().FloatValue());
    }
    
    auto decoded = curve.decode_batch(values);
    
    Napi::Array result = Napi::Array::New(env, decoded.size());
    for (size_t i = 0; i < decoded.size(); i++) {
      result.Set(static_cast<uint32_t>(i), Napi::Number::New(env, decoded[i]));
    }
    return result;
  }
  
  Napi::TypeError::New(env, "Expected number or array").ThrowAsJavaScriptException();
  return env.Null();
}

// ============================================================================
// Display-Aware Quantization
// ============================================================================

Napi::Value Quantization::CreateDisplayProfile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected profile type string").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string profileType = info[0].As<Napi::String>().Utf8Value();
  
  Napi::Object profile = Napi::Object::New(env);
  
  if (profileType == "sdr" || profileType == "SDR") {
    profile.Set("type", Napi::String::New(env, "SDR"));
    profile.Set("curve", Napi::String::New(env, "Gamma22"));
    profile.Set("peakLuminance", Napi::Number::New(env, 100.0));
    profile.Set("blackLevel", Napi::Number::New(env, 0.1));
    profile.Set("gamma", Napi::Number::New(env, 2.2));
  } else if (profileType == "hdr10" || profileType == "HDR10") {
    profile.Set("type", Napi::String::New(env, "HDR10"));
    profile.Set("curve", Napi::String::New(env, "PQ_ST2084"));
    profile.Set("peakLuminance", Napi::Number::New(env, 1000.0));
    profile.Set("blackLevel", Napi::Number::New(env, 0.005));
    profile.Set("maxCLL", Napi::Number::New(env, 1000.0));  // Max Content Light Level
    profile.Set("maxFALL", Napi::Number::New(env, 400.0));  // Max Frame-Average Light Level
  } else if (profileType == "dolbyvision" || profileType == "DolbyVision") {
    profile.Set("type", Napi::String::New(env, "DolbyVision"));
    profile.Set("curve", Napi::String::New(env, "PQ_ST2084"));
    profile.Set("peakLuminance", Napi::Number::New(env, 4000.0));
    profile.Set("blackLevel", Napi::Number::New(env, 0.0001));
    profile.Set("dynamicMetadata", Napi::Boolean::New(env, true));
    profile.Set("profile", Napi::Number::New(env, 5));  // Profile 5 = MEL
  } else if (profileType == "hlg" || profileType == "HLG") {
    profile.Set("type", Napi::String::New(env, "HLG"));
    profile.Set("curve", Napi::String::New(env, "HLG_Rec2100"));
    profile.Set("peakLuminance", Napi::Number::New(env, 1000.0));
    profile.Set("systemGamma", Napi::Number::New(env, 1.2));
  } else if (profileType == "hdr1000" || profileType == "HDR1000") {
    profile.Set("type", Napi::String::New(env, "HDR1000"));
    profile.Set("curve", Napi::String::New(env, "PQ_ST2084"));
    profile.Set("peakLuminance", Napi::Number::New(env, 1000.0));
    profile.Set("blackLevel", Napi::Number::New(env, 0.05));
    profile.Set("colorGamut", Napi::String::New(env, "Rec2020"));
  } else {
    Napi::Error::New(env, "Unknown profile type. Use: SDR, HDR10, DolbyVision, HLG, HDR1000").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  return profile;
}

Napi::Value Quantization::QuantizeForDisplay(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 2 || !info[0].IsArray() || !info[1].IsObject()) {
    Napi::TypeError::New(env, "Expected vector array and display profile").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  vdb::Vector input = JSArrayToVector(info[0].As<Napi::Array>());
  Napi::Object profile = info[1].As<Napi::Object>();
  
  std::string curveType = "Linear";
  if (profile.Has("curve")) {
    curveType = profile.Get("curve").As<Napi::String>().Utf8Value();
  }
  
  vdb::quantization::PerceptualCurve curve = vdb::quantization::PerceptualCurve::Linear;
  if (curveType == "PQ_ST2084") {
    curve = vdb::quantization::PerceptualCurve::PQ_ST2084;
  } else if (curveType == "HLG_Rec2100") {
    curve = vdb::quantization::PerceptualCurve::HLG_Rec2100;
  } else if (curveType == "Gamma22") {
    curve = vdb::quantization::PerceptualCurve::Gamma22;
  } else if (curveType == "Gamma24") {
    curve = vdb::quantization::PerceptualCurve::Gamma24;
  }
  
  vdb::quantization::PerceptualTransferFunction ptf(curve);
  vdb::Vector encoded = ptf.encode(input.view());
  
  return VectorToJSArray(env, encoded);
}

// ============================================================================
// HDR Metadata
// ============================================================================

Napi::Value Quantization::GetHDRMetadata(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  Napi::Object metadata = Napi::Object::New(env);
  
  // SMPTE ST 2086 Mastering Display Metadata
  Napi::Object st2086 = Napi::Object::New(env);
  st2086.Set("standard", Napi::String::New(env, "SMPTE ST 2086"));
  st2086.Set("description", Napi::String::New(env, "Mastering Display Color Volume"));
  
  // Primary chromaticity coordinates (Rec. 2020)
  Napi::Object primaries = Napi::Object::New(env);
  primaries.Set("redX", Napi::Number::New(env, 0.708));
  primaries.Set("redY", Napi::Number::New(env, 0.292));
  primaries.Set("greenX", Napi::Number::New(env, 0.170));
  primaries.Set("greenY", Napi::Number::New(env, 0.797));
  primaries.Set("blueX", Napi::Number::New(env, 0.131));
  primaries.Set("blueY", Napi::Number::New(env, 0.046));
  primaries.Set("whiteX", Napi::Number::New(env, 0.3127));
  primaries.Set("whiteY", Napi::Number::New(env, 0.3290));
  st2086.Set("primaries", primaries);
  
  metadata.Set("ST2086", st2086);
  
  // SMPTE ST 2094 Dynamic Metadata
  Napi::Object st2094 = Napi::Object::New(env);
  st2094.Set("standard", Napi::String::New(env, "SMPTE ST 2094"));
  st2094.Set("description", Napi::String::New(env, "Dynamic HDR Metadata"));
  
  Napi::Array appVersions = Napi::Array::New(env, 4);
  appVersions.Set(0u, Napi::String::New(env, "ST 2094-10 (Dolby)"));
  appVersions.Set(1u, Napi::String::New(env, "ST 2094-20 (ETSI SL-HDR1)"));
  appVersions.Set(2u, Napi::String::New(env, "ST 2094-30 (Technicolor SL-HDR2)"));
  appVersions.Set(3u, Napi::String::New(env, "ST 2094-40 (HDR10+)"));
  st2094.Set("applicationVersions", appVersions);
  
  metadata.Set("ST2094", st2094);
  
  // CTA-861-H Content Light Level
  Napi::Object cta861 = Napi::Object::New(env);
  cta861.Set("standard", Napi::String::New(env, "CTA-861-H"));
  cta861.Set("description", Napi::String::New(env, "Content Light Level Information"));
  cta861.Set("maxCLLRange", Napi::String::New(env, "1-65535 cd/m²"));
  cta861.Set("maxFALLRange", Napi::String::New(env, "1-65535 cd/m²"));
  metadata.Set("CTA861", cta861);
  
  return metadata;
}

Napi::Value Quantization::GetST2084Constants(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  // SMPTE ST 2084 PQ constants
  Napi::Object constants = Napi::Object::New(env);
  constants.Set("m1", Napi::Number::New(env, vdb::quantization::PQCurve::M1));
  constants.Set("m2", Napi::Number::New(env, vdb::quantization::PQCurve::M2));
  constants.Set("c1", Napi::Number::New(env, vdb::quantization::PQCurve::C1));
  constants.Set("c2", Napi::Number::New(env, vdb::quantization::PQCurve::C2));
  constants.Set("c3", Napi::Number::New(env, vdb::quantization::PQCurve::C3));
  constants.Set("peakLuminance", Napi::Number::New(env, vdb::quantization::PQCurve::PEAK_LUMINANCE));
  
  // Derived values
  constants.Set("inv_m1", Napi::Number::New(env, vdb::quantization::PQCurve::INV_M1));
  constants.Set("inv_m2", Napi::Number::New(env, vdb::quantization::PQCurve::INV_M2));
  
  return constants;
}

} // namespace hektor_native
