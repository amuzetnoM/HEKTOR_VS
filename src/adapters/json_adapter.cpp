// ============================================================================
// VectorDB - JSON Adapter Implementation
// ============================================================================

#include "vdb/adapters/json_adapter.hpp"
#include <fstream>
#include <sstream>

namespace vdb::adapters {

bool JSONAdapter::can_handle(const fs::path& path) const {
    auto ext = path.extension().string();
    return ext == ".json" || ext == ".JSON" || ext == ".jsonl" || ext == ".JSONL";
}

bool JSONAdapter::can_handle(std::string_view content) const {
    if (content.empty()) return false;
    
    // Trim whitespace
    size_t start = 0;
    while (start < content.size() && std::isspace(static_cast<unsigned char>(content[start]))) {
        ++start;
    }
    
    if (start >= content.size()) return false;
    
    // Check if starts with { or [
    return content[start] == '{' || content[start] == '[';
}

Result<NormalizedData> JSONAdapter::parse(
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

Result<NormalizedData> JSONAdapter::parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view source_hint
) {
    try {
        json j = json::parse(content);
        
        NormalizedData result;
        result.format = DataFormat::JSON;
        result.source_path = std::string(source_hint);
        
        // Extract global metadata
        extract_metadata(j, result.global_metadata);
        
        // Process JSON structure
        result.chunks = process_json(j);
        
        // Apply chunking strategy
        if (config.strategy != ChunkStrategy::Document) {
            std::vector<DataChunk> rechunked;
            for (const auto& chunk : result.chunks) {
                auto new_chunks = chunk_text(chunk.content, config, chunk.metadata);
                rechunked.insert(rechunked.end(), new_chunks.begin(), new_chunks.end());
            }
            result.chunks = std::move(rechunked);
        }
        
        result.confidence = 0.99f;
        result.sanitized = false;
        
        return result;
        
    } catch (const json::exception& e) {
        return tl::unexpected(Error{ErrorCode::InvalidData, 
            std::string("JSON parse error: ") + e.what()});
    }
}

Result<void> JSONAdapter::sanitize(NormalizedData& data) {
    for (auto& chunk : data.chunks) {
        chunk.content = sanitize_text(chunk.content);
    }
    data.sanitized = true;
    return {};
}

JSONStructure JSONAdapter::detect_structure(const json& j) const {
    if (j.is_object()) {
        // Check if deeply nested
        int max_depth = 0;
        std::function<int(const json&, int)> calc_depth = [&](const json& node, int depth) {
            if (!node.is_object() && !node.is_array()) return depth;
            int max_child = depth;
            if (node.is_object()) {
                for (auto& [key, value] : node.items()) {
                    max_child = std::max(max_child, calc_depth(value, depth + 1));
                }
            } else if (node.is_array()) {
                for (auto& item : node) {
                    max_child = std::max(max_child, calc_depth(item, depth + 1));
                }
            }
            return max_child;
        };
        max_depth = calc_depth(j, 0);
        
        return max_depth > 3 ? JSONStructure::Nested : JSONStructure::Object;
    } else if (j.is_array()) {
        return JSONStructure::Array;
    }
    return JSONStructure::Object;
}

std::vector<DataChunk> JSONAdapter::process_json(const json& j) const {
    std::vector<DataChunk> chunks;
    
    if (j.is_array()) {
        // Process each array element as a chunk
        size_t idx = 0;
        for (const auto& item : j) {
            DataChunk chunk;
            chunk.content = flatten_json_to_text(item);
            chunk.chunk_index = idx++;
            chunk.total_chunks = j.size();
            extract_metadata(item, chunk.metadata);
            chunk.numerical_features = extract_numerical_features(chunk.content);
            chunks.push_back(std::move(chunk));
        }
    } else if (j.is_object()) {
        // Single object - create one chunk
        DataChunk chunk;
        chunk.content = flatten_json_to_text(j);
        chunk.chunk_index = 0;
        chunk.total_chunks = 1;
        extract_metadata(j, chunk.metadata);
        chunk.numerical_features = extract_numerical_features(chunk.content);
        chunks.push_back(std::move(chunk));
    }
    
    return chunks;
}

std::string JSONAdapter::flatten_json_to_text(const json& j) const {
    std::ostringstream oss;
    
    std::function<void(const json&, const std::string&, int)> flatten;
    flatten = [&](const json& node, const std::string& prefix, int depth) {
        if (depth > 10) return; // Prevent infinite recursion
        
        if (node.is_object()) {
            for (auto& [key, value] : node.items()) {
                std::string new_prefix = prefix.empty() ? key : prefix + "." + key;
                if (value.is_object() || value.is_array()) {
                    flatten(value, new_prefix, depth + 1);
                } else {
                    oss << new_prefix << ": ";
                    if (value.is_string()) {
                        oss << value.get<std::string>();
                    } else {
                        oss << value.dump();
                    }
                    oss << "\n";
                }
            }
        } else if (node.is_array()) {
            size_t idx = 0;
            for (const auto& item : node) {
                std::string new_prefix = prefix + "[" + std::to_string(idx++) + "]";
                flatten(item, new_prefix, depth + 1);
            }
        } else {
            oss << prefix << ": " << node.dump() << "\n";
        }
    };
    
    flatten(j, "", 0);
    return oss.str();
}

void JSONAdapter::extract_metadata(const json& j, std::unordered_map<std::string, std::string>& meta) const {
    if (!j.is_object()) return;
    
    // Look for common metadata fields
    const std::vector<std::string> meta_fields = {
        "id", "title", "name", "date", "timestamp", "created_at", "updated_at",
        "author", "source", "type", "category", "tags"
    };
    
    for (const auto& field : meta_fields) {
        if (j.contains(field)) {
            const auto& value = j[field];
            if (value.is_string()) {
                meta[field] = value.get<std::string>();
            } else if (value.is_number() || value.is_boolean()) {
                meta[field] = value.dump();
            }
        }
    }
}

Result<void> JSONAdapter::write(
    const fs::path& path,
    const NormalizedData& data,
    bool pretty_print
) {
    try {
        nlohmann::json output;
        
        // Write global metadata
        output["format"] = format_to_string(data.format);
        output["source_path"] = data.source_path;
        output["confidence"] = data.confidence;
        output["sanitized"] = data.sanitized;
        
        if (!data.global_metadata.empty()) {
            output["metadata"] = data.global_metadata;
        }
        
        if (!data.warnings.empty()) {
            output["warnings"] = data.warnings;
        }
        
        // Write chunks
        nlohmann::json chunks_array = nlohmann::json::array();
        for (const auto& chunk : data.chunks) {
            nlohmann::json chunk_obj;
            chunk_obj["chunk_index"] = chunk.chunk_index;
            chunk_obj["total_chunks"] = chunk.total_chunks;
            chunk_obj["content"] = chunk.content;
            
            if (chunk.title) {
                chunk_obj["title"] = *chunk.title;
            }
            if (chunk.date) {
                chunk_obj["date"] = *chunk.date;
            }
            if (chunk.source) {
                chunk_obj["source"] = *chunk.source;
            }
            
            if (!chunk.metadata.empty()) {
                chunk_obj["metadata"] = chunk.metadata;
            }
            
            if (!chunk.numerical_features.empty()) {
                chunk_obj["numerical_features"] = chunk.numerical_features;
            }
            
            chunks_array.push_back(chunk_obj);
        }
        output["chunks"] = chunks_array;
        
        // Write to file
        std::ofstream file(path);
        if (!file) {
            return tl::unexpected(Error{
                ErrorCode::IoError,
                "Failed to open file for writing: " + path.string()
            });
        }
        
        if (pretty_print) {
            file << output.dump(2); // 2-space indentation
        } else {
            file << output.dump();
        }
        
        if (!file.good()) {
            return tl::unexpected(Error{
                ErrorCode::IoError,
                "Failed to write JSON file: " + path.string()
            });
        }
        
        return {};
        
    } catch (const nlohmann::json::exception& e) {
        return tl::unexpected(Error{
            ErrorCode::ParseError,
            "JSON serialization error: " + std::string(e.what())
        });
    }
}

} // namespace vdb::adapters
