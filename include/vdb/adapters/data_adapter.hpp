#pragma once
// ============================================================================
// VectorDB - Universal Data Adapter System
// Intelligent data ingestion with automatic type detection and normalization
// ============================================================================

#include "../core.hpp"
#include <filesystem>
#include <memory>
#include <variant>
#include <unordered_map>

namespace vdb::adapters {

namespace fs = std::filesystem;

// ============================================================================
// Data Types & Formats
// ============================================================================

enum class DataFormat {
    // Text formats
    PlainText,
    Markdown,
    JSON,
    CSV,
    XML,
    
    // Document formats
    PDF,
    DOCX,
    TXT,
    
    // Structured data
    SQL,
    Excel,
    Parquet,
    
    // Web & APIs
    HTML,
    API_JSON,
    RSS,
    
    // Time series & Financial
    Yahoo_Finance,
    Alpha_Vantage,
    
    // Images
    PNG,
    JPEG,
    
    Unknown
};

// ============================================================================
// Normalized Data Structure
// ============================================================================

struct DataChunk {
    std::string content;                    // Main text content
    std::unordered_map<std::string, std::string> metadata;  // Key-value metadata
    std::optional<std::string> title;
    std::optional<std::string> date;
    std::optional<std::string> source;
    std::vector<float> numerical_features;  // Extracted numeric data
    size_t chunk_index = 0;                 // Position in original document
    size_t total_chunks = 1;
};

struct NormalizedData {
    std::vector<DataChunk> chunks;
    DataFormat format;
    std::string source_path;
    std::unordered_map<std::string, std::string> global_metadata;
    
    // Quality metrics
    float confidence = 1.0f;  // Type detection confidence
    bool sanitized = false;
    std::vector<std::string> warnings;
};

// ============================================================================
// Chunking Strategies
// ============================================================================

enum class ChunkStrategy {
    Fixed,          // Fixed character/token count
    Sentence,       // Split by sentences
    Paragraph,      // Split by paragraphs
    Semantic,       // Semantic similarity-based
    Sliding,        // Sliding window with overlap
    Document,       // One chunk per document
};

struct ChunkConfig {
    ChunkStrategy strategy = ChunkStrategy::Paragraph;
    size_t max_chunk_size = 1000;  // characters/tokens
    size_t overlap = 100;           // for sliding window
    bool preserve_structure = true; // Keep headers, formatting
};

// ============================================================================
// Base Data Adapter Interface
// ============================================================================

class IDataAdapter {
public:
    virtual ~IDataAdapter() = default;
    
    /// Detect if this adapter can handle the given data
    [[nodiscard]] virtual bool can_handle(const fs::path& path) const = 0;
    [[nodiscard]] virtual bool can_handle(std::string_view content) const = 0;
    
    /// Parse and normalize data (Read operation)
    [[nodiscard]] virtual Result<NormalizedData> parse(
        const fs::path& path,
        const ChunkConfig& config = {}
    ) = 0;
    
    [[nodiscard]] virtual Result<NormalizedData> parse_content(
        std::string_view content,
        const ChunkConfig& config = {},
        std::string_view source_hint = ""
    ) = 0;
    
    /// Write normalized data to file/database (Write operation)
    [[nodiscard]] virtual Result<void> write(
        const NormalizedData& data,
        const fs::path& path
    ) {
        // Default implementation returns not implemented
        return std::unexpected(Error{ErrorCode::NotImplemented, 
            "Write operation not implemented for " + name() + " adapter"});
    }
    
    /// Validate and sanitize data
    [[nodiscard]] virtual Result<void> sanitize(NormalizedData& data) = 0;
    
    /// Get adapter name and capabilities
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual std::vector<DataFormat> supported_formats() const = 0;
    
    /// Check if adapter supports write operations
    [[nodiscard]] virtual bool supports_write() const {
        return false;  // Default: read-only
    }
};

// ============================================================================
// Universal Data Adapter Manager
// ============================================================================

class DataAdapterManager {
public:
    DataAdapterManager();
    
    /// Register a custom adapter
    void register_adapter(std::unique_ptr<IDataAdapter> adapter);
    
    /// Auto-detect format and parse
    [[nodiscard]] Result<NormalizedData> auto_parse(
        const fs::path& path,
        const ChunkConfig& config = {}
    );
    
    [[nodiscard]] Result<NormalizedData> auto_parse_content(
        std::string_view content,
        const ChunkConfig& config = {},
        std::string_view hint = ""
    );
    
    /// Detect format without parsing
    [[nodiscard]] DataFormat detect_format(const fs::path& path) const;
    [[nodiscard]] DataFormat detect_format_content(std::string_view content) const;
    
    /// Get supported formats
    [[nodiscard]] std::vector<DataFormat> supported_formats() const;
    
    /// Batch processing
    [[nodiscard]] Result<std::vector<NormalizedData>> parse_batch(
        const std::vector<fs::path>& paths,
        const ChunkConfig& config = {},
        size_t max_parallel = 4
    );

private:
    std::vector<std::unique_ptr<IDataAdapter>> adapters_;
    
    IDataAdapter* find_adapter(const fs::path& path) const;
    IDataAdapter* find_adapter_for_content(std::string_view content) const;
};

// ============================================================================
// Helper Functions
// ============================================================================

/// Convert format enum to string
[[nodiscard]] std::string format_to_string(DataFormat format);

/// Detect format from file extension
[[nodiscard]] DataFormat detect_from_extension(const fs::path& path);

/// Sanitize text content (remove control chars, normalize whitespace, etc.)
[[nodiscard]] std::string sanitize_text(std::string_view text);

/// Extract numerical features from text
[[nodiscard]] std::vector<float> extract_numerical_features(std::string_view text);

/// Chunk text based on strategy
[[nodiscard]] std::vector<DataChunk> chunk_text(
    std::string_view text,
    const ChunkConfig& config,
    const std::unordered_map<std::string, std::string>& metadata = {}
);

} // namespace vdb::adapters
