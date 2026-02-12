// ============================================================================
// VectorDB - Database Implementation
// ============================================================================

#include "vdb/database.hpp"
#include <fstream>
#include <mutex>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace vdb {

using json = nlohmann::json;
#ifdef VDB_USE_ONNX_RUNTIME
using embeddings::TextEncoderConfig;
using embeddings::ImageEncoderConfig;
#endif

// ============================================================================
// Database Paths
// ============================================================================

DatabasePaths::DatabasePaths(const fs::path& root_path)
    : root(root_path)
    , vectors(root / "vectors.bin")
    , index(root / "index.hnsw")
    , metadata(root / "metadata.jsonl")
    , config(root / "config.json")
    , models(root / "models")
    , text_model(models / "all-MiniLM-L6-v2.onnx")
    , image_model(models / "clip-vit-b32.onnx")
    , projection(models / "projection.bin")
{}

Result<void> DatabasePaths::ensure_dirs() const {
    try {
        fs::create_directories(root);
        fs::create_directories(models);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::IoError, e.what()});
    }
}

bool DatabasePaths::exists() const {
    return fs::exists(config);
}

// ============================================================================
// VectorDatabase
// ============================================================================

VectorDatabase::VectorDatabase(const DatabaseConfig& config)
    : config_(config)
    , paths_(config.path)
{}

VectorDatabase::~VectorDatabase() {
    if (ready_) {
        (void)sync();  // Ignore result in destructor
    }
}

VectorDatabase::VectorDatabase(VectorDatabase&& other) noexcept
    : config_(std::move(other.config_))
    , paths_(std::move(other.paths_))
    , index_(std::move(other.index_))
    , vectors_(std::move(other.vectors_))
    , metadata_(std::move(other.metadata_))
#ifdef VDB_USE_ONNX_RUNTIME
    , text_encoder_(std::move(other.text_encoder_))
    , image_encoder_(std::move(other.image_encoder_))
    , text_projection_(std::move(other.text_projection_))
#endif
    , next_id_(other.next_id_)
    , ready_(other.ready_)
    // mutex_ is default-constructed (not moved, as mutexes aren't moveable)
{
    other.ready_ = false;
    other.next_id_ = 1;
}

VectorDatabase& VectorDatabase::operator=(VectorDatabase&& other) noexcept {
    if (this != &other) {
        // Sync current state before overwriting
        if (ready_) {
            (void)sync();
        }
        
        config_ = std::move(other.config_);
        paths_ = std::move(other.paths_);
        index_ = std::move(other.index_);
        vectors_ = std::move(other.vectors_);
        metadata_ = std::move(other.metadata_);
#ifdef VDB_USE_ONNX_RUNTIME
        text_encoder_ = std::move(other.text_encoder_);
        image_encoder_ = std::move(other.image_encoder_);
        text_projection_ = std::move(other.text_projection_);
#endif
        next_id_ = other.next_id_;
        ready_ = other.ready_;
        // mutex_ remains in place (not moved)
        
        other.ready_ = false;
        other.next_id_ = 1;
    }
    return *this;
}

