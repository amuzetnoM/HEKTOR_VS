// ============================================================================
// VectorDB - Storage Implementation
// Memory-mapped file I/O for high-performance vector storage
// ============================================================================

#include "vdb/storage.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstring>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>

#ifdef VDB_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace vdb {

using json = nlohmann::json;

// ============================================================================
// Memory-Mapped File - Platform-specific implementations
// ============================================================================

MemoryMappedFile::~MemoryMappedFile() {
    close();
}

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept
    : data_(other.data_)
    , size_(other.size_)
    , capacity_(other.capacity_)
    , path_(std::move(other.path_))
    , writable_(other.writable_)
#ifdef VDB_PLATFORM_WINDOWS
    , file_handle_(other.file_handle_)
    , mapping_handle_(other.mapping_handle_)
#else
    , fd_(other.fd_)
#endif
{
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
    other.writable_ = false;
#ifdef VDB_PLATFORM_WINDOWS
    other.file_handle_ = INVALID_HANDLE_VALUE;
    other.mapping_handle_ = nullptr;
#else
    other.fd_ = -1;
#endif
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) noexcept {
    if (this != &other) {
        close();
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        path_ = std::move(other.path_);
        writable_ = other.writable_;
#ifdef VDB_PLATFORM_WINDOWS
        file_handle_ = other.file_handle_;
        mapping_handle_ = other.mapping_handle_;
        other.file_handle_ = INVALID_HANDLE_VALUE;
        other.mapping_handle_ = nullptr;
#else
        fd_ = other.fd_;
        other.fd_ = -1;
#endif
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        other.writable_ = false;
    }
    return *this;
}

void MemoryMappedFile::unmap() {
    if (data_ == nullptr) return;
    
#ifdef VDB_PLATFORM_WINDOWS
    FlushViewOfFile(data_, 0);
    UnmapViewOfFile(data_);
#else
    if (writable_) {
        msync(data_, capacity_, MS_SYNC);
    }
    munmap(data_, capacity_);
#endif
    data_ = nullptr;
}

void MemoryMappedFile::close() {
    unmap();
    
#ifdef VDB_PLATFORM_WINDOWS
    if (mapping_handle_ != nullptr) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
    }
    if (file_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
    size_ = 0;
    capacity_ = 0;
}

Result<void> MemoryMappedFile::open_read(const fs::path& path) {
    close();
    path_ = path;
    writable_ = false;
    
    if (!fs::exists(path)) {
        return std::unexpected(Error{ErrorCode::IoError, "File does not exist: " + path.string()});
    }
    
    size_ = fs::file_size(path);
    capacity_ = size_;
    
    if (size_ == 0) {
        return {};  // Empty file, nothing to map
    }
    
#ifdef VDB_PLATFORM_WINDOWS
    file_handle_ = CreateFileW(
        path.wstring().c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (file_handle_ == INVALID_HANDLE_VALUE) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file: " + path.string()});
    }
    
    mapping_handle_ = CreateFileMappingW(
        file_handle_,
        nullptr,
        PAGE_READONLY,
        0, 0,
        nullptr
    );
    
    if (mapping_handle_ == nullptr) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to create file mapping"});
    }
    
    data_ = static_cast<uint8_t*>(MapViewOfFile(
        mapping_handle_,
        FILE_MAP_READ,
        0, 0, 0
    ));
    
    if (data_ == nullptr) {
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
        mapping_handle_ = nullptr;
        file_handle_ = INVALID_HANDLE_VALUE;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to map view of file"});
    }
#else
    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file: " + path.string()});
    }
    
    data_ = static_cast<uint8_t*>(mmap(
        nullptr,
        size_,
        PROT_READ,
        MAP_PRIVATE,
        fd_,
        0
    ));
    
    if (data_ == MAP_FAILED) {
        data_ = nullptr;
        ::close(fd_);
        fd_ = -1;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to mmap file"});
    }
    
    // Advise kernel for sequential access
    madvise(data_, size_, MADV_SEQUENTIAL);
#endif
    
    return {};
}

