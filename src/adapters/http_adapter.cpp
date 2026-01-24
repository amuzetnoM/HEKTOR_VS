// ============================================================================
// VectorDB - HTTP Data Adapter Implementation
// ============================================================================

#include "vdb/adapters/http_adapter.hpp"
#include "vdb/adapters/json_adapter.hpp"
#include "vdb/adapters/xml_adapter.hpp"
#include "vdb/adapters/csv_adapter.hpp"
#include <algorithm>
#include <cctype>
#include <regex>

namespace vdb::adapters {

// ============================================================================
// HTTPAdapter Implementation
// ============================================================================

HTTPAdapter::HTTPAdapter(HTTPConfig config)
    : config_(std::move(config))
    , client_(HttpClientConfig{
        .user_agent = "VectorDB-HTTPAdapter/1.0",
        .retry = RetryConfig{.max_retries = 3, .enable = true},
        .cache = CacheConfig{.enable = true}
    })
{
    // Set default headers from config
    for (const auto& [key, value] : config_.default_headers) {
        client_.set_default_header(key, value);
    }
}

bool HTTPAdapter::can_handle(const fs::path& path) const {
    return is_url(path.string());
}

bool HTTPAdapter::can_handle(std::string_view content) const {
    // Check if content starts with http:// or https://
    // This is for cases where a URL is passed as content
    return is_url(content);
}

Result<NormalizedData> HTTPAdapter::parse(
    const fs::path& path,
    const ChunkConfig& config
) {
    std::string url = build_url(path);
    
    // Make HTTP GET request
    auto response_result = client_.get(url, config_.default_headers);
    if (!response_result) {
        return tl::unexpected(Error{
            ErrorCode::IoError,
            "HTTP request failed: " + response_result.error().message
        });
    }
    
    const auto& response = *response_result;
    
    // Check for HTTP errors
    if (!response.is_success()) {
        return tl::unexpected(Error{
            ErrorCode::IoError,
            "HTTP error " + std::to_string(response.status_code) + ": " + response.error_message
        });
    }
    
    return parse_response(response, url, config);
}

Result<NormalizedData> HTTPAdapter::parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view source_hint
) {
    // If content is a URL, fetch and parse it
    if (is_url(content)) {
        return parse(fs::path(std::string(content)), config);
    }
    
    // Otherwise, try to parse it as data directly
    // Detect format and delegate to appropriate adapter
    DataFormat format = sniff_content_format(content);
    
    NormalizedData result;
    result.format = format;
    result.source_path = std::string(source_hint);
    
    switch (format) {
        case DataFormat::JSON:
        case DataFormat::API_JSON: {
            JSONAdapter json_adapter;
            return json_adapter.parse_content(content, config, source_hint);
        }
        case DataFormat::XML: {
            XMLAdapter xml_adapter;
            return xml_adapter.parse_content(content, config, source_hint);
        }
        case DataFormat::CSV: {
            CSVAdapter csv_adapter;
            return csv_adapter.parse_content(content, config, source_hint);
        }
        default: {
            // Treat as plain text
            auto chunks = chunk_text(content, config);
            result.chunks = std::move(chunks);
            result.format = DataFormat::PlainText;
            result.confidence = 0.5f;
            return result;
        }
    }
}

Result<void> HTTPAdapter::sanitize(NormalizedData& data) {
    // Sanitize all text content
    for (auto& chunk : data.chunks) {
        chunk.content = sanitize_text(chunk.content);
        
        // Extract numerical features if not already present
        if (chunk.numerical_features.empty()) {
            chunk.numerical_features = extract_numerical_features(chunk.content);
        }
    }
    
    data.sanitized = true;
    return {};
}

std::vector<DataFormat> HTTPAdapter::supported_formats() const {
    return {
        DataFormat::API_JSON,
        DataFormat::JSON,
        DataFormat::XML,
        DataFormat::CSV,
        DataFormat::PlainText,
        DataFormat::HTML
    };
}

void HTTPAdapter::set_header(const std::string& key, const std::string& value) {
    config_.default_headers[key] = value;
    client_.set_default_header(key, value);
}

void HTTPAdapter::remove_header(const std::string& key) {
    config_.default_headers.erase(key);
    client_.remove_default_header(key);
}

void HTTPAdapter::set_base_url(const std::string& url) {
    config_.base_url = url;
}

// ============================================================================
// Private Implementation
// ============================================================================

bool HTTPAdapter::is_url(std::string_view str) {
    if (str.length() < 7) return false;
    
    // Check for http:// or https://
    std::string lower(str.substr(0, 8));
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    return lower.starts_with("http://") || lower.starts_with("https://");
}

std::string HTTPAdapter::build_url(const fs::path& path) const {
    std::string path_str = path.string();
    
    // If already a full URL, return as-is
    if (is_url(path_str)) {
        return path_str;
    }
    
    // Otherwise, combine with base URL
    if (config_.base_url.empty()) {
        return path_str;
    }
    
    std::string url = config_.base_url;
    if (!url.ends_with('/') && !path_str.starts_with('/')) {
        url += '/';
    } else if (url.ends_with('/') && path_str.starts_with('/')) {
        url.pop_back();
    }
    url += path_str;
    
    return url;
}