Result<void> VectorDatabase::init() {
    // Idempotent: skip if already initialized
    if (ready_) {
        return {};
    }
    
    // Create directories
    auto dir_result = paths_.ensure_dirs();
    if (!dir_result) {
        return dir_result;
    }
    
    // Initialize or load index
    if (fs::exists(paths_.index)) {
        auto index_result = HnswIndex::load(paths_.index.string());
        if (!index_result) {
            return std::unexpected(index_result.error());
        }
        index_ = std::make_unique<HnswIndex>(std::move(*index_result));
    } else {
        HnswConfig hnsw_config;
        hnsw_config.dimension = config_.dimension;
        hnsw_config.M = config_.hnsw_m;
        hnsw_config.ef_construction = config_.hnsw_ef_construction;
        hnsw_config.ef_search = config_.hnsw_ef_search;
        hnsw_config.max_elements = config_.max_elements;
        hnsw_config.metric = config_.metric;
        index_ = std::make_unique<HnswIndex>(hnsw_config);
    }
    
    // Initialize vector storage
    VectorStoreConfig store_config;
    store_config.path = paths_.root;
    store_config.dimension = config_.dimension;
    store_config.memory_only = config_.memory_only;
    vectors_ = std::make_unique<VectorStore>(store_config);
    auto store_result = vectors_->init();
    if (!store_result) {
        return store_result;
    }
    
    // Initialize metadata storage
    metadata_ = std::make_unique<MetadataStore>(paths_.metadata);
    auto meta_result = metadata_->init();
    if (!meta_result) {
        return meta_result;
    }
    
    // Load next ID from metadata count
    next_id_ = metadata_->size() + 1;
    
#ifdef VDB_USE_ONNX_RUNTIME
    // Initialize embeddings if models are available
    if (!config_.text_model_path.empty() || fs::exists(paths_.text_model)) {
        TextEncoderConfig text_config;
        text_config.model_path = config_.text_model_path.empty() 
            ? paths_.text_model.string() 
            : config_.text_model_path;
        text_config.vocab_path = config_.vocab_path;
        text_config.device = Device::CPU;  // Default to CPU
        
        text_encoder_ = std::make_unique<TextEncoder>();
        if (fs::exists(text_config.model_path)) {
            auto text_result = text_encoder_->init(text_config);
            if (!text_result) {
                // Log warning but continue - can still use pre-computed vectors
            }
        }
        
        // Create projection matrix for text -> unified dimension
        if (text_encoder_->dimension() != config_.dimension) {
            text_projection_ = std::make_unique<ProjectionMatrix>(
                text_encoder_->dimension(),
                config_.dimension
            );
        }
    }
    
    if (!config_.image_model_path.empty() || fs::exists(paths_.image_model)) {
        ImageEncoderConfig image_config;
        image_config.model_path = config_.image_model_path.empty()
            ? paths_.image_model.string()
            : config_.image_model_path;
        image_config.device = Device::CPU;  // Default to CPU
        
        image_encoder_ = std::make_unique<ImageEncoder>();
        if (fs::exists(image_config.model_path)) {
            auto image_result = image_encoder_->init(image_config);
            if (!image_result) {
                // Log warning but continue
            }
        }
    }
#endif
    
    // Save config
    json config_json;
    config_json["version"] = VERSION_MAJOR;
    config_json["dimension"] = config_.dimension;
    config_json["metric"] = static_cast<int>(config_.metric);
    config_json["hnsw_m"] = config_.hnsw_m;
    
    std::ofstream config_file(paths_.config);
    config_file << config_json.dump(2);
    
    ready_ = true;
    return {};
}

VectorId VectorDatabase::next_id() {
    return next_id_++;
}

// ============================================================================
// Text Operations
// ============================================================================

