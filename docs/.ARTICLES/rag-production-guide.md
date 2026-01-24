---
title: Retrieval-Augmented Generation
description: "From Theory to Production"
version: 4.1.5
last_updated: 2026-01-24
sidebar_position: 1
category: Introduction
---

# RAG Comprehensive
> From Theory to Production

**ARTIFACT VIRTUAL Technical Article Series**  
*Author: Ali A. Shakil*  
*Published: January 24, 2026*

---

## Abstract

Retrieval-Augmented Generation (RAG) combines the knowledge retrieval capabilities of vector databases with the generative power of large language models. This article presents a production-grade RAG architecture, covering document processing, chunking strategies, embedding selection, retrieval optimization, and context management.

---

## 1. Introduction

Large Language Models (LLMs) have a fundamental limitation: their knowledge is frozen at training time. RAG solves this by:

1. **Indexing**: Store documents as vector embeddings
2. **Retrieval**: Find relevant documents for a query
3. **Generation**: Augment the LLM prompt with retrieved context

```
┌─────────────────────────────────────────────────────────────────┐
│                        RAG Pipeline                              │
│                                                                  │
│  Query ──► Embed ──► Search ──► Rerank ──► Context ──► LLM     │
│              │         │          │          │          │       │
│              ▼         ▼          ▼          ▼          ▼       │
│           Vector    HNSW      Cross-      Prompt     Response   │
│           Model     Index     Encoder     Template              │
└─────────────────────────────────────────────────────────────────┘
```

## 2. Document Processing

### 2.1 Chunking Strategies

#### Fixed-Size Chunking

Simple but often suboptimal:

```python
def fixed_chunk(text: str, chunk_size: int = 512, overlap: int = 50) -> list[str]:
    chunks = []
    start = 0
    while start < len(text):
        end = min(start + chunk_size, len(text))
        chunks.append(text[start:end])
        start += chunk_size - overlap
    return chunks
```

**Problems:**
- Cuts mid-sentence
- Ignores document structure
- No semantic boundaries

#### Semantic Chunking (Hektor Default)

```python
def semantic_chunk(text: str, model, threshold: float = 0.5) -> list[str]:
    # Split into sentences
    sentences = split_sentences(text)
    
    # Embed each sentence
    embeddings = model.encode(sentences)
    
    # Find natural break points (low similarity between adjacent)
    chunks = []
    current_chunk = [sentences[0]]
    
    for i in range(1, len(sentences)):
        similarity = cosine_similarity(embeddings[i-1], embeddings[i])
        
        if similarity < threshold:
            # Semantic boundary - start new chunk
            chunks.append(' '.join(current_chunk))
            current_chunk = [sentences[i]]
        else:
            current_chunk.append(sentences[i])
    
    if current_chunk:
        chunks.append(' '.join(current_chunk))
    
    return chunks
```

#### Recursive Character Splitting

Respects document structure:

```python
def recursive_chunk(
    text: str,
    chunk_size: int = 1000,
    separators: list[str] = ["\n\n", "\n", ". ", " "]
) -> list[str]:
    if len(text) <= chunk_size:
        return [text]
    
    for separator in separators:
        if separator in text:
            parts = text.split(separator)
            chunks = []
            current = ""
            
            for part in parts:
                if len(current) + len(part) + len(separator) <= chunk_size:
                    current += (separator if current else "") + part
                else:
                    if current:
                        chunks.append(current)
                    current = part
            
            if current:
                chunks.append(current)
            
            # Recursively process chunks that are still too large
            result = []
            for chunk in chunks:
                if len(chunk) > chunk_size:
                    result.extend(recursive_chunk(chunk, chunk_size, separators[1:]))
                else:
                    result.append(chunk)
            
            return result
    
    # Fallback to fixed chunking
    return fixed_chunk(text, chunk_size)
```

### 2.2 Metadata Enrichment

