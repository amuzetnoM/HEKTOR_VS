#pragma once

#include <napi.h>
#include "common.h"
#include <memory>

// Forward declarations
namespace vdb {
    namespace framework {
        class RAGEngine;
        class LangChainAdapter;
        class LlamaIndexAdapter;
        class DocumentChunker;
    }
    namespace llm {
        class LLMEngine;
    }
}

namespace hektor_native {

// RAGEngine wrapper
class RAGEngineWrap : public Napi::ObjectWrap<RAGEngineWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    RAGEngineWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value ChunkDocument(const Napi::CallbackInfo& info);
    Napi::Value BuildContext(const Napi::CallbackInfo& info);
    Napi::Value FormatPrompt(const Napi::CallbackInfo& info);
    Napi::Value Rerank(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::framework::RAGEngine> engine_;
};

// LLMEngine wrapper
class LLMEngineWrap : public Napi::ObjectWrap<LLMEngineWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    LLMEngineWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Load(const Napi::CallbackInfo& info);
    Napi::Value IsLoaded(const Napi::CallbackInfo& info);
    Napi::Value Unload(const Napi::CallbackInfo& info);
    Napi::Value ModelName(const Napi::CallbackInfo& info);
    Napi::Value ContextSize(const Napi::CallbackInfo& info);
    Napi::Value VocabSize(const Napi::CallbackInfo& info);
    Napi::Value Generate(const Napi::CallbackInfo& info);
    Napi::Value Chat(const Napi::CallbackInfo& info);
    Napi::Value CountTokens(const Napi::CallbackInfo& info);
    Napi::Value Embed(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::llm::LLMEngine> engine_;
};

// LangChainAdapter wrapper
class LangChainAdapterWrap : public Napi::ObjectWrap<LangChainAdapterWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    LangChainAdapterWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value SimilaritySearch(const Napi::CallbackInfo& info);
    Napi::Value SimilaritySearchWithScore(const Napi::CallbackInfo& info);
    Napi::Value MaxMarginalRelevanceSearch(const Napi::CallbackInfo& info);
    Napi::Value AsRetriever(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::framework::LangChainAdapter> adapter_;
};

// LlamaIndexAdapter wrapper
class LlamaIndexAdapterWrap : public Napi::ObjectWrap<LlamaIndexAdapterWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    LlamaIndexAdapterWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value BuildIndex(const Napi::CallbackInfo& info);
    Napi::Value Query(const Napi::CallbackInfo& info);
    Napi::Value Insert(const Napi::CallbackInfo& info);
    Napi::Value DeleteDocument(const Napi::CallbackInfo& info);
    Napi::Value Save(const Napi::CallbackInfo& info);
    static Napi::Value Load(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::framework::LlamaIndexAdapter> adapter_;
};

// DocumentChunker wrapper
class DocumentChunkerWrap : public Napi::ObjectWrap<DocumentChunkerWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    DocumentChunkerWrap(const Napi::CallbackInfo& info);
    
private:
    Napi::Value Chunk(const Napi::CallbackInfo& info);
    Napi::Value ChunkWithMetadata(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::framework::DocumentChunker> chunker_;
};

// Utility functions
class RAGUtils {
public:
    static void Init(Napi::Env env, Napi::Object& exports);
    
    static Napi::Value FindGgufModels(const Napi::CallbackInfo& info);
    static Napi::Value ReadGgufMetadata(const Napi::CallbackInfo& info);
    static Napi::Value ApplyChatTemplate(const Napi::CallbackInfo& info);
};

} // namespace hektor_native
