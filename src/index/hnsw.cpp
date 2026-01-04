// ============================================================================
// VectorDB - HNSW Index Implementation
// ============================================================================

#include "vdb/index.hpp"
#include <queue>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <unordered_set>
#include <mutex>

namespace vdb {

// ============================================================================
// HNSW Index
// ============================================================================

HnswIndex::HnswIndex(const HnswConfig& config)
    : config_(config)
    , rng_(config.seed)
    , level_mult_(1.0 / std::log(static_cast<double>(config.M)))
{
    nodes_.reserve(config.max_elements);
}

HnswIndex::~HnswIndex() = default;

HnswIndex::HnswIndex(HnswIndex&& other) noexcept
    : config_(std::move(other.config_))
    , nodes_(std::move(other.nodes_))
    , id_to_index_(std::move(other.id_to_index_))
    , entry_point_(other.entry_point_)
    , max_level_(other.max_level_)
    , element_count_(other.element_count_)
    , rng_(std::move(other.rng_))
    , level_mult_(other.level_mult_)
{}

HnswIndex& HnswIndex::operator=(HnswIndex&& other) noexcept {
    if (this != &other) {
        config_ = std::move(other.config_);
        nodes_ = std::move(other.nodes_);
        id_to_index_ = std::move(other.id_to_index_);
        entry_point_ = other.entry_point_;
        max_level_ = other.max_level_;
        element_count_ = other.element_count_;
        rng_ = std::move(other.rng_);
        level_mult_ = other.level_mult_;
    }
    return *this;
}

int HnswIndex::random_level() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(rng_);
    return static_cast<int>(-std::log(r) * level_mult_);
}

Result<void> HnswIndex::add(VectorId id, VectorView vector) {
    if (vector.dim() != config_.dimension) {
        return std::unexpected(Error{ErrorCode::InvalidDimension, 
            "Expected dimension " + std::to_string(config_.dimension) + 
            ", got " + std::to_string(vector.dim())});
    }
    
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (element_count_ >= config_.max_elements) {
        return std::unexpected(Error{ErrorCode::IndexFull, "Index capacity reached"});
    }
    
    // Check for existing ID
    if (id_to_index_.contains(id)) {
        if (!config_.allow_replace) {
            return std::unexpected(Error{ErrorCode::InvalidVectorId, "Vector ID already exists"});
        }
        // Remove old entry
        // TODO: Implement proper removal
    }
    
    // Determine level for new node
    int level = random_level();
    level = std::min(level, max_level_ + 1);  // Don't jump too high
    
    // Create node
    Node node;
    node.id = id;
    node.level = level;
    node.connections.resize(level + 1);
    node.vector = Vector(std::vector<Scalar>(vector.begin(), vector.end()));
    
    size_t node_index = nodes_.size();
    nodes_.push_back(std::move(node));
    id_to_index_[id] = node_index;
    
    if (element_count_ == 0) {
        // First element
        entry_point_ = id;
        max_level_ = level;
        element_count_ = 1;
        return {};
    }
    
    // Find entry point at the highest level
    VectorId current = entry_point_;
    
    // Traverse from top to insertion level, greedy search
    for (int lv = max_level_; lv > level; --lv) {
        auto neighbors = search_layer(vector, current, 1, lv);
        if (!neighbors.empty()) {
            current = neighbors[0];
        }
    }
    
    // Insert at each level from insertion level down to 0
    for (int lv = std::min(level, max_level_); lv >= 0; --lv) {
        auto candidates = search_layer(vector, current, config_.ef_construction, lv);
        auto neighbors = select_neighbors(vector, candidates, config_.M, lv);
        
        // Connect new node to neighbors
        nodes_[node_index].connections[lv] = neighbors;
        
        // Connect neighbors back to new node
        for (VectorId neighbor_id : neighbors) {
            connect_nodes(neighbor_id, id, lv);
        }
        
        if (!candidates.empty()) {
            current = candidates[0];
        }
    }
    
    // Update entry point if new node has higher level
    if (level > max_level_) {
        max_level_ = level;
        entry_point_ = id;
    }
    
    element_count_++;
    return {};
}

