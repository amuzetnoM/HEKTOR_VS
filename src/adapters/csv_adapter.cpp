// ============================================================================
// VectorDB - CSV Adapter Implementation
// ============================================================================

#include "vdb/adapters/csv_adapter.hpp"
#include <fstream>
#include <sstream>

namespace vdb::adapters {

CSVAdapter::CSVAdapter(const CSVConfig& config)
    : config_(config) {}

bool CSVAdapter::can_handle(const fs::path& path) const {
    auto ext = path.extension().string();
    return ext == ".csv" || ext == ".CSV" || ext == ".tsv" || ext == ".TSV";
}

bool CSVAdapter::can_handle(std::string_view content) const {
    // Simple heuristic: check if content looks like CSV
    if (content.empty()) return false;
    
    // Count delimiters in first few lines
    size_t line_end = content.find('\n');
    if (line_end == std::string_view::npos) line_end = content.size();
    
    std::string_view first_line = content.substr(0, line_end);
    size_t comma_count = std::count(first_line.begin(), first_line.end(), ',');
    size_t tab_count = std::count(first_line.begin(), first_line.end(), '\t');
    
    return (comma_count >= 1 || tab_count >= 1);
}

Result<NormalizedData> CSVAdapter::parse(
    const fs::path& path,
    const ChunkConfig& config
) {
    std::ifstream file(path);
    if (!file) {
        return tl::unexpected(Error{ErrorCode::IoError, "Cannot open file: " + path.string()});
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse_content(buffer.str(), config, path.string());
}

Result<NormalizedData> CSVAdapter::parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view source_hint
) {
    auto rows = parse_csv_content(content);
    if (rows.empty()) {
        return tl::unexpected(Error{ErrorCode::InvalidData, "Empty CSV data"});
    }
    
    NormalizedData result;
    result.format = DataFormat::CSV;
    result.source_path = std::string(source_hint);
    
    // Extract headers
    std::vector<std::string> headers;
    size_t data_start = 0;
    
    if (config_.has_header && !rows.empty()) {
        headers = rows[0].fields;
        data_start = 1;
    } else if (!config_.column_names.empty()) {
        headers = config_.column_names;
    } else {
        // Generate default headers
        size_t num_cols = rows.empty() ? 0 : rows[0].fields.size();
        for (size_t i = 0; i < num_cols; ++i) {
            headers.push_back("column_" + std::to_string(i));
        }
    }
    
    result.global_metadata["num_columns"] = std::to_string(headers.size());
    result.global_metadata["num_rows"] = std::to_string(rows.size() - data_start);
    
    // Convert rows to chunks
    std::string chunk_content;
    size_t rows_in_chunk = 0;
    size_t chunk_idx = 0;
    
    for (size_t i = data_start; i < rows.size(); ++i) {
        const auto& row = rows[i];
        
        // Format row as text
        std::string row_text;
        for (size_t j = 0; j < std::min(row.fields.size(), headers.size()); ++j) {
            if (j > 0) row_text += ", ";
            row_text += headers[j] + ": " + row.fields[j];
        }
        row_text += "\n";
        
        chunk_content += row_text;
        rows_in_chunk++;
        
        // Create chunk when size limit reached or last row
        if (chunk_content.size() >= config.max_chunk_size || i == rows.size() - 1) {
            DataChunk chunk;
            chunk.content = chunk_content;
            chunk.chunk_index = chunk_idx++;
            chunk.metadata["row_start"] = std::to_string(i - rows_in_chunk + 1);
            chunk.metadata["row_end"] = std::to_string(i);
            chunk.metadata["rows_in_chunk"] = std::to_string(rows_in_chunk);
            
            // Extract numerical features from chunk
            chunk.numerical_features = extract_numerical_features(chunk_content);
            
            result.chunks.push_back(std::move(chunk));
            
            chunk_content.clear();
            rows_in_chunk = 0;
        }
    }
    
    // Update total chunks
    for (auto& chunk : result.chunks) {
        chunk.total_chunks = result.chunks.size();
    }
    
    result.confidence = 0.95f;
    return result;
}

Result<void> CSVAdapter::sanitize(NormalizedData& data) {
    for (auto& chunk : data.chunks) {
        chunk.content = sanitize_text(chunk.content);
    }
    data.sanitized = true;
    return {};
}

std::vector<CSVAdapter::ParsedRow> CSVAdapter::parse_csv_content(std::string_view content) const {
    std::vector<ParsedRow> rows;
    
    std::string content_str(content);
    std::istringstream stream(content_str);
    std::string line;
    size_t row_num = 0;
    
    while (std::getline(stream, line)) {
        if (config_.skip_empty_rows && line.empty()) continue;
        
        ParsedRow row;
        row.fields = parse_csv_line(line);
        row.row_number = row_num++;
        rows.push_back(std::move(row));
    }
    
    return rows;
}

std::vector<std::string> CSVAdapter::parse_csv_line(std::string_view line) const {
    std::vector<std::string> fields;
    std::string current_field;
    bool in_quotes = false;
    
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        
        if (c == config_.quote) {
            in_quotes = !in_quotes;
        } else if (c == config_.delimiter && !in_quotes) {
            fields.push_back(current_field);
            current_field.clear();
        } else {
            current_field += c;
        }
    }
    
    fields.push_back(current_field);
    return fields;
}

Result<void> CSVAdapter::write(
    const fs::path& path,
    const NormalizedData& data
) {
    std::ofstream file(path);
    if (!file) {
        return tl::unexpected(Error{
            ErrorCode::IoError,
            "Failed to open file for writing: " + path.string()
        });
    }
    
    // Write header
    file << "chunk_index" << config_.delimiter
         << "content" << config_.delimiter
         << "source" << config_.delimiter
         << "metadata\n";
    
    // Write each chunk as a row
    for (const auto& chunk : data.chunks) {
        // Escape content if needed
        auto write_field = [&](const std::string& field) {
            bool needs_quotes = field.find(config_.delimiter) != std::string::npos ||
                              field.find(config_.quote) != std::string::npos ||
                              field.find('\n') != std::string::npos;
            
            if (needs_quotes) {
                file << config_.quote;
                for (char c : field) {
                    if (c == config_.quote) {
                        file << config_.quote << config_.quote; // Escape quotes
                    } else {
                        file << c;
                    }
                }
                file << config_.quote;
            } else {
                file << field;
            }
        };
        
        // Write row
        file << chunk.chunk_index << config_.delimiter;
        write_field(chunk.content);
        file << config_.delimiter;
        write_field(chunk.source.value_or(""));
        file << config_.delimiter;
        
        // Serialize metadata
        std::string metadata_str;
        for (const auto& [key, value] : chunk.metadata) {
            metadata_str += key + "=" + value + ";";
        }
        write_field(metadata_str);
        file << "\n";
    }
    
    if (!file.good()) {
        return tl::unexpected(Error{
            ErrorCode::IoError,
            "Failed to write CSV file: " + path.string()
        });
    }
    
    return {};
}

} // namespace vdb::adapters
