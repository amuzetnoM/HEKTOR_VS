// ============================================================================
// VectorDB - Distance Functions Implementation
// SIMD-optimized distance calculations
// ============================================================================

#include "vdb/distance.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

// SIMD intrinsics
#if defined(__AVX512F__)
    #include <immintrin.h>
    #define VDB_USE_AVX512 1
#elif defined(__AVX2__)
    #include <immintrin.h>
    #define VDB_USE_AVX2 1
#elif defined(__SSE4_1__)
    #include <smmintrin.h>
    #define VDB_USE_SSE4 1
#endif

namespace vdb {

// ============================================================================
// Scalar Fallback Implementations
// ============================================================================

namespace scalar {

float dot_product(const float* a, const float* b, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        sum += a[i] * b[i];
    }
    return sum;
}

float squared_l2(const float* a, const float* b, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

float norm(const float* a, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        sum += a[i] * a[i];
    }
    return std::sqrt(sum);
}

} // namespace scalar

// ============================================================================
// AVX2 Implementations
// ============================================================================

#ifdef VDB_USE_AVX2

namespace avx2 {

float dot_product(const float* a, const float* b, size_t n) {
    __m256 sum = _mm256_setzero_ps();
    
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        sum = _mm256_fmadd_ps(va, vb, sum);
    }
    
    // Horizontal sum
    __m128 hi = _mm256_extractf128_ps(sum, 1);
    __m128 lo = _mm256_castps256_ps128(sum);
    __m128 sum128 = _mm_add_ps(lo, hi);
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    
    float result = _mm_cvtss_f32(sum128);
    
    // Handle remainder
    for (; i < n; ++i) {
        result += a[i] * b[i];
    }
    
    return result;
}

float squared_l2(const float* a, const float* b, size_t n) {
    __m256 sum = _mm256_setzero_ps();
    
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(va, vb);
        sum = _mm256_fmadd_ps(diff, diff, sum);
    }
    
    // Horizontal sum
    __m128 hi = _mm256_extractf128_ps(sum, 1);
    __m128 lo = _mm256_castps256_ps128(sum);
    __m128 sum128 = _mm_add_ps(lo, hi);
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    
    float result = _mm_cvtss_f32(sum128);
    
    // Handle remainder
    for (; i < n; ++i) {
        float diff = a[i] - b[i];
        result += diff * diff;
    }
    
    return result;
}

float norm(const float* a, size_t n) {
    __m256 sum = _mm256_setzero_ps();
    
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        sum = _mm256_fmadd_ps(va, va, sum);
    }
    
    // Horizontal sum
    __m128 hi = _mm256_extractf128_ps(sum, 1);
    __m128 lo = _mm256_castps256_ps128(sum);
    __m128 sum128 = _mm_add_ps(lo, hi);
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    
    float result = _mm_cvtss_f32(sum128);
    
    // Handle remainder
    for (; i < n; ++i) {
        result += a[i] * a[i];
    }
    
    return std::sqrt(result);
}

} // namespace avx2

#endif // VDB_USE_AVX2

// ============================================================================
// Public API - Dispatch to best available implementation
// ============================================================================

float dot_product(const Scalar* a, const Scalar* b, Dim n) {
#ifdef VDB_USE_AVX2
    return avx2::dot_product(a, b, n);
#else
    return scalar::dot_product(a, b, n);
#endif
}

float euclidean_distance(const Scalar* a, const Scalar* b, Dim n) {
#ifdef VDB_USE_AVX2
    return std::sqrt(avx2::squared_l2(a, b, n));
#else
    return std::sqrt(scalar::squared_l2(a, b, n));
#endif
}

float squared_euclidean(const Scalar* a, const Scalar* b, Dim n) {
#ifdef VDB_USE_AVX2
    return avx2::squared_l2(a, b, n);
#else
    return scalar::squared_l2(a, b, n);
#endif
}

float cosine_similarity(const Scalar* a, const Scalar* b, Dim n) {
    float dot = dot_product(a, b, n);
    
#ifdef VDB_USE_AVX2
    float norm_a = avx2::norm(a, n);
    float norm_b = avx2::norm(b, n);
#else
    float norm_a = scalar::norm(a, n);
    float norm_b = scalar::norm(b, n);
#endif
    
    if (norm_a < 1e-12f || norm_b < 1e-12f) {
        return 0.0f;
    }
    
    return dot / (norm_a * norm_b);
}

float cosine_distance(const Scalar* a, const Scalar* b, Dim n) {
    return 1.0f - cosine_similarity(a, b, n);
}

// ============================================================================
// VectorView distance methods
// ============================================================================

float VectorView::dot(const VectorView& other) const {
    if (dim() != other.dim()) return 0.0f;
    return dot_product(data(), other.data(), dim());
}

float VectorView::cosine_similarity(const VectorView& other) const {
    if (dim() != other.dim()) return 0.0f;
    return vdb::cosine_similarity(data(), other.data(), dim());
}

float VectorView::euclidean_distance(const VectorView& other) const {
    if (dim() != other.dim()) return 0.0f;
    return vdb::euclidean_distance(data(), other.data(), dim());
}

} // namespace vdb