Result<void> HnswIndex::add_batch(
    std::span<const VectorId> ids,
    std::span<const Vector> vectors
) {
    if (ids.size() != vectors.size()) {
        return std::unexpected(Error{ErrorCode::InvalidInput, "IDs and vectors count mismatch"});
    }
    
    for (size_t i = 0; i < ids.size(); ++i) {
        auto result = add(ids[i], vectors[i].view());
        if (!result) {
            return result;
        }
    }
    
    return {};
}

std::vector<VectorId> HnswIndex::search_layer(
    VectorView query,
    VectorId entry_point,
    size_t ef,
    int layer
) const {
    // Priority queue: (distance, id) - min-heap for candidates
    using DistId = std::pair<Distance, VectorId>;
    std::priority_queue<DistId, std::vector<DistId>, std::greater<>> candidates;
    std::priority_queue<DistId> results;  // max-heap for results
    
    std::unordered_set<VectorId> visited;
    
    Distance entry_dist = distance_to_node(query, entry_point);
    candidates.emplace(entry_dist, entry_point);
    results.emplace(entry_dist, entry_point);
    visited.insert(entry_point);
    
    while (!candidates.empty()) {
        auto [dist, current] = candidates.top();
        candidates.pop();
        
        // Stop if current is further than worst result
        if (results.size() >= ef && dist > results.top().first) {
            break;
        }
        
        // Get node index
        auto it = id_to_index_.find(current);
        if (it == id_to_index_.end()) continue;
        
        const Node& node = nodes_[it->second];
        if (layer >= static_cast<int>(node.connections.size())) continue;
        
        // Explore neighbors
        for (VectorId neighbor_id : node.connections[layer]) {
            if (visited.contains(neighbor_id)) continue;
            visited.insert(neighbor_id);
            
            // Skip deleted nodes
            auto neighbor_it = id_to_index_.find(neighbor_id);
            if (neighbor_it == id_to_index_.end()) continue;
            if (nodes_[neighbor_it->second].deleted) continue;
            
            Distance neighbor_dist = distance_to_node(query, neighbor_id);
            
            if (results.size() < ef || neighbor_dist < results.top().first) {
                candidates.emplace(neighbor_dist, neighbor_id);
                results.emplace(neighbor_dist, neighbor_id);
                
                if (results.size() > ef) {
                    results.pop();
                }
            }
        }
    }
    
    // Extract results, filtering out deleted nodes
    std::vector<VectorId> result_ids;
    result_ids.reserve(results.size());
    while (!results.empty()) {
        VectorId id = results.top().second;
        results.pop();
        
        // Double-check node is not deleted
        auto it = id_to_index_.find(id);
        if (it != id_to_index_.end() && !nodes_[it->second].deleted) {
            result_ids.push_back(id);
        }
    }
    std::reverse(result_ids.begin(), result_ids.end());
    
    return result_ids;
}

std::vector<VectorId> HnswIndex::select_neighbors(
    VectorView query,
    std::vector<VectorId>& candidates,
    size_t M,
    [[maybe_unused]] int layer
) const {
    // Simple heuristic: keep closest M
    if (candidates.size() <= M) {
        return candidates;
    }
    
    // Sort by distance
    std::sort(candidates.begin(), candidates.end(), 
        [this, &query](VectorId a, VectorId b) {
            return distance_to_node(query, a) < distance_to_node(query, b);
        });
    
    candidates.resize(M);
    return candidates;
}

Distance HnswIndex::distance_to_node(VectorView query, VectorId node_id) const {
    auto it = id_to_index_.find(node_id);
    if (it == id_to_index_.end()) {
        return std::numeric_limits<Distance>::max();
    }
    
    return compute_distance(query, nodes_[it->second].vector.view(), config_.metric);
}