```cpp
struct EnrichedChunk {
    std::string text;
    std::string source_document;
    size_t chunk_index;
    size_t start_char;
    size_t end_char;
    
    // Semantic metadata
    std::vector<std::string> keywords;
    std::string summary;
    float importance_score;
    
    // Structural metadata
    std::optional<std::string> section_title;
    int heading_level;
    bool is_code;
    std::string language;  // for code chunks
};
```

## 3. Embedding Selection

### 3.1 Model Comparison

| Model | Dim | MTEB Score | Speed | License |
|-------|-----|------------|-------|---------|
| OpenAI text-embedding-3-large | 3072 | 64.6 | API | Commercial |
| OpenAI text-embedding-3-small | 1536 | 62.3 | API | Commercial |
| Cohere embed-v3 | 1024 | 64.5 | API | Commercial |
| **E5-large-v2** | 1024 | 62.0 | Local | MIT |
| **BGE-large-en-v1.5** | 1024 | 63.6 | Local | MIT |
| **Nomic-embed-text-v1.5** | 768 | 62.3 | Local | Apache |

For production with Hektor, we recommend **BGE-large-en-v1.5** (best quality) or **Nomic-embed-text-v1.5** (best speed/quality tradeoff).

### 3.2 Local Embedding with ONNX Runtime

```cpp
class ONNXEmbedder {
    Ort::Session session_;
    Ort::Env env_;
    
public:
    ONNXEmbedder(const std::string& model_path)
        : env_(ORT_LOGGING_LEVEL_WARNING, "hektor")
        , session_(env_, model_path.c_str(), Ort::SessionOptions{})
    {}
    
    std::vector<float> embed(const std::string& text) {
        // Tokenize
        auto tokens = tokenize(text);
        
        // Create input tensors
        std::vector<int64_t> input_ids = tokens.input_ids;
        std::vector<int64_t> attention_mask = tokens.attention_mask;
        
        // Run inference
        auto output = session_.Run(
            Ort::RunOptions{nullptr},
            input_names_.data(),
            input_tensors_.data(),
            input_tensors_.size(),
            output_names_.data(),
            output_names_.size()
        );
        
        // Extract embeddings (mean pooling)
        return mean_pool(output[0], attention_mask);
    }
};
```

## 4. Retrieval Optimization

### 4.1 Two-Stage Retrieval

```
Stage 1: Fast Approximate Search (HNSW)
├── Retrieve top-100 candidates
├── Latency: ~2ms
└── Recall@100: ~95%

Stage 2: Precise Reranking (Cross-Encoder)
├── Rerank top-100 to top-10
├── Latency: ~50ms
└── Precision@10: ~98%
```

### 4.2 Hektor's Hybrid RAG Pipeline

```cpp
class RAGPipeline {
    HybridIndex index_;          // Vector + BM25
    CrossEncoder reranker_;      // Optional reranking
    LLMEngine llm_;              // Local or API
    
public:
    std::string query(const std::string& question, const RAGConfig& config) {
        // Stage 1: Hybrid retrieval
        auto query_embedding = embed(question);
        auto candidates = index_.hybrid_search(
            question,
            query_embedding,
            config.initial_k  // e.g., 100
        );
        
        // Stage 2: Reranking (if enabled)
        if (config.use_reranker) {
            candidates = reranker_.rerank(question, candidates, config.final_k);
        } else {
            candidates.resize(config.final_k);
        }
        
        // Stage 3: Context building
        std::string context = build_context(candidates, config.max_context_tokens);
        
        // Stage 4: Generation
        std::string prompt = format_prompt(question, context, config.prompt_template);
        return llm_.generate(prompt, config.generation_params);
    }
    
private:
    std::string build_context(
        const std::vector<RetrievedDoc>& docs,
        size_t max_tokens
    ) {
        std::string context;
        size_t token_count = 0;
        
        for (const auto& doc : docs) {
            size_t doc_tokens = count_tokens(doc.text);
            if (token_count + doc_tokens > max_tokens) break;
            
            context += format_document(doc);
            token_count += doc_tokens;
        }
        
        return context;
    }
};
```

