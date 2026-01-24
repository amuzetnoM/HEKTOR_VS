// ============================================================================
// VectorDB - Vector Operations (SIMD Optimized)
// ============================================================================

#include "vdb/core.hpp"
#include "vdb/distance.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>

#if defined(__AVX512F__)
#include <immintrin.h>
#elif defined(__AVX2__) || defined(_M_AMD64)
#include <immintrin.h>
#elif defined(__SSE4_1__)
#include <smmintrin.h>
#endif

namespace vdb
{

    // ============================================================================
    // SIMD-Optimized Dot Product
    // ============================================================================

#if defined(__AVX512F__)

    static float dot_product_avx512(const float *a, const float *b, size_t n)
    {
        __m512 sum = _mm512_setzero_ps();
        size_t i = 0;

        for (; i + 16 <= n; i += 16)
        {
            __m512 va = _mm512_loadu_ps(a + i);
            __m512 vb = _mm512_loadu_ps(b + i);
            sum = _mm512_fmadd_ps(va, vb, sum);
        }

        float result = _mm512_reduce_add_ps(sum);

        // Handle remaining elements
        for (; i < n; ++i)
        {
            result += a[i] * b[i];
        }

        return result;
    }

#elif defined(__AVX2__) || defined(_M_AMD64)

    static float dot_product_avx2(const float *a, const float *b, size_t n)
    {
        __m256 sum = _mm256_setzero_ps();
        size_t i = 0;

        for (; i + 8 <= n; i += 8)
        {
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

        // Handle remaining elements
        for (; i < n; ++i)
        {
            result += a[i] * b[i];
        }

        return result;
    }

#elif defined(__SSE4_1__)

    static float dot_product_sse4(const float *a, const float *b, size_t n)
    {
        __m128 sum = _mm_setzero_ps();
        size_t i = 0;

        for (; i + 4 <= n; i += 4)
        {
            __m128 va = _mm_loadu_ps(a + i);
            __m128 vb = _mm_loadu_ps(b + i);
            sum = _mm_add_ps(sum, _mm_mul_ps(va, vb));
        }

        // Horizontal sum
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        float result = _mm_cvtss_f32(sum);

        // Handle remaining elements
        for (; i < n; ++i)
        {
            result += a[i] * b[i];
        }

        return result;
    }

#endif

    static float dot_product_scalar(const float *a, const float *b, size_t n)
    {
        float sum = 0.0f;
        for (size_t i = 0; i < n; ++i)
        {
            sum += a[i] * b[i];
        }
        return sum;
    }

    // ============================================================================
    // Public Interface
    // ============================================================================

    Distance dot_product(VectorView a, VectorView b)
    {
        if (a.dim() != b.dim() || a.dim() == 0)
        {
            return 0.0f;
        }

#if defined(__AVX512F__)
        return dot_product_avx512(a.data(), b.data(), a.dim());
#elif defined(__AVX2__) || defined(_M_AMD64)
        return dot_product_avx2(a.data(), b.data(), a.dim());
#elif defined(__SSE4_1__)
        return dot_product_sse4(a.data(), b.data(), a.dim());
#else
        return dot_product_scalar(a.data(), b.data(), a.dim());
#endif
    }

    Distance l2_norm(VectorView v)
    {
        return std::sqrt(dot_product(v, v));
    }

    Distance l2_squared_distance(VectorView a, VectorView b)
    {
        if (a.dim() != b.dim())
        {
            return std::numeric_limits<Distance>::max();
        }

        float sum = 0.0f;

#if defined(__AVX2__) || defined(_M_AMD64)
        __m256 vsum = _mm256_setzero_ps();
        size_t i = 0;

        for (; i + 8 <= a.dim(); i += 8)
        {
            __m256 va = _mm256_loadu_ps(a.data() + i);
            __m256 vb = _mm256_loadu_ps(b.data() + i);
            __m256 diff = _mm256_sub_ps(va, vb);
            vsum = _mm256_fmadd_ps(diff, diff, vsum);
        }

        // Horizontal sum
        __m128 hi = _mm256_extractf128_ps(vsum, 1);
        __m128 lo = _mm256_castps256_ps128(vsum);
        __m128 sum128 = _mm_add_ps(lo, hi);
        sum128 = _mm_hadd_ps(sum128, sum128);
        sum128 = _mm_hadd_ps(sum128, sum128);
        sum = _mm_cvtss_f32(sum128);

        for (; i < a.dim(); ++i)
        {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
#else
        for (size_t i = 0; i < a.dim(); ++i)
        {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
#endif

        return sum;
    }

    Distance l2_distance(VectorView a, VectorView b)
    {
        return std::sqrt(l2_squared_distance(a, b));
    }

    Distance cosine_similarity(VectorView a, VectorView b)
    {
        if (a.dim() != b.dim() || a.dim() == 0)
        {
            return 0.0f;
        }

        float dot = dot_product(a, b);
        float norm_a = l2_norm(a);
        float norm_b = l2_norm(b);

        if (norm_a < 1e-9f || norm_b < 1e-9f)
        {
            return 0.0f;
        }

        return dot / (norm_a * norm_b);
    }

    Distance cosine_distance(VectorView a, VectorView b)
    {
        return 1.0f - cosine_similarity(a, b);
    }

    Distance compute_distance(VectorView a, VectorView b, DistanceMetric metric)
    {
        switch (metric)
        {
        case DistanceMetric::Cosine:
            return cosine_distance(a, b);
        case DistanceMetric::L2:
            return l2_distance(a, b);
        case DistanceMetric::L2Squared:
            return l2_squared_distance(a, b);
        case DistanceMetric::DotProduct:
            return -dot_product(a, b); // Negative for distance
        default:
            return cosine_distance(a, b);
        }
    }

    // ============================================================================
    // Vector Operations
    // ============================================================================

    void normalize(Vector &v)
    {
        float norm = l2_norm(v.view());
        if (norm < 1e-9f)
            return;

        float inv_norm = 1.0f / norm;

#if defined(__AVX2__) || defined(_M_AMD64)
        __m256 vnorm = _mm256_set1_ps(inv_norm);
        size_t i = 0;

        for (; i + 8 <= v.dim(); i += 8)
        {
            __m256 vec = _mm256_loadu_ps(v.data() + i);
            vec = _mm256_mul_ps(vec, vnorm);
            _mm256_storeu_ps(v.data() + i, vec);
        }

        for (; i < v.dim(); ++i)
        {
            v[i] *= inv_norm;
        }
#else
        for (size_t i = 0; i < v.dim(); ++i)
        {
            v[i] *= inv_norm;
        }
#endif
    }

    Vector normalized(VectorView v)
    {
        Vector result(std::vector<Scalar>(v.begin(), v.end()));
        normalize(result);
        return result;
    }

    Vector add(VectorView a, VectorView b)
    {
        if (a.dim() != b.dim())
        {
            return Vector();
        }

        Vector result(a.dim());

#if defined(__AVX2__) || defined(_M_AMD64)
        size_t i = 0;
        for (; i + 8 <= a.dim(); i += 8)
        {
            __m256 va = _mm256_loadu_ps(a.data() + i);
            __m256 vb = _mm256_loadu_ps(b.data() + i);
            _mm256_storeu_ps(result.data() + i, _mm256_add_ps(va, vb));
        }
        for (; i < a.dim(); ++i)
        {
            result[i] = a[i] + b[i];
        }
#else
        for (size_t i = 0; i < a.dim(); ++i)
        {
            result[i] = a[i] + b[i];
        }
#endif

        return result;
    }

    Vector subtract(VectorView a, VectorView b)
    {
        if (a.dim() != b.dim())
        {
            return Vector();
        }

        Vector result(a.dim());
        for (size_t i = 0; i < a.dim(); ++i)
        {
            result[i] = a[i] - b[i];
        }
        return result;
    }

    Vector scale(VectorView v, Scalar s)
    {
        Vector result(v.dim());

#if defined(__AVX2__) || defined(_M_AMD64)
        __m256 vs = _mm256_set1_ps(s);
        size_t i = 0;
        for (; i + 8 <= v.dim(); i += 8)
        {
            __m256 vec = _mm256_loadu_ps(v.data() + i);
            _mm256_storeu_ps(result.data() + i, _mm256_mul_ps(vec, vs));
        }
        for (; i < v.dim(); ++i)
        {
            result[i] = v[i] * s;
        }
#else
        for (size_t i = 0; i < v.dim(); ++i)
        {
            result[i] = v[i] * s;
        }
#endif

        return result;
    }

    Vector add_scaled(VectorView a, VectorView b, Scalar s)
    {
        if (a.dim() != b.dim())
        {
            return Vector();
        }

        Vector result(a.dim());
        for (size_t i = 0; i < a.dim(); ++i)
        {
            result[i] = a[i] + b[i] * s;
        }
        return result;
    }

    Vector multiply(VectorView a, VectorView b)
    {
        if (a.dim() != b.dim())
        {
            return Vector();
        }

        Vector result(a.dim());
        for (size_t i = 0; i < a.dim(); ++i)
        {
            result[i] = a[i] * b[i];
        }
        return result;
    }

    Vector mean(std::span<const Vector> vectors)
    {
        if (vectors.empty())
        {
            return Vector();
        }

        Dim dim = vectors[0].dim();
        Vector result(dim);

        for (const auto &v : vectors)
        {
            if (v.dim() != dim)
                continue;
            for (size_t i = 0; i < dim; ++i)
            {
                result[i] += v[i];
            }
        }

        float inv_n = 1.0f / static_cast<float>(vectors.size());
        for (size_t i = 0; i < dim; ++i)
        {
            result[i] *= inv_n;
        }

        return result;
    }

    // ============================================================================
    // Batch Operations
    // ============================================================================

    std::vector<Distance> batch_distance(
        VectorView query,
        std::span<const Vector> targets,
        DistanceMetric metric)
    {
        std::vector<Distance> results;
        results.reserve(targets.size());

        for (const auto &target : targets)
        {
            results.push_back(compute_distance(query, target.view(), metric));
        }

        return results;
    }

    SearchResults brute_force_knn(
        VectorView query,
        std::span<const Vector> vectors,
        size_t k,
        DistanceMetric metric)
    {
        std::vector<SearchResult> all_results;
        all_results.reserve(vectors.size());

        for (size_t i = 0; i < vectors.size(); ++i)
        {
            Distance dist = compute_distance(query, vectors[i].view(), metric);
            all_results.push_back({static_cast<VectorId>(i), dist});
        }

        // Partial sort for top k
        k = std::min(k, all_results.size());
        std::partial_sort(all_results.begin(), all_results.begin() + k, all_results.end());

        all_results.resize(k);
        return all_results;
    }

    // ============================================================================
    // Projection Matrix
    // ============================================================================

    ProjectionMatrix::ProjectionMatrix(Dim input_dim, Dim output_dim)
        : input_dim_(input_dim), output_dim_(output_dim), weights_(static_cast<size_t>(input_dim) * output_dim)
    {
        init_random_orthogonal();
    }

    void ProjectionMatrix::init_random_orthogonal(uint64_t seed)
    {
        // Random Gaussian initialization, then orthogonalize
        std::mt19937_64 rng(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f / std::sqrt(static_cast<float>(input_dim_)));

        for (auto &w : weights_)
        {
            w = dist(rng);
        }

        // Simple orthogonalization via Gram-Schmidt would go here
        // For now, random projection is sufficient (approximately preserves distances)
    }

    Vector ProjectionMatrix::project(VectorView input) const
    {
        if (input.dim() != input_dim_)
        {
            return Vector();
        }

        Vector output(output_dim_);

        // Matrix-vector multiply: output = weights * input
        for (Dim i = 0; i < output_dim_; ++i)
        {
            float sum = 0.0f;
            const float *row = weights_.data() + static_cast<size_t>(i) * input_dim_;

#if defined(__AVX2__) || defined(_M_AMD64)
            __m256 vsum = _mm256_setzero_ps();
            size_t j = 0;
            for (; j + 8 <= input_dim_; j += 8)
            {
                __m256 vw = _mm256_loadu_ps(row + j);
                __m256 vi = _mm256_loadu_ps(input.data() + j);
                vsum = _mm256_fmadd_ps(vw, vi, vsum);
            }
            // Horizontal sum
            __m128 hi = _mm256_extractf128_ps(vsum, 1);
            __m128 lo = _mm256_castps256_ps128(vsum);
            __m128 sum128 = _mm_add_ps(lo, hi);
            sum128 = _mm_hadd_ps(sum128, sum128);
            sum128 = _mm_hadd_ps(sum128, sum128);
            sum = _mm_cvtss_f32(sum128);
            for (; j < input_dim_; ++j)
            {
                sum += row[j] * input[j];
            }
#else
            for (Dim j = 0; j < input_dim_; ++j)
            {
                sum += row[j] * input[j];
            }
#endif

            output[i] = sum;
        }

        return output;
    }

    Result<void> ProjectionMatrix::load(std::string_view path)
    {
        // TODO: Implement file loading
        (void)path;
        return tl::unexpected(Error{ErrorCode::NotImplemented, "ProjectionMatrix::load not implemented"});
    }

    Result<void> ProjectionMatrix::save(std::string_view path) const
    {
        // TODO: Implement file saving
        (void)path;
        return tl::unexpected(Error{ErrorCode::NotImplemented, "ProjectionMatrix::save not implemented"});
    }

} // namespace vdb