void HnswIndex::connect_nodes(VectorId from, VectorId to, int layer) {
    auto it = id_to_index_.find(from);
    if (it == id_to_index_.end()) return;
    
    Node& node = nodes_[it->second];
    if (layer >= static_cast<int>(node.connections.size())) return;
    
    auto& connections = node.connections[layer];
    
    // Check if already connected
    if (std::find(connections.begin(), connections.end(), to) != connections.end()) {
        return;
    }
    
    connections.push_back(to);
    
    // Prune if too many connections
    size_t max_connections = (layer == 0) ? config_.M * 2 : config_.M;
    if (connections.size() > max_connections) {
        // Keep closest ones
        std::sort(connections.begin(), connections.end(),
            [this, &node](VectorId a, VectorId b) {
                return distance_to_node(node.vector.view(), a) < 
                       distance_to_node(node.vector.view(), b);
            });
        connections.resize(max_connections);
    }
}

SearchResults HnswIndex::search(VectorView query, size_t k) const {
    if (query.dim() != config_.dimension) {
        return {};
    }
    
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (element_count_ == 0) {
        return {};
    }
    
    VectorId current = entry_point_;
    
    // Traverse from top level to level 1
    for (int lv = max_level_; lv > 0; --lv) {
        auto neighbors = search_layer(query, current, 1, lv);
        if (!neighbors.empty()) {
            current = neighbors[0];
        }
    }
    
    // Search at level 0 with ef_search
    auto candidates = search_layer(query, current, config_.ef_search, 0);
    
    // Convert to SearchResults
    SearchResults results;
    results.reserve(std::min(k, candidates.size()));
    
    for (size_t i = 0; i < std::min(k, candidates.size()); ++i) {
        VectorId id = candidates[i];
        Distance dist = distance_to_node(query, id);
        results.push_back({id, dist});
    }
    
    return results;
}

SearchResults HnswIndex::search_filtered(
    VectorView query,
    size_t k,
    std::function<bool(VectorId)> filter
) const {
    // Search with larger ef to account for filtering
    size_t ef_multiplier = 3;
    auto candidates = search(query, k * ef_multiplier);
    
    SearchResults filtered;
    filtered.reserve(k);
    
    for (const auto& result : candidates) {
        if (filter(result.id)) {
            filtered.push_back(result);
            if (filtered.size() >= k) break;
        }
    }
    
    return filtered;
}

Result<void> HnswIndex::remove(VectorId id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = id_to_index_.find(id);
    if (it == id_to_index_.end()) {
        return std::unexpected(Error{ErrorCode::VectorNotFound, "Vector ID not found"});
    }
    
    // Mark as deleted (lazy deletion)
    // The node remains in the graph but is skipped during search
    // Full removal would require reconnecting the graph which is complex
    size_t node_index = it->second;
    nodes_[node_index].deleted = true;
    
    // Remove from lookup map
    id_to_index_.erase(it);
    element_count_--;
    
    return {};
}

bool HnswIndex::contains(VectorId id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return id_to_index_.contains(id);
}

std::optional<Vector> HnswIndex::get_vector(VectorId id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = id_to_index_.find(id);
    if (it == id_to_index_.end()) {
        return std::nullopt;
    }
    
    return nodes_[it->second].vector;
}

size_t HnswIndex::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return element_count_;
}

IndexStats HnswIndex::stats() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    IndexStats stats;
    stats.total_vectors = element_count_;
    stats.dimension = config_.dimension;
    stats.metric = config_.metric;
    stats.index_type = "HNSW";
    
    // Estimate memory usage
    size_t vector_memory = element_count_ * config_.dimension * sizeof(Scalar);
    size_t connection_memory = 0;
    for (const auto& node : nodes_) {
        for (const auto& level : node.connections) {
            connection_memory += level.size() * sizeof(VectorId);
        }
    }
    stats.memory_usage_bytes = vector_memory + connection_memory;
    stats.index_size_bytes = connection_memory;
    
    return stats;
}

void HnswIndex::set_ef_search(size_t ef) {
    config_.ef_search = ef;
}

Result<void> HnswIndex::resize(size_t new_max_elements) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (new_max_elements < element_count_) {
        return std::unexpected(Error{ErrorCode::InvalidInput, 
            "New capacity must be at least current element count"});
    }
    
    // Simply update the capacity - no need to rebuild the graph
    // The nodes_ vector will grow as needed
    config_.max_elements = new_max_elements;
    nodes_.reserve(new_max_elements);
    
    return {};
}

