// ============================================================================
// VectorDB - SQLite Database Adapter Implementation
// ============================================================================

#include "vdb/adapters/sqlite_adapter.hpp"
#include <fstream>
#include <sstream>

// SQLite3 integration (conditional on availability)
#ifdef HAVE_SQLITE3
#include <sqlite3.h>
#endif

namespace vdb::adapters {

namespace {

// Helper function to quote SQL identifiers to prevent SQL injection
std::string quote_identifier(const std::string& identifier) {
    // SQLite uses double quotes for identifiers
    // Escape any existing double quotes by doubling them
    std::string quoted = "\"";
    for (char c : identifier) {
        if (c == '"') {
            quoted += "\"\"";
        } else {
            quoted += c;
        }
    }
    quoted += "\"";
    return quoted;
}

} // anonymous namespace

// ============================================================================
// SQLiteAdapter Implementation
// ============================================================================

SQLiteAdapter::SQLiteAdapter(const SQLiteConfig& config)
    : config_(config) {}

bool SQLiteAdapter::can_handle(const fs::path& path) const {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".db" || ext == ".sqlite" || ext == ".sqlite3" || ext == ".sql";
}

bool SQLiteAdapter::can_handle(std::string_view content) const {
    // SQLite magic number: "SQLite format 3\0"
    if (content.size() < 16) return false;
    return content.substr(0, 15) == "SQLite format 3";
}

Result<NormalizedData> SQLiteAdapter::parse(
    const fs::path& path,
    const ChunkConfig& config
) {
    return parse_sqlite_db(path, config);
}

Result<NormalizedData> SQLiteAdapter::parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view source_hint
) {
    // SQLite requires file-based access, cannot parse from memory directly
    // without creating a temporary file
    return tl::unexpected(Error{
        ErrorCode::NotImplemented,
        "SQLite content parsing requires file-based access. Please provide a file path."
    });
}

#ifdef HAVE_SQLITE3

Result<std::vector<std::string>> SQLiteAdapter::get_table_names(void* db_ptr) {
    sqlite3* db = static_cast<sqlite3*>(db_ptr);
    std::vector<std::string> tables;
    
    const char* query = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::ParseError, "Failed to query table names"});
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (name) {
            tables.push_back(name);
        }
    }
    
    sqlite3_finalize(stmt);
    return tables;
}

Result<std::vector<DataChunk>> SQLiteAdapter::extract_table_data(
    void* db_ptr,
    const std::string& table_name,
    size_t& chunk_offset
) {
    sqlite3* db = static_cast<sqlite3*>(db_ptr);
    std::vector<DataChunk> chunks;
    
    // Build query with properly quoted table name
    std::string query = "SELECT * FROM " + quote_identifier(table_name);
    if (!config_.query.empty()) {
        query = config_.query;
    }
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::ParseError, "Failed to prepare SQL query for table: " + table_name});
    }
    
    // Get column names
    int col_count = sqlite3_column_count(stmt);
    std::vector<std::string> column_names;
    for (int i = 0; i < col_count; ++i) {
        const char* name = sqlite3_column_name(stmt, i);
        if (name) {
            column_names.push_back(name);
        }
    }
    
    // Extract rows
    size_t row_idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DataChunk chunk;
        std::stringstream ss;
        
        for (int col = 0; col < col_count; ++col) {
            if (config_.extract_column_names && !column_names.empty()) {
                ss << column_names[col] << ": ";
            }
            
            int type = sqlite3_column_type(stmt, col);
            switch (type) {
                case SQLITE_INTEGER: {
                    int64_t val = sqlite3_column_int64(stmt, col);
                    ss << val;
                    chunk.numerical_features.push_back(static_cast<float>(val));
                    break;
                }
                case SQLITE_FLOAT: {
                    double val = sqlite3_column_double(stmt, col);
                    ss << val;
                    chunk.numerical_features.push_back(static_cast<float>(val));
                    break;
                }
                case SQLITE_TEXT: {
                    const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
                    if (val) {
                        ss << val;
                    }
                    break;
                }
                case SQLITE_BLOB: {
                    ss << "[BLOB]";
                    break;
                }
                case SQLITE_NULL: {
                    ss << "NULL";
                    break;
                }
            }
            
            if (col < col_count - 1) {
                ss << ", ";
            }
        }
        
        chunk.content = ss.str();
        chunk.metadata["table"] = table_name;
        chunk.metadata["row_number"] = std::to_string(row_idx);
        chunk.chunk_index = chunk_offset + row_idx;
        chunk.total_chunks = 0; // Will be updated later
        
        chunks.push_back(std::move(chunk));
        row_idx++;
    }
    
    sqlite3_finalize(stmt);
    chunk_offset += row_idx;
    
    return chunks;
}