Result<void> MemoryMappedFile::open_readwrite(const fs::path& path) {
    close();
    path_ = path;
    writable_ = true;
    
    if (!fs::exists(path)) {
        return std::unexpected(Error{ErrorCode::IoError, "File does not exist: " + path.string()});
    }
    
    size_ = fs::file_size(path);
    capacity_ = size_;
    
    if (size_ == 0) {
        return {};  // Empty file, nothing to map
    }
    
#ifdef VDB_PLATFORM_WINDOWS
    file_handle_ = CreateFileW(
        path.wstring().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (file_handle_ == INVALID_HANDLE_VALUE) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file for read-write: " + path.string()});
    }
    
    mapping_handle_ = CreateFileMappingW(
        file_handle_,
        nullptr,
        PAGE_READWRITE,
        0, 0,
        nullptr
    );
    
    if (mapping_handle_ == nullptr) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to create file mapping"});
    }
    
    data_ = static_cast<uint8_t*>(MapViewOfFile(
        mapping_handle_,
        FILE_MAP_ALL_ACCESS,
        0, 0, 0
    ));
    
    if (data_ == nullptr) {
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
        mapping_handle_ = nullptr;
        file_handle_ = INVALID_HANDLE_VALUE;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to map view of file"});
    }
#else
    fd_ = open(path.c_str(), O_RDWR);
    if (fd_ < 0) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file for read-write: " + path.string()});
    }
    
    data_ = static_cast<uint8_t*>(mmap(
        nullptr,
        size_,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd_,
        0
    ));
    
    if (data_ == MAP_FAILED) {
        data_ = nullptr;
        ::close(fd_);
        fd_ = -1;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to mmap file for read-write"});
    }
    
    madvise(data_, size_, MADV_SEQUENTIAL);
#endif
    
    return {};
}

Result<void> MemoryMappedFile::open_write(const fs::path& path, size_t initial_size) {
    close();
    path_ = path;
    writable_ = true;
    capacity_ = initial_size;
    size_ = initial_size;  // FIX: Set size to match capacity for newly created files
    
    // Create parent directories if needed
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
    
#ifdef VDB_PLATFORM_WINDOWS
    file_handle_ = CreateFileW(
        path.wstring().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (file_handle_ == INVALID_HANDLE_VALUE) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to create file: " + path.string()});
    }
    
    // Set initial file size
    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(initial_size);
    if (!SetFilePointerEx(file_handle_, li, nullptr, FILE_BEGIN) ||
        !SetEndOfFile(file_handle_)) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to set file size"});
    }
    
    mapping_handle_ = CreateFileMappingW(
        file_handle_,
        nullptr,
        PAGE_READWRITE,
        static_cast<DWORD>(initial_size >> 32),
        static_cast<DWORD>(initial_size & 0xFFFFFFFF),
        nullptr
    );
    
    if (mapping_handle_ == nullptr) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to create file mapping"});
    }
    
    data_ = static_cast<uint8_t*>(MapViewOfFile(
        mapping_handle_,
        FILE_MAP_ALL_ACCESS,
        0, 0, 0
    ));
    
    if (data_ == nullptr) {
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
        mapping_handle_ = nullptr;
        file_handle_ = INVALID_HANDLE_VALUE;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to map view of file"});
    }
#else
    fd_ = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd_ < 0) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to create file: " + path.string()});
    }
    
    // Set file size
    if (ftruncate(fd_, static_cast<off_t>(initial_size)) < 0) {
        ::close(fd_);
        fd_ = -1;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to set file size"});
    }
    
    data_ = static_cast<uint8_t*>(mmap(
        nullptr,
        initial_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd_,
        0
    ));
    
    if (data_ == MAP_FAILED) {
        data_ = nullptr;
        ::close(fd_);
        fd_ = -1;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to mmap file"});
    }
#endif
    
    return {};
}

