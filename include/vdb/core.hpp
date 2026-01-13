#pragma once
// ============================================================================
// VectorDB - Core Type Definitions and Configuration
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <expected>
#include <chrono>

namespace vdb {

// ============================================================================
// Version
// ============================================================================
inline constexpr int VERSION_MAJOR = 1;
inline constexpr int VERSION_MINOR = 0;
inline constexpr int VERSION_PATCH = 0;

// ============================================================================
// Core Types
// ============================================================================

/// Vector ID type (64-bit for large datasets)
using VectorId = std::uint64_t;

/// Dimension type
using Dim = std::uint32_t;

/// Distance/similarity score
using Distance = float;

/// Timestamp for versioning
using Timestamp = std::int64_t;

/// Vector data type (float32 for compatibility with ONNX/ML)
using Scalar = float;

// ============================================================================
// SIMD Configuration (compile-time detection)
// ============================================================================

enum class SimdLevel : uint8_t {
    None = 0,
    SSE4 = 1,
    AVX2 = 2,
    AVX512 = 3
};

#if defined(__AVX512F__)
    inline constexpr SimdLevel SIMD_LEVEL = SimdLevel::AVX512;
    inline constexpr size_t SIMD_WIDTH = 16;  // 16 floats
#elif defined(__AVX2__) || defined(_M_AMD64)
    inline constexpr SimdLevel SIMD_LEVEL = SimdLevel::AVX2;
    inline constexpr size_t SIMD_WIDTH = 8;   // 8 floats
#elif defined(__SSE4_1__)
    inline constexpr SimdLevel SIMD_LEVEL = SimdLevel::SSE4;
    inline constexpr size_t SIMD_WIDTH = 4;   // 4 floats
#else
    inline constexpr SimdLevel SIMD_LEVEL = SimdLevel::None;
    inline constexpr size_t SIMD_WIDTH = 1;
#endif

// Alignment requirement for SIMD operations
inline constexpr size_t VECTOR_ALIGNMENT = SIMD_WIDTH * sizeof(Scalar);

// ============================================================================
// Constants
// ============================================================================

/// Default embedding dimensions
inline constexpr Dim TEXT_EMBEDDING_DIM = 384;    // all-MiniLM-L6-v2
inline constexpr Dim IMAGE_EMBEDDING_DIM = 512;   // CLIP ViT-B/32
inline constexpr Dim UNIFIED_DIM = 512;           // Projected dimension

/// HNSW defaults
inline constexpr size_t HNSW_M = 16;              // Max connections per layer
inline constexpr size_t HNSW_EF_CONSTRUCTION = 200;
inline constexpr size_t HNSW_EF_SEARCH = 50;
inline constexpr size_t HNSW_MAX_ELEMENTS = 1000000;

/// Storage
inline constexpr size_t PAGE_SIZE = 4096;
inline constexpr size_t MMAP_CHUNK_SIZE = 64 * 1024 * 1024;  // 64MB chunks

// ============================================================================
// Error Handling
// ============================================================================

enum class ErrorCode : uint8_t {
    Success = 0,
    InvalidDimension,
    InvalidVectorId,
    InvalidState,
    InvalidData,
    VectorNotFound,
    IndexFull,
    IndexCorrupted,
    StorageError,
    IoError,
    ModelLoadError,
    InferenceError,
    InvalidInput,
    OutOfMemory,
    NotImplemented,
    NetworkError,
    ParseError,
    SystemError,
    Unknown
};

struct Error {
    ErrorCode code;
    std::string message;
    
    Error(ErrorCode c, std::string msg = "") : code(c), message(std::move(msg)) {}
    
    [[nodiscard]] bool ok() const { return code == ErrorCode::Success; }
    [[nodiscard]] explicit operator bool() const { return !ok(); }
};

template<typename T>
using Result = std::expected<T, Error>;

// Helper functions for Result types
template<typename T>
inline Result<T> Ok(T&& value) {
    return Result<T>(std::forward<T>(value));
}

template<typename T>
inline Result<T> Err(Error error) {
    return std::unexpected(error);
}

// Specialization for string error messages
template<typename T>
inline Result<T> Err(const std::string& message) {
    return std::unexpected(Error(ErrorCode::Unknown, message));
}

// ============================================================================
// Vector View (non-owning)
// ============================================================================

class VectorView {
public:
    constexpr VectorView() noexcept : data_(nullptr), dim_(0) {}
    constexpr VectorView(const Scalar* data, Dim dim) noexcept : data_(data), dim_(dim) {}
    
    template<size_t N>
    constexpr VectorView(const std::array<Scalar, N>& arr) noexcept 
        : data_(arr.data()), dim_(static_cast<Dim>(N)) {}
    
    VectorView(const std::vector<Scalar>& vec) noexcept
        : data_(vec.data()), dim_(static_cast<Dim>(vec.size())) {}
    
    [[nodiscard]] constexpr const Scalar* data() const noexcept { return data_; }
    [[nodiscard]] constexpr Dim dim() const noexcept { return dim_; }
    [[nodiscard]] constexpr size_t size() const noexcept { return dim_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return dim_ == 0; }
    
    [[nodiscard]] constexpr Scalar operator[](size_t i) const { return data_[i]; }
    
    [[nodiscard]] constexpr const Scalar* begin() const noexcept { return data_; }
    [[nodiscard]] constexpr const Scalar* end() const noexcept { return data_ + dim_; }
    
    // Distance methods (implemented in distance.cpp)
    [[nodiscard]] float dot(const VectorView& other) const;
    [[nodiscard]] float cosine_similarity(const VectorView& other) const;
    [[nodiscard]] float euclidean_distance(const VectorView& other) const;

private:
    const Scalar* data_;
    Dim dim_;
};

// ============================================================================
// Aligned Vector (owning, SIMD-aligned)
// ============================================================================

class alignas(VECTOR_ALIGNMENT) Vector {
public:
    Vector() = default;
    