#ifdef VDB_USE_ONNX_RUNTIME
Result<VectorId> VectorDatabase::add_text(
    std::string_view text,
    const Metadata& metadata,
    [[maybe_unused]] const IngestOptions& options
) {
    if (!text_encoder_ || !text_encoder_->is_ready()) {
        return std::unexpected(Error{ErrorCode::ModelLoadError, "Text encoder not initialized"});
    }
    
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Generate embedding
    auto embed_result = text_encoder_->encode(text);
    if (!embed_result) {
        return std::unexpected(embed_result.error());
    }
    
    // Project to unified dimension if needed
    Vector embedding(std::move(*embed_result));
    if (text_projection_) {
        embedding = text_projection_->project(embedding.view());
    }
    
    // Get ID
    VectorId id = next_id();
    
    // Add to index
    auto index_result = index_->add(id, embedding.view());
    if (!index_result) {
        return std::unexpected(index_result.error());
    }
    
    // Add to storage
    auto store_result = vectors_->add(id, embedding.view());
    if (!store_result) {
        index_->remove(id);
        return std::unexpected(store_result.error());
    }
    
    // Add metadata
    Metadata meta = metadata;
    meta.id = id;
    meta.content_hash = std::to_string(std::hash<std::string_view>{}(text));
    
    auto meta_result = metadata_->add(meta);
    if (!meta_result) {
        index_->remove(id);
        vectors_->remove(id);
        return std::unexpected(meta_result.error());
    }
    
    if (config_.auto_sync) {
        sync();
    }
    
    return id;
}
#else
Result<VectorId> VectorDatabase::add_text(
    [[maybe_unused]] std::string_view text,
    [[maybe_unused]] const Metadata& metadata,
    [[maybe_unused]] const IngestOptions& options
) {
    return std::unexpected(Error{ErrorCode::NotImplemented, 
        "Text encoding requires ONNX Runtime (VDB_USE_ONNX_RUNTIME)"});
}
#endif

Result<VectorId> VectorDatabase::add_text(
    std::string_view text,
    DocumentType type,
    std::string_view date,
    const IngestOptions& options
) {
    Metadata meta;
    meta.type = type;
    meta.date = std::string(date);
    meta.created_at = now_timestamp();
    meta.updated_at = now_timestamp();
    
    return add_text(text, meta, options);
}

#ifdef VDB_USE_ONNX_RUNTIME
Result<QueryResults> VectorDatabase::query_text(
    std::string_view query,
    const QueryOptions& options
) {
    if (!text_encoder_ || !text_encoder_->is_ready()) {
        return std::unexpected(Error{ErrorCode::ModelLoadError, "Text encoder not initialized"});
    }
    
    // Generate query embedding
    auto embed_result = text_encoder_->encode(query);
    if (!embed_result) {
        return std::unexpected(embed_result.error());
    }
    
    Vector embedding(std::move(*embed_result));
    if (text_projection_) {
        embedding = text_projection_->project(embedding.view());
    }
    
    return query_vector(embedding.view(), options);
}
#else
Result<QueryResults> VectorDatabase::query_text(
    [[maybe_unused]] std::string_view query,
    [[maybe_unused]] const QueryOptions& options
) {
    return std::unexpected(Error{ErrorCode::NotImplemented,
        "Text query requires ONNX Runtime (VDB_USE_ONNX_RUNTIME)"});
}
#endif

// ============================================================================
// Image Operations
// ============================================================================

#ifdef VDB_USE_ONNX_RUNTIME
Result<VectorId> VectorDatabase::add_image(
    const fs::path& image_path,
    const Metadata& metadata,
    [[maybe_unused]] const IngestOptions& options
) {
    if (!image_encoder_ || !image_encoder_->is_ready()) {
        return std::unexpected(Error{ErrorCode::ModelLoadError, "Image encoder not initialized"});
    }
    
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Generate embedding
    auto embed_result = image_encoder_->encode(image_path);
    if (!embed_result) {
        return std::unexpected(embed_result.error());
    }
    
    Vector embedding(std::move(*embed_result));
    VectorId id = next_id();
    
    // Add to index
    auto index_result = index_->add(id, embedding.view());
    if (!index_result) {
        return std::unexpected(index_result.error());
    }
    
    // Add to storage
    auto store_result = vectors_->add(id, embedding.view());
    if (!store_result) {
        index_->remove(id);
        return std::unexpected(store_result.error());
    }
    
    // Add metadata
    Metadata meta = metadata;
    meta.id = id;
    meta.source_file = image_path.string();
    meta.type = DocumentType::Chart;
    
    auto meta_result = metadata_->add(meta);
    if (!meta_result) {
        index_->remove(id);
        vectors_->remove(id);
        return std::unexpected(meta_result.error());
    }
    
    if (config_.auto_sync) {
        sync();
    }
    
    return id;
}
#else
Result<VectorId> VectorDatabase::add_image(
    [[maybe_unused]] const fs::path& image_path,
    [[maybe_unused]] const Metadata& metadata,
    [[maybe_unused]] const IngestOptions& options
) {
    return std::unexpected(Error{ErrorCode::NotImplemented,
        "Image encoding requires ONNX Runtime (VDB_USE_ONNX_RUNTIME)"});
}
#endif

