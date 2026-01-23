#pragma once

#include <napi.h>
#include "common.h"

namespace hektor_native {

class Utils {
public:
    static void Init(Napi::Env env, Napi::Object& exports);
    
    // Distance functions
    static Napi::Value DotProduct(const Napi::CallbackInfo& info);
    static Napi::Value EuclideanDistance(const Napi::CallbackInfo& info);
    static Napi::Value SquaredEuclidean(const Napi::CallbackInfo& info);
    static Napi::Value CosineSimilarity(const Napi::CallbackInfo& info);
    static Napi::Value CosineDistance(const Napi::CallbackInfo& info);
    static Napi::Value ComputeDistance(const Napi::CallbackInfo& info);
    static Napi::Value L2Distance(const Napi::CallbackInfo& info);
    static Napi::Value L2SquaredDistance(const Napi::CallbackInfo& info);
    
    // Vector operations
    static Napi::Value Normalize(const Napi::CallbackInfo& info);
    static Napi::Value Normalized(const Napi::CallbackInfo& info);
    static Napi::Value L2Norm(const Napi::CallbackInfo& info);
    static Napi::Value Add(const Napi::CallbackInfo& info);
    static Napi::Value Subtract(const Napi::CallbackInfo& info);
    static Napi::Value Scale(const Napi::CallbackInfo& info);
    static Napi::Value AddScaled(const Napi::CallbackInfo& info);
    static Napi::Value Multiply(const Napi::CallbackInfo& info);
    static Napi::Value Mean(const Napi::CallbackInfo& info);
    static Napi::Value BatchDistance(const Napi::CallbackInfo& info);
    static Napi::Value BruteForceKNN(const Napi::CallbackInfo& info);
    
    // Utility functions
    static Napi::Value NowTimestamp(const Napi::CallbackInfo& info);
    static Napi::Value TimestampToIso(const Napi::CallbackInfo& info);
    static Napi::Value DocumentTypeName(const Napi::CallbackInfo& info);
    static Napi::Value DetectBestDevice(const Napi::CallbackInfo& info);
    static Napi::Value DeviceName(const Napi::CallbackInfo& info);
    static Napi::Value FormatToString(const Napi::CallbackInfo& info);
    
    // Ingest utility functions
    static Napi::Value ParseMarkdown(const Napi::CallbackInfo& info);
    static Napi::Value DetectDocumentType(const Napi::CallbackInfo& info);
    static Napi::Value ExtractDateFromFilename(const Napi::CallbackInfo& info);
    static Napi::Value ExtractMarketData(const Napi::CallbackInfo& info);
    static Napi::Value ChunkDocument(const Napi::CallbackInfo& info);
    static Napi::Value ChunkText(const Napi::CallbackInfo& info);
    static Napi::Value ContentHash(const Napi::CallbackInfo& info);
    static Napi::Value FileHash(const Napi::CallbackInfo& info);
    
    // Image functions
    static Napi::Value LoadImage(const Napi::CallbackInfo& info);
    static Napi::Value LoadImageMemory(const Napi::CallbackInfo& info);
    static Napi::Value SaveImage(const Napi::CallbackInfo& info);
    static Napi::Value ParseChartPath(const Napi::CallbackInfo& info);
    static Napi::Value PreprocessChart(const Napi::CallbackInfo& info);
    
    // Factory functions
    static Napi::Value CreateGoldStandardDb(const Napi::CallbackInfo& info);
    static Napi::Value OpenDatabase(const Napi::CallbackInfo& info);
    static Napi::Value CreateLLMEngine(const Napi::CallbackInfo& info);
    
    // SIMD detection
    static Napi::Value DetectSimdLevel(const Napi::CallbackInfo& info);
};

} // namespace hektor_native
