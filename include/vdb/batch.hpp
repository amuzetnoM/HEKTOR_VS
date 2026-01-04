#pragma once
// ============================================================================
// VectorDB - Batch Operations API
// ============================================================================

#include "core.hpp"
#include "storage.hpp"
#include <vector>
#include <functional>

namespace vdb {

// ============================================================================
// Batch Insert Request
// ============================================================================

struct BatchInsertItem {
    VectorView vector;
    Metadata metadata;
};

using BatchInsertRequest = std::vector<BatchInsertItem>;

struct BatchInsertResult {
    std::vector<VectorId> ids;
    size_t successful = 0;
    size_t failed = 0;
    std::vector<std::string> errors;
};

// ============================================================================
// Batch Delete Request
// ============================================================================

using BatchDeleteRequest = std::vector<VectorId>;

struct BatchDeleteResult {
    size_t successful = 0;
    size_t failed = 0;
    std::vector<std::string> errors;
};

// ============================================================================
// Batch Update Request
// ============================================================================

struct BatchUpdateItem {
    VectorId id;
    Metadata metadata;
};

using BatchUpdateRequest = std::vector<BatchUpdateItem>;

struct BatchUpdateResult {
    size_t successful = 0;
    size_t failed = 0;
    std::vector<std::string> errors;
};

// ============================================================================
// Batch Query Request
// ============================================================================

struct BatchQueryItem {
    VectorView query;
    size_t k = 10;
};

using BatchQueryRequest = std::vector<BatchQueryItem>;
using BatchQueryResult = std::vector<std::vector<QueryResult>>;

// ============================================================================
// Batch Configuration
// ============================================================================

struct BatchConfig {
    size_t batch_size = 1000;              // Items per batch
    size_t num_threads = 0;                 // 0 = auto
    bool fail_fast = false;                 // Stop on first error
    bool transactional = false;             // All-or-nothing
    std::function<void(size_t, size_t)> progress_callback = nullptr;
};

} // namespace vdb