Result<NormalizedData> SQLiteAdapter::parse_sqlite_db(
    const fs::path& path,
    const ChunkConfig& chunk_config
) {
    NormalizedData data;
    data.format = DataFormat::SQL;
    data.source_path = path.string();
    data.confidence = 1.0f;
    
    // Open database
    sqlite3* db;
    int rc = sqlite3_open(path.string().c_str(), &db);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to open SQLite database: " + std::string(sqlite3_errmsg(db))});
    }
    
    // Get table names
    auto tables_result = get_table_names(db);
    if (!tables_result) {
        sqlite3_close(db);
        return tl::unexpected(tables_result.error());
    }
    
    std::vector<std::string> tables = *tables_result;
    
    // Filter tables if specified in config
    if (!config_.tables.empty()) {
        std::vector<std::string> filtered_tables;
        for (const auto& table : tables) {
            if (std::find(config_.tables.begin(), config_.tables.end(), table) != config_.tables.end()) {
                filtered_tables.push_back(table);
            }
        }
        tables = std::move(filtered_tables);
    }
    
    if (tables.empty()) {
        sqlite3_close(db);
        data.warnings.push_back("No tables found in SQLite database");
        return data;
    }
    
    // Store table names in metadata
    data.global_metadata["num_tables"] = std::to_string(tables.size());
    for (size_t i = 0; i < tables.size(); ++i) {
        data.global_metadata["table_" + std::to_string(i)] = tables[i];
    }
    
    // Extract data from each table
    size_t chunk_offset = 0;
    for (const auto& table : tables) {
        auto chunks_result = extract_table_data(db, table, chunk_offset);
        if (!chunks_result) {
            data.warnings.push_back("Failed to extract data from table: " + table);
            continue;
        }
        
        auto& table_chunks = *chunks_result;
        data.chunks.insert(data.chunks.end(), 
                          std::make_move_iterator(table_chunks.begin()),
                          std::make_move_iterator(table_chunks.end()));
    }
    
    sqlite3_close(db);
    
    // Update total_chunks for all chunks
    for (auto& chunk : data.chunks) {
        chunk.total_chunks = data.chunks.size();
    }
    
    if (data.chunks.empty()) {
        data.warnings.push_back("No data extracted from SQLite database");
    }
    
    // Sanitize
    auto sanitize_result = sanitize(data);
    if (!sanitize_result) {
        return tl::unexpected(sanitize_result.error());
    }
    
    data.sanitized = true;
    return data;
}

#else // !HAVE_SQLITE3

Result<NormalizedData> SQLiteAdapter::parse_sqlite_db(
    const fs::path& path,
    const ChunkConfig& chunk_config
) {
    NormalizedData data;
    data.format = DataFormat::SQL;
    data.source_path = path.string();
    data.confidence = 1.0f;
    
    data.warnings.push_back(
        "SQLite support not enabled. Build with SQLite3 library to enable this feature. "
        "Install libsqlite3-dev (Ubuntu) or sqlite3 (Homebrew/vcpkg) and rebuild."
    );
    
    DataChunk chunk;
    chunk.content = "SQLite file: " + path.filename().string();
    chunk.metadata["format"] = "sqlite";
    chunk.metadata["path"] = path.string();
    chunk.metadata["status"] = "sqlite3_required";
    chunk.title = path.filename().string();
    chunk.chunk_index = 0;
    chunk.total_chunks = 1;
    
    data.chunks.push_back(std::move(chunk));
    data.sanitized = true;
    
    return data;
}

Result<std::vector<std::string>> SQLiteAdapter::get_table_names(void* db_ptr) {
    return tl::unexpected(Error{ErrorCode::NotImplemented, "SQLite3 library not available"});
}