Result<VectorId> VectorDatabase::add_image(
    const fs::path& image_path,
    const IngestOptions& options
) {
    Metadata meta;
    meta.type = DocumentType::Chart;
    meta.source_file = image_path.string();
    
    // Parse filename for asset
    std::string filename = image_path.filename().string();
    auto dot_pos = filename.rfind('.');
    if (dot_pos != std::string::npos) {
        meta.asset = filename.substr(0, dot_pos);
    }
    
    meta.created_at = now_timestamp();
    meta.updated_at = now_timestamp();
    
    return add_image(image_path, meta, options);
}

#ifdef VDB_USE_ONNX_RUNTIME
Result<QueryResults> VectorDatabase::query_image(
    const fs::path& image_path,
    const QueryOptions& options
) {
    if (!image_encoder_ || !image_encoder_->is_ready()) {
        return std::unexpected(Error{ErrorCode::ModelLoadError, "Image encoder not initialized"});
    }
    
    auto embed_result = image_encoder_->encode(image_path);
    if (!embed_result) {
        return std::unexpected(embed_result.error());
    }
    
    Vector embedding(std::move(*embed_result));
    return query_vector(embedding.view(), options);
}
#else
Result<QueryResults> VectorDatabase::query_image(
    [[maybe_unused]] const fs::path& image_path,
    [[maybe_unused]] const QueryOptions& options
) {
    return std::unexpected(Error{ErrorCode::NotImplemented,
        "Image query requires ONNX Runtime (VDB_USE_ONNX_RUNTIME)"});
}
#endif

// ============================================================================
// Vector Operations
// ============================================================================

Result<VectorId> VectorDatabase::add_vector(
    VectorView vector,
    const Metadata& metadata
) {
    if (vector.dim() != config_.dimension) {
        return std::unexpected(Error{ErrorCode::InvalidDimension, "Dimension mismatch"});
    }
    
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    VectorId id = next_id();
    
    auto index_result = index_->add(id, vector);
    if (!index_result) {
        return std::unexpected(index_result.error());
    }
    
    auto store_result = vectors_->add(id, vector);
    if (!store_result) {
        index_->remove(id);
        return std::unexpected(store_result.error());
    }
    
    Metadata meta = metadata;
    meta.id = id;
    
    auto meta_result = metadata_->add(meta);
    if (!meta_result) {
        index_->remove(id);
        vectors_->remove(id);
        return std::unexpected(meta_result.error());
    }
    
    return id;
}

Result<QueryResults> VectorDatabase::query_vector(
    VectorView query,
    const QueryOptions& options
) {
    if (query.dim() != config_.dimension) {
        return std::unexpected(Error{ErrorCode::InvalidDimension, "Query dimension mismatch"});
    }
    
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    // Set search ef if specified
    if (options.ef_search > 0) {
        index_->set_ef_search(options.ef_search);
    }
    
    // Search
    SearchResults raw_results;
    
    if (options.type_filter || options.date_filter || options.asset_filter) {
        // Use filtered search
        raw_results = index_->search_filtered(query, options.k * 2, 
            [this, &options](VectorId id) {
                auto meta = metadata_->get(id);
                if (!meta) return false;
                
                if (options.type_filter && meta->type != *options.type_filter) {
                    return false;
                }
                if (options.date_filter && meta->date != *options.date_filter) {
                    return false;
                }
                if (options.asset_filter && meta->asset != *options.asset_filter) {
                    return false;
                }
                
                return true;
            });
    } else {
        raw_results = index_->search(query, options.k);
    }
    
    return apply_filters(raw_results, options);
}

