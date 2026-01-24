#pragma once
#include <napi.h>
#include <memory>
#include "vdb/database.hpp"

namespace hektor_native {

class Quantization : public Napi::ObjectWrap<Quantization> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Quantization(const Napi::CallbackInfo& info);
  
private:
  // Product Quantization
  Napi::Value ApplyPQ(const Napi::CallbackInfo& info);
  
  // Scalar Quantization
  Napi::Value ApplySQ(const Napi::CallbackInfo& info);
  
  // Get quantization info
  Napi::Value GetQuantizationInfo(const Napi::CallbackInfo& info);
  
  // Perceptual Curve Methods - SMPTE ST 2084 (PQ) for HDR
  Napi::Value EncodePQ(const Napi::CallbackInfo& info);
  Napi::Value DecodePQ(const Napi::CallbackInfo& info);
  
  // HLG (Rec. 2100) for HDR
  Napi::Value EncodeHLG(const Napi::CallbackInfo& info);
  Napi::Value DecodeHLG(const Napi::CallbackInfo& info);
  
  // Gamma Curves (sRGB, DCI-P3)
  Napi::Value EncodeGamma(const Napi::CallbackInfo& info);
  Napi::Value DecodeGamma(const Napi::CallbackInfo& info);
  
  // Display-Aware Quantization
  Napi::Value CreateDisplayProfile(const Napi::CallbackInfo& info);
  Napi::Value QuantizeForDisplay(const Napi::CallbackInfo& info);
  
  // HDR Metadata
  Napi::Value GetHDRMetadata(const Napi::CallbackInfo& info);
  Napi::Value GetST2084Constants(const Napi::CallbackInfo& info);
  
  std::shared_ptr<vdb::VectorDatabase> db_;
  std::string db_id_;
};

} // namespace hektor_native
