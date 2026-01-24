#pragma once

#include <napi.h>
#include "common.h"
#include <memory>
#include "vdb/hybrid_search.hpp"

namespace hektor_native {

// BM25Engine wrapper
class BM25EngineWrap : public Napi::ObjectWrap<BM25EngineWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    BM25EngineWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value AddDocument(const Napi::CallbackInfo& info);
    Napi::Value RemoveDocument(const Napi::CallbackInfo& info);
    Napi::Value UpdateDocument(const Napi::CallbackInfo& info);
    Napi::Value Search(const Napi::CallbackInfo& info);
    Napi::Value DocumentCount(const Napi::CallbackInfo& info);
    Napi::Value TermCount(const Napi::CallbackInfo& info);
    Napi::Value AverageDocumentLength(const Napi::CallbackInfo& info);
    Napi::Value Save(const Napi::CallbackInfo& info);
    static Napi::Value Load(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::hybrid::BM25Engine> engine_;
};

// KeywordExtractor wrapper - full implementation
class KeywordExtractorWrap : public Napi::ObjectWrap<KeywordExtractorWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    KeywordExtractorWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Extract(const Napi::CallbackInfo& info);
    Napi::Value Train(const Napi::CallbackInfo& info);
    Napi::Value Save(const Napi::CallbackInfo& info);
    static Napi::Value Load(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::hybrid::KeywordExtractor> extractor_;
};

// HybridSearchEngine wrapper
class HybridSearchEngineWrap : public Napi::ObjectWrap<HybridSearchEngineWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    HybridSearchEngineWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Combine(const Napi::CallbackInfo& info);
    
    // Static fusion methods
    static Napi::Value WeightedSum(const Napi::CallbackInfo& info);
    static Napi::Value ReciprocalRankFusion(const Napi::CallbackInfo& info);
    static Napi::Value CombSum(const Napi::CallbackInfo& info);
    static Napi::Value CombMnz(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::hybrid::HybridSearchEngine> engine_;
};

// QueryRewriter wrapper - full implementation
class QueryRewriterWrap : public Napi::ObjectWrap<QueryRewriterWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    QueryRewriterWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Rewrite(const Napi::CallbackInfo& info);
    Napi::Value AddSynonym(const Napi::CallbackInfo& info);
    Napi::Value LoadSynonyms(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::hybrid::QueryRewriter> rewriter_;
};

} // namespace hektor_native