void HnswIndex::optimize() {
    // TODO: Implement optimization (compact memory, rebalance graph)
}

Result<void> HnswIndex::save(std::string_view path) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::ofstream file(std::string(path), std::ios::binary);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file for writing"});
    }
    
    // Write header
    uint32_t magic = 0x564442; // "VDB"
    uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    // Write config
    file.write(reinterpret_cast<const char*>(&config_.dimension), sizeof(config_.dimension));
    file.write(reinterpret_cast<const char*>(&config_.M), sizeof(config_.M));
    file.write(reinterpret_cast<const char*>(&config_.metric), sizeof(config_.metric));
    
    // Write state
    file.write(reinterpret_cast<const char*>(&element_count_), sizeof(element_count_));
    file.write(reinterpret_cast<const char*>(&max_level_), sizeof(max_level_));
    file.write(reinterpret_cast<const char*>(&entry_point_), sizeof(entry_point_));
    
    // Write nodes
    uint64_t node_count = nodes_.size();
    file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
    
    for (const auto& node : nodes_) {
        file.write(reinterpret_cast<const char*>(&node.id), sizeof(node.id));
        file.write(reinterpret_cast<const char*>(&node.level), sizeof(node.level));
        
        // Write vector
        file.write(reinterpret_cast<const char*>(node.vector.data()), 
                   node.vector.dim() * sizeof(Scalar));
        
        // Write connections
        for (int lv = 0; lv <= node.level; ++lv) {
            uint32_t conn_count = static_cast<uint32_t>(node.connections[lv].size());
            file.write(reinterpret_cast<const char*>(&conn_count), sizeof(conn_count));
            file.write(reinterpret_cast<const char*>(node.connections[lv].data()),
                       conn_count * sizeof(VectorId));
        }
    }
    
    return {};
}

Result<HnswIndex> HnswIndex::load(std::string_view path) {
    std::ifstream file(std::string(path), std::ios::binary);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file for reading"});
    }
    
    // Read header
    uint32_t magic, version;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (magic != 0x564442) {
        return std::unexpected(Error{ErrorCode::IndexCorrupted, "Invalid file format"});
    }
    
    HnswConfig config;
    file.read(reinterpret_cast<char*>(&config.dimension), sizeof(config.dimension));
    file.read(reinterpret_cast<char*>(&config.M), sizeof(config.M));
    file.read(reinterpret_cast<char*>(&config.metric), sizeof(config.metric));
    
    HnswIndex index(config);
    
    file.read(reinterpret_cast<char*>(&index.element_count_), sizeof(index.element_count_));
    file.read(reinterpret_cast<char*>(&index.max_level_), sizeof(index.max_level_));
    file.read(reinterpret_cast<char*>(&index.entry_point_), sizeof(index.entry_point_));
    
    uint64_t node_count;
    file.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
    
    index.nodes_.reserve(node_count);
    
    for (uint64_t i = 0; i < node_count; ++i) {
        Node node;
        file.read(reinterpret_cast<char*>(&node.id), sizeof(node.id));
        file.read(reinterpret_cast<char*>(&node.level), sizeof(node.level));
        
        // Read vector
        node.vector.resize(config.dimension);
        file.read(reinterpret_cast<char*>(node.vector.data()),
                  config.dimension * sizeof(Scalar));
        
        // Read connections
        node.connections.resize(node.level + 1);
        for (int lv = 0; lv <= node.level; ++lv) {
            uint32_t conn_count;
            file.read(reinterpret_cast<char*>(&conn_count), sizeof(conn_count));
            node.connections[lv].resize(conn_count);
            file.read(reinterpret_cast<char*>(node.connections[lv].data()),
                      conn_count * sizeof(VectorId));
        }
        
        index.id_to_index_[node.id] = index.nodes_.size();
        index.nodes_.push_back(std::move(node));
    }
    
    return index;
}

// ============================================================================
// Flat Index (Brute Force)
// ============================================================================

