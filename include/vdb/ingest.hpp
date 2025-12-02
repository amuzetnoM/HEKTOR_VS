#pragma once
// ============================================================================
// VectorDB - Gold Standard Ingest Module
// ============================================================================

#include "database.hpp"
#include <regex>

namespace vdb {

// ============================================================================
// Markdown Parser
// ============================================================================

struct ParsedDocument {
    std::string title;
    std::string date;
    DocumentType type = DocumentType::Unknown;
    std::string bias;
    std::string content;                    // Full text content
    std::string summary;                    // Executive summary if present
    
    // Extracted values
    std::optional<float> gold_price;
    std::optional<float> silver_price;
    std::optional<float> gsr;
    std::optional<float> dxy;
    std::optional<float> vix;
    std::optional<float> yield_10y;
    
    // Sections for chunking
    std::vector<std::string> sections;
    
    // Chart references
    std::vector<std::string> chart_paths;
    
    // Tables (for structured data)
    struct Table {
        std::vector<std::string> headers;
        std::vector<std::vector<std::string>> rows;
    };
    std::vector<Table> tables;
};

/// Parse Gold Standard markdown document
[[nodiscard]] Result<ParsedDocument> parse_markdown(
    std::string_view content,
    std::string_view filename = ""
);

/// Detect document type from filename or content
[[nodiscard]] DocumentType detect_document_type(
    std::string_view filename,
    std::string_view content = ""
);

/// Extract date from filename (e.g., "Journal_2025-12-01.md" -> "2025-12-01")
[[nodiscard]] std::optional<std::string> extract_date_from_filename(std::string_view filename);

/// Extract market data from header line
/// e.g., "Gold: $4,220.50 | DXY: 103.00 | VIX: 17.00 | 10Y: 4.30%"
struct MarketData {
    std::optional<float> gold_price;
    std::optional<float> silver_price;
    std::optional<float> dxy;
    std::optional<float> vix;
    std::optional<float> yield_10y;
    std::optional<float> gsr;
    std::optional<std::string> bias;
};

[[nodiscard]] MarketData extract_market_data(std::string_view text);

// ============================================================================
// Gold Standard Ingest Pipeline
// ============================================================================

struct IngestStats {
    size_t journals_added = 0;
    size_t charts_added = 0;
    size_t reports_added = 0;
    size_t skipped = 0;
    size_t errors = 0;
    std::vector<std::string> error_messages;
};

struct IngestConfig {
    fs::path gold_standard_output;          // path/to/gold_standard/output
    bool include_journals = true;
    bool include_charts = true;
    bool include_reports = true;
    bool overwrite_existing = false;
    bool incremental = true;                // Only add new files
    std::optional<std::string> since_date;  // Only files after this date
};

class GoldStandardIngest {
public:
    explicit GoldStandardIngest(VectorDatabase& db);
    
    /// Run full ingest from Gold Standard output directory
    [[nodiscard]] Result<IngestStats> ingest(const IngestConfig& config);
    
    /// Ingest single journal file
    [[nodiscard]] Result<VectorId> ingest_journal(const fs::path& path);
    
    /// Ingest single chart file
    [[nodiscard]] Result<VectorId> ingest_chart(const fs::path& path);
    
    /// Ingest single report file
    [[nodiscard]] Result<VectorId> ingest_report(const fs::path& path);
    
    /// Watch directory for new files (returns on stop signal)
    [[nodiscard]] Result<void> watch(
        const IngestConfig& config,
        std::function<bool()> should_stop
    );
    
    /// Get last ingest stats
    [[nodiscard]] const IngestStats& last_stats() const { return last_stats_; }

private:
    /// Discover all files in output directory
    [[nodiscard]] std::vector<fs::path> discover_files(const IngestConfig& config);
    
    /// Check if file already ingested
    [[nodiscard]] bool is_ingested(const fs::path& path) const;
    
    /// Create metadata from parsed document
    [[nodiscard]] Metadata create_metadata(
        const ParsedDocument& doc,
        const fs::path& source_path
    );
    
    VectorDatabase& db_;
    IngestStats last_stats_;
    std::unordered_set<std::string> ingested_files_;
};

// ============================================================================
// Chunking Strategies
// ============================================================================

enum class ChunkStrategy {
    None,           // Embed entire document
    BySection,      // Split by markdown headers
    ByParagraph,    // Split by double newline
    FixedSize,      // Fixed character count with overlap
    Semantic        // Smart sentence-based splitting
};

struct ChunkConfig {
    ChunkStrategy strategy = ChunkStrategy::BySection;
    size_t max_chunk_size = 512;            // Characters
    size_t overlap = 50;                    // Character overlap for FixedSize
    bool include_header_context = true;     // Prepend section header to chunks
};

/// Split document into chunks for embedding
[[nodiscard]] std::vector<std::string> chunk_document(
    const ParsedDocument& doc,
    const ChunkConfig& config = {}
);

/// Split text into chunks
[[nodiscard]] std::vector<std::string> chunk_text(
    std::string_view text,
    const ChunkConfig& config = {}
);

// ============================================================================
// Content Hashing
// ============================================================================

/// Compute SHA-256 hash of content
[[nodiscard]] std::string content_hash(std::string_view content);

/// Compute hash of file
[[nodiscard]] Result<std::string> file_hash(const fs::path& path);

} // namespace vdb
