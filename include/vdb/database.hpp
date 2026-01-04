#pragma once
// ============================================================================
// VectorDB - Main Database Interface
// ============================================================================

#include "core.hpp"
#include "index.hpp"
#include "storage.hpp"
#include "distance.hpp"
#ifdef VDB_USE_ONNX_RUNTIME
#include "embeddings/text.hpp"
#include "embeddings/image.hpp"
#endif
#include <filesystem>

namespace vdb {

namespace fs = std::filesystem;

#ifdef VDB_USE_ONNX_RUNTIME
// Import embeddings types when ONNX Runtime is available
using embeddings::TextEncoder;
using embeddings::ImageEncoder;
using embeddings::Device;
#endif

// ============================================================================
// Database Configuration
// ============================================================================

struct DatabaseConfig {
    fs::path path;                          // Database directory
    Dim dimension = UNIFIED_DIM;            // 512 unified dimension
    DistanceMetric metric = DistanceMetric::Cosine;
    
    // Index settings
    size_t hnsw_m = HNSW_M;
    size_t hnsw_ef_construction = HNSW_EF_CONSTRUCTION;
    size_t hnsw_ef_search = HNSW_EF_SEARCH;
    size_t max_elements = HNSW_MAX_ELEMENTS;
    
    // Embedding settings
    std::string text_model_path;            // Path to text ONNX model
    std::string image_model_path;           // Path to image ONNX model
    std::string vocab_path;                 // Path to tokenizer vocab
    bool auto_download_models = true;       // Download if missing
    
    // Execution
    int num_threads = 0;                    // 0 = auto
    
    // Storage
    bool memory_only = false;               // For testing
    bool auto_sync = true;                  // Sync after each write
    size_t sync_interval_ms = 5000;         // Batch sync interval
};

// ============================================================================
// Query Options
// ============================================================================

struct QueryOptions {
    size_t k = 10;                          // Number of results
    size_t ef_search = 0;                   // 0 = use default
    
    // Filters
    std::optional<DocumentType> type_filter;
    std::optional<std::string> date_filter;      // Exact date match
    std::optional<std::string> date_from;        // Date range start
    std::optional<std::string> date_to;          // Date range end
    std::optional<std::string> asset_filter;     // For charts
    std::optional<std::string> bias_filter;      // BULLISH, BEARISH, NEUTRAL
    
    // Reranking
    bool include_metadata = true;
    bool deduplicate_by_date = false;       // One result per date
};

// ============================================================================
// Query Result
// ============================================================================

struct QueryResult {
    VectorId id;
    Distance distance;
    float score;                            // 1.0 - distance (similarity)
    std::optional<Metadata> metadata;
    
    bool operator<(const QueryResult& other) const {
        return score > other.score;  // Higher score first
    }
};

using QueryResults = std::vector<QueryResult>;

// ============================================================================
// Ingest Options
// ============================================================================

struct IngestOptions {
    bool overwrite = false;                 // Replace if exists
    bool extract_metadata = true;           // Parse metadata from content
    std::optional<std::string> date;        // Override date
    std::optional<DocumentType> type;       // Override type
    std::optional<std::string> bias;        // Override bias
};

// ============================================================================
// VectorDatabase - Main Interface
// ============================================================================

class VectorDatabase {
public:
    /// Create or open database at path
    explicit VectorDatabase(const DatabaseConfig& config);
    ~VectorDatabase();
    
    // Non-copyable, but movable with custom implementation
    VectorDatabase(const VectorDatabase&) = delete;
    VectorDatabase& operator=(const VectorDatabase&) = delete;
    VectorDatabase(VectorDatabase&&) noexcept;
    VectorDatabase& operator=(VectorDatabase&&) noexcept;
    
    // ========================================================================
    // Initialization
    // ========================================================================
    
    /// Initialize database (load or create)
    [[nodiscard]] Result<void> init();
    
    /// Check if ready
    [[nodiscard]] bool is_ready() const { return ready_; }
    
    /// Get configuration
    [[nodiscard]] const DatabaseConfig& config() const { return config_; }
    
    // ========================================================================
    // Text Operations
    // ========================================================================
    
    /// Add text document
    [[nodiscard]] Result<VectorId> add_text(
        std::string_view text,
        const Metadata& metadata,
        const IngestOptions& options = {}
    );
    
    /// Add text with auto-generated metadata
    [[nodiscard]] Result<VectorId> add_text(
        std::string_view text,
        DocumentType type,
        std::string_view date,
        const IngestOptions& options = {}
    );
    
    /// Query by text
    [[nodiscard]] Result<QueryResults> query_text(
        std::string_view query,
        const QueryOptions& options = {}
    );
    