Result<void> MemoryMappedFile::resize(size_t new_size) {
    if (!writable_) {
        return std::unexpected(Error{ErrorCode::IoError, "Cannot resize read-only mapping"});
    }
    
    if (new_size <= capacity_) {
        return {};  // Already have enough space
    }
    
    // Round up to nearest page size (4KB) for efficiency
    constexpr size_t page_size = 4096;
    new_size = (new_size + page_size - 1) & ~(page_size - 1);
    
#ifdef VDB_PLATFORM_WINDOWS
    // Unmap current view
    FlushViewOfFile(data_, 0);
    UnmapViewOfFile(data_);
    CloseHandle(mapping_handle_);
    
    // Extend file
    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(new_size);
    if (!SetFilePointerEx(file_handle_, li, nullptr, FILE_BEGIN) ||
        !SetEndOfFile(file_handle_)) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to extend file"});
    }
    
    // Recreate mapping
    mapping_handle_ = CreateFileMappingW(
        file_handle_,
        nullptr,
        PAGE_READWRITE,
        static_cast<DWORD>(new_size >> 32),
        static_cast<DWORD>(new_size & 0xFFFFFFFF),
        nullptr
    );
    
    if (mapping_handle_ == nullptr) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to create file mapping after resize"});
    }
    
    data_ = static_cast<uint8_t*>(MapViewOfFile(
        mapping_handle_,
        FILE_MAP_ALL_ACCESS,
        0, 0, 0
    ));
    
    if (data_ == nullptr) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to map view after resize"});
    }
#else
    // Unmap current region
    msync(data_, capacity_, MS_SYNC);
    munmap(data_, capacity_);
    
    // Extend file
    if (ftruncate(fd_, static_cast<off_t>(new_size)) < 0) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to extend file"});
    }
    
    // Remap with new size
    data_ = static_cast<uint8_t*>(mmap(
        nullptr,
        new_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd_,
        0
    ));
    
    if (data_ == MAP_FAILED) {
        data_ = nullptr;
        return std::unexpected(Error{ErrorCode::IoError, "Failed to remap file after resize"});
    }
#endif
    
    capacity_ = new_size;
    // FIX: Update size to match new capacity after resize
    // Without this, all subsequent bounds checks in get_slot_ptr() would fail
    // because they compare against size(), causing "Failed to get slot pointer" errors
    size_ = new_size;
    return {};
}

Result<void> MemoryMappedFile::sync() {
    if (data_ == nullptr || !writable_) {
        return {};
    }
    
#ifdef VDB_PLATFORM_WINDOWS
    if (!FlushViewOfFile(data_, 0)) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to flush file view"});
    }
    if (!FlushFileBuffers(file_handle_)) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to flush file buffers"});
    }
#else
    if (msync(data_, capacity_, MS_SYNC) < 0) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to sync mmap"});
    }
#endif
    
    return {};
}

// ============================================================================
// Vector Store - Memory-mapped vector storage with slot management
// ============================================================================

// File header structure for the vectors file
struct VectorFileHeader {
    uint32_t magic;           // 'VDB\0'
    uint32_t version;         // File format version
    uint32_t dimension;       // Vector dimension
    uint32_t reserved;        // Padding
    uint64_t vector_count;    // Number of vectors stored
    uint64_t capacity;        // Total slot capacity
    uint64_t free_list_head;  // Head of free slot linked list (or UINT64_MAX if none)
    uint8_t  padding[24];     // Future use, align to 64 bytes
    
    static constexpr uint32_t MAGIC = 0x00424456;  // "VDB\0"
    static constexpr uint32_t CURRENT_VERSION = 1;
    static constexpr size_t SIZE = 64;
};

static_assert(sizeof(VectorFileHeader) == VectorFileHeader::SIZE, 
              "Header size must be 64 bytes");

VectorStore::VectorStore(const VectorStoreConfig& config)
    : config_(config)
    , vector_size_bytes_(config.dimension * sizeof(Scalar))
{}

VectorStore::~VectorStore() {
    sync();
}

