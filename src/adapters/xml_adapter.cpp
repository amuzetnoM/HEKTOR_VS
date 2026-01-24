// ============================================================================
// VectorDB - XML Data Adapter Implementation
// ============================================================================

#include "vdb/adapters/xml_adapter.hpp"
#include "vdb/logging.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <stack>

namespace vdb::adapters {

using namespace vdb::logging;

// Helper function to escape XML special characters
static std::string escape_xml(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    
    for (char c : str) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default: result += c; break;
        }
    }
    
    return result;
}

// ============================================================================
// Simple XML Parser (minimal implementation)
// For production, consider using libxml2 or pugixml
// ============================================================================

struct XMLNode {
    std::string name;
    std::string text;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<XMLNode> children;
};

// Simple XML parser implementation
class SimpleXMLParser {
public:
    static Result<XMLNode> parse(std::string_view xml_content) {
        XMLNode root;
        std::stack<XMLNode*> node_stack;
        node_stack.push(&root);
        
        size_t pos = 0;
        while (pos < xml_content.size()) {
            // Skip whitespace
            while (pos < xml_content.size() && std::isspace(xml_content[pos])) {
                pos++;
            }
            
            if (pos >= xml_content.size()) break;
            
            // Find next tag
            if (xml_content[pos] == '<') {
                size_t tag_end = xml_content.find('>', pos);
                if (tag_end == std::string_view::npos) {
                    return tl::unexpected(Error{ErrorCode::ParseError, "Unclosed XML tag"});
                }
                
                std::string_view tag = xml_content.substr(pos + 1, tag_end - pos - 1);
                
                // Handle empty tags
                if (tag.empty()) {
                    pos = tag_end + 1;
                    continue;
                }
                
                // Handle closing tag
                if (tag[0] == '/') {
                    if (node_stack.size() > 1) {
                        node_stack.pop();
                    }
                    pos = tag_end + 1;
                    continue;
                }
                
                // Handle self-closing or opening tag
                bool self_closing = (!tag.empty() && tag.back() == '/');
                if (self_closing) {
                    tag = tag.substr(0, tag.size() - 1);
                }
                
                // Parse tag name and attributes
                XMLNode node;
                size_t space_pos = tag.find(' ');
                if (space_pos != std::string_view::npos) {
                    node.name = std::string(tag.substr(0, space_pos));
                    // Parse attributes (simplified - supports double quotes)
                    // Note: This simplified parser only handles double-quoted attributes
                    // For production, consider using libxml2 or pugixml
                    std::string attr_str(tag.substr(space_pos + 1));
                    std::regex attr_regex(R"((\w+)=\"([^\"]*)\")");
                    std::smatch match;
                    std::string::const_iterator search_start(attr_str.cbegin());
                    while (std::regex_search(search_start, attr_str.cend(), match, attr_regex)) {
                        node.attributes[match[1].str()] = match[2].str();
                        search_start = match.suffix().first;
                    }
                } else {
                    node.name = std::string(tag);
                }
                
                if (!self_closing) {
                    node_stack.top()->children.push_back(std::move(node));
                    node_stack.push(&node_stack.top()->children.back());
                } else {
                    node_stack.top()->children.push_back(std::move(node));
                }
                
                pos = tag_end + 1;
            } else {
                // Text content
                size_t next_tag = xml_content.find('<', pos);
                if (next_tag == std::string_view::npos) {
                    next_tag = xml_content.size();
                }
                
                std::string text(xml_content.substr(pos, next_tag - pos));
                // Trim whitespace
                text.erase(0, text.find_first_not_of(" \t\n\r"));
                text.erase(text.find_last_not_of(" \t\n\r") + 1);
                
                if (!text.empty() && !node_stack.empty()) {
                    node_stack.top()->text += text;
                }
                
                pos = next_tag;
            }
        }
        
        return root.children.empty() ? root : root.children[0];
    }
};

// ============================================================================
// XMLAdapter Implementation
// ============================================================================

XMLAdapter::XMLAdapter(const XMLConfig& config)
    : config_(config) {}

bool XMLAdapter::can_handle(const fs::path& path) const {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    bool result = (ext == ".xml");
    LOG_DEBUG(std::string("XML adapter can_handle ") + path.string() + ": " + (result ? "true" : "false"));
    return result;
}

bool XMLAdapter::can_handle(std::string_view content) const {
    // Check for XML declaration or root element
    if (content.empty()) return false;
    
    // Trim leading whitespace
    size_t start = 0;
    while (start < content.size() && std::isspace(content[start])) {
        start++;
    }
    
    if (start >= content.size()) return false;
    
    // Check for XML declaration or opening tag
    return content.substr(start).starts_with("<?xml") || 
           content.substr(start).starts_with("<");
}

Result<NormalizedData> XMLAdapter::parse(
    const fs::path& path,
    const ChunkConfig& config
) {
    LOG_INFO(std::string("Parsing XML file: ") + path.string());
    
    std::ifstream file(path);
    if (!file) {
        LOG_ERROR(std::string("Failed to open XML file: ") + path.string());
        LOG_ANOMALY(AnomalyType::PARSE_ERROR, "XML file could not be opened: " + path.string());
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to open XML file: " + path.string()});
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    LOG_DEBUG(std::string("XML file size: ") + std::to_string(content.size()) + " bytes");
    
    return parse_content(content, config, path.string());
}

Result<NormalizedData> XMLAdapter::parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view source_hint
) {
    return parse_xml_string(content, config, source_hint);
}