    // ========================================================================
    // Image Operations
    // ========================================================================
    
    /// Add image from file
    [[nodiscard]] Result<VectorId> add_image(
        const fs::path& image_path,
        const Metadata& metadata,
        const IngestOptions& options = {}
    );
    
    /// Add image with auto-generated metadata
    [[nodiscard]] Result<VectorId> add_image(
        const fs::path& image_path,
        const IngestOptions& options = {}
    );
    
    /// Query by image
    [[nodiscard]] Result<QueryResults> query_image(
        const fs::path& image_path,
        const QueryOptions& options = {}
    );
    
    // ========================================================================
    // Direct Vector Operations
    // ========================================================================
    
    /// Add pre-computed vector
    [[nodiscard]] Result<VectorId> add_vector(
        VectorView vector,
        const Metadata& metadata
    );
    
    /// Query by vector
    [[nodiscard]] Result<QueryResults> query_vector(
        VectorView query,
        const QueryOptions& options = {}
    );
    
    /// Get vector by ID
    [[nodiscard]] std::optional<Vector> get_vector(VectorId id) const;
    
    // ========================================================================
    // Metadata Operations
    // ========================================================================
    
    /// Get metadata by ID
    [[nodiscard]] std::optional<Metadata> get_metadata(VectorId id) const;
    
    /// Update metadata
    [[nodiscard]] Result<void> update_metadata(VectorId id, const Metadata& metadata);
    
    /// Find by metadata
    [[nodiscard]] std::vector<Metadata> find_by_date(std::string_view date) const;
    [[nodiscard]] std::vector<Metadata> find_by_type(DocumentType type) const;
    [[nodiscard]] std::vector<Metadata> find_by_asset(std::string_view asset) const;
    
    // ========================================================================
    // Deletion
    // ========================================================================
    
    /// Remove vector and metadata
    [[nodiscard]] Result<void> remove(VectorId id);
    
    /// Remove all vectors for a date
    [[nodiscard]] Result<size_t> remove_by_date(std::string_view date);
    
    // ========================================================================
    // Bulk Operations
    // ========================================================================
    
    /// Add multiple texts
    [[nodiscard]] Result<std::vector<VectorId>> add_texts(
        std::span<const std::string> texts,
        std::span<const Metadata> metadata
    );
    
    /// Add multiple images
    [[nodiscard]] Result<std::vector<VectorId>> add_images(
        std::span<const fs::path> paths,
        std::span<const Metadata> metadata
    );
    
    // ========================================================================
    // Statistics & Management
    // ========================================================================
    
    /// Get total vector count
    [[nodiscard]] size_t size() const;
    
    /// Get count by type
    [[nodiscard]] size_t count_by_type(DocumentType type) const;
    
    /// Get all dates
    [[nodiscard]] std::vector<std::string> all_dates() const;
    
    /// Get index statistics
    [[nodiscard]] IndexStats stats() const;
    
    /// Optimize index
    void optimize();
    
    /// Sync to disk
    [[nodiscard]] Result<void> sync();
    
    /// Compact storage
    [[nodiscard]] Result<void> compact();
    
    // ========================================================================
    // Export
    // ========================================================================
    
    /// Export all data for training (text + metadata as JSONL)
    [[nodiscard]] Result<void> export_training_data(const fs::path& output_path) const;

private:
    /// Generate next vector ID
    [[nodiscard]] VectorId next_id();
    
    /// Apply filters to search results
    [[nodiscard]] QueryResults apply_filters(
        const SearchResults& raw_results,
        const QueryOptions& options
    ) const;
    
    /// Project text embedding to unified dimension
    [[nodiscard]] Vector project_text_embedding(const Vector& text_emb);
    
    /// Ensure models are downloaded
    [[nodiscard]] Result<void> ensure_models();
    
    DatabaseConfig config_;
    DatabasePaths paths_;
    
    std::unique_ptr<HnswIndex> index_;
    std::unique_ptr<VectorStore> vectors_;
    std::unique_ptr<MetadataStore> metadata_;
#ifdef VDB_USE_ONNX_RUNTIME
    std::unique_ptr<TextEncoder> text_encoder_;
    std::unique_ptr<ImageEncoder> image_encoder_;
    std::unique_ptr<ProjectionMatrix> text_projection_;
#endif
    
    VectorId next_id_ = 1;
    bool ready_ = false;
    mutable std::shared_mutex mutex_;
};

// ============================================================================
// Convenience Factory
// ============================================================================

/// Create database with default settings for Gold Standard
[[nodiscard]] Result<VectorDatabase> create_gold_standard_db(const fs::path& path);

/// Open existing database
[[nodiscard]] Result<VectorDatabase> open_database(const fs::path& path);

} // namespace vdb