VectorStore::VectorStore(VectorStore&& other) noexcept
    : config_(other.config_)
    , vectors_file_(std::move(other.vectors_file_))
    , id_to_offset_(std::move(other.id_to_offset_))
    , free_slots_(std::move(other.free_slots_))
    , capacity_(other.capacity_)
    , vector_size_bytes_(other.vector_size_bytes_)
{
    other.capacity_ = 0;
}

VectorStore& VectorStore::operator=(VectorStore&& other) noexcept {
    if (this != &other) {
        sync();
        config_ = other.config_;
        vectors_file_ = std::move(other.vectors_file_);
        id_to_offset_ = std::move(other.id_to_offset_);
        free_slots_ = std::move(other.free_slots_);
        capacity_ = other.capacity_;
        vector_size_bytes_ = other.vector_size_bytes_;
        other.capacity_ = 0;
    }
    return *this;
}

Result<void> VectorStore::init() {
    fs::path vectors_path = config_.path / "vectors.bin";
    
    if (fs::exists(vectors_path)) {
        // Open existing file in read-write mode so vectors can be added/modified.
        // Using open_read() here would create a read-only mapping, causing a
        // segfault when add_vector() later writes to the mmap region. This was
        // the root cause of the double-init SIGSEGV: create_gold_standard_db()
        // calls init() internally (creating files with open_write), then if the
        // user calls init() again, it finds existing files and used to reopen
        // them read-only.
        auto result = vectors_file_.open_readwrite(vectors_path);
        if (!result) return std::unexpected(result.error());
        
        // Validate header
        if (vectors_file_.size() < VectorFileHeader::SIZE) {
            return std::unexpected(Error{ErrorCode::IoError, "Vectors file too small"});
        }
        
        // CRITICAL: Null check before dereferencing header pointer
        if (vectors_file_.data() == nullptr) {
            return std::unexpected(Error{ErrorCode::IoError, "Failed to map vectors file"});
        }
        
        auto* header = reinterpret_cast<const VectorFileHeader*>(vectors_file_.data());
        if (header->magic != VectorFileHeader::MAGIC) {
            return std::unexpected(Error{ErrorCode::IoError, "Invalid vectors file magic"});
        }
        if (header->version != VectorFileHeader::CURRENT_VERSION) {
            return std::unexpected(Error{ErrorCode::IoError, "Unsupported vectors file version"});
        }
        if (header->dimension != config_.dimension) {
            return std::unexpected(Error{ErrorCode::InvalidDimension, 
                        "Dimension mismatch: file has " + std::to_string(header->dimension) +
                        " but config has " + std::to_string(config_.dimension)});
        }
        
        capacity_ = header->capacity;
        
        // Rebuild id_to_offset from metadata (caller must do this)
        // The vectors file only stores raw vectors, not IDs
        
    } else {
        // Create new file
        fs::create_directories(config_.path);
        
        capacity_ = config_.initial_capacity;
        size_t initial_file_size = VectorFileHeader::SIZE + capacity_ * vector_size_bytes_;
        
        auto result = vectors_file_.open_write(vectors_path, initial_file_size);
        if (!result) return std::unexpected(result.error());
        
        // Write header - CRITICAL: Null check before dereferencing
        if (vectors_file_.data() == nullptr) {
            return std::unexpected(Error{ErrorCode::IoError, "Failed to map vectors file for writing"});
        }
        
        auto* header = reinterpret_cast<VectorFileHeader*>(vectors_file_.data());
        header->magic = VectorFileHeader::MAGIC;
        header->version = VectorFileHeader::CURRENT_VERSION;
        header->dimension = config_.dimension;
        header->reserved = 0;
        header->vector_count = 0;
        header->capacity = capacity_;
        header->free_list_head = UINT64_MAX;
        std::memset(header->padding, 0, sizeof(header->padding));
    }
    
    return {};
}

