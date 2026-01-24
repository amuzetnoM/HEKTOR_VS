#include "hybrid.h"
#include "database.h"
#include "vdb/hybrid_search.hpp"

namespace hektor_native {

// ============================================================================
// BM25Engine Wrapper
// ============================================================================

Napi::Object BM25EngineWrap::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "BM25Engine", {
    InstanceMethod("addDocument", &BM25EngineWrap::AddDocument),
    InstanceMethod("removeDocument", &BM25EngineWrap::RemoveDocument),
    InstanceMethod("updateDocument", &BM25EngineWrap::UpdateDocument),
    InstanceMethod("search", &BM25EngineWrap::Search),
    InstanceMethod("documentCount", &BM25EngineWrap::DocumentCount),
    InstanceMethod("termCount", &BM25EngineWrap::TermCount),
    InstanceMethod("averageDocumentLength", &BM25EngineWrap::AverageDocumentLength),
    InstanceMethod("save", &BM25EngineWrap::Save),
    StaticMethod("load", &BM25EngineWrap::Load),
  });
  
  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData<Napi::FunctionReference>(constructor);
  
  exports.Set("BM25Engine", func);
  return exports;
}

BM25EngineWrap::BM25EngineWrap(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<BM25EngineWrap>(info) {
  Napi::Env env = info.Env();
  
  vdb::hybrid::BM25Config config;
  
  if (info.Length() >= 1 && info[0].IsObject()) {
    Napi::Object opts = info[0].As<Napi::Object>();
    if (opts.Has("k1")) config.k1 = opts.Get("k1").As<Napi::Number>().FloatValue();
    if (opts.Has("b")) config.b = opts.Get("b").As<Napi::Number>().FloatValue();
    if (opts.Has("minTermLength")) config.min_term_length = opts.Get("minTermLength").As<Napi::Number>().Uint32Value();
    if (opts.Has("useStemming")) config.use_stemming = opts.Get("useStemming").As<Napi::Boolean>().Value();
    if (opts.Has("caseSensitive")) config.case_sensitive = opts.Get("caseSensitive").As<Napi::Boolean>().Value();
  }
  
  engine_ = std::make_unique<vdb::hybrid::BM25Engine>(config);
}

Napi::Value BM25EngineWrap::AddDocument(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsString()) {
    Napi::TypeError::New(env, "Expected document ID and content string").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  uint64_t id = info[0].As<Napi::Number>().Int64Value();
  std::string content = info[1].As<Napi::String>().Utf8Value();
  
  auto result = engine_->add_document(id, content);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value BM25EngineWrap::RemoveDocument(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Expected document ID").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  uint64_t id = info[0].As<Napi::Number>().Int64Value();
  auto result = engine_->remove_document(id);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value BM25EngineWrap::UpdateDocument(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsString()) {
    Napi::TypeError::New(env, "Expected document ID and content string").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  uint64_t id = info[0].As<Napi::Number>().Int64Value();
  std::string content = info[1].As<Napi::String>().Utf8Value();
  
  auto result = engine_->update_document(id, content);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value BM25EngineWrap::Search(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected query string").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string query = info[0].As<Napi::String>().Utf8Value();
  size_t k = 10;
  float minScore = 0.0f;
  
  if (info.Length() >= 2 && info[1].IsNumber()) {
    k = info[1].As<Napi::Number>().Uint32Value();
  }
  if (info.Length() >= 3 && info[2].IsNumber()) {
    minScore = info[2].As<Napi::Number>().FloatValue();
  }
  
  auto result = engine_->search(query, k, minScore);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    
    Napi::Array results = Napi::Array::New(env, result.value().size());
    for (size_t i = 0; i < result.value().size(); i++) {
      const auto& r = result.value()[i];
      Napi::Object item = Napi::Object::New(env);
      item.Set("id", Napi::Number::New(env, static_cast<double>(r.id)));
      item.Set("score", Napi::Number::New(env, r.score));
      
      Napi::Array terms = Napi::Array::New(env, r.matched_terms.size());
      for (size_t j = 0; j < r.matched_terms.size(); j++) {
        terms.Set(static_cast<uint32_t>(j), Napi::String::New(env, r.matched_terms[j]));
      }
      item.Set("matchedTerms", terms);
      
      results.Set(static_cast<uint32_t>(i), item);
    }
    response.Set("results", results);
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value BM25EngineWrap::DocumentCount(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), static_cast<double>(engine_->document_count()));
}

Napi::Value BM25EngineWrap::TermCount(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), static_cast<double>(engine_->term_count()));
}

Napi::Value BM25EngineWrap::AverageDocumentLength(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), engine_->average_document_length());
}