namespace {
// File format magic numbers
constexpr uint32_t FLAT_INDEX_MAGIC = 0x464C4154;  // "FLAT"
constexpr uint32_t FLAT_INDEX_VERSION = 1;
}  // anonymous namespace

FlatIndex::FlatIndex(Dim dimension, DistanceMetric metric)
    : dimension_(dimension)
    , metric_(metric)
{}

Result<void> FlatIndex::add(VectorId id, VectorView vector) {
    if (vector.dim() != dimension_) {
        return std::unexpected(Error{ErrorCode::InvalidDimension, "Dimension mismatch"});
    }
    
    if (id_to_index_.contains(id)) {
        return std::unexpected(Error{ErrorCode::InvalidVectorId, "ID already exists"});
    }
    
    id_to_index_[id] = vectors_.size();
    ids_.push_back(id);
    vectors_.emplace_back(std::vector<Scalar>(vector.begin(), vector.end()));
    
    return {};
}

SearchResults FlatIndex::search(VectorView query, size_t k) const {
    if (query.dim() != dimension_) {
        return {};
    }
    
    auto results = brute_force_knn(query, vectors_, k, metric_);
    
    // Map internal indices to IDs
    for (auto& result : results) {
        result.id = ids_[result.id];
    }
    
    return results;
}

bool FlatIndex::contains(VectorId id) const {
    return id_to_index_.contains(id);
}

std::optional<Vector> FlatIndex::get_vector(VectorId id) const {
    auto it = id_to_index_.find(id);
    if (it == id_to_index_.end()) {
        return std::nullopt;
    }
    return vectors_[it->second];
}

Result<void> FlatIndex::save(std::string_view path) const {
    std::ofstream file(std::string(path), std::ios::binary);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file for writing"});
    }
    
    // Write header
    file.write(reinterpret_cast<const char*>(&FLAT_INDEX_MAGIC), sizeof(FLAT_INDEX_MAGIC));
    file.write(reinterpret_cast<const char*>(&FLAT_INDEX_VERSION), sizeof(FLAT_INDEX_VERSION));
    
    // Write config
    file.write(reinterpret_cast<const char*>(&dimension_), sizeof(dimension_));
    file.write(reinterpret_cast<const char*>(&metric_), sizeof(metric_));
    
    // Write vector count
    uint64_t count = vectors_.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    // Write vectors with IDs
    for (size_t i = 0; i < vectors_.size(); ++i) {
        VectorId id = ids_[i];
        file.write(reinterpret_cast<const char*>(&id), sizeof(id));
        file.write(reinterpret_cast<const char*>(vectors_[i].data()),
                   vectors_[i].dim() * sizeof(Scalar));
    }
    
    return {};
}

Result<FlatIndex> FlatIndex::load(std::string_view path) {
    std::ifstream file(std::string(path), std::ios::binary);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file for reading"});
    }
    
    // Read header
    uint32_t magic, version;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (magic != FLAT_INDEX_MAGIC) {
        return std::unexpected(Error{ErrorCode::IndexCorrupted, "Invalid file format"});
    }
    
    if (version != FLAT_INDEX_VERSION) {
        return std::unexpected(Error{ErrorCode::IndexCorrupted, 
            "Unsupported file version: " + std::to_string(version)});
    }
    
    // Read config
    Dim dimension;
    DistanceMetric metric;
    file.read(reinterpret_cast<char*>(&dimension), sizeof(dimension));
    file.read(reinterpret_cast<char*>(&metric), sizeof(metric));
    
    FlatIndex index(dimension, metric);
    
    // Read vector count
    uint64_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    // Read vectors
    index.ids_.reserve(count);
    index.vectors_.reserve(count);
    
    for (uint64_t i = 0; i < count; ++i) {
        VectorId id;
        file.read(reinterpret_cast<char*>(&id), sizeof(id));
        
        Vector vec(dimension);
        file.read(reinterpret_cast<char*>(vec.data()),
                  dimension * sizeof(Scalar));
        
        index.id_to_index_[id] = index.vectors_.size();
        index.ids_.push_back(id);
        index.vectors_.push_back(std::move(vec));
    }
    
    return index;
}

} // namespace vdb