Result<std::vector<DataChunk>> SQLiteAdapter::extract_table_data(
    void* db_ptr,
    const std::string& table_name,
    size_t& chunk_offset
) {
    return tl::unexpected(Error{ErrorCode::NotImplemented, "SQLite3 library not available"});
}

#endif // HAVE_SQLITE3

Result<void> SQLiteAdapter::sanitize(NormalizedData& data) {
    for (auto& chunk : data.chunks) {
        chunk.content = sanitize_text(chunk.content);
    }
    return {};
}

std::vector<DataFormat> SQLiteAdapter::supported_formats() const {
    return {DataFormat::SQL};
}

#ifdef HAVE_SQLITE3

Result<void> SQLiteAdapter::write(
    const NormalizedData& data,
    const fs::path& path
) {
    // Open/create database
    sqlite3* db;
    int rc = sqlite3_open(path.string().c_str(), &db);
    if (rc != SQLITE_OK) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to create SQLite database: " + std::string(sqlite3_errmsg(db))});
    }
    
    // Create table based on data structure
    std::string table_name = "data";
    if (!config_.tables.empty()) {
        table_name = config_.tables[0];
    }
    
    // Build CREATE TABLE query
    std::stringstream create_query;
    create_query << "CREATE TABLE IF NOT EXISTS " << quote_identifier(table_name) << " ("
                 << "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                 << "content TEXT, "
                 << "chunk_index INTEGER, "
                 << "total_chunks INTEGER";
    
    // Add columns for common metadata fields
    create_query << ", title TEXT"
                 << ", date TEXT"
                 << ", source TEXT";
    
    // Determine additional columns from first chunk's metadata
    if (!data.chunks.empty() && !data.chunks[0].metadata.empty()) {
        for (const auto& [key, value] : data.chunks[0].metadata) {
            create_query << ", " << quote_identifier(key) << " TEXT";
        }
    }
    
    create_query << ");";
    
    char* err_msg = nullptr;
    rc = sqlite3_exec(db, create_query.str().c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string error = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return tl::unexpected(Error{ErrorCode::ParseError, "Failed to create table: " + error});
    }
    
    // Insert data
    for (const auto& chunk : data.chunks) {
        std::stringstream insert_query;
        insert_query << "INSERT INTO " << quote_identifier(table_name) 
                     << " (content, chunk_index, total_chunks, title, date, source";
        
        // Add metadata column names
        for (const auto& [key, value] : chunk.metadata) {
            insert_query << ", " << quote_identifier(key);
        }
        
        insert_query << ") VALUES (?, ?, ?, ?, ?, ?";
        
        // Add placeholders for metadata values
        for (size_t i = 0; i < chunk.metadata.size(); ++i) {
            insert_query << ", ?";
        }
        
        insert_query << ");";
        
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, insert_query.str().c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            sqlite3_close(db);
            return tl::unexpected(Error{ErrorCode::ParseError, "Failed to prepare insert statement"});
        }
        
        // Bind values
        sqlite3_bind_text(stmt, 1, chunk.content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, chunk.chunk_index);
        sqlite3_bind_int(stmt, 3, chunk.total_chunks);
        sqlite3_bind_text(stmt, 4, chunk.title ? chunk.title->c_str() : "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, chunk.date ? chunk.date->c_str() : "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, chunk.source ? chunk.source->c_str() : "", -1, SQLITE_TRANSIENT);
        
        // Bind metadata values
        int param_idx = 7;
        for (const auto& [key, value] : chunk.metadata) {
            sqlite3_bind_text(stmt, param_idx++, value.c_str(), -1, SQLITE_TRANSIENT);
        }
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            sqlite3_close(db);
            return tl::unexpected(Error{ErrorCode::ParseError, "Failed to insert data"});
        }
    }
    
    sqlite3_close(db);
    return {};
}

#else // !HAVE_SQLITE3

Result<void> SQLiteAdapter::write(
    const NormalizedData& data,
    const fs::path& path
) {
    return tl::unexpected(Error{
        ErrorCode::NotImplemented,
        "SQLite write support requires SQLite3 library. Install libsqlite3-dev and rebuild."
    });
}

#endif // HAVE_SQLITE3

} // namespace vdb::adapters
