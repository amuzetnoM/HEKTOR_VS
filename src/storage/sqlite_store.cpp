#include "vdb/storage/sqlite_store.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <nlohmann/json.hpp>

namespace vdb {
namespace storage {

namespace {
    auto time_to_string(const std::chrono::system_clock::time_point& tp) -> std::string {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    auto string_to_time(const std::string& str) -> std::chrono::system_clock::time_point {
        std::tm tm = {};
        std::istringstream ss(str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
}

SqliteStore::SqliteStore(const SqliteConfig& config) 
    : config_(config) {}

SqliteStore::~SqliteStore() {
    close();
}

SqliteStore::SqliteStore(SqliteStore&& other) noexcept
    : config_(std::move(other.config_))
    , db_(other.db_)
    , initialized_(other.initialized_) {
    other.db_ = nullptr;
    other.initialized_ = false;
}

auto SqliteStore::operator=(SqliteStore&& other) noexcept -> SqliteStore& {
    if (this != &other) {
        close();
        config_ = std::move(other.config_);
        db_ = other.db_;
        initialized_ = other.initialized_;
        other.db_ = nullptr;
        other.initialized_ = false;
    }
    return *this;
}

auto SqliteStore::init() -> Result<void> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return {};
    }

    int rc = sqlite3_open(config_.db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to open SQLite database: " + error});
    }

    sqlite3_busy_timeout(db_, config_.busy_timeout_ms);

    if (config_.enable_wal) {
        auto result = exec_sql("PRAGMA journal_mode=WAL");
        if (!result) {
            close();
            return result;
        }
    }

    auto cache_size = static_cast<int>(-config_.cache_size_mb * 1024);
    auto result = exec_sql("PRAGMA cache_size=" + std::to_string(cache_size));
    if (!result) {
        close();
        return result;
    }

    result = create_tables();
    if (!result) {
        close();
        return result;
    }

    initialized_ = true;
    return {};
}

auto SqliteStore::close() -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    initialized_ = false;
}

auto SqliteStore::create_tables() -> Result<void> {
    auto result = exec_sql(R"(
        CREATE TABLE IF NOT EXISTS document_metadata (
            id TEXT PRIMARY KEY,
            source TEXT,
            content_type TEXT,
            title TEXT,
            author TEXT,
            date TEXT,
            custom_fields TEXT,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        )
    )");
    if (!result) return result;

    result = exec_sql(R"(
        CREATE TABLE IF NOT EXISTS query_cache (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            timestamp TEXT NOT NULL,
            access_count INTEGER DEFAULT 0
        )
    )");
    if (!result) return result;

    result = exec_sql(R"(
        CREATE TABLE IF NOT EXISTS configuration (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        )
    )");
    if (!result) return result;

    exec_sql("CREATE INDEX IF NOT EXISTS idx_metadata_source ON document_metadata(source)");
    exec_sql("CREATE INDEX IF NOT EXISTS idx_metadata_content_type ON document_metadata(content_type)");
    exec_sql("CREATE INDEX IF NOT EXISTS idx_cache_timestamp ON query_cache(timestamp)");

    return {};
}

auto SqliteStore::exec_sql(const std::string& sql) -> Result<void> {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    
    if (rc != SQLITE_OK) {
        std::string error = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        return tl::unexpected(Error{ErrorCode::IoError, "SQL execution failed: " + error});
    }
    
    return {};
}

auto SqliteStore::begin_transaction() -> Result<void> {
    return exec_sql("BEGIN TRANSACTION");
}

auto SqliteStore::commit_transaction() -> Result<void> {
    return exec_sql("COMMIT");
}

auto SqliteStore::rollback_transaction() -> Result<void> {
    return exec_sql("ROLLBACK");
}

auto SqliteStore::store_metadata(const DocumentMetadata& metadata) -> Result<void> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    nlohmann::json custom_json(metadata.custom_fields);
    std::string custom_str = custom_json.dump();

