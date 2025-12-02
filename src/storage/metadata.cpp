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
        return Error{ErrorCode::IoError, 
                    std::string("Failed to create directories: ") + e.what()};
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
        case DocumentType::JOURNAL: return "journal";
        case DocumentType::CHART: return "chart";
        case DocumentType::CATALYST_WATCHLIST: return "catalyst_watchlist";
        case DocumentType::INSTITUTIONAL_MATRIX: return "institutional_matrix";
        case DocumentType::ECONOMIC_CALENDAR: return "economic_calendar";
        case DocumentType::WEEKLY_RUNDOWN: return "weekly_rundown";
        case DocumentType::THREE_MONTH_REPORT: return "three_month_report";
        case DocumentType::ONE_YEAR_REPORT: return "one_year_report";
        case DocumentType::PREMARKET: return "premarket";
        default: return "unknown";
    }
}

DocumentType string_to_document_type(std::string_view str) {
    if (str == "journal") return DocumentType::JOURNAL;
    if (str == "chart") return DocumentType::CHART;
    if (str == "catalyst_watchlist") return DocumentType::CATALYST_WATCHLIST;
    if (str == "institutional_matrix") return DocumentType::INSTITUTIONAL_MATRIX;
    if (str == "economic_calendar") return DocumentType::ECONOMIC_CALENDAR;
    if (str == "weekly_rundown") return DocumentType::WEEKLY_RUNDOWN;
    if (str == "three_month_report") return DocumentType::THREE_MONTH_REPORT;
    if (str == "one_year_report") return DocumentType::ONE_YEAR_REPORT;
    if (str == "premarket") return DocumentType::PREMARKET;
    return DocumentType::JOURNAL;  // Default
}

} // namespace vdb