DataFormat HTTPAdapter::detect_format_from_response(
    const HttpResponse& response,
    const std::string& url
) const {
    // Try Content-Type header first
    auto it = response.headers.find("Content-Type");
    if (it == response.headers.end()) {
        it = response.headers.find("content-type");
    }
    
    if (it != response.headers.end()) {
        DataFormat format = parse_content_type(it->second);
        if (format != DataFormat::Unknown) {
            return format;
        }
    }
    
    // Try URL extension
    DataFormat format = detect_from_extension(fs::path(url));
    if (format != DataFormat::Unknown) {
        return format;
    }
    
    // Fall back to content sniffing
    return sniff_content_format(response.body);
}

Result<NormalizedData> HTTPAdapter::parse_response(
    const HttpResponse& response,
    const std::string& url,
    const ChunkConfig& config
) {
    DataFormat format = detect_format_from_response(response, url);
    
    NormalizedData result;
    result.source_path = url;
    result.format = format;
    result.global_metadata["http_status"] = std::to_string(response.status_code);
    result.global_metadata["elapsed_ms"] = std::to_string(response.elapsed.count());
    
    // Add response headers to metadata
    for (const auto& [key, value] : response.headers) {
        result.global_metadata["header_" + key] = value;
    }
    
    // Delegate parsing based on format
    switch (format) {
        case DataFormat::JSON:
        case DataFormat::API_JSON: {
            JSONAdapter json_adapter;
            auto parsed = json_adapter.parse_content(response.body, config, url);
            if (parsed) {
                result.chunks = std::move(parsed->chunks);
                result.confidence = parsed->confidence;
                result.warnings = std::move(parsed->warnings);
            } else {
                return tl::unexpected(parsed.error());
            }
            break;
        }
        case DataFormat::XML: {
            XMLAdapter xml_adapter;
            auto parsed = xml_adapter.parse_content(response.body, config, url);
            if (parsed) {
                result.chunks = std::move(parsed->chunks);
                result.confidence = parsed->confidence;
                result.warnings = std::move(parsed->warnings);
            } else {
                return tl::unexpected(parsed.error());
            }
            break;
        }
        case DataFormat::CSV: {
            CSVAdapter csv_adapter;
            auto parsed = csv_adapter.parse_content(response.body, config, url);
            if (parsed) {
                result.chunks = std::move(parsed->chunks);
                result.confidence = parsed->confidence;
                result.warnings = std::move(parsed->warnings);
            } else {
                return tl::unexpected(parsed.error());
            }
            break;
        }
        case DataFormat::HTML: {
            // For HTML, create simple text chunks (could be enhanced with HTML parsing)
            auto chunks = chunk_text(response.body, config);
            result.chunks = std::move(chunks);
            result.confidence = 0.7f;
            result.warnings.push_back("HTML content parsed as plain text");
            break;
        }
        default: {
            // Plain text or unknown format
            auto chunks = chunk_text(response.body, config);
            result.chunks = std::move(chunks);
            result.format = DataFormat::PlainText;
            result.confidence = 0.5f;
            break;
        }
    }
    
    return result;
}

DataFormat HTTPAdapter::parse_content_type(const std::string& content_type) {
    std::string lower = content_type;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // JSON types
    if (lower.find("application/json") != std::string::npos ||
        lower.find("text/json") != std::string::npos ||
        lower.find("+json") != std::string::npos) {
        return DataFormat::API_JSON;
    }
    
    // XML types
    if (lower.find("application/xml") != std::string::npos ||
        lower.find("text/xml") != std::string::npos ||
        lower.find("+xml") != std::string::npos) {
        return DataFormat::XML;
    }
    
    // CSV
    if (lower.find("text/csv") != std::string::npos ||
        lower.find("application/csv") != std::string::npos) {
        return DataFormat::CSV;
    }
    
    // HTML
    if (lower.find("text/html") != std::string::npos) {
        return DataFormat::HTML;
    }
    
    // Plain text
    if (lower.find("text/plain") != std::string::npos) {
        return DataFormat::PlainText;
    }
    
    return DataFormat::Unknown;
}

DataFormat HTTPAdapter::sniff_content_format(std::string_view content) {
    if (content.empty()) {
        return DataFormat::Unknown;
    }
    
    // Skip leading whitespace
    size_t start = 0;
    while (start < content.size() && std::isspace(content[start])) {
        ++start;
    }
    
    if (start >= content.size()) {
        return DataFormat::Unknown;
    }
    
    char first_char = content[start];
    
    // JSON detection
    if (first_char == '{' || first_char == '[') {
        return DataFormat::JSON;
    }
    
    // XML detection
    if (first_char == '<') {
        // Check for HTML vs XML
        std::string_view peek = content.substr(start, std::min(size_t(100), content.size() - start));
        std::string lower(peek);
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (lower.find("<!doctype html") != std::string::npos ||
            lower.find("<html") != std::string::npos) {
            return DataFormat::HTML;
        }
        return DataFormat::XML;
    }
    
    // CSV detection (look for comma-separated values with consistent columns)
    size_t comma_count = std::count(content.begin(), content.end(), ',');
    size_t newline_count = std::count(content.begin(), content.end(), '\n');
    if (comma_count > 0 && newline_count > 0 && 
        static_cast<float>(comma_count) / newline_count > 1.5f) {
        return DataFormat::CSV;
    }
    
    return DataFormat::PlainText;
}

} // namespace vdb::adapters
