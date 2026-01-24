// ============================================================================
// VectorDB - Metadata Storage Implementation
// This is part of mmap_store.cpp, but defined here for organizational clarity
// ============================================================================

// Note: The MetadataStore implementation is in mmap_store.cpp
// This file exists for future expansion of metadata capabilities

#include "vdb/storage.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

namespace vdb {

// ============================================================================
// DatabasePaths Implementation
// ============================================================================

DatabasePaths::DatabasePaths(const fs::path& root_path)
    : root(root_path)
    , vectors(root / "vectors.bin")
    , index(root / "index.hnsw")
    , metadata(root / "metadata.jsonl")
    , config(root / "config.json")
    , models(root / "models")
    , text_model(models / "text_encoder.onnx")
    , image_model(models / "image_encoder.onnx")
    , projection(models / "projection.bin")
{}

Result<void> DatabasePaths::ensure_dirs() const {
    try {
        fs::create_directories(root);
        fs::create_directories(models);
        return {};
    } catch (const fs::filesystem_error& e) {
        return tl::unexpected(Error{ErrorCode::IoError, 
                    std::string("Failed to create directories: ") + e.what()});
    }
}

bool DatabasePaths::exists() const {
    return fs::exists(vectors) && fs::exists(metadata);
}

// ============================================================================
// Metadata Serialization Helpers
// ============================================================================

std::string document_type_to_string(DocumentType type) {
    switch (type) {
        case DocumentType::Journal: return "journal";
        case DocumentType::Chart: return "chart";
        case DocumentType::CatalystWatchlist: return "catalyst_watchlist";
        case DocumentType::InstitutionalMatrix: return "institutional_matrix";
        case DocumentType::EconomicCalendar: return "economic_calendar";
        case DocumentType::WeeklyRundown: return "weekly_rundown";
        case DocumentType::ThreeMonthReport: return "three_month_report";
        case DocumentType::OneYearReport: return "one_year_report";
        case DocumentType::PreMarket: return "premarket";
        default: return "unknown";
    }
}

DocumentType string_to_document_type(std::string_view str) {
    if (str == "journal") return DocumentType::Journal;
    if (str == "chart") return DocumentType::Chart;
    if (str == "catalyst_watchlist") return DocumentType::CatalystWatchlist;
    if (str == "institutional_matrix") return DocumentType::InstitutionalMatrix;
    if (str == "economic_calendar") return DocumentType::EconomicCalendar;
    if (str == "weekly_rundown") return DocumentType::WeeklyRundown;
    if (str == "three_month_report") return DocumentType::ThreeMonthReport;
    if (str == "one_year_report") return DocumentType::OneYearReport;
    if (str == "premarket") return DocumentType::PreMarket;
    return DocumentType::Journal;  // Default
}

} // namespace vdb