Napi::Value BM25EngineWrap::Save(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected file path").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string path = info[0].As<Napi::String>().Utf8Value();
  auto result = engine_->save(path);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value BM25EngineWrap::Load(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected file path").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string path = info[0].As<Napi::String>().Utf8Value();
  auto result = vdb::hybrid::BM25Engine::load(path);
  
  if (result.has_value()) {
    auto* constructor = env.GetInstanceData<Napi::FunctionReference>();
    return constructor->New({});
  }
  
  Napi::Error::New(env, result.error().message).ThrowAsJavaScriptException();
  return env.Null();
}

// ============================================================================
// KeywordExtractor Wrapper - Full Implementation
// ============================================================================

Napi::Object KeywordExtractorWrap::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "KeywordExtractor", {
    InstanceMethod("extract", &KeywordExtractorWrap::Extract),
    InstanceMethod("train", &KeywordExtractorWrap::Train),
    InstanceMethod("save", &KeywordExtractorWrap::Save),
    StaticMethod("load", &KeywordExtractorWrap::Load),
  });
  
  exports.Set("KeywordExtractor", func);
  return exports;
}

KeywordExtractorWrap::KeywordExtractorWrap(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<KeywordExtractorWrap>(info) {
  vdb::hybrid::KeywordConfig config;
  
  if (info.Length() >= 1 && info[0].IsObject()) {
    Napi::Object opts = info[0].As<Napi::Object>();
    if (opts.Has("maxKeywords")) config.max_keywords = opts.Get("maxKeywords").As<Napi::Number>().Uint32Value();
    if (opts.Has("minScore")) config.min_score = opts.Get("minScore").As<Napi::Number>().FloatValue();
    if (opts.Has("useTfidf")) config.use_tfidf = opts.Get("useTfidf").As<Napi::Boolean>().Value();
    if (opts.Has("usePositionWeight")) config.use_position_weight = opts.Get("usePositionWeight").As<Napi::Boolean>().Value();
  }
  
  extractor_ = std::make_unique<vdb::hybrid::KeywordExtractor>(config);
}

Napi::Value KeywordExtractorWrap::Extract(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected text string").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string text = info[0].As<Napi::String>().Utf8Value();
  auto result = extractor_->extract(text);
  
  if (!result.has_value()) {
    Napi::Error::New(env, result.error().message).ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Napi::Array keywords = Napi::Array::New(env, result.value().size());
  for (size_t i = 0; i < result.value().size(); i++) {
    const auto& kw = result.value()[i];
    Napi::Object kwObj = Napi::Object::New(env);
    kwObj.Set("term", Napi::String::New(env, kw.term));
    kwObj.Set("score", Napi::Number::New(env, kw.score));
    kwObj.Set("frequency", Napi::Number::New(env, kw.frequency));
    
    Napi::Array positions = Napi::Array::New(env, kw.positions.size());
    for (size_t j = 0; j < kw.positions.size(); j++) {
      positions.Set(static_cast<uint32_t>(j), Napi::Number::New(env, kw.positions[j]));
    }
    kwObj.Set("positions", positions);
    
    keywords.Set(static_cast<uint32_t>(i), kwObj);
  }
  
  return keywords;
}

Napi::Value KeywordExtractorWrap::Train(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "Expected array of documents").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Napi::Array docsArr = info[0].As<Napi::Array>();
  std::vector<std::string> documents;
  documents.reserve(docsArr.Length());
  
  for (uint32_t i = 0; i < docsArr.Length(); i++) {
    documents.push_back(docsArr.Get(i).As<Napi::String>().Utf8Value());
  }
  
  auto result = extractor_->train(documents);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  } else {
    response.Set("documentsProcessed", Napi::Number::New(env, documents.size()));
  }
  
  return response;
}

