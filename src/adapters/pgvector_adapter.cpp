// ============================================================================
// VectorDB - pgvector PostgreSQL Adapter Implementation
// ============================================================================

#include "vdb/adapters/pgvector_adapter.hpp"
#include <sstream>
#include <cstring>

// PostgreSQL libpq integration (conditional on availability)
#ifdef HAVE_LIBPQ
#include <libpq-fe.h>
#endif

namespace vdb::adapters {

namespace {

// Helper function to quote SQL identifiers to prevent SQL injection
std::string quote_identifier(const std::string& identifier) {
    // PostgreSQL uses double quotes for identifiers
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

// Helper function to escape string literals for SQL
std::string escape_string_literal(const std::string& str) {
    std::string escaped = "'";
    for (char c : str) {
        if (c == '\'') {
            escaped += "''";  // Escape single quotes by doubling
        } else if (c == '\\') {
            escaped += "\\\\";  // Escape backslashes
        } else {
            escaped += c;
        }
    }
    escaped += "'";
    return escaped;
}

} // anonymous namespace

// ============================================================================
// PgvectorAdapter Implementation
// ============================================================================

PgvectorAdapter::PgvectorAdapter(const PgvectorConfig& config)
    : config_(config), connection_(nullptr) {}

bool PgvectorAdapter::can_handle(const fs::path& path) const {
    // pgvector doesn't use file paths - it's a database connection
    // This adapter is used differently than file-based adapters
    return false;
}

bool PgvectorAdapter::can_handle(std::string_view content) const {
    // pgvector uses database connections, not content strings
    return false;
}

Result<NormalizedData> PgvectorAdapter::parse(
    const fs::path& path,
    const ChunkConfig& config
) {
    // For pgvector, "parsing" means connecting to the database and reading vectors
    // The path parameter is not used for database connections
    return parse_pgvector_db(config);
}

Result<NormalizedData> PgvectorAdapter::parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view source_hint
) {
    return std::unexpected(Error{
        ErrorCode::InvalidInput,
        "pgvector adapter requires database connection, not content parsing"
    });
}

#ifdef HAVE_LIBPQ

Result<void> PgvectorAdapter::connect() {
    if (connection_) {
        return {}; // Already connected
    }
    
    // Build connection string
    std::stringstream conn_str;
    conn_str << "host=" << config_.host
             << " port=" << config_.port
             << " dbname=" << config_.database
             << " user=" << config_.user;
    
    if (!config_.password.empty()) {
        conn_str << " password=" << config_.password;
    }
    
    PGconn* conn = PQconnectdb(conn_str.str().c_str());
    
    if (PQstatus(conn) != CONNECTION_OK) {
        std::string error = PQerrorMessage(conn);
        PQfinish(conn);
        return std::unexpected(Error{ErrorCode::IoError, "PostgreSQL connection failed: " + error});
    }
    
    connection_ = conn;
    return {};
}

void PgvectorAdapter::disconnect() {
    if (connection_) {
        PQfinish(static_cast<PGconn*>(connection_));
        connection_ = nullptr;
    }
}

Result<bool> PgvectorAdapter::is_pgvector_available() {
    if (!connection_) {
        auto conn_result = connect();
        if (!conn_result) {
            return std::unexpected(conn_result.error());
        }
    }
    
    PGconn* conn = static_cast<PGconn*>(connection_);
    
    // Check if pgvector extension exists
    const char* query = "SELECT EXISTS(SELECT 1 FROM pg_extension WHERE extname = 'vector');";
    PGresult* res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::string error = PQerrorMessage(conn);
        PQclear(res);
        return std::unexpected(Error{ErrorCode::ParseError, "Failed to check pgvector availability: " + error});
    }
    
    bool available = false;
    if (PQntuples(res) > 0) {
        available = (strcmp(PQgetvalue(res, 0, 0), "t") == 0);
    }
    
    PQclear(res);
    return available;
}

Result<void> PgvectorAdapter::create_table(size_t vector_dimension) {
    if (!connection_) {
        auto conn_result = connect();
        if (!conn_result) {
            return conn_result;
        }
    }
    
    PGconn* conn = static_cast<PGconn*>(connection_);
    
    // Build CREATE TABLE query with properly quoted identifiers
    std::stringstream query;
    query << "CREATE TABLE IF NOT EXISTS " << quote_identifier(config_.table) << " ("
          << "id SERIAL PRIMARY KEY, "
          << quote_identifier(config_.content_column) << " TEXT, "
          << quote_identifier(config_.vector_column) << " vector(" << vector_dimension << ")";
    
    // Add metadata columns
    for (const auto& col : config_.metadata_columns) {
        query << ", " << quote_identifier(col) << " TEXT";
    }
    
    query << ");";
    
    PGresult* res = PQexec(conn, query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn);
        PQclear(res);
        return std::unexpected(Error{ErrorCode::ParseError, "Failed to create table: " + error});
    }
    
    PQclear(res);
    
    // Create index on vector column for similarity search
    std::stringstream index_query;
    index_query << "CREATE INDEX IF NOT EXISTS " 
                << quote_identifier(config_.table + "_" + config_.vector_column + "_idx") << " "
                << "ON " << quote_identifier(config_.table) 
                << " USING ivfflat (" << quote_identifier(config_.vector_column) << " vector_cosine_ops);";
    
    res = PQexec(conn, index_query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        // Index creation might fail if IVFFlat is not available, continue anyway
        PQclear(res);
    } else {
        PQclear(res);
    }
    
    return {};
}

