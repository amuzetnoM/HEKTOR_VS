#pragma once

#include "vdb/core.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Windows headers define DELETE as a macro - undefine it
#ifdef DELETE
#undef DELETE
#endif

namespace vdb::adapters
{

    // HTTP request methods
    enum class HttpMethod
    {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH,
        HEAD
    };

    // HTTP response structure
    struct HttpResponse
    {
        int status_code{0};
        std::string body;
        std::map<std::string, std::string> headers;
        std::chrono::milliseconds elapsed{0};
        std::string error_message;

        auto is_success() const -> bool { return status_code >= 200 && status_code < 300; }
        auto is_client_error() const -> bool { return status_code >= 400 && status_code < 500; }
        auto is_server_error() const -> bool { return status_code >= 500 && status_code < 600; }
        auto is_rate_limited() const -> bool { return status_code == 429; }
    };

    // HTTP request configuration
    struct HttpRequest
    {
        HttpMethod method{HttpMethod::GET};
        std::string url;
        std::map<std::string, std::string> headers;
        std::string body;
        std::chrono::seconds timeout{30};
        bool follow_redirects{true};
        int max_redirects{5};
        bool verify_ssl{true};
    };

    // Rate limiting configuration
    struct RateLimitConfig
    {
        size_t requests_per_second{10};
        size_t requests_per_minute{100};
        size_t requests_per_hour{1000};
        size_t burst_size{20}; // Max burst requests
        bool enable{true};
    };

    // Retry configuration
    struct RetryConfig
    {
        size_t max_retries{3};
        std::chrono::milliseconds initial_backoff{1000};
        double backoff_multiplier{2.0};
        std::chrono::milliseconds max_backoff{30000};
        bool retry_on_timeout{true};
        bool retry_on_connection_error{true};
        bool retry_on_rate_limit{true};
        std::chrono::seconds rate_limit_retry_after{60};
        std::vector<int> retryable_status_codes{408, 429, 500, 502, 503, 504};
        bool enable{true};
    };

    // Cache configuration
    struct CacheConfig
    {
        std::string cache_dir;
        std::chrono::seconds default_ttl{3600}; // 1 hour
        size_t max_cache_size_mb{100};
        bool enable{true};
        bool cache_errors{false};
    };

    // HTTP client configuration
    struct HttpClientConfig
    {
        std::string user_agent{"VectorDB/1.0"};
        RateLimitConfig rate_limit;
        RetryConfig retry;
        CacheConfig cache;
        bool enable_compression{true};
        bool enable_keepalive{true};
        std::chrono::seconds keepalive_timeout{60};
        size_t max_concurrent_requests{10};
    };

    // Progress callback for large downloads
    using ProgressCallback = std::function<void(size_t current, size_t total)>;

    /**
     * @brief Production-ready HTTP client with retry, rate limiting, and caching
     *
     * Thread-safe HTTP client built on libcurl with:
     * - Automatic retry with exponential backoff
     * - Per-endpoint rate limiting
     * - Response caching with TTL
     * - Connection pooling and keepalive
     * - SSL/TLS support with certificate verification
     * - Compression (gzip, deflate)
     * - Progress callbacks for large downloads
     */
    class HttpClient
    {
    public:
        explicit HttpClient(HttpClientConfig config = {});
        ~HttpClient();

        // Non-copyable, moveable
        HttpClient(const HttpClient &) = delete;
        auto operator=(const HttpClient &) -> HttpClient & = delete;
        HttpClient(HttpClient &&) noexcept;
        auto operator=(HttpClient &&) noexcept -> HttpClient &;

        // Synchronous requests
        auto get(const std::string &url,
                 const std::map<std::string, std::string> &headers = {})
            -> Result<HttpResponse>;

        auto post(const std::string &url,
                  const std::string &body,
                  const std::map<std::string, std::string> &headers = {})
            -> Result<HttpResponse>;

        auto put(const std::string &url,
                 const std::string &body,
                 const std::map<std::string, std::string> &headers = {})
            -> Result<HttpResponse>;

        auto del(const std::string &url,
                 const std::map<std::string, std::string> &headers = {})
            -> Result<HttpResponse>;

        // Generic request
        auto request(const HttpRequest &req) -> Result<HttpResponse>;

        // Download to file with progress
        auto download(const std::string &url,
                      const std::string &output_path,
                      ProgressCallback callback = nullptr)
            -> Result<void>;

        // Configuration updates
        auto set_default_header(const std::string &key, const std::string &value) -> void;
        auto remove_default_header(const std::string &key) -> void;
        auto set_rate_limit(const std::string &endpoint, RateLimitConfig config) -> void;
        auto clear_cache() -> Result<void>;
        auto clear_cache_for_url(const std::string &url) -> Result<void>;

        // Statistics
        struct Statistics
        {
            size_t total_requests{0};
            size_t successful_requests{0};
            size_t failed_requests{0};
            size_t cached_responses{0};
            size_t retried_requests{0};
            size_t rate_limited_requests{0};
            std::chrono::milliseconds total_time{0};
            std::chrono::milliseconds average_time{0};
        };
        auto get_statistics() const -> Statistics;
        auto reset_statistics() -> void;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

        auto execute_with_retry(const HttpRequest &req) -> Result<HttpResponse>;
        auto check_rate_limit(const std::string &url) -> Result<void>;
        auto get_cached_response(const std::string &cache_key) -> std::optional<HttpResponse>;
        auto cache_response(const std::string &cache_key, const HttpResponse &response) -> void;
        auto calculate_backoff(size_t attempt) -> std::chrono::milliseconds;
    };

    /**
     * @brief Helper for building URLs with query parameters
     */
    class UrlBuilder
    {
    public:
        explicit UrlBuilder(std::string base_url);

        auto add_param(const std::string &key, const std::string &value) -> UrlBuilder &;
        auto add_param(const std::string &key, int value) -> UrlBuilder &;
        auto add_param(const std::string &key, double value) -> UrlBuilder &;
        auto add_param(const std::string &key, bool value) -> UrlBuilder &;

        auto add_path(const std::string &segment) -> UrlBuilder &;

        auto build() const -> std::string;

    private:
        std::string base_url_;
        std::vector<std::string> path_segments_;
        std::map<std::string, std::string> params_;

        static auto url_encode(const std::string &value) -> std::string;
    };

    /**
     * @brief Helper for parsing JSON responses
     */
    class HttpJsonHelper
    {
    public:
        static auto parse_json(const HttpResponse &response) -> Result<nlohmann::json>;
        static auto create_json_request(HttpMethod method,
                                        const std::string &url,
                                        const nlohmann::json &body) -> HttpRequest;
    };

} // namespace vdb::adapters