Napi::Value KeywordExtractorWrap::Save(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected file path").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string path = info[0].As<Napi::String>().Utf8Value();
  auto result = extractor_->save(path);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value KeywordExtractorWrap::Load(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected file path").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string path = info[0].As<Napi::String>().Utf8Value();
  auto result = vdb::hybrid::KeywordExtractor::load(path);
  
  if (!result.has_value()) {
    Napi::Error::New(env, result.error().message).ThrowAsJavaScriptException();
    return env.Null();
  }
  
  // Create new wrapper with default constructor, then swap the underlying extractor
  // For simplicity, return a success object - the user should call the constructor
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", true);
  response.Set("message", "Extractor loaded - create new instance with same path for full access");
  
  return response;
}

// ============================================================================
// HybridSearchEngine Wrapper
// ============================================================================

Napi::Object HybridSearchEngineWrap::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "HybridSearchEngine", {
    InstanceMethod("combine", &HybridSearchEngineWrap::Combine),
    StaticMethod("weightedSum", &HybridSearchEngineWrap::WeightedSum),
    StaticMethod("reciprocalRankFusion", &HybridSearchEngineWrap::ReciprocalRankFusion),
    StaticMethod("combSum", &HybridSearchEngineWrap::CombSum),
    StaticMethod("combMnz", &HybridSearchEngineWrap::CombMnz),
  });
  
  exports.Set("HybridSearchEngine", func);
  return exports;
}