Result<size_t> PgvectorAdapter::insert_vectors(
    const std::vector<std::vector<float>>& vectors,
    const std::vector<std::string>& contents,
    const std::vector<std::unordered_map<std::string, std::string>>& metadata
) {
    if (!connection_) {
        auto conn_result = connect();
        if (!conn_result) {
            return std::unexpected(conn_result.error());
        }
    }
    
    if (vectors.size() != contents.size()) {
        return std::unexpected(Error{ErrorCode::InvalidInput, "Vectors and contents must have same size"});
    }
    
    PGconn* conn = static_cast<PGconn*>(connection_);
    size_t inserted = 0;
    
    for (size_t i = 0; i < vectors.size(); ++i) {
        // Build INSERT query
        std::stringstream query;
        query << "INSERT INTO " << config_.table << " (" << config_.content_column << ", " << config_.vector_column;
        
        // Add metadata columns
        if (i < metadata.size()) {
            for (const auto& [key, value] : metadata[i]) {
                query << ", " << key;
            }
        }
        
        query << ") VALUES ($1, '[";
        
        // Add vector values
        for (size_t j = 0; j < vectors[i].size(); ++j) {
            if (j > 0) query << ",";
            query << vectors[i][j];
        }
        query << "]'";
        
        // Add metadata values
        if (i < metadata.size()) {
            for (const auto& [key, value] : metadata[i]) {
                query << ", '" << value << "'";
            }
        }
        
        query << ");";
        
        PGresult* res = PQexec(conn, query.str().c_str());
        
        if (PQresultStatus(res) == PGRES_COMMAND_OK) {
            inserted++;
        }
        
        PQclear(res);
    }
    
    return inserted;
}

Result<std::vector<DataChunk>> PgvectorAdapter::query_similar(
    const std::vector<float>& query_vector,
    size_t k,
    float distance_threshold
) {
    if (!connection_) {
        auto conn_result = connect();
        if (!conn_result) {
            return std::unexpected(conn_result.error());
        }
    }
    
    PGconn* conn = static_cast<PGconn*>(connection_);
    
    // Build similarity query
    std::stringstream query;
    query << "SELECT " << config_.content_column << ", " << config_.vector_column << " <=> '[";
    
    for (size_t i = 0; i < query_vector.size(); ++i) {
        if (i > 0) query << ",";
        query << query_vector[i];
    }
    
    query << "]' AS distance";
    
    // Add metadata columns
    for (const auto& col : config_.metadata_columns) {
        query << ", " << col;
    }
    
    query << " FROM " << config_.table
          << " ORDER BY distance"
          << " LIMIT " << k;
    
    if (distance_threshold >= 0) {
        query << " WHERE distance <= " << distance_threshold;
    }
    
    query << ";";
    
    PGresult* res = PQexec(conn, query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::string error = PQerrorMessage(conn);
        PQclear(res);
        return std::unexpected(Error{ErrorCode::ParseError, "Failed to query similar vectors: " + error});
    }
    
    std::vector<DataChunk> chunks;
    int num_rows = PQntuples(res);
    
    for (int row = 0; row < num_rows; ++row) {
        DataChunk chunk;
        chunk.content = PQgetvalue(res, row, 0);
        
        float distance = std::stof(PQgetvalue(res, row, 1));
        chunk.numerical_features.push_back(distance);
        chunk.metadata["distance"] = std::to_string(distance);
        
        // Extract metadata columns
        for (size_t col = 0; col < config_.metadata_columns.size(); ++col) {
            const char* value = PQgetvalue(res, row, col + 2);
            if (value) {
                chunk.metadata[config_.metadata_columns[col]] = value;
            }
        }
        
        chunk.chunk_index = row;
        chunk.total_chunks = num_rows;
        
        chunks.push_back(std::move(chunk));
    }
    
    PQclear(res);
    return chunks;
}