    const char* sql = R"(
        INSERT OR REPLACE INTO document_metadata 
        (id, source, content_type, title, author, date, custom_fields, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    auto now_str = time_to_string(std::chrono::system_clock::now());

    sqlite3_bind_text(stmt, 1, metadata.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, metadata.source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, metadata.content_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, metadata.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, metadata.author.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, metadata.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, custom_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, time_to_string(metadata.created_at).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, now_str.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to insert metadata"});
    }

    return {};
}

auto SqliteStore::get_metadata(const std::string& id) -> Result<DocumentMetadata> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    const char* sql = "SELECT * FROM document_metadata WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return tl::unexpected(Error{ErrorCode::VectorNotFound, "Metadata not found"});
    }

    DocumentMetadata metadata;
    metadata.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (auto* text = sqlite3_column_text(stmt, 1)) metadata.source = reinterpret_cast<const char*>(text);
    if (auto* text = sqlite3_column_text(stmt, 2)) metadata.content_type = reinterpret_cast<const char*>(text);
    if (auto* text = sqlite3_column_text(stmt, 3)) metadata.title = reinterpret_cast<const char*>(text);
    if (auto* text = sqlite3_column_text(stmt, 4)) metadata.author = reinterpret_cast<const char*>(text);
    if (auto* text = sqlite3_column_text(stmt, 5)) metadata.date = reinterpret_cast<const char*>(text);
    
    if (auto* text = sqlite3_column_text(stmt, 6)) {
        std::string custom_str = reinterpret_cast<const char*>(text);
        if (!custom_str.empty()) {
            auto custom_json = nlohmann::json::parse(custom_str);
            metadata.custom_fields = custom_json.get<std::unordered_map<std::string, std::string>>();
        }
    }

    if (auto* text = sqlite3_column_text(stmt, 7)) {
        metadata.created_at = string_to_time(reinterpret_cast<const char*>(text));
    }
    if (auto* text = sqlite3_column_text(stmt, 8)) {
        metadata.updated_at = string_to_time(reinterpret_cast<const char*>(text));
    }

    sqlite3_finalize(stmt);
    return metadata;
}

auto SqliteStore::update_metadata(const DocumentMetadata& metadata) -> Result<void> {
    return store_metadata(metadata);
}

auto SqliteStore::delete_metadata(const std::string& id) -> Result<void> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    const char* sql = "DELETE FROM document_metadata WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to delete metadata"});
    }

    return {};
}

auto SqliteStore::list_metadata(size_t limit, size_t offset) -> Result<std::vector<DocumentMetadata>> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    const char* sql = "SELECT * FROM document_metadata LIMIT ? OFFSET ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_int64(stmt, 1, limit);
    sqlite3_bind_int64(stmt, 2, offset);

    std::vector<DocumentMetadata> results;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        DocumentMetadata metadata;
        metadata.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (auto* text = sqlite3_column_text(stmt, 1)) metadata.source = reinterpret_cast<const char*>(text);
        if (auto* text = sqlite3_column_text(stmt, 2)) metadata.content_type = reinterpret_cast<const char*>(text);
        if (auto* text = sqlite3_column_text(stmt, 3)) metadata.title = reinterpret_cast<const char*>(text);
        if (auto* text = sqlite3_column_text(stmt, 4)) metadata.author = reinterpret_cast<const char*>(text);
        if (auto* text = sqlite3_column_text(stmt, 5)) metadata.date = reinterpret_cast<const char*>(text);
        
        if (auto* text = sqlite3_column_text(stmt, 6)) {
            std::string custom_str = reinterpret_cast<const char*>(text);
            if (!custom_str.empty()) {
                auto custom_json = nlohmann::json::parse(custom_str);
                metadata.custom_fields = custom_json.get<std::unordered_map<std::string, std::string>>();
            }
        }

        if (auto* text = sqlite3_column_text(stmt, 7)) {
            metadata.created_at = string_to_time(reinterpret_cast<const char*>(text));
        }
        if (auto* text = sqlite3_column_text(stmt, 8)) {
            metadata.updated_at = string_to_time(reinterpret_cast<const char*>(text));
        }

        results.push_back(std::move(metadata));
    }

    sqlite3_finalize(stmt);
    return results;
}

auto SqliteStore::cache_put(const std::string& key, const std::string& value) -> Result<void> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_ || !config_.enable_cache) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Cache not available"});
    }

    const char* sql = R"(
        INSERT OR REPLACE INTO query_cache (key, value, timestamp, access_count)
        VALUES (?, ?, ?, 1)
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    auto now_str = time_to_string(std::chrono::system_clock::now());

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, now_str.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to insert cache entry"});
    }

    return {};
}

