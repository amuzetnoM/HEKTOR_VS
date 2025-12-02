// ============================================================================
// VectorDB - Markdown Parser Implementation
// Parses Gold Standard markdown files for text embedding
// ============================================================================

#include "vdb/ingest.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

namespace vdb {

// ============================================================================
// Markdown Parsing Utilities
// ============================================================================

namespace markdown {

std::string read_file(const fs::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string strip_markdown(const std::string& content) {
    std::string result = content;
    
    // Remove code blocks
    result = std::regex_replace(result, std::regex("```[\\s\\S]*?```"), " ");
    
    // Remove inline code
    result = std::regex_replace(result, std::regex("`[^`]+`"), " ");
    
    // Remove headers (keep text)
    result = std::regex_replace(result, std::regex("^#{1,6}\\s*"), "");
    
    // Remove bold/italic markers
    result = std::regex_replace(result, std::regex("\\*{1,2}([^*]+)\\*{1,2}"), "$1");
    result = std::regex_replace(result, std::regex("_{1,2}([^_]+)_{1,2}"), "$1");
    
    // Remove links, keep text
    result = std::regex_replace(result, std::regex("\\[([^\\]]+)\\]\\([^)]+\\)"), "$1");
    
    // Remove images
    result = std::regex_replace(result, std::regex("!\\[[^\\]]*\\]\\([^)]+\\)"), "");
    
    // Remove horizontal rules
    result = std::regex_replace(result, std::regex("^[-*_]{3,}$"), "");
    
    // Remove blockquotes marker
    result = std::regex_replace(result, std::regex("^>\\s*"), "");
    
    // Remove list markers
    result = std::regex_replace(result, std::regex("^[\\s]*[-*+]\\s+"), "");
    result = std::regex_replace(result, std::regex("^[\\s]*\\d+\\.\\s+"), "");
    
    // Collapse multiple whitespace
    result = std::regex_replace(result, std::regex("\\s+"), " ");
    
    // Trim
    auto start = result.find_first_not_of(" \t\n\r");
    auto end = result.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    return result.substr(start, end - start + 1);
}

std::vector<std::string> split_into_sections(const std::string& content) {
    std::vector<std::string> sections;
    
    // Split by headers
    std::regex header_pattern("^#{1,6}\\s+.*$", std::regex::multiline);
    std::sregex_token_iterator iter(content.begin(), content.end(), header_pattern, -1);
    std::sregex_token_iterator end;
    
    for (; iter != end; ++iter) {
        std::string section = iter->str();
        if (!section.empty() && section.find_first_not_of(" \t\n\r") != std::string::npos) {
            sections.push_back(section);
        }
    }
    
    // If no sections found, return whole content
    if (sections.empty() && !content.empty()) {
        sections.push_back(content);
    }
    
    return sections;
}

std::vector<std::string> split_into_chunks(const std::string& content, 
                                           size_t max_chunk_size,
                                           size_t overlap) {
    std::vector<std::string> chunks;
    
    if (content.size() <= max_chunk_size) {
        chunks.push_back(content);
        return chunks;
    }
    
    // Split by sentences first
    std::vector<size_t> sentence_ends;
    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];
        if (c == '.' || c == '!' || c == '?' || c == '\n') {
            sentence_ends.push_back(i + 1);
        }
    }
    
    // Build chunks respecting sentence boundaries
    size_t start = 0;
    while (start < content.size()) {
        size_t end = std::min(start + max_chunk_size, content.size());
        
        // Find last sentence end before max_chunk_size
        for (auto it = sentence_ends.rbegin(); it != sentence_ends.rend(); ++it) {
            if (*it > start && *it <= start + max_chunk_size) {
                end = *it;
                break;
            }
        }
        
        chunks.push_back(content.substr(start, end - start));
        
        // Move start with overlap
        if (end >= content.size()) break;
        start = (end > overlap) ? end - overlap : end;
    }
    
    return chunks;
}

// Extract key-value pairs from YAML frontmatter
std::unordered_map<std::string, std::string> parse_frontmatter(const std::string& content) {
    std::unordered_map<std::string, std::string> result;
    
    // Check for frontmatter
    if (content.substr(0, 3) != "---") {
        return result;
    }
    
    // Find end of frontmatter
    size_t end_pos = content.find("---", 3);
    if (end_pos == std::string::npos) {
        return result;
    }
    
    std::string frontmatter = content.substr(3, end_pos - 3);
    
    // Parse key: value pairs
    std::regex kv_pattern(R"(^\s*([^:]+):\s*(.*)$)", std::regex::multiline);
    auto begin = std::sregex_iterator(frontmatter.begin(), frontmatter.end(), kv_pattern);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        std::string key = (*it)[1].str();
        std::string value = (*it)[2].str();
        
        // Trim
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t\"'"));
        value.erase(value.find_last_not_of(" \t\"'") + 1);
        
        result[key] = value;
    }
    
    return result;
}

std::string extract_body(const std::string& content) {
    // Skip frontmatter if present
    if (content.substr(0, 3) == "---") {
        size_t end_pos = content.find("---", 3);
        if (end_pos != std::string::npos) {
            return content.substr(end_pos + 3);
        }
    }
    return content;
}

} // namespace markdown

} // namespace vdb