size_t VectorStore::allocate_slot() {
    if (!free_slots_.empty()) {
        size_t slot = free_slots_.back();
        free_slots_.pop_back();
        return slot;
    }
    
    // Need to allocate a new slot
    size_t slot = id_to_offset_.size();
    
    // Grow file if needed
    if (slot >= capacity_) {
        size_t new_capacity = capacity_ * 2;
        size_t new_file_size = VectorFileHeader::SIZE + new_capacity * vector_size_bytes_;
        auto result = vectors_file_.resize(new_file_size);
        if (!result) {
            // Failed to resize, return max as error indicator
            return SIZE_MAX;
        }
        
        // Update header - CRITICAL: Check pointer validity after resize
        if (vectors_file_.data() == nullptr) {
            // Resize succeeded but mapping failed - critical error
            return SIZE_MAX;
        }
        auto* header = reinterpret_cast<VectorFileHeader*>(vectors_file_.data());
        header->capacity = new_capacity;
        capacity_ = new_capacity;
    }
    
    return slot;
}

Scalar* VectorStore::get_slot_ptr(size_t slot) {
    if (vectors_file_.data() == nullptr || slot >= capacity_) {
        return nullptr;
    }
    
    size_t offset = VectorFileHeader::SIZE + slot * vector_size_bytes_;
    
    // Critical: Validate that offset + vector_size_bytes_ doesn't overflow
    // and stays within the mapped region to prevent segfaults
    // Use safe arithmetic to prevent overflow in the addition
    if (offset > vectors_file_.size() || 
        vector_size_bytes_ > vectors_file_.size() - offset) {
        return nullptr;
    }
    
    return reinterpret_cast<Scalar*>(vectors_file_.data() + offset);
}

const Scalar* VectorStore::get_slot_ptr(size_t slot) const {
    if (vectors_file_.data() == nullptr || slot >= capacity_) {
        return nullptr;
    }
    
    size_t offset = VectorFileHeader::SIZE + slot * vector_size_bytes_;
    
    // Critical: Validate that offset + vector_size_bytes_ doesn't overflow
    // and stays within the mapped region to prevent segfaults
    // Use safe arithmetic to prevent overflow in the addition
    if (offset > vectors_file_.size() || 
        vector_size_bytes_ > vectors_file_.size() - offset) {
        return nullptr;
    }
    
    return reinterpret_cast<const Scalar*>(vectors_file_.data() + offset);
}

Result<void> VectorStore::add(VectorId id, VectorView vector) {
    if (vector.dim() != config_.dimension) {
        return std::unexpected(Error{ErrorCode::InvalidDimension, 
                    "Expected dimension " + std::to_string(config_.dimension) +
                    " but got " + std::to_string(vector.dim())});
    }
    
    // CRITICAL: Use unique_lock for writes to prevent concurrent modifications
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (id_to_offset_.contains(id)) {
        return std::unexpected(Error{ErrorCode::InvalidVectorId, "Vector ID already exists"});
    }
    
    // Allocate a slot
    size_t slot = allocate_slot();
    if (slot == SIZE_MAX) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to allocate vector slot"});
    }
    
    // Copy vector data to the slot
    Scalar* slot_ptr = get_slot_ptr(slot);
    if (slot_ptr == nullptr) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to get slot pointer"});
    }
    
    std::memcpy(slot_ptr, vector.data(), vector_size_bytes_);
    
    // Update index
    id_to_offset_[id] = slot;
    
    // Update header - CRITICAL: Check pointer validity
    if (vectors_file_.data() == nullptr) {
        return std::unexpected(Error{ErrorCode::IoError, "File mapping invalid"});
    }
    auto* header = reinterpret_cast<VectorFileHeader*>(vectors_file_.data());
    header->vector_count = id_to_offset_.size();
    
    return {};
}

std::optional<VectorView> VectorStore::get(VectorId id) const {
    // Use shared_lock for reads to allow concurrent reads
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = id_to_offset_.find(id);
    if (it == id_to_offset_.end()) {
        return std::nullopt;
    }
    
    const Scalar* data = get_slot_ptr(it->second);
    if (data == nullptr) {
        return std::nullopt;
    }
    
    // WARNING: The returned VectorView holds a raw pointer that becomes invalid
    // when this function returns and the lock is released. Callers must either:
    // 1) Immediately copy the data (like HNSW does), OR
    // 2) Ensure no concurrent resize operations can occur
    // This is a known limitation of the current API for performance reasons.
    return VectorView(data, config_.dimension);
}

