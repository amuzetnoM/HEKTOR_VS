#pragma once
// ============================================================================
// VectorDB - Deep Learning Framework Integration
// ============================================================================

#include "core.hpp"
#include "database.hpp"
#include <string>
#include <vector>
#include <functional>

namespace vdb {
namespace framework {

// ============================================================================
// TensorFlow Integration
// ============================================================================

struct TensorFlowConfig {
    std::string model_path;          // Path to SavedModel directory
    std::string input_tensor_name = "serving_default_input";
    std::string output_tensor_name = "serving_default_output";
    int num_threads = 0;             // 0 = auto
    bool use_gpu = false;
};

class TensorFlowEmbedder {
public:
    explicit TensorFlowEmbedder(const TensorFlowConfig& config);
    ~TensorFlowEmbedder();
    
    // Embed single text/data
    Result<Vector> embed(const std::string& text) const;
    Result<std::vector<Vector>> embed_batch(const std::vector<std::string>& texts) const;
    
    // Export vectors for TensorFlow training
    static Result<void> export_for_training(
        const std::vector<Vector>& vectors,
        const std::vector<std::string>& labels,
        const std::string& output_path
    );
    
    // Model info
    Dim dimension() const;
    bool is_loaded() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// PyTorch Integration
// ============================================================================

struct PyTorchConfig {
    std::string model_path;          // Path to TorchScript model (.pt)
    std::string device = "cpu";      // "cpu", "cuda", "cuda:0", etc.
    int num_threads = 0;
    bool use_half_precision = false; // FP16 for faster inference
};

class PyTorchEmbedder {
public:
    explicit PyTorchEmbedder(const PyTorchConfig& config);
    ~PyTorchEmbedder();
    
    // Embed single text/data
    Result<Vector> embed(const std::string& text) const;
    Result<std::vector<Vector>> embed_batch(const std::vector<std::string>& texts) const;
    
    // Export vectors for PyTorch training
    static Result<void> export_for_training(
        const std::vector<Vector>& vectors,
        const std::vector<std::string>& labels,
        const std::string& output_path
    );
    
    // Load trained model
    static Result<PyTorchEmbedder> from_trained(
        const std::string& model_path,
        const std::string& device = "cpu"
    );
    
    // Model info
    Dim dimension() const;
    bool is_loaded() const;
    std::string device() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// RAG (Retrieval Augmented Generation) Toolkit
// ============================================================================

struct RAGConfig {
    size_t top_k = 5;                // Number of documents to retrieve
    size_t max_context_length = 2048; // Maximum context tokens
    float relevance_threshold = 0.7f; // Minimum similarity score
    bool rerank = true;               // Apply reranking
    std::string chunking_strategy = "sentence";  // sentence, fixed, semantic
    size_t chunk_size = 512;          // Characters per chunk
    size_t chunk_overlap = 50;        // Overlap between chunks
};

struct RAGContext {
    std::vector<std::string> retrieved_chunks;
    std::vector<float> relevance_scores;
    std::vector<VectorId> document_ids;
    std::string formatted_context;
    size_t total_tokens;
};

class RAGEngine {
public:
    explicit RAGEngine(const RAGConfig& config = {});
    ~RAGEngine();
    
    // Chunk documents for indexing
    Result<std::vector<std::string>> chunk_document(
        const std::string& document
    ) const;
    
    // Build context from query
    Result<RAGContext> build_context(
        const std::string& query,
        const std::vector<QueryResult>& search_results
    ) const;
    
    // Format context for LLM
    std::string format_prompt(
        const std::string& query,
        const RAGContext& context,
        const std::string& system_prompt = ""
    ) const;
    
    // Reranking
    Result<std::vector<QueryResult>> rerank(
        const std::string& query,
        const std::vector<QueryResult>& results
    ) const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// LangChain Integration
// ============================================================================

struct LangChainConfig {
    std::string vectorstore_type = "vdb";
    bool use_mmr = false;              // Maximal Marginal Relevance
    float mmr_lambda = 0.5f;           // Diversity vs relevance tradeoff
    size_t fetch_k = 20;               // Initial retrieval before MMR
};

class LangChainAdapter {
public:
    explicit LangChainAdapter(const LangChainConfig& config = {});
    ~LangChainAdapter();
    