### 4.3 Query Transformation

Improve retrieval by transforming queries:

```cpp
class QueryTransformer {
public:
    // HyDE: Hypothetical Document Embedding
    std::string hyde_transform(const std::string& query, LLMEngine& llm) {
        std::string prompt = 
            "Write a passage that would answer this question:\n" + query;
        return llm.generate(prompt);
    }
    
    // Multi-query: Generate query variations
    std::vector<std::string> multi_query(const std::string& query, LLMEngine& llm) {
        std::string prompt = 
            "Generate 3 different ways to ask this question:\n" + query;
        return parse_queries(llm.generate(prompt));
    }
    
    // Step-back: Generate broader queries
    std::string step_back(const std::string& query, LLMEngine& llm) {
        std::string prompt = 
            "What broader concept is this question about?\n" + query;
        return llm.generate(prompt);
    }
};
```

## 5. Context Management

### 5.1 Context Window Optimization

```cpp
struct ContextWindow {
    size_t max_tokens;
    size_t reserved_for_query;
    size_t reserved_for_response;
    
    size_t available_for_context() const {
        return max_tokens - reserved_for_query - reserved_for_response;
    }
};

std::string optimize_context(
    const std::vector<RetrievedDoc>& docs,
    const ContextWindow& window,
    const std::string& query
) {
    size_t available = window.available_for_context();
    
    // Strategy 1: Truncate long documents
    std::vector<std::string> truncated;
    for (const auto& doc : docs) {
        if (count_tokens(doc.text) > available / docs.size()) {
            truncated.push_back(truncate_to_tokens(doc.text, available / docs.size()));
        } else {
            truncated.push_back(doc.text);
        }
    }
    
    // Strategy 2: Interleave for diversity
    std::string context;
    size_t used = 0;
    
    for (size_t i = 0; i < truncated.size() && used < available; ++i) {
        size_t doc_tokens = count_tokens(truncated[i]);
        if (used + doc_tokens <= available) {
            context += "\n\n[Document " + std::to_string(i + 1) + "]\n";
            context += truncated[i];
            used += doc_tokens;
        }
    }
    
    return context;
}
```

### 5.2 Lost-in-the-Middle Mitigation

LLMs attend more to the beginning and end of context. Mitigation:

```cpp
std::string reorder_context(const std::vector<RetrievedDoc>& docs) {
    // Place most relevant at start AND end
    std::vector<RetrievedDoc> reordered;
    
    // Odd indices from start
    for (size_t i = 0; i < docs.size(); i += 2) {
        reordered.push_back(docs[i]);
    }
    
    // Even indices from end (reversed)
    for (size_t i = (docs.size() > 1 ? docs.size() - 1 : 0); i > 0; i -= 2) {
        reordered.push_back(docs[i]);
    }
    
    return format_documents(reordered);
}
```

## 6. Prompt Engineering

### 6.1 System Prompt Template

```cpp
const std::string SYSTEM_PROMPT = R"(
You are a helpful assistant that answers questions based on the provided context.

Rules:
1. Only use information from the context to answer
2. If the context doesn't contain the answer, say "I don't have enough information"
3. Cite sources using [Document N] format
4. Be concise but complete

Context:
{context}
)";
```

### 6.2 Few-Shot Examples

```cpp
const std::string FEW_SHOT_EXAMPLES = R"(
Example 1:
Context: [Document 1] The Eiffel Tower was built in 1889.
Question: When was the Eiffel Tower built?
Answer: The Eiffel Tower was built in 1889 [Document 1].

Example 2:
Context: [Document 1] Python was created by Guido van Rossum.
Question: What is the population of France?
Answer: I don't have enough information to answer this question. The provided context only discusses Python's creator.
)";
```