// Helper function to extract text from XML node recursively
static void extract_text_recursive(
    const XMLNode& node,
    std::string& text,
    std::unordered_map<std::string, std::string>& metadata,
    const XMLConfig& config,
    const std::string& path = ""
) {
    // Add node text
    if (!node.text.empty() && config.extract_text_content) {
        if (!text.empty()) text += " ";
        text += node.text;
    }
    
    // Add attributes to metadata
    if (config.extract_attributes) {
        for (const auto& [key, value] : node.attributes) {
            std::string attr_key = path.empty() ? key : path + "." + key;
            metadata[attr_key] = value;
        }
    }
    
    // Recurse into children
    for (const auto& child : node.children) {
        std::string child_path = path.empty() ? child.name : path + "." + child.name;
        extract_text_recursive(child, text, metadata, config, child_path);
    }
}

Result<NormalizedData> XMLAdapter::parse_xml_string(
    std::string_view xml_content,
    const ChunkConfig& chunk_config,
    std::string_view source_path
) {
    // Parse XML
    auto parse_result = SimpleXMLParser::parse(xml_content);
    if (!parse_result) {
        return tl::unexpected(parse_result.error());
    }
    
    XMLNode root = std::move(*parse_result);
    
    // Extract data
    NormalizedData data;
    data.format = DataFormat::XML;
    data.source_path = std::string(source_path);
    data.confidence = 1.0f;
    
    // Extract text and metadata
    std::string text;
    std::unordered_map<std::string, std::string> metadata;
    extract_text_recursive(root, text, metadata, config_);
    
    // Create chunks
    if (text.empty()) {
        data.warnings.push_back("No text content found in XML");
    } else {
        DataChunk chunk;
        chunk.content = text;
        chunk.metadata = metadata;
        chunk.chunk_index = 0;
        chunk.total_chunks = 1;
        
        // Store root element name as title
        if (!root.name.empty()) {
            chunk.title = root.name;
            data.global_metadata["root_element"] = root.name;
        }
        
        data.chunks.push_back(std::move(chunk));
    }
    
    // Apply chunking if needed
    if (chunk_config.strategy != ChunkStrategy::Document && !data.chunks.empty()) {
        auto chunked = chunk_text(data.chunks[0].content, chunk_config, metadata);
        data.chunks = std::move(chunked);
        
        // Update total chunks
        for (auto& chunk : data.chunks) {
            chunk.total_chunks = data.chunks.size();
        }
    }
    
    // Sanitize
    auto sanitize_result = sanitize(data);
    if (!sanitize_result) {
        return tl::unexpected(sanitize_result.error());
    }
    
    data.sanitized = true;
    return data;
}

Result<void> XMLAdapter::sanitize(NormalizedData& data) {
    for (auto& chunk : data.chunks) {
        chunk.content = sanitize_text(chunk.content);
    }
    return {};
}

std::vector<DataFormat> XMLAdapter::supported_formats() const {
    return {DataFormat::XML};
}

Result<void> XMLAdapter::write(
    const NormalizedData& data,
    const fs::path& path
) {
    std::ofstream file(path);
    if (!file) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to create XML file: " + path.string()});
    }
    
    // Write XML declaration
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    
    // Determine root element name from metadata or use default
    std::string root_name = "document";
    if (data.global_metadata.count("root_element")) {
        root_name = data.global_metadata.at("root_element");
    }
    
    file << "<" << root_name << ">\n";
    
    // Write each chunk as an element
    for (const auto& chunk : data.chunks) {
        file << "  <chunk";
        
        // Add chunk metadata as attributes
        file << " index=\"" << chunk.chunk_index << "\"";
        file << " total=\"" << chunk.total_chunks << "\"";
        
        if (chunk.title) {
            file << " title=\"" << escape_xml(*chunk.title) << "\"";
        }
        if (chunk.date) {
            file << " date=\"" << escape_xml(*chunk.date) << "\"";
        }
        if (chunk.source) {
            file << " source=\"" << escape_xml(*chunk.source) << "\"";
        }
        
        file << ">\n";
        
        // Write metadata as sub-elements
        if (!chunk.metadata.empty()) {
            file << "    <metadata>\n";
            for (const auto& [key, value] : chunk.metadata) {
                file << "      <" << escape_xml(key) << ">" 
                     << escape_xml(value) 
                     << "</" << escape_xml(key) << ">\n";
            }
            file << "    </metadata>\n";
        }
        
        // Write content
        file << "    <content>" << escape_xml(chunk.content) << "</content>\n";
        
        // Write numerical features if present
        if (!chunk.numerical_features.empty()) {
            file << "    <numerical_features>";
            for (size_t i = 0; i < chunk.numerical_features.size(); ++i) {
                if (i > 0) file << ",";
                file << chunk.numerical_features[i];
            }
            file << "</numerical_features>\n";
        }
        
        file << "  </chunk>\n";
    }
    
    file << "</" << root_name << ">\n";
    
    if (!file) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to write XML file: " + path.string()});
    }
    
    return {};
}

} // namespace vdb::adapters