std::optional<Vector> VectorDatabase::get_vector(VectorId id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return index_->get_vector(id);
}

QueryResults VectorDatabase::apply_filters(
    const SearchResults& raw_results,
    const QueryOptions& options
) const {
    QueryResults results;
    results.reserve(raw_results.size());
    
    std::unordered_set<std::string> seen_dates;
    
    for (const auto& result : raw_results) {
        QueryResult qr;
        qr.id = result.id;
        qr.distance = result.distance;
        qr.score = 1.0f - result.distance;  // Convert distance to similarity
        
        if (options.include_metadata) {
            qr.metadata = metadata_->get(result.id);
            
            // Apply additional filters
            if (qr.metadata) {
                if (options.date_from && qr.metadata->date < *options.date_from) {
                    continue;
                }
                if (options.date_to && qr.metadata->date > *options.date_to) {
                    continue;
                }
                if (options.bias_filter && qr.metadata->bias != *options.bias_filter) {
                    continue;
                }
                
                // Deduplication
                if (options.deduplicate_by_date) {
                    if (seen_dates.contains(qr.metadata->date)) {
                        continue;
                    }
                    seen_dates.insert(qr.metadata->date);
                }
            }
        }
        
        results.push_back(std::move(qr));
        
        if (results.size() >= options.k) {
            break;
        }
    }
    
    return results;
}

// ============================================================================
// Metadata Operations
// ============================================================================

std::optional<Metadata> VectorDatabase::get_metadata(VectorId id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return metadata_->get(id);
}

Result<void> VectorDatabase::update_metadata(VectorId id, const Metadata& metadata) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return metadata_->update(metadata);
}

std::vector<Metadata> VectorDatabase::find_by_date(std::string_view date) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return metadata_->find_by_date(date);
}

std::vector<Metadata> VectorDatabase::find_by_type(DocumentType type) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return metadata_->find_by_type(type);
}

std::vector<Metadata> VectorDatabase::find_by_asset(std::string_view asset) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return metadata_->find_by_asset(asset);
}

// ============================================================================
// Deletion
// ============================================================================

Result<void> VectorDatabase::remove(VectorId id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto index_result = index_->remove(id);
    if (!index_result) {
        return index_result;
    }
    
    vectors_->remove(id);
    metadata_->remove(id);
    
    return {};
}

Result<size_t> VectorDatabase::remove_by_date(std::string_view date) {
    auto metas = find_by_date(date);
    size_t count = 0;
    
    for (const auto& meta : metas) {
        auto result = remove(meta.id);
        if (result) {
            count++;
        }
    }
    
    return count;
}

// ============================================================================
// Bulk Operations
// ============================================================================

Result<std::vector<VectorId>> VectorDatabase::add_texts(
    std::span<const std::string> texts,
    std::span<const Metadata> metadata
) {
    if (texts.size() != metadata.size()) {
        return std::unexpected(Error{ErrorCode::InvalidInput, 
            "Texts and metadata arrays must have the same size"});
    }
    
    std::vector<VectorId> ids;
    ids.reserve(texts.size());
    
    for (size_t i = 0; i < texts.size(); ++i) {
        auto result = add_text(texts[i], metadata[i]);
        if (!result) {
            // Return error with partial results info
            return std::unexpected(Error{ErrorCode::InvalidData,
                "Failed at index " + std::to_string(i) + ": " + result.error().message});
        }
        ids.push_back(*result);
    }
    
    return ids;
}

