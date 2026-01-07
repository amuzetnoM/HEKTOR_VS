#pragma once
// ============================================================================
// VectorDB - HTTP Data Adapter
// Fetch and parse data from HTTP/HTTPS endpoints with auto-format detection
// ============================================================================

#include "data_adapter.hpp"
#include "http_client.hpp"
#include <chrono>
#include <map>

namespace vdb::adapters {

// ============================================================================
// HTTP Adapter Configuration
// ============================================================================

struct HTTPConfig {
    std::string base_url;                              // Optional base URL for relative paths
    std::map<std::string, std::string> default_headers; // Default headers (auth, API keys, etc.)
    std::chrono::seconds timeout{30};
    bool verify_ssl{true};
    bool follow_redirects{true};
    int max_redirects{5};
};

// ============================================================================
// HTTP Data Adapter
// ============================================================================

/**
 * @brief Generic HTTP adapter for fetching and parsing data from web endpoints
 * 
 * This adapter can:
 * - Handle HTTP/HTTPS URLs as data sources
 * - Auto-detect response format from Content-Type headers
 * - Parse JSON, XML, CSV, and plain text responses
 * - Support authentication via custom headers
 * - Leverage the HttpClient for retry, rate limiting, and caching
 */
class HTTPAdapter : public IDataAdapter {
public:
    explicit HTTPAdapter(HTTPConfig config = {});
    ~HTTPAdapter() override = default;
    
    // IDataAdapter interface
    [[nodiscard]] bool can_handle(const fs::path& path) const override;
    [[nodiscard]] bool can_handle(std::string_view content) const override;
    
    [[nodiscard]] Result<NormalizedData> parse(
        const fs::path& path,
        const ChunkConfig& config = {}
    ) override;
    
    [[nodiscard]] Result<NormalizedData> parse_content(
        std::string_view content,
        const ChunkConfig& config = {},
        std::string_view source_hint = ""
    ) override;
    
    [[nodiscard]] Result<void> sanitize(NormalizedData& data) override;
    
    [[nodiscard]] std::string name() const override { return "HTTP"; }
    [[nodiscard]] std::vector<DataFormat> supported_formats() const override;
    
    // HTTP-specific methods
    
    /// Set a default header for all requests
    void set_header(const std::string& key, const std::string& value);
    
    /// Remove a default header
    void remove_header(const std::string& key);
    
    /// Set base URL for relative paths
    void set_base_url(const std::string& url);
    
    /// Get the underlying HTTP client for advanced configuration
    HttpClient& get_client() { return client_; }
    
private:
    HTTPConfig config_;
    HttpClient client_;
    
    /// Detect data format from HTTP response
    [[nodiscard]] DataFormat detect_format_from_response(
        const HttpResponse& response,
        const std::string& url
    ) const;
    
    /// Parse response body based on detected format
    [[nodiscard]] Result<NormalizedData> parse_response(
        const HttpResponse& response,
        const std::string& url,
        const ChunkConfig& config
    );
    
    /// Check if a string looks like a URL
    [[nodiscard]] static bool is_url(std::string_view str);
    
    /// Build full URL from path (handles base_url)
    [[nodiscard]] std::string build_url(const fs::path& path) const;
    
    /// Extract format from Content-Type header
    [[nodiscard]] static DataFormat parse_content_type(const std::string& content_type);
    
    /// Detect format by sniffing content
    [[nodiscard]] static DataFormat sniff_content_format(std::string_view content);
};

} // namespace vdb::adapters