## 7. Evaluation Metrics

### 7.1 Retrieval Metrics

```cpp
struct RetrievalMetrics {
    // Recall@k: fraction of relevant docs in top-k
    float recall_at_k(
        const std::vector<uint64_t>& retrieved,
        const std::set<uint64_t>& relevant,
        size_t k
    ) {
        size_t found = 0;
        for (size_t i = 0; i < std::min(k, retrieved.size()); ++i) {
            if (relevant.count(retrieved[i])) found++;
        }
        return static_cast<float>(found) / relevant.size();
    }
    
    // MRR: Mean Reciprocal Rank
    float mrr(
        const std::vector<uint64_t>& retrieved,
        const std::set<uint64_t>& relevant
    ) {
        for (size_t i = 0; i < retrieved.size(); ++i) {
            if (relevant.count(retrieved[i])) {
                return 1.0f / (i + 1);
            }
        }
        return 0.0f;
    }
    
    // NDCG: Normalized Discounted Cumulative Gain
    float ndcg_at_k(
        const std::vector<uint64_t>& retrieved,
        const std::map<uint64_t, float>& relevance,
        size_t k
    );
};
```

### 7.2 Generation Metrics

| Metric | Description | Tool |
|--------|-------------|------|
| Faithfulness | Does answer match context? | RAGAS |
| Answer Relevance | Does answer address question? | RAGAS |
| Context Precision | Are retrieved docs relevant? | RAGAS |
| Context Recall | Are all needed docs retrieved? | RAGAS |

## 8. Production Considerations

### 8.1 Caching

```cpp
class RAGCache {
    LRUCache<std::string, std::vector<float>> embedding_cache_;
    LRUCache<std::string, std::vector<RetrievedDoc>> retrieval_cache_;
    
public:
    std::vector<float> get_or_embed(
        const std::string& text,
        Embedder& embedder
    ) {
        if (auto cached = embedding_cache_.get(text)) {
            return *cached;
        }
        
        auto embedding = embedder.embed(text);
        embedding_cache_.put(text, embedding);
        return embedding;
    }
};
```

### 8.2 Streaming Responses

```cpp
void stream_rag_response(
    const std::string& query,
    std::function<void(const std::string&)> on_token
) {
    // Retrieve first (fast)
    auto docs = retrieve(query);
    
    // Stream generation
    std::string context = build_context(docs);
    std::string prompt = format_prompt(query, context);
    
    llm_.generate_streaming(prompt, [&](const std::string& token) {
        on_token(token);
    });
}
```

## 9. Hektor RAG API

### 9.1 Python Interface

```python
from hektor import RAG, Index

# Create index
index = Index(dim=1024, metric="cosine")

# Add documents
index.add_documents([
    {"id": 1, "text": "Document 1 content..."},
    {"id": 2, "text": "Document 2 content..."},
])

# Create RAG pipeline
rag = RAG(
    index=index,
    embedder="bge-large-en-v1.5",
    llm="llama-3-8b",
    reranker="bge-reranker-large"
)

# Query
response = rag.query(
    "What is the capital of France?",
    k=5,
    max_tokens=500
)

print(response.answer)
print(response.sources)
```

## 10. Conclusion

Production RAG requires careful attention to:

1. **Chunking**: Preserve semantic coherence
2. **Retrieval**: Combine vector and lexical search
3. **Reranking**: Precision over recall
4. **Context**: Optimize for LLM attention patterns
5. **Prompting**: Guide the model to use context

Hektor provides all components in a unified, high-performance package.

---

## References

1. Lewis, P., et al. (2020). Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks.
2. Gao, L., et al. (2023). Precise Zero-Shot Dense Retrieval without Relevance Labels.
3. Liu, N., et al. (2023). Lost in the Middle: How Language Models Use Long Contexts.

---

*© 2026 ARTIFACT VIRTUAL. Technical article for Hektor Vector Database.*