    explicit Vector(Dim dim) : data_(dim), dim_(dim) {}
    
    Vector(std::initializer_list<Scalar> init) 
        : data_(init), dim_(static_cast<Dim>(init.size())) {}
    
    explicit Vector(std::vector<Scalar> data)
        : data_(std::move(data)), dim_(static_cast<Dim>(data_.size())) {}
    
    // Copy/Move
    Vector(const Vector&) = default;
    Vector(Vector&&) noexcept = default;
    Vector& operator=(const Vector&) = default;
    Vector& operator=(Vector&&) noexcept = default;
    
    // Accessors
    [[nodiscard]] Scalar* data() noexcept { return data_.data(); }
    [[nodiscard]] const Scalar* data() const noexcept { return data_.data(); }
    [[nodiscard]] Dim dim() const noexcept { return dim_; }
    [[nodiscard]] size_t size() const noexcept { return dim_; }
    [[nodiscard]] bool empty() const noexcept { return dim_ == 0; }
    
    [[nodiscard]] Scalar& operator[](size_t i) { return data_[i]; }
    [[nodiscard]] Scalar operator[](size_t i) const { return data_[i]; }
    
    // Iterators
    [[nodiscard]] Scalar* begin() noexcept { return data_.data(); }
    [[nodiscard]] Scalar* end() noexcept { return data_.data() + dim_; }
    [[nodiscard]] const Scalar* begin() const noexcept { return data_.data(); }
    [[nodiscard]] const Scalar* end() const noexcept { return data_.data() + dim_; }
    
    // Conversion
    [[nodiscard]] operator VectorView() const noexcept { return {data_.data(), dim_}; }
    [[nodiscard]] VectorView view() const noexcept { return {data_.data(), dim_}; }
    
    // Resize (for building embeddings)
    void resize(Dim new_dim) {
        data_.resize(new_dim);
        dim_ = new_dim;
    }
    
    void reserve(Dim capacity) {
        data_.reserve(capacity);
    }

private:
    std::vector<Scalar> data_;
    Dim dim_ = 0;
};

// ============================================================================
// Search Result
// ============================================================================

struct SearchResult {
    VectorId id;
    Distance distance;
    float score;  // Similarity score (higher = better)
    
    bool operator<(const SearchResult& other) const {
        return distance < other.distance;
    }
    
    bool operator>(const SearchResult& other) const {
        return distance > other.distance;
    }
};

using SearchResults = std::vector<SearchResult>;

// ============================================================================
// Distance Metric
// ============================================================================

enum class DistanceMetric : uint8_t {
    Cosine,      // 1 - cos(a, b) - most common for embeddings
    L2,          // Euclidean distance
    DotProduct,  // Negative dot product (for normalized vectors)
    L2Squared    // Squared L2 (faster, no sqrt)
};

// ============================================================================
// Document Types (Gold Standard specific)
// ============================================================================

enum class DocumentType : uint8_t {
    Journal,           // Daily journal
    Chart,             // Asset chart PNG
    CatalystWatchlist, // Catalyst report
    InstitutionalMatrix,
    EconomicCalendar,
    WeeklyRundown,
    ThreeMonthReport,
    OneYearReport,
    MonthlyReport,
    YearlyReport,
    PreMarket,
    Unknown
};

[[nodiscard]] constexpr std::string_view document_type_name(DocumentType type) {
    switch (type) {
        case DocumentType::Journal: return "journal";
        case DocumentType::Chart: return "chart";
        case DocumentType::CatalystWatchlist: return "catalyst_watchlist";
        case DocumentType::InstitutionalMatrix: return "institutional_matrix";
        case DocumentType::EconomicCalendar: return "economic_calendar";
        case DocumentType::WeeklyRundown: return "weekly_rundown";
        case DocumentType::ThreeMonthReport: return "3m_report";
        case DocumentType::OneYearReport: return "1y_report";
        case DocumentType::MonthlyReport: return "monthly_report";
        case DocumentType::YearlyReport: return "yearly_report";
        case DocumentType::PreMarket: return "premarket";
        default: return "unknown";
    }
}

// ============================================================================
// Metadata
// ============================================================================

struct Metadata {
    VectorId id = 0;
    DocumentType type = DocumentType::Unknown;
    std::string date;           // YYYY-MM-DD
    std::string source_file;    // Original file path
    std::string asset;          // For charts: GOLD, SILVER, etc.
    std::string bias;           // BULLISH, BEARISH, NEUTRAL
    std::optional<float> gold_price;
    std::optional<float> silver_price;
    std::optional<float> gsr;   // Gold/Silver ratio
    std::optional<float> dxy;
    std::optional<float> vix;
    std::optional<float> yield_10y;
    std::string content_hash;   // SHA-256 of content
    Timestamp created_at = 0;
    Timestamp updated_at = 0;
    std::string extra_json;     // Additional structured data
};

// ============================================================================
// Index Statistics
// ============================================================================

struct IndexStats {
    size_t total_vectors = 0;
    size_t dimension = 0;
    size_t memory_usage_bytes = 0;
    size_t index_size_bytes = 0;
    DistanceMetric metric = DistanceMetric::Cosine;
    std::string index_type;
};

// ============================================================================
// Time Utilities
// ============================================================================

[[nodiscard]] inline Timestamp now_timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

[[nodiscard]] inline std::string timestamp_to_iso(Timestamp ts) {
    auto time = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(ts)
    );
    auto time_t = std::chrono::system_clock::to_time_t(time);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time_t));
    return buf;
}

} // namespace vdb