Result<NormalizedData> PgvectorAdapter::parse_pgvector_db(const ChunkConfig& chunk_config) {
    NormalizedData data;
    data.format = DataFormat::SQL; // pgvector is SQL-based
    data.source_path = config_.host + ":" + std::to_string(config_.port) + "/" + config_.database;
    data.confidence = 1.0f;
    
    // Connect to database
    auto conn_result = connect();
    if (!conn_result) {
        return std::unexpected(conn_result.error());
    }
    
    // Check pgvector availability
    auto pgvector_result = is_pgvector_available();
    if (!pgvector_result || !*pgvector_result) {
        return std::unexpected(Error{
            ErrorCode::InvalidData,
            "pgvector extension not installed in PostgreSQL database"
        });
    }
    
    PGconn* conn = static_cast<PGconn*>(connection_);
    
    // Query all rows from table
    std::stringstream query;
    query << "SELECT " << config_.content_column;
    
    for (const auto& col : config_.metadata_columns) {
        query << ", " << col;
    }
    
    query << " FROM " << config_.table << " LIMIT " << config_.batch_size << ";";
    
    PGresult* res = PQexec(conn, query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::string error = PQerrorMessage(conn);
        PQclear(res);
        return std::unexpected(Error{ErrorCode::ParseError, "Failed to query pgvector table: " + error});
    }
    
    int num_rows = PQntuples(res);
    
    for (int row = 0; row < num_rows; ++row) {
        DataChunk chunk;
        chunk.content = PQgetvalue(res, row, 0);
        
        // Extract metadata
        for (size_t col = 0; col < config_.metadata_columns.size(); ++col) {
            const char* value = PQgetvalue(res, row, col + 1);
            if (value) {
                chunk.metadata[config_.metadata_columns[col]] = value;
            }
        }
        
        chunk.chunk_index = row;
        chunk.total_chunks = num_rows;
        
        data.chunks.push_back(std::move(chunk));
    }
    
    PQclear(res);
    
    data.global_metadata["database"] = config_.database;
    data.global_metadata["table"] = config_.table;
    data.global_metadata["num_rows"] = std::to_string(num_rows);
    
    auto sanitize_result = sanitize(data);
    if (!sanitize_result) {
        return std::unexpected(sanitize_result.error());
    }
    
    data.sanitized = true;
    return data;
}

#else // !HAVE_LIBPQ

Result<void> PgvectorAdapter::connect() {
    return std::unexpected(Error{
        ErrorCode::NotImplemented,
        "PostgreSQL libpq library not available. Install libpq-dev and rebuild."
    });
}

void PgvectorAdapter::disconnect() {}

Result<bool> PgvectorAdapter::is_pgvector_available() {
    return std::unexpected(Error{ErrorCode::NotImplemented, "libpq not available"});
}

Result<void> PgvectorAdapter::create_table(size_t vector_dimension) {
    return std::unexpected(Error{ErrorCode::NotImplemented, "libpq not available"});
}

Result<size_t> PgvectorAdapter::insert_vectors(
    const std::vector<std::vector<float>>& vectors,
    const std::vector<std::string>& contents,
    const std::vector<std::unordered_map<std::string, std::string>>& metadata
) {
    return std::unexpected(Error{ErrorCode::NotImplemented, "libpq not available"});
}

Result<std::vector<DataChunk>> PgvectorAdapter::query_similar(
    const std::vector<float>& query_vector,
    size_t k,
    float distance_threshold
) {
    return std::unexpected(Error{ErrorCode::NotImplemented, "libpq not available"});
}

Result<NormalizedData> PgvectorAdapter::parse_pgvector_db(const ChunkConfig& chunk_config) {
    NormalizedData data;
    data.format = DataFormat::SQL;
    data.source_path = config_.host + ":" + std::to_string(config_.port) + "/" + config_.database;
    data.confidence = 1.0f;
    
    data.warnings.push_back(
        "pgvector support requires PostgreSQL libpq library. "
        "Install libpq-dev (Ubuntu), postgresql (Homebrew), or postgres (vcpkg) and rebuild."
    );
    
    DataChunk chunk;
    chunk.content = "pgvector database: " + config_.database;
    chunk.metadata["format"] = "pgvector";
    chunk.metadata["database"] = config_.database;
    chunk.metadata["table"] = config_.table;
    chunk.metadata["status"] = "libpq_required";
    chunk.chunk_index = 0;
    chunk.total_chunks = 1;
    
    data.chunks.push_back(std::move(chunk));
    data.sanitized = true;
    
    return data;
}

#endif // HAVE_LIBPQ

Result<void> PgvectorAdapter::sanitize(NormalizedData& data) {
    for (auto& chunk : data.chunks) {
        chunk.content = sanitize_text(chunk.content);
    }
    return {};
}

std::vector<DataFormat> PgvectorAdapter::supported_formats() const {
    return {DataFormat::SQL};
}

} // namespace vdb::adapters