auto SqliteStore::cache_get(const std::string& key) -> Result<std::string> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_ || !config_.enable_cache) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Cache not available"});
    }

    const char* sql = "SELECT value, timestamp FROM query_cache WHERE key = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return tl::unexpected(Error{ErrorCode::VectorNotFound, "Cache entry not found"});
    }

    std::string value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    std::string timestamp_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    
    sqlite3_finalize(stmt);

    auto timestamp = string_to_time(timestamp_str);
    auto now = std::chrono::system_clock::now();
    if ((now - timestamp) > config_.cache_ttl) {
        // Ignore result of cache_delete - entry will be cleaned up by eviction later if delete fails
        (void)cache_delete(key);
        return tl::unexpected(Error{ErrorCode::VectorNotFound, "Cache entry expired"});
    }

    // Update access count using prepared statement to prevent SQL injection
    const char* update_sql = "UPDATE query_cache SET access_count = access_count + 1 WHERE key = ?";
    sqlite3_stmt* update_stmt;
    int update_rc = sqlite3_prepare_v2(db_, update_sql, -1, &update_stmt, nullptr);
    if (update_rc == SQLITE_OK) {
        sqlite3_bind_text(update_stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(update_stmt);
        sqlite3_finalize(update_stmt);
    }

    return value;
}

auto SqliteStore::cache_delete(const std::string& key) -> Result<void> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    const char* sql = "DELETE FROM query_cache WHERE key = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to delete cache entry"});
    }

    return {};
}

auto SqliteStore::cache_clear() -> Result<void> {
    return exec_sql("DELETE FROM query_cache");
}

auto SqliteStore::cache_size() -> Result<size_t> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    const char* sql = "SELECT COUNT(*) FROM query_cache";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    rc = sqlite3_step(stmt);
    size_t count = 0;
    if (rc == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

auto SqliteStore::config_set(const std::string& key, const std::string& value) -> Result<void> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    const char* sql = "INSERT OR REPLACE INTO configuration (key, value) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to set configuration"});
    }

    return {};
}

auto SqliteStore::config_get(const std::string& key) -> Result<std::string> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    const char* sql = "SELECT value FROM configuration WHERE key = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return tl::unexpected(Error{ErrorCode::VectorNotFound, "Configuration key not found"});
    }

    std::string value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);

    return value;
}

auto SqliteStore::config_delete(const std::string& key) -> Result<void> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    const char* sql = "DELETE FROM configuration WHERE key = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to delete configuration"});
    }

    return {};
}

auto SqliteStore::config_list() -> Result<std::unordered_map<std::string, std::string>> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    const char* sql = "SELECT key, value FROM configuration";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    std::unordered_map<std::string, std::string> config;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        config[key] = value;
    }

    sqlite3_finalize(stmt);
    return config;
}

auto SqliteStore::get_stats() -> Result<std::unordered_map<std::string, size_t>> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Store not initialized"});
    }

    std::unordered_map<std::string, size_t> stats;

    // Count metadata entries
    const char* sql = "SELECT COUNT(*) FROM document_metadata";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["metadata_count"] = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Count cache entries
    sql = "SELECT COUNT(*) FROM query_cache";
    rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["cache_count"] = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Count configuration entries
    sql = "SELECT COUNT(*) FROM configuration";
    rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["config_count"] = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}

auto SqliteStore::vacuum() -> Result<void> {
    return exec_sql("VACUUM");
}

auto SqliteStore::evict_expired_cache() -> Result<size_t> {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_ || !config_.enable_cache) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Cache not available"});
    }

    auto cutoff_time = std::chrono::system_clock::now() - config_.cache_ttl;
    auto cutoff_str = time_to_string(cutoff_time);

    const char* count_sql = "SELECT COUNT(*) FROM query_cache WHERE timestamp < ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, count_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_text(stmt, 1, cutoff_str.c_str(), -1, SQLITE_TRANSIENT);
    
    size_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);

    const char* delete_sql = "DELETE FROM query_cache WHERE timestamp < ?";
    rc = sqlite3_prepare_v2(db_, delete_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to prepare statement"});
    }

    sqlite3_bind_text(stmt, 1, cutoff_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return count;
}

} // namespace storage
} // namespace vdb