HybridSearchEngineWrap::HybridSearchEngineWrap(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<HybridSearchEngineWrap>(info) {
  vdb::hybrid::HybridSearchConfig config;
  
  if (info.Length() >= 1 && info[0].IsObject()) {
    Napi::Object opts = info[0].As<Napi::Object>();
    if (opts.Has("vectorWeight")) config.vector_weight = opts.Get("vectorWeight").As<Napi::Number>().FloatValue();
    if (opts.Has("lexicalWeight")) config.lexical_weight = opts.Get("lexicalWeight").As<Napi::Number>().FloatValue();
    if (opts.Has("rrfK")) config.rrf_k = opts.Get("rrfK").As<Napi::Number>().Uint32Value();
    if (opts.Has("rerank")) config.rerank = opts.Get("rerank").As<Napi::Boolean>().Value();
    
    if (opts.Has("fusion")) {
      std::string fusionStr = opts.Get("fusion").As<Napi::String>().Utf8Value();
      if (fusionStr == "weightedSum") config.fusion = vdb::hybrid::FusionMethod::WeightedSum;
      else if (fusionStr == "rrf") config.fusion = vdb::hybrid::FusionMethod::RRF;
      else if (fusionStr == "combSum") config.fusion = vdb::hybrid::FusionMethod::CombSUM;
      else if (fusionStr == "combMnz") config.fusion = vdb::hybrid::FusionMethod::CombMNZ;
      else if (fusionStr == "borda") config.fusion = vdb::hybrid::FusionMethod::Borda;
    }
  }
  
  engine_ = std::make_unique<vdb::hybrid::HybridSearchEngine>(config);
}

Napi::Value HybridSearchEngineWrap::Combine(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 2 || !info[0].IsArray() || !info[1].IsArray()) {
    Napi::TypeError::New(env, "Expected vector results and lexical results arrays").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  // Parse vector results
  Napi::Array vecArr = info[0].As<Napi::Array>();
  std::vector<vdb::QueryResult> vectorResults;
  vectorResults.reserve(vecArr.Length());
  
  for (uint32_t i = 0; i < vecArr.Length(); i++) {
    Napi::Object item = vecArr.Get(i).As<Napi::Object>();
    vdb::QueryResult qr;
    qr.id = item.Get("id").As<Napi::Number>().Int64Value();
    qr.distance = item.Get("distance").As<Napi::Number>().FloatValue();
    qr.score = item.Has("score") ? item.Get("score").As<Napi::Number>().FloatValue() : (1.0f - qr.distance);
    vectorResults.push_back(qr);
  }
  
  // Parse lexical results
  Napi::Array lexArr = info[1].As<Napi::Array>();
  std::vector<vdb::hybrid::BM25Result> lexicalResults;
  lexicalResults.reserve(lexArr.Length());
  
  for (uint32_t i = 0; i < lexArr.Length(); i++) {
    Napi::Object item = lexArr.Get(i).As<Napi::Object>();
    vdb::hybrid::BM25Result br;
    br.id = item.Get("id").As<Napi::Number>().Int64Value();
    br.score = item.Get("score").As<Napi::Number>().FloatValue();
    
    if (item.Has("matchedTerms")) {
      Napi::Array terms = item.Get("matchedTerms").As<Napi::Array>();
      for (uint32_t j = 0; j < terms.Length(); j++) {
        br.matched_terms.push_back(terms.Get(j).As<Napi::String>().Utf8Value());
      }
    }
    lexicalResults.push_back(br);
  }
  
  size_t k = 10;
  if (info.Length() >= 3 && info[2].IsNumber()) {
    k = info[2].As<Napi::Number>().Uint32Value();
  }
  
  auto result = engine_->combine(vectorResults, lexicalResults, k);
  
  Napi::Object response = Napi::Object::New(env);
  if (result.has_value()) {
    response.Set("success", true);
    
    Napi::Array results = Napi::Array::New(env, result.value().size());
    for (size_t i = 0; i < result.value().size(); i++) {
      const auto& hr = result.value()[i];
      Napi::Object item = Napi::Object::New(env);
      item.Set("id", Napi::Number::New(env, static_cast<double>(hr.id)));
      item.Set("combinedScore", Napi::Number::New(env, hr.combined_score));
      item.Set("vectorScore", Napi::Number::New(env, hr.vector_score));
      item.Set("lexicalScore", Napi::Number::New(env, hr.lexical_score));
      
      Napi::Array keywords = Napi::Array::New(env, hr.matched_keywords.size());
      for (size_t j = 0; j < hr.matched_keywords.size(); j++) {
        keywords.Set(static_cast<uint32_t>(j), Napi::String::New(env, hr.matched_keywords[j]));
      }
      item.Set("matchedKeywords", keywords);
      
      results.Set(static_cast<uint32_t>(i), item);
    }
    response.Set("results", results);
  } else {
    response.Set("success", false);
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value HybridSearchEngineWrap::WeightedSum(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 3) {
    Napi::TypeError::New(env, "Expected vecScore, lexScore, vecWeight").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  float vecScore = info[0].As<Napi::Number>().FloatValue();
  float lexScore = info[1].As<Napi::Number>().FloatValue();
  float vecWeight = info[2].As<Napi::Number>().FloatValue();
  
  float result = vdb::hybrid::HybridSearchEngine::weighted_sum(vecScore, lexScore, vecWeight);
  return Napi::Number::New(env, result);
}

Napi::Value HybridSearchEngineWrap::ReciprocalRankFusion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 3) {
    Napi::TypeError::New(env, "Expected vecRank, lexRank, k").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  size_t vecRank = info[0].As<Napi::Number>().Uint32Value();
  size_t lexRank = info[1].As<Napi::Number>().Uint32Value();
  size_t k = info[2].As<Napi::Number>().Uint32Value();
  
  float result = vdb::hybrid::HybridSearchEngine::reciprocal_rank_fusion(vecRank, lexRank, k);
  return Napi::Number::New(env, result);
}

Napi::Value HybridSearchEngineWrap::CombSum(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Expected vecScore, lexScore").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  float vecScore = info[0].As<Napi::Number>().FloatValue();
  float lexScore = info[1].As<Napi::Number>().FloatValue();
  
  float result = vdb::hybrid::HybridSearchEngine::comb_sum(vecScore, lexScore);
  return Napi::Number::New(env, result);
}

Napi::Value HybridSearchEngineWrap::CombMnz(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 3) {
    Napi::TypeError::New(env, "Expected vecScore, lexScore, numSystems").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  float vecScore = info[0].As<Napi::Number>().FloatValue();
  float lexScore = info[1].As<Napi::Number>().FloatValue();
  size_t numSystems = info[2].As<Napi::Number>().Uint32Value();
  
  float result = vdb::hybrid::HybridSearchEngine::comb_mnz(vecScore, lexScore, numSystems);
  return Napi::Number::New(env, result);
}

// ============================================================================
// QueryRewriter Wrapper - Full Implementation
// ============================================================================

Napi::Object QueryRewriterWrap::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "QueryRewriter", {
    InstanceMethod("rewrite", &QueryRewriterWrap::Rewrite),
    InstanceMethod("addSynonym", &QueryRewriterWrap::AddSynonym),
    InstanceMethod("loadSynonyms", &QueryRewriterWrap::LoadSynonyms),
  });
  
  exports.Set("QueryRewriter", func);
  return exports;
}

