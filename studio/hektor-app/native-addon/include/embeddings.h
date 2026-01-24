#pragma once

#include <napi.h>
#include "common.h"
#include <memory>

// Embeddings functionality is optional and depends on ONNX Runtime
// These wrappers provide stub implementations when ONNX is not available

namespace hektor_native {

// TextEncoder wrapper (stubs - full impl requires ONNX Runtime)
class TextEncoderWrap : public Napi::ObjectWrap<TextEncoderWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    TextEncoderWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Init(const Napi::CallbackInfo& info);
    Napi::Value IsReady(const Napi::CallbackInfo& info);
    Napi::Value Encode(const Napi::CallbackInfo& info);
    Napi::Value EncodeBatch(const Napi::CallbackInfo& info);
    Napi::Value Dimension(const Napi::CallbackInfo& info);
    Napi::Value Device(const Napi::CallbackInfo& info);
};

// ImageEncoder wrapper (stubs - full impl requires ONNX Runtime)
class ImageEncoderWrap : public Napi::ObjectWrap<ImageEncoderWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    ImageEncoderWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Init(const Napi::CallbackInfo& info);
    Napi::Value IsReady(const Napi::CallbackInfo& info);
    Napi::Value Encode(const Napi::CallbackInfo& info);
    Napi::Value EncodePath(const Napi::CallbackInfo& info);
    Napi::Value EncodeBatch(const Napi::CallbackInfo& info);
    Napi::Value Dimension(const Napi::CallbackInfo& info);
    Napi::Value Device(const Napi::CallbackInfo& info);
};

// OnnxSession wrapper (stubs - full impl requires ONNX Runtime)
class OnnxSessionWrap : public Napi::ObjectWrap<OnnxSessionWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    OnnxSessionWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Run(const Napi::CallbackInfo& info);
    Napi::Value Device(const Napi::CallbackInfo& info);
    Napi::Value InputNames(const Napi::CallbackInfo& info);
    Napi::Value OutputNames(const Napi::CallbackInfo& info);
    Napi::Value InputShape(const Napi::CallbackInfo& info);
    Napi::Value OutputShape(const Napi::CallbackInfo& info);
};

// Tokenizer wrapper (stubs - full impl requires ONNX Runtime)
class TokenizerWrap : public Napi::ObjectWrap<TokenizerWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    TokenizerWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Encode(const Napi::CallbackInfo& info);
    Napi::Value Decode(const Napi::CallbackInfo& info);
    Napi::Value VocabSize(const Napi::CallbackInfo& info);
    Napi::Value HasToken(const Napi::CallbackInfo& info);
};

// ImagePreprocessor wrapper (stubs - full impl requires ONNX Runtime)
class ImagePreprocessorWrap : public Napi::ObjectWrap<ImagePreprocessorWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    ImagePreprocessorWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Process(const Napi::CallbackInfo& info);
    Napi::Value ProcessFile(const Napi::CallbackInfo& info);
    Napi::Value CenterCropAndProcess(const Napi::CallbackInfo& info);
    Napi::Value TargetSize(const Napi::CallbackInfo& info);
    Napi::Value OutputSize(const Napi::CallbackInfo& info);
};

} // namespace hektor_native
