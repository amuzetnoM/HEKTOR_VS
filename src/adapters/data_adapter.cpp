// ============================================================================
// VectorDB - Universal Data Adapter Implementation
// ============================================================================

#include "vdb/adapters/data_adapter.hpp"
#include "vdb/adapters/csv_adapter.hpp"
#include "vdb/adapters/json_adapter.hpp"
#include "vdb/adapters/xml_adapter.hpp"
#include "vdb/adapters/parquet_adapter.hpp"
#include "vdb/adapters/sqlite_adapter.hpp"
#include "vdb/adapters/pgvector_adapter.hpp"
#include "vdb/adapters/http_adapter.hpp"
#include <algorithm>
#include <regex>
#include <cctype>

namespace vdb::adapters {

// ============================================================================
// DataAdapterManager Implementation
// ============================================================================

DataAdapterManager::DataAdapterManager() {
    // Register built-in adapters
    register_adapter(std::make_unique<CSVAdapter>());
    register_adapter(std::make_unique<JSONAdapter>());
    register_adapter(std::make_unique<XMLAdapter>());
    register_adapter(std::make_unique<ParquetAdapter>());
    register_adapter(std::make_unique<SQLiteAdapter>());
    register_adapter(std::make_unique<HTTPAdapter>());
    // PgvectorAdapter requires configuration, registered separately when needed
}

void DataAdapterManager::register_adapter(std::unique_ptr<IDataAdapter> adapter) {
    adapters_.push_back(std::move(adapter));
}

Result<NormalizedData> DataAdapterManager::auto_parse(
    const fs::path& path,
    const ChunkConfig& config
) {
    if (!fs::exists(path)) {
        return tl::unexpected(Error{ErrorCode::IoError, "File does not exist: " + path.string()});
    }
    
    auto* adapter = find_adapter(path);
    if (!adapter) {
        return tl::unexpected(Error{ErrorCode::InvalidData, "No adapter found for: " + path.string()});
    }
    
    return adapter->parse(path, config);
}

Result<NormalizedData> DataAdapterManager::auto_parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view hint
) {
    auto* adapter = find_adapter_for_content(content);
    if (!adapter) {
        return tl::unexpected(Error{ErrorCode::InvalidData, "No adapter found for content"});
    }
    
    return adapter->parse_content(content, config, hint);
}

DataFormat DataAdapterManager::detect_format(const fs::path& path) const {
    // Try extension first
    auto format = detect_from_extension(path);
    if (format != DataFormat::Unknown) {
        return format;
    }
    
    // Try adapters
    for (const auto& adapter : adapters_) {
        if (adapter->can_handle(path)) {
            auto formats = adapter->supported_formats();
            if (!formats.empty()) {
                return formats[0];
            }
        }
    }
    
    return DataFormat::Unknown;
}

DataFormat DataAdapterManager::detect_format_content(std::string_view content) const {
    for (const auto& adapter : adapters_) {
        if (adapter->can_handle(content)) {
            auto formats = adapter->supported_formats();
            if (!formats.empty()) {
                return formats[0];
            }
        }
    }
    return DataFormat::Unknown;
}

std::vector<DataFormat> DataAdapterManager::supported_formats() const {
    std::vector<DataFormat> formats;
    for (const auto& adapter : adapters_) {
        auto adapter_formats = adapter->supported_formats();
        formats.insert(formats.end(), adapter_formats.begin(), adapter_formats.end());
    }
    return formats;
}

Result<std::vector<NormalizedData>> DataAdapterManager::parse_batch(
    const std::vector<fs::path>& paths,
    const ChunkConfig& config,
    size_t max_parallel
) {
    std::vector<NormalizedData> results;
    results.reserve(paths.size());
    
    // Simple sequential implementation for now
    // TODO: Add parallel processing with thread pool
    for (const auto& path : paths) {
        auto result = auto_parse(path, config);
        if (!result) {
            return tl::unexpected(result.error());
        }
        results.push_back(std::move(*result));
    }
    
    return results;
}

IDataAdapter* DataAdapterManager::find_adapter(const fs::path& path) const {
    for (const auto& adapter : adapters_) {
        if (adapter->can_handle(path)) {
            return adapter.get();
        }
    }
    return nullptr;
}

IDataAdapter* DataAdapterManager::find_adapter_for_content(std::string_view content) const {
    for (const auto& adapter : adapters_) {
        if (adapter->can_handle(content)) {
            return adapter.get();
        }
    }
    return nullptr;
}

// ============================================================================
// Helper Functions
// ============================================================================

std::string format_to_string(DataFormat format) {
    switch (format) {
        case DataFormat::PlainText: return "PlainText";
        case DataFormat::Markdown: return "Markdown";
        case DataFormat::JSON: return "JSON";
        case DataFormat::CSV: return "CSV";
        case DataFormat::XML: return "XML";
        case DataFormat::PDF: return "PDF";
        case DataFormat::DOCX: return "DOCX";
        case DataFormat::TXT: return "TXT";
        case DataFormat::SQL: return "SQL";
        case DataFormat::Excel: return "Excel";
        case DataFormat::Parquet: return "Parquet";
        case DataFormat::HTML: return "HTML";
        case DataFormat::API_JSON: return "API_JSON";
        case DataFormat::RSS: return "RSS";
        case DataFormat::Yahoo_Finance: return "Yahoo_Finance";
        case DataFormat::Alpha_Vantage: return "Alpha_Vantage";
        case DataFormat::PNG: return "PNG";
        case DataFormat::JPEG: return "JPEG";
        default: return "Unknown";
    }
}

