#include "embeddings.h"

namespace hektor_native {

// Stub implementations for TextEncoderWrap
Napi::Object TextEncoderWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "TextEncoder", {
        InstanceMethod("init", &TextEncoderWrap::Init),
        InstanceMethod("isReady", &TextEncoderWrap::IsReady),
        InstanceMethod("encode", &TextEncoderWrap::Encode),
        InstanceMethod("encodeBatch", &TextEncoderWrap::EncodeBatch),
        InstanceMethod("dimension", &TextEncoderWrap::Dimension),
        InstanceMethod("device", &TextEncoderWrap::Device),
    });
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    exports.Set("TextEncoder", func);
    return exports;
}

TextEncoderWrap::TextEncoderWrap(const Napi::CallbackInfo& info) : Napi::ObjectWrap<TextEncoderWrap>(info) {}
Napi::Value TextEncoderWrap::Init(const Napi::CallbackInfo& info) { return info.Env().Undefined(); }
Napi::Value TextEncoderWrap::IsReady(const Napi::CallbackInfo& info) { return Napi::Boolean::New(info.Env(), false); }
Napi::Value TextEncoderWrap::Encode(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value TextEncoderWrap::EncodeBatch(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value TextEncoderWrap::Dimension(const Napi::CallbackInfo& info) { return Napi::Number::New(info.Env(), 384); }
Napi::Value TextEncoderWrap::Device(const Napi::CallbackInfo& info) { return Napi::String::New(info.Env(), "cpu"); }

// Similar stubs for ImageEncoderWrap, OnnxSessionWrap, TokenizerWrap, ImagePreprocessorWrap
Napi::Object ImageEncoderWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "ImageEncoder", {});
    exports.Set("ImageEncoder", func);
    return exports;
}
ImageEncoderWrap::ImageEncoderWrap(const Napi::CallbackInfo& info) : Napi::ObjectWrap<ImageEncoderWrap>(info) {}
Napi::Value ImageEncoderWrap::Init(const Napi::CallbackInfo& info) { return info.Env().Undefined(); }
Napi::Value ImageEncoderWrap::IsReady(const Napi::CallbackInfo& info) { return Napi::Boolean::New(info.Env(), false); }
Napi::Value ImageEncoderWrap::Encode(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value ImageEncoderWrap::EncodePath(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value ImageEncoderWrap::EncodeBatch(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value ImageEncoderWrap::Dimension(const Napi::CallbackInfo& info) { return Napi::Number::New(info.Env(), 512); }
Napi::Value ImageEncoderWrap::Device(const Napi::CallbackInfo& info) { return Napi::String::New(info.Env(), "cpu"); }

Napi::Object OnnxSessionWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "OnnxSession", {});
    exports.Set("OnnxSession", func);
    return exports;
}
OnnxSessionWrap::OnnxSessionWrap(const Napi::CallbackInfo& info) : Napi::ObjectWrap<OnnxSessionWrap>(info) {}
Napi::Value OnnxSessionWrap::Run(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value OnnxSessionWrap::Device(const Napi::CallbackInfo& info) { return Napi::String::New(info.Env(), "cpu"); }
Napi::Value OnnxSessionWrap::InputNames(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value OnnxSessionWrap::OutputNames(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value OnnxSessionWrap::InputShape(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value OnnxSessionWrap::OutputShape(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }

Napi::Object TokenizerWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "Tokenizer", {});
    exports.Set("Tokenizer", func);
    return exports;
}
TokenizerWrap::TokenizerWrap(const Napi::CallbackInfo& info) : Napi::ObjectWrap<TokenizerWrap>(info) {}
Napi::Value TokenizerWrap::Encode(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value TokenizerWrap::Decode(const Napi::CallbackInfo& info) { return Napi::String::New(info.Env(), ""); }
Napi::Value TokenizerWrap::VocabSize(const Napi::CallbackInfo& info) { return Napi::Number::New(info.Env(), 30000); }
Napi::Value TokenizerWrap::HasToken(const Napi::CallbackInfo& info) { return Napi::Boolean::New(info.Env(), false); }

Napi::Object ImagePreprocessorWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "ImagePreprocessor", {});
    exports.Set("ImagePreprocessor", func);
    return exports;
}
ImagePreprocessorWrap::ImagePreprocessorWrap(const Napi::CallbackInfo& info) : Napi::ObjectWrap<ImagePreprocessorWrap>(info) {}
Napi::Value ImagePreprocessorWrap::Process(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value ImagePreprocessorWrap::ProcessFile(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value ImagePreprocessorWrap::CenterCropAndProcess(const Napi::CallbackInfo& info) { return Napi::Array::New(info.Env()); }
Napi::Value ImagePreprocessorWrap::TargetSize(const Napi::CallbackInfo& info) { return Napi::Object::New(info.Env()); }
Napi::Value ImagePreprocessorWrap::OutputSize(const Napi::CallbackInfo& info) { return Napi::Number::New(info.Env(), 0); }

} // namespace hektor_native