bool VectorStore::contains(VectorId id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return id_to_offset_.contains(id);
}

Result<void> VectorStore::remove(VectorId id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = id_to_offset_.find(id);
    if (it == id_to_offset_.end()) {
        return std::unexpected(Error{ErrorCode::VectorNotFound, "Vector ID not found"});
    }
    
    // Add slot to free list
    free_slots_.push_back(it->second);
    
    // Remove from index
    id_to_offset_.erase(it);
    
    // Update header - CRITICAL: Check pointer validity
    if (vectors_file_.data() == nullptr) {
        return std::unexpected(Error{ErrorCode::IoError, "File mapping invalid"});
    }
    auto* header = reinterpret_cast<VectorFileHeader*>(vectors_file_.data());
    header->vector_count = id_to_offset_.size();
    
    return {};
}

std::vector<VectorId> VectorStore::all_ids() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<VectorId> ids;
    ids.reserve(id_to_offset_.size());
    for (const auto& [id, _] : id_to_offset_) {
        ids.push_back(id);
    }
    return ids;
}

Result<void> VectorStore::sync() {
    return vectors_file_.sync();
}

Result<void> VectorStore::compact() {
    if (free_slots_.empty()) {
        return {};  // Nothing to compact
    }
    
    // Sort free slots in descending order
    std::sort(free_slots_.begin(), free_slots_.end(), std::greater<size_t>());
    
    // Move vectors from high slots to fill free slots
    for (size_t free_slot : free_slots_) {
        // Find the highest occupied slot
        size_t highest_slot = 0;
        VectorId highest_id = 0;
        for (const auto& [id, slot] : id_to_offset_) {
            if (slot > highest_slot) {
                highest_slot = slot;
                highest_id = id;
            }
        }
        
        // If free slot is higher than highest occupied, we're done
        if (free_slot >= highest_slot) {
            continue;
        }
        
        // Move vector from highest_slot to free_slot
        Scalar* src = get_slot_ptr(highest_slot);
        Scalar* dst = get_slot_ptr(free_slot);
        if (src && dst) {
            std::memcpy(dst, src, vector_size_bytes_);
            id_to_offset_[highest_id] = free_slot;
        }
    }
    
    free_slots_.clear();
    
    // Optionally shrink file (not implemented to avoid complexity)
    
    return {};
}

size_t VectorStore::memory_usage() const {
    size_t index_memory = id_to_offset_.size() * (sizeof(VectorId) + sizeof(size_t));
    size_t file_memory = capacity_ * vector_size_bytes_;
    return index_memory + file_memory;
}

// ============================================================================
// Metadata Store
// ============================================================================

MetadataStore::MetadataStore(const fs::path& path)
    : path_(path)
{}

MetadataStore::~MetadataStore() {
    sync();
}

Result<void> MetadataStore::init() {
    if (fs::exists(path_)) {
        return load();
    }
    return {};
}

Result<void> MetadataStore::load() {
    std::ifstream file(path_);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open metadata file"});
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        try {
            json j = json::parse(line);
            Metadata meta;
            meta.id = j.value("id", VectorId{0});
            meta.type = static_cast<DocumentType>(j.value("type", 0));
            meta.date = j.value("date", "");
            meta.source_file = j.value("source_file", "");
            meta.asset = j.value("asset", "");
            meta.bias = j.value("bias", "");
            
            if (j.contains("gold_price")) meta.gold_price = j["gold_price"];
            if (j.contains("silver_price")) meta.silver_price = j["silver_price"];
            if (j.contains("gsr")) meta.gsr = j["gsr"];
            if (j.contains("dxy")) meta.dxy = j["dxy"];
            if (j.contains("vix")) meta.vix = j["vix"];
            if (j.contains("yield_10y")) meta.yield_10y = j["yield_10y"];
            
            meta.content_hash = j.value("content_hash", "");
            meta.created_at = j.value("created_at", Timestamp{0});
            meta.updated_at = j.value("updated_at", Timestamp{0});
            
            metadata_[meta.id] = std::move(meta);
        } catch (...) {
            // Skip malformed lines
        }
    }
    
    return {};
}