    // Similarity search (standard retrieval)
    Result<std::vector<QueryResult>> similarity_search(
        const std::string& query,
        size_t k = 4
    ) const;
    
    // Similarity search with score
    Result<std::vector<QueryResult>> similarity_search_with_score(
        const std::string& query,
        size_t k = 4
    ) const;
    
    // Maximal Marginal Relevance search
    Result<std::vector<QueryResult>> max_marginal_relevance_search(
        const std::string& query,
        size_t k = 4,
        size_t fetch_k = 20,
        float lambda = 0.5f
    ) const;
    
    // As retriever (returns documents)
    Result<std::vector<std::string>> as_retriever(
        const std::string& query,
        size_t k = 4
    ) const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// LlamaIndex Integration
// ============================================================================

struct LlamaIndexConfig {
    std::string index_type = "vector_store";  // vector_store, keyword_table, list
    size_t chunk_size = 1024;
    size_t chunk_overlap = 20;
    bool include_metadata = true;
};

class LlamaIndexAdapter {
public:
    explicit LlamaIndexAdapter(const LlamaIndexConfig& config = {});
    ~LlamaIndexAdapter();
    
    // Build index from documents
    Result<void> build_index(
        const std::vector<std::string>& documents,
        const std::vector<Metadata>& metadata = {}
    );
    
    // Query index
    Result<RAGContext> query(
        const std::string& query,
        size_t k = 5
    ) const;
    
    // Update index
    Result<void> insert(const std::string& document, const Metadata& metadata = {});
    Result<void> delete_document(VectorId id);
    
    // Export/Import
    Result<void> save(const std::string& path) const;
    static Result<LlamaIndexAdapter> load(const std::string& path);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Document Chunking Strategies
// ============================================================================

enum class ChunkingStrategy {
    Fixed,           // Fixed character/token length
    Sentence,        // Split by sentences
    Paragraph,       // Split by paragraphs
    Semantic,        // Semantic similarity-based splitting
    Recursive        // Recursive character text splitting
};

struct ChunkingConfig {
    ChunkingStrategy strategy = ChunkingStrategy::Sentence;
    size_t chunk_size = 512;
    size_t chunk_overlap = 50;
    std::string separator = "\n\n";
    bool keep_separator = false;
};

class DocumentChunker {
public:
    explicit DocumentChunker(const ChunkingConfig& config = {});
    ~DocumentChunker();
    
    // Chunk document
    Result<std::vector<std::string>> chunk(const std::string& document) const;
    
    // Chunk with metadata preservation
    struct ChunkWithMetadata {
        std::string text;
        size_t start_char;
        size_t end_char;
        Metadata metadata;
    };
    
    Result<std::vector<ChunkWithMetadata>> chunk_with_metadata(
        const std::string& document,
        const Metadata& base_metadata = {}
    ) const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Training Data Export
// ============================================================================

enum class ExportFormat {
    TFRecord,        // TensorFlow TFRecord
    PyTorchDataset,  // PyTorch Dataset
    HuggingFace,     // HuggingFace datasets
    JSONL,           // JSON Lines
    Parquet          // Apache Parquet
};

struct ExportConfig {
    ExportFormat format = ExportFormat::JSONL;
    bool include_metadata = true;
    bool include_embeddings = true;
    bool include_text = true;
    size_t batch_size = 1000;
};

class TrainingExporter {
public:
    explicit TrainingExporter(const ExportConfig& config = {});
    ~TrainingExporter();
    
    // Export for supervised learning
    Result<void> export_supervised(
        const std::vector<Vector>& embeddings,
        const std::vector<std::string>& labels,
        const std::string& output_path
    ) const;
    
    // Export for contrastive learning (pairs/triplets)
    Result<void> export_contrastive_pairs(
        const std::vector<std::pair<Vector, Vector>>& positive_pairs,
        const std::vector<std::pair<Vector, Vector>>& negative_pairs,
        const std::string& output_path
    ) const;
    
    Result<void> export_triplets(
        const std::vector<std::tuple<Vector, Vector, Vector>>& triplets,  // anchor, positive, negative
        const std::string& output_path
    ) const;
    
    // Export full database
    Result<void> export_database(
        const std::string& database_path,
        const std::string& output_path
    ) const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace framework
} // namespace vdb