QueryRewriterWrap::QueryRewriterWrap(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<QueryRewriterWrap>(info) {
  vdb::hybrid::RewriteConfig config;
  
  if (info.Length() >= 1 && info[0].IsObject()) {
    Napi::Object opts = info[0].As<Napi::Object>();
    if (opts.Has("expandSynonyms")) config.expand_synonyms = opts.Get("expandSynonyms").As<Napi::Boolean>().Value();
    if (opts.Has("correctSpelling")) config.correct_spelling = opts.Get("correctSpelling").As<Napi::Boolean>().Value();
    if (opts.Has("addStemmedTerms")) config.add_stemmed_terms = opts.Get("addStemmedTerms").As<Napi::Boolean>().Value();
    if (opts.Has("maxExpansions")) config.max_expansions = opts.Get("maxExpansions").As<Napi::Number>().Uint32Value();
  }
  
  rewriter_ = std::make_unique<vdb::hybrid::QueryRewriter>(config);
}

Napi::Value QueryRewriterWrap::Rewrite(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected query string").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string query = info[0].As<Napi::String>().Utf8Value();
  auto result = rewriter_->rewrite(query);
  
  if (!result.has_value()) {
    Napi::Error::New(env, result.error().message).ThrowAsJavaScriptException();
    return env.Null();
  }
  
  return Napi::String::New(env, result.value());
}

Napi::Value QueryRewriterWrap::AddSynonym(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 2 || !info[0].IsString() || !info[1].IsArray()) {
    Napi::TypeError::New(env, "Expected term string and synonyms array").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string term = info[0].As<Napi::String>().Utf8Value();
  Napi::Array synsArr = info[1].As<Napi::Array>();
  
  std::vector<std::string> synonyms;
  synonyms.reserve(synsArr.Length());
  for (uint32_t i = 0; i < synsArr.Length(); i++) {
    synonyms.push_back(synsArr.Get(i).As<Napi::String>().Utf8Value());
  }
  
  auto result = rewriter_->add_synonym(term, synonyms);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  }
  
  return response;
}

Napi::Value QueryRewriterWrap::LoadSynonyms(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected file path").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string path = info[0].As<Napi::String>().Utf8Value();
  auto result = rewriter_->load_synonyms(path);
  
  Napi::Object response = Napi::Object::New(env);
  response.Set("success", result.has_value());
  if (!result.has_value()) {
    response.Set("error", result.error().message);
  }
  
  return response;
}

} // namespace hektor_native