Result<std::vector<VectorId>> VectorDatabase::add_images(
    std::span<const fs::path> paths,
    std::span<const Metadata> metadata
) {
    if (paths.size() != metadata.size()) {
        return std::unexpected(Error{ErrorCode::InvalidInput,
            "Paths and metadata arrays must have the same size"});
    }
    
    std::vector<VectorId> ids;
    ids.reserve(paths.size());
    
    for (size_t i = 0; i < paths.size(); ++i) {
        auto result = add_image(paths[i], metadata[i]);
        if (!result) {
            return std::unexpected(Error{ErrorCode::InvalidData,
                "Failed at index " + std::to_string(i) + ": " + result.error().message});
        }
        ids.push_back(*result);
    }
    
    return ids;
}

// ============================================================================
// Statistics & Management
// ============================================================================

size_t VectorDatabase::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return index_->size();
}

size_t VectorDatabase::count_by_type(DocumentType type) const {
    return find_by_type(type).size();
}

std::vector<std::string> VectorDatabase::all_dates() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::unordered_set<std::string> unique_dates;
    for (const auto& meta : metadata_->all()) {
        if (!meta.date.empty()) {
            unique_dates.insert(meta.date);
        }
    }
    
    std::vector<std::string> dates(unique_dates.begin(), unique_dates.end());
    std::sort(dates.begin(), dates.end());
    return dates;
}

IndexStats VectorDatabase::stats() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return index_->stats();
}

void VectorDatabase::optimize() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_->optimize();
}

Result<void> VectorDatabase::sync() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto index_result = index_->save(paths_.index.string());
    if (!index_result) {
        return index_result;
    }
    
    auto vector_result = vectors_->sync();
    if (!vector_result) {
        return vector_result;
    }
    
    return metadata_->sync();
}

Result<void> VectorDatabase::compact() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return vectors_->compact();
}

// ============================================================================
// Export
// ============================================================================

Result<void> VectorDatabase::export_training_data(const fs::path& output_path) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::ofstream file(output_path);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to create output file"});
    }
    
    for (const auto& meta : metadata_->all()) {
        json entry;
        entry["id"] = meta.id;
        entry["type"] = std::string(document_type_name(meta.type));
        entry["date"] = meta.date;
        entry["source"] = meta.source_file;
        entry["asset"] = meta.asset;
        entry["bias"] = meta.bias;
        
        if (meta.gold_price) entry["gold_price"] = *meta.gold_price;
        if (meta.silver_price) entry["silver_price"] = *meta.silver_price;
        if (meta.gsr) entry["gsr"] = *meta.gsr;
        if (meta.dxy) entry["dxy"] = *meta.dxy;
        if (meta.vix) entry["vix"] = *meta.vix;
        if (meta.yield_10y) entry["yield_10y"] = *meta.yield_10y;
        
        file << entry.dump() << "\n";
    }
    
    return {};
}

// ============================================================================
// Factory Functions
// ============================================================================

Result<VectorDatabase> create_gold_standard_db(const fs::path& path) {
    DatabaseConfig config;
    config.path = path;
    config.dimension = UNIFIED_DIM;
    config.metric = DistanceMetric::Cosine;
    
    VectorDatabase db(config);
    auto result = db.init();
    if (!result) {
        return std::unexpected(result.error());
    }
    
    return db;
}

Result<VectorDatabase> open_database(const fs::path& path) {
    DatabasePaths paths(path);
    if (!paths.exists()) {
        return std::unexpected(Error{ErrorCode::IoError, "Database not found at path"});
    }
    
    // Load config
    std::ifstream config_file(paths.config);
    if (!config_file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to read config file"});
    }
    
    json config_json = json::parse(config_file);
    
    DatabaseConfig config;
    config.path = path;
    config.dimension = config_json.value("dimension", UNIFIED_DIM);
    config.metric = static_cast<DistanceMetric>(config_json.value("metric", 0));
    config.hnsw_m = config_json.value("hnsw_m", HNSW_M);
    
    VectorDatabase db(config);
    auto result = db.init();
    if (!result) {
        return std::unexpected(result.error());
    }
    
    return db;
}

} // namespace vdb
