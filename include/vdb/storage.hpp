#pragma once
// ============================================================================
// VectorDB - Storage Layer (Memory-Mapped Files)
// ============================================================================

#include "core.hpp"
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <shared_mutex>

namespace vdb {

namespace fs = std::filesystem;

// ============================================================================
// Memory-Mapped File (Cross-Platform)
// ============================================================================

class MemoryMappedFile {
public:
    MemoryMappedFile() = default;
    ~MemoryMappedFile();
    
    // Non-copyable
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;
    
    // Movable
    MemoryMappedFile(MemoryMappedFile&& other) noexcept;
    MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept;
    
    /// Open file for reading
    [[nodiscard]] Result<void> open_read(const fs::path& path);
    
    /// Open existing file for read/write (preserves contents, no truncation)
    [[nodiscard]] Result<void> open_readwrite(const fs::path& path);
    
    /// Open file for read/write (creates if not exists)
    [[nodiscard]] Result<void> open_write(const fs::path& path, size_t initial_size = 0);
    
    /// Close the file
    void close();
    
    /// Check if open
    [[nodiscard]] bool is_open() const { return data_ != nullptr; }
    
    /// Get data pointer
    [[nodiscard]] uint8_t* data() { return data_; }
    [[nodiscard]] const uint8_t* data() const { return data_; }
    
    /// Get file size
    [[nodiscard]] size_t size() const { return size_; }
    
    /// Resize file (may invalidate pointers)
    [[nodiscard]] Result<void> resize(size_t new_size);
    
    /// Sync to disk
    [[nodiscard]] Result<void> sync();

private:
    void unmap();
    
    uint8_t* data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
    fs::path path_;
    bool writable_ = false;
    
#ifdef VDB_PLATFORM_WINDOWS
    void* file_handle_ = nullptr;
    void* mapping_handle_ = nullptr;
#else
    int fd_ = -1;
#endif
};

// ============================================================================
// Vector Storage (Persistent)
// ============================================================================

struct VectorStoreConfig {
    fs::path path;              // Directory for vector storage
    Dim dimension = UNIFIED_DIM;
    size_t initial_capacity = 10000;
    bool memory_only = false;   // For testing
};

class VectorStore {
public:
    explicit VectorStore(const VectorStoreConfig& config);
    ~VectorStore();
    
    // Non-copyable, movable
    VectorStore(const VectorStore&) = delete;
    VectorStore& operator=(const VectorStore&) = delete;
    VectorStore(VectorStore&&) noexcept;
    VectorStore& operator=(VectorStore&&) noexcept;
    
    /// Initialize storage (create files if needed)
    [[nodiscard]] Result<void> init();
    
    /// Add vector
    [[nodiscard]] Result<void> add(VectorId id, VectorView vector);
    
    /// Get vector by ID
    [[nodiscard]] std::optional<VectorView> get(VectorId id) const;
    
    /// Check if vector exists
    [[nodiscard]] bool contains(VectorId id) const;
    
    /// Remove vector
    [[nodiscard]] Result<void> remove(VectorId id);
    
    /// Get all vector IDs
    [[nodiscard]] std::vector<VectorId> all_ids() const;
    
    /// Get count
    [[nodiscard]] size_t size() const { return id_to_offset_.size(); }
    
    /// Get capacity
    [[nodiscard]] size_t capacity() const { return capacity_; }
    
    /// Sync to disk
    [[nodiscard]] Result<void> sync();
    
    /// Compact storage (remove gaps from deletions)
    [[nodiscard]] Result<void> compact();
    
    /// Get storage stats
    [[nodiscard]] size_t memory_usage() const;

private:
    /// Allocate a slot for a new vector
    [[nodiscard]] size_t allocate_slot();
    
    /// Get pointer to slot data
    [[nodiscard]] Scalar* get_slot_ptr(size_t slot);
    [[nodiscard]] const Scalar* get_slot_ptr(size_t slot) const;
    
    VectorStoreConfig config_;
    MemoryMappedFile vectors_file_;
    std::unordered_map<VectorId, size_t> id_to_offset_;
    std::vector<size_t> free_slots_;
    size_t capacity_ = 0;
    size_t vector_size_bytes_ = 0;
    
    // CRITICAL: Mutex for thread-safe concurrent access during resize
    mutable std::shared_mutex mutex_;
};

// ============================================================================
// Metadata Storage (JSON Lines)
// ============================================================================

class MetadataStore {
public:
    explicit MetadataStore(const fs::path& path);
    ~MetadataStore();
    
    /// Initialize (load existing if present)
    [[nodiscard]] Result<void> init();
    
    /// Add metadata
    [[nodiscard]] Result<void> add(const Metadata& meta);
    
    /// Update metadata
    [[nodiscard]] Result<void> update(const Metadata& meta);
    
    /// Get metadata by ID
    [[nodiscard]] std::optional<Metadata> get(VectorId id) const;
    
    /// Get all metadata
    [[nodiscard]] std::vector<Metadata> all() const;
    
    /// Find by date
    [[nodiscard]] std::vector<Metadata> find_by_date(std::string_view date) const;
    
    /// Find by type
    [[nodiscard]] std::vector<Metadata> find_by_type(DocumentType type) const;
    
    /// Find by asset
    [[nodiscard]] std::vector<Metadata> find_by_asset(std::string_view asset) const;
    
    /// Remove metadata
    [[nodiscard]] Result<void> remove(VectorId id);
    
    /// Get count
    [[nodiscard]] size_t size() const { return metadata_.size(); }
    
    /// Sync to disk
    [[nodiscard]] Result<void> sync();

private:
    [[nodiscard]] Result<void> load();
    [[nodiscard]] Result<void> append_to_file(const Metadata& meta);
    
    fs::path path_;
    std::unordered_map<VectorId, Metadata> metadata_;
    std::ofstream append_stream_;
    bool dirty_ = false;
};

// ============================================================================
// Database Directory Structure
// ============================================================================

struct DatabasePaths {
    fs::path root;
    fs::path vectors;       // vectors.bin
    fs::path index;         // index.hnsw
    fs::path metadata;      // metadata.jsonl
    fs::path config;        // config.json
    fs::path models;        // models/
    fs::path text_model;    // models/text_encoder.onnx
    fs::path image_model;   // models/image_encoder.onnx
    fs::path projection;    // models/projection.bin
    
    explicit DatabasePaths(const fs::path& root_path);
    
    /// Create directory structure
    [[nodiscard]] Result<void> ensure_dirs() const;
    
    /// Check if database exists
    [[nodiscard]] bool exists() const;
};

} // namespace vdb