Result<void> MetadataStore::add(const Metadata& meta) {
    metadata_[meta.id] = meta;
    dirty_ = true;
    return append_to_file(meta);
}

Result<void> MetadataStore::update(const Metadata& meta) {
    metadata_[meta.id] = meta;
    dirty_ = true;
    // Full rewrite on sync
    return {};
}

std::optional<Metadata> MetadataStore::get(VectorId id) const {
    auto it = metadata_.find(id);
    if (it == metadata_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<Metadata> MetadataStore::all() const {
    std::vector<Metadata> result;
    result.reserve(metadata_.size());
    for (const auto& [_, meta] : metadata_) {
        result.push_back(meta);
    }
    return result;
}

std::vector<Metadata> MetadataStore::find_by_date(std::string_view date) const {
    std::vector<Metadata> result;
    for (const auto& [_, meta] : metadata_) {
        if (meta.date == date) {
            result.push_back(meta);
        }
    }
    return result;
}

std::vector<Metadata> MetadataStore::find_by_type(DocumentType type) const {
    std::vector<Metadata> result;
    for (const auto& [_, meta] : metadata_) {
        if (meta.type == type) {
            result.push_back(meta);
        }
    }
    return result;
}

std::vector<Metadata> MetadataStore::find_by_asset(std::string_view asset) const {
    std::vector<Metadata> result;
    for (const auto& [_, meta] : metadata_) {
        if (meta.asset == asset) {
            result.push_back(meta);
        }
    }
    return result;
}

Result<void> MetadataStore::remove(VectorId id) {
    metadata_.erase(id);
    dirty_ = true;
    return {};
}

Result<void> MetadataStore::sync() {
    if (!dirty_) return {};
    
    // Rewrite entire file
    std::ofstream file(path_);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open metadata file for writing"});
    }
    
    for (const auto& [_, meta] : metadata_) {
        json j;
        j["id"] = meta.id;
        j["type"] = static_cast<int>(meta.type);
        j["date"] = meta.date;
        j["source_file"] = meta.source_file;
        j["asset"] = meta.asset;
        j["bias"] = meta.bias;
        
        if (meta.gold_price) j["gold_price"] = *meta.gold_price;
        if (meta.silver_price) j["silver_price"] = *meta.silver_price;
        if (meta.gsr) j["gsr"] = *meta.gsr;
        if (meta.dxy) j["dxy"] = *meta.dxy;
        if (meta.vix) j["vix"] = *meta.vix;
        if (meta.yield_10y) j["yield_10y"] = *meta.yield_10y;
        
        j["content_hash"] = meta.content_hash;
        j["created_at"] = meta.created_at;
        j["updated_at"] = meta.updated_at;
        
        file << j.dump() << "\n";
    }
    
    dirty_ = false;
    return {};
}

Result<void> MetadataStore::append_to_file(const Metadata& meta) {
    if (!append_stream_.is_open()) {
        append_stream_.open(path_, std::ios::app);
    }
    
    if (!append_stream_) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open metadata file for append"});
    }
    
    json j;
    j["id"] = meta.id;
    j["type"] = static_cast<int>(meta.type);
    j["date"] = meta.date;
    j["source_file"] = meta.source_file;
    j["asset"] = meta.asset;
    j["bias"] = meta.bias;
    
    if (meta.gold_price) j["gold_price"] = *meta.gold_price;
    if (meta.silver_price) j["silver_price"] = *meta.silver_price;
    if (meta.gsr) j["gsr"] = *meta.gsr;
    
    j["content_hash"] = meta.content_hash;
    j["created_at"] = meta.created_at;
    j["updated_at"] = meta.updated_at;
    
    append_stream_ << j.dump() << "\n";
    append_stream_.flush();
    
    return {};
}

} // namespace vdb
