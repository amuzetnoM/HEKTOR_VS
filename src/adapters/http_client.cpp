#include "vdb/adapters/http_client.hpp"
#include <nlohmann/json.hpp>
#include <curl/curl.h>

// Windows headers define DELETE as a macro - undefine it after curl includes windows.h
#ifdef DELETE
#undef DELETE
#endif

#include <fmt/format.h>
#include <deque>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace vdb::adapters
{

    // Rate limiter implementation
    class RateLimiter
    {
    public:
        explicit RateLimiter(RateLimitConfig config) : config_(config) {}

        auto check_and_wait() -> void
        {
            if (!config_.enable)
                return;

            std::unique_lock lock(mutex_);
            auto now = std::chrono::steady_clock::now();

            // Remove old timestamps
            while (!timestamps_.empty() &&
                   std::chrono::duration_cast<std::chrono::seconds>(now - timestamps_.front()).count() >= 3600)
            {
                timestamps_.pop_front();
            }

            // Check limits
            auto count_last_second = count_since(now - std::chrono::seconds(1));
            auto count_last_minute = count_since(now - std::chrono::minutes(1));
            auto count_last_hour = timestamps_.size();

            // Wait if limits exceeded
            if (count_last_second >= config_.requests_per_second)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / config_.requests_per_second));
            }
            else if (count_last_minute >= config_.requests_per_minute)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(60000 / config_.requests_per_minute));
            }
            else if (count_last_hour >= config_.requests_per_hour)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3600000 / config_.requests_per_hour));
            }

            timestamps_.push_back(std::chrono::steady_clock::now());
        }

    private:
        RateLimitConfig config_;
        std::deque<std::chrono::steady_clock::time_point> timestamps_;
        std::mutex mutex_;

        auto count_since(std::chrono::steady_clock::time_point since) -> size_t
        {
            return std::count_if(timestamps_.begin(), timestamps_.end(),
                                 [since](const auto &t)
                                 { return t >= since; });
        }
    };

    // CURL callback for writing response data
    static auto write_callback(void *contents, size_t size, size_t nmemb, void *userp) -> size_t
    {
        auto *str = static_cast<std::string *>(userp);
        size_t total_size = size * nmemb;
        str->append(static_cast<char *>(contents), total_size);
        return total_size;
    }

    // CURL callback for writing to file
    static auto write_file_callback(void *contents, size_t size, size_t nmemb, void *userp) -> size_t
    {
        auto *file = static_cast<std::ofstream *>(userp);
        size_t total_size = size * nmemb;
        file->write(static_cast<char *>(contents), total_size);
        return file->good() ? total_size : 0;
    }

    // CURL callback for progress
    static auto progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                                  curl_off_t, curl_off_t) -> int
    {
        if (clientp && dltotal > 0)
        {
            auto *callback = static_cast<ProgressCallback *>(clientp);
            (*callback)(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
        }
        return 0;
    }

    // HttpClient implementation details
    struct HttpClient::Impl
    {
        HttpClientConfig config;
        std::map<std::string, std::string> default_headers;
        std::unordered_map<std::string, std::unique_ptr<RateLimiter>> rate_limiters;
        Statistics stats;
        std::mutex stats_mutex;
        std::mutex rate_limit_mutex;

        Impl(HttpClientConfig cfg) : config(std::move(cfg))
        {
            curl_global_init(CURL_GLOBAL_DEFAULT);
        }

        ~Impl()
        {
            curl_global_cleanup();
        }
    };

    HttpClient::HttpClient(HttpClientConfig config)
        : impl_(std::make_unique<Impl>(std::move(config))) {}

    HttpClient::~HttpClient() = default;

    HttpClient::HttpClient(HttpClient &&) noexcept = default;
    auto HttpClient::operator=(HttpClient &&) noexcept -> HttpClient & = default;

    auto HttpClient::get(const std::string &url,
                         const std::map<std::string, std::string> &headers)
        -> Result<HttpResponse>
    {
        HttpRequest req;
        req.method = HttpMethod::GET;
        req.url = url;
        req.headers = headers;
        return request(req);
    }

    auto HttpClient::post(const std::string &url,
                          const std::string &body,
                          const std::map<std::string, std::string> &headers)
        -> Result<HttpResponse>
    {
        HttpRequest req;
        req.method = HttpMethod::POST;
        req.url = url;
        req.body = body;
        req.headers = headers;
        return request(req);
    }

    auto HttpClient::put(const std::string &url,
                         const std::string &body,
                         const std::map<std::string, std::string> &headers)
        -> Result<HttpResponse>
    {
        HttpRequest req;
        req.method = HttpMethod::PUT;
        req.url = url;
        req.body = body;
        req.headers = headers;
        return request(req);
    }

    auto HttpClient::del(const std::string &url,
                         const std::map<std::string, std::string> &headers)
        -> Result<HttpResponse>
    {
        HttpRequest req;
        req.method = HttpMethod::DELETE;
        req.url = url;
        req.headers = headers;
        return request(req);
    }

    auto HttpClient::request(const HttpRequest &req) -> Result<HttpResponse>
    {
        // Check cache first
        if (impl_->config.cache.enable && req.method == HttpMethod::GET)
        {
            auto cache_key = req.url;
            if (auto cached = get_cached_response(cache_key))
            {
                std::lock_guard lock(impl_->stats_mutex);
                impl_->stats.cached_responses++;
                return *cached;
            }
        }

        // Check rate limit
        if (auto result = check_rate_limit(req.url); !result)
        {
            return std::unexpected(result.error());
        }

        // Execute with retry
        return execute_with_retry(req);
    }

    auto HttpClient::execute_with_retry(const HttpRequest &req) -> Result<HttpResponse>
    {
        auto start_time = std::chrono::steady_clock::now();
        size_t attempt = 0;

        while (true)
        {
            attempt++;

            // Initialize CURL
            auto *curl = curl_easy_init();
            if (!curl)
            {
                return std::unexpected(Error{ErrorCode::SystemError, "Failed to initialize CURL"});
            }

            // Set URL
            curl_easy_setopt(curl, CURLOPT_URL, req.url.c_str());

            // Set method
            switch (req.method)
            {
            case HttpMethod::GET:
                curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                break;
            case HttpMethod::POST:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
                break;
            case HttpMethod::PUT:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
                break;
            case HttpMethod::DELETE:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
            case HttpMethod::PATCH:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
                break;
            case HttpMethod::HEAD:
                curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
                break;
            }

            // Set headers
            struct curl_slist *headers = nullptr;

            // Add default headers
            for (const auto &[key, value] : impl_->default_headers)
            {
                auto header = fmt::format("{}: {}", key, value);
                headers = curl_slist_append(headers, header.c_str());
            }

            // Add request headers
            for (const auto &[key, value] : req.headers)
            {
                auto header = fmt::format("{}: {}", key, value);
                headers = curl_slist_append(headers, header.c_str());
            }

            if (headers)
            {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }

            // Set response handling
            std::string response_body;
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

            // Set options
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, req.timeout.count());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, req.follow_redirects ? 1L : 0L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, static_cast<long>(req.max_redirects));
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, req.verify_ssl ? 1L : 0L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, impl_->config.user_agent.c_str());

            if (impl_->config.enable_compression)
            {
                curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
            }

            // Perform request
            auto curl_res = curl_easy_perform(curl);

            // Get response info
            HttpResponse response;
            long status_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
            response.status_code = static_cast<int>(status_code);
            response.body = std::move(response_body);

            // Cleanup
            if (headers)
            {
                curl_slist_free_all(headers);
            }
            curl_easy_cleanup(curl);

            auto end_time = std::chrono::steady_clock::now();
            response.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            // Update statistics
            {
                std::lock_guard lock(impl_->stats_mutex);
                impl_->stats.total_requests++;
                impl_->stats.total_time += response.elapsed;
                if (impl_->stats.total_requests > 0)
                {
                    impl_->stats.average_time = std::chrono::milliseconds(
                        impl_->stats.total_time.count() / impl_->stats.total_requests);
                }
            }

            // Check for errors
            if (curl_res != CURLE_OK)
            {
                response.error_message = curl_easy_strerror(curl_res);

                // Should we retry?
                if (impl_->config.retry.enable && attempt < impl_->config.retry.max_retries)
                {
                    bool should_retry = false;

                    if (curl_res == CURLE_OPERATION_TIMEDOUT && impl_->config.retry.retry_on_timeout)
                    {
                        should_retry = true;
                    }
                    if ((curl_res == CURLE_COULDNT_CONNECT || curl_res == CURLE_COULDNT_RESOLVE_HOST) &&
                        impl_->config.retry.retry_on_connection_error)
                    {
                        should_retry = true;
                    }

                    if (should_retry)
                    {
                        std::lock_guard lock(impl_->stats_mutex);
                        impl_->stats.retried_requests++;
                        auto backoff = calculate_backoff(attempt);
                        std::this_thread::sleep_for(backoff);
                        continue;
                    }
                }

                std::lock_guard lock(impl_->stats_mutex);
                impl_->stats.failed_requests++;
                return std::unexpected(Error{ErrorCode::NetworkError, response.error_message});
            }

            // Check if we should retry based on status code
            if (impl_->config.retry.enable && attempt < impl_->config.retry.max_retries)
            {
                bool should_retry = false;

                if (response.is_rate_limited() && impl_->config.retry.retry_on_rate_limit)
                {
                    should_retry = true;
                    std::lock_guard lock(impl_->stats_mutex);
                    impl_->stats.rate_limited_requests++;
                    std::this_thread::sleep_for(impl_->config.retry.rate_limit_retry_after);
                }
                else
                {
                    for (int code : impl_->config.retry.retryable_status_codes)
                    {
                        if (response.status_code == code)
                        {
                            should_retry = true;
                            break;
                        }
                    }
                }

                if (should_retry)
                {
                    std::lock_guard lock(impl_->stats_mutex);
                    impl_->stats.retried_requests++;
                    auto backoff = calculate_backoff(attempt);
                    std::this_thread::sleep_for(backoff);
                    continue;
                }
            }

            // Success or non-retryable error
            if (response.is_success())
            {
                std::lock_guard lock(impl_->stats_mutex);
                impl_->stats.successful_requests++;

                // Cache if enabled
                if (impl_->config.cache.enable && req.method == HttpMethod::GET)
                {
                    cache_response(req.url, response);
                }
            }
            else
            {
                std::lock_guard lock(impl_->stats_mutex);
                impl_->stats.failed_requests++;
            }

            return response;
        }
    }

    auto HttpClient::download(const std::string &url,
                              const std::string &output_path,
                              ProgressCallback callback)
        -> Result<void>
    {
        // Check rate limit
        if (auto result = check_rate_limit(url); !result)
        {
            return std::unexpected(result.error());
        }

        auto *curl = curl_easy_init();
        if (!curl)
        {
            return std::unexpected(Error{ErrorCode::SystemError, "Failed to initialize CURL"});
        }

        std::ofstream output_file(output_path, std::ios::binary);
        if (!output_file)
        {
            curl_easy_cleanup(curl);
            return std::unexpected(Error{ErrorCode::IoError, "Failed to open output file"});
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output_file);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, impl_->config.user_agent.c_str());

        if (callback)
        {
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &callback);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }

        auto curl_res = curl_easy_perform(curl);
        output_file.close();

        if (curl_res != CURLE_OK)
        {
            curl_easy_cleanup(curl);
            return std::unexpected(Error{ErrorCode::NetworkError, curl_easy_strerror(curl_res)});
        }

        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
        curl_easy_cleanup(curl);

        if (status_code < 200 || status_code >= 300)
        {
            return std::unexpected(Error{ErrorCode::NetworkError,
                                         fmt::format("HTTP error: {}", status_code)});
        }

        return {};
    }

    auto HttpClient::check_rate_limit(const std::string &url) -> Result<void>
    {
        if (!impl_->config.rate_limit.enable)
        {
            return {};
        }

        // Extract host from URL for rate limiting
        std::string host = url;
        if (auto pos = url.find("://"); pos != std::string::npos)
        {
            host = url.substr(pos + 3);
        }
        if (auto pos = host.find('/'); pos != std::string::npos)
        {
            host = host.substr(0, pos);
        }

        std::lock_guard lock(impl_->rate_limit_mutex);

        if (impl_->rate_limiters.find(host) == impl_->rate_limiters.end())
        {
            impl_->rate_limiters[host] = std::make_unique<RateLimiter>(impl_->config.rate_limit);
        }

        impl_->rate_limiters[host]->check_and_wait();
        return {};
    }

    auto HttpClient::get_cached_response(const std::string &cache_key) -> std::optional<HttpResponse>
    {
        // TODO: Implement file-based caching
        return std::nullopt;
    }

    auto HttpClient::cache_response(const std::string &cache_key, const HttpResponse &response) -> void
    {
        // TODO: Implement file-based caching
    }

    auto HttpClient::calculate_backoff(size_t attempt) -> std::chrono::milliseconds
    {
        auto backoff = impl_->config.retry.initial_backoff.count() *
                       std::pow(impl_->config.retry.backoff_multiplier, attempt - 1);
        backoff = std::min(backoff, static_cast<double>(impl_->config.retry.max_backoff.count()));
        return std::chrono::milliseconds(static_cast<long long>(backoff));
    }

    auto HttpClient::set_default_header(const std::string &key, const std::string &value) -> void
    {
        impl_->default_headers[key] = value;
    }

    auto HttpClient::remove_default_header(const std::string &key) -> void
    {
        impl_->default_headers.erase(key);
    }

    auto HttpClient::set_rate_limit(const std::string &endpoint, RateLimitConfig config) -> void
    {
        std::lock_guard lock(impl_->rate_limit_mutex);
        impl_->rate_limiters[endpoint] = std::make_unique<RateLimiter>(config);
    }

    auto HttpClient::clear_cache() -> Result<void>
    {
        // TODO: Implement cache clearing
        return {};
    }

    auto HttpClient::clear_cache_for_url(const std::string &url) -> Result<void>
    {
        // TODO: Implement URL-specific cache clearing
        return {};
    }

    auto HttpClient::get_statistics() const -> Statistics
    {
        std::lock_guard lock(impl_->stats_mutex);
        return impl_->stats;
    }

    auto HttpClient::reset_statistics() -> void
    {
        std::lock_guard lock(impl_->stats_mutex);
        impl_->stats = Statistics{};
    }

    // UrlBuilder implementation
    UrlBuilder::UrlBuilder(std::string base_url) : base_url_(std::move(base_url)) {}

    auto UrlBuilder::add_param(const std::string &key, const std::string &value) -> UrlBuilder &
    {
        params_[key] = value;
        return *this;
    }

    auto UrlBuilder::add_param(const std::string &key, int value) -> UrlBuilder &
    {
        return add_param(key, std::to_string(value));
    }

    auto UrlBuilder::add_param(const std::string &key, double value) -> UrlBuilder &
    {
        return add_param(key, fmt::format("{}", value));
    }

    auto UrlBuilder::add_param(const std::string &key, bool value) -> UrlBuilder &
    {
        return add_param(key, value ? "true" : "false");
    }

    auto UrlBuilder::add_path(const std::string &segment) -> UrlBuilder &
    {
        path_segments_.push_back(segment);
        return *this;
    }

    auto UrlBuilder::build() const -> std::string
    {
        std::string url = base_url_;

        // Add path segments
        for (const auto &segment : path_segments_)
        {
            if (!url.empty() && url.back() != '/')
            {
                url += '/';
            }
            url += url_encode(segment);
        }

        // Add query parameters
        if (!params_.empty())
        {
            url += '?';
            bool first = true;
            for (const auto &[key, value] : params_)
            {
                if (!first)
                {
                    url += '&';
                }
                url += url_encode(key) + '=' + url_encode(value);
                first = false;
            }
        }

        return url;
    }

    auto UrlBuilder::url_encode(const std::string &value) -> std::string
    {
        auto *curl = curl_easy_init();
        if (!curl)
        {
            return value;
        }

        char *encoded = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
        std::string result(encoded);
        curl_free(encoded);
        curl_easy_cleanup(curl);

        return result;
    }

    // HttpJsonHelper implementation
    auto HttpJsonHelper::parse_json(const HttpResponse &response) -> Result<nlohmann::json>
    {
        try
        {
            return nlohmann::json::parse(response.body);
        }
        catch (const nlohmann::json::exception &e)
        {
            return std::unexpected(Error{ErrorCode::ParseError,
                                         fmt::format("JSON parse error: {}", e.what())});
        }
    }

    auto HttpJsonHelper::create_json_request(HttpMethod method,
                                             const std::string &url,
                                             const nlohmann::json &body) -> HttpRequest
    {
        HttpRequest req;
        req.method = method;
        req.url = url;
        req.body = body.dump();
        req.headers["Content-Type"] = "application/json";
        return req;
    }

} // namespace vdb::adapters