DataFormat detect_from_extension(const fs::path& path) {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".csv") return DataFormat::CSV;
    if (ext == ".json") return DataFormat::JSON;
    if (ext == ".jsonl") return DataFormat::JSON;
    if (ext == ".md" || ext == ".markdown") return DataFormat::Markdown;
    if (ext == ".txt") return DataFormat::TXT;
    if (ext == ".pdf") return DataFormat::PDF;
    if (ext == ".docx") return DataFormat::DOCX;
    if (ext == ".xml") return DataFormat::XML;
    if (ext == ".html" || ext == ".htm") return DataFormat::HTML;
    if (ext == ".png") return DataFormat::PNG;
    if (ext == ".jpg" || ext == ".jpeg") return DataFormat::JPEG;
    if (ext == ".xlsx" || ext == ".xls") return DataFormat::Excel;
    if (ext == ".parquet") return DataFormat::Parquet;
    if (ext == ".db" || ext == ".sqlite" || ext == ".sqlite3" || ext == ".sql") return DataFormat::SQL;
    
    return DataFormat::Unknown;
}

std::string sanitize_text(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    
    bool in_whitespace = false;
    for (char c : text) {
        // Remove control characters except newline and tab
        if (c < 32 && c != '\n' && c != '\t') {
            continue;
        }
        
        // Normalize whitespace
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!in_whitespace) {
                result += ' ';
                in_whitespace = true;
            }
        } else {
            result += c;
            in_whitespace = false;
        }
    }
    
    // Trim trailing whitespace
    while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) {
        result.pop_back();
    }
    
    return result;
}

std::vector<float> extract_numerical_features(std::string_view text) {
    std::vector<float> features;
    
    // Extract all numbers from text
    std::regex number_regex(R"([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)");
    std::string str(text);
    
    auto numbers_begin = std::sregex_iterator(str.begin(), str.end(), number_regex);
    auto numbers_end = std::sregex_iterator();
    
    for (auto it = numbers_begin; it != numbers_end && features.size() < 100; ++it) {
        try {
            features.push_back(std::stof(it->str()));
        } catch (...) {
            // Skip invalid numbers
        }
    }
    
    return features;
}

std::vector<DataChunk> chunk_text(
    std::string_view text,
    const ChunkConfig& config,
    const std::unordered_map<std::string, std::string>& metadata
) {
    std::vector<DataChunk> chunks;
    
    switch (config.strategy) {
        case ChunkStrategy::Document: {
            // Single chunk
            DataChunk chunk;
            chunk.content = std::string(text);
            chunk.metadata = metadata;
            chunk.chunk_index = 0;
            chunk.total_chunks = 1;
            chunks.push_back(std::move(chunk));
            break;
        }
        
        case ChunkStrategy::Paragraph: {
            // Split by double newline
            std::string current;
            size_t chunk_idx = 0;
            
            for (size_t i = 0; i < text.size(); ++i) {
                current += text[i];
                
                // Check for paragraph boundary
                if (text[i] == '\n' && i + 1 < text.size() && text[i + 1] == '\n') {
                    if (current.size() >= config.max_chunk_size || i + 2 >= text.size()) {
                        DataChunk chunk;
                        chunk.content = sanitize_text(current);
                        chunk.metadata = metadata;
                        chunk.chunk_index = chunk_idx++;
                        chunks.push_back(std::move(chunk));
                        current.clear();
                    }
                } else if (current.size() >= config.max_chunk_size) {
                    DataChunk chunk;
                    chunk.content = sanitize_text(current);
                    chunk.metadata = metadata;
                    chunk.chunk_index = chunk_idx++;
                    chunks.push_back(std::move(chunk));
                    current.clear();
                }
            }
            
            if (!current.empty()) {
                DataChunk chunk;
                chunk.content = sanitize_text(current);
                chunk.metadata = metadata;
                chunk.chunk_index = chunk_idx++;
                chunks.push_back(std::move(chunk));
            }
            
            // Update total chunks
            for (auto& chunk : chunks) {
                chunk.total_chunks = chunks.size();
            }
            break;
        }
        
        case ChunkStrategy::Fixed:
        case ChunkStrategy::Sliding:
        default: {
            // Simple fixed-size chunking
            size_t chunk_idx = 0;
            for (size_t i = 0; i < text.size(); i += config.max_chunk_size - config.overlap) {
                size_t chunk_size = std::min(config.max_chunk_size, text.size() - i);
                DataChunk chunk;
                chunk.content = sanitize_text(text.substr(i, chunk_size));
                chunk.metadata = metadata;
                chunk.chunk_index = chunk_idx++;
                chunks.push_back(std::move(chunk));
                
                if (i + chunk_size >= text.size()) break;
            }
            
            for (auto& chunk : chunks) {
                chunk.total_chunks = chunks.size();
            }
            break;
        }
    }
    
    return chunks;
}

} // namespace vdb::adapters
