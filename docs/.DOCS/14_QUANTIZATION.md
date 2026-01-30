---
title: "Quantization"
description: "Vector compression and quantization techniques"
version: "4.1.5"
last_updated: "2026-01-24"
sidebar_position: 14
category: "Optimization"
---
![Version](https://img.shields.io/badge/version-4.1.5-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--24-green?style=flat-square)
![Type](https://img.shields.io/badge/type-advanced-red?style=flat-square)

## Table of Contents

1. [Overview](#overview)
2. [Quantization Methods](#quantization-methods)
3. [Product Quantization](#product-quantization)
4. [Scalar Quantization](#scalar-quantization)
5. [Perceptual Quantization (NEW)](#perceptual-quantization)
6. [API Reference](#api-reference)
7. [Usage Examples](#usage-examples)
8. [Performance Comparison](#performance-comparison)
9. [Best Practices](#best-practices)
10. [When to Use Quantization](#when-to-use-quantization)

---

## Overview

Vector quantization compresses high-dimensional vectors to reduce memory usage while maintaining search quality. Vector Studio supports two quantization methods:

| Method | Compression | Accuracy | Speed | Use Case |
|--------|-------------|----------|-------|----------|
| **Scalar Quantization** | 4x | High (95-98%) | Fast | Memory-constrained systems |
| **Product Quantization** | 8-32x | Good (85-95%) | Very Fast | Large-scale databases |

### Why Use Quantization?

**Benefits**:
- 🗜️ **Reduce memory** by 4-32x
- ⚡ **Faster search** with distance table precomputation
- 📈 **Scale larger** datasets on same hardware
- 💰 **Lower costs** for cloud deployments

**Trade-offs**:
- 📉 Slight accuracy loss (5-15% depending on method)
- 🔧 Requires training phase
- 🧮 Additional computational overhead for encoding

---

## Quantization Methods

### Comparison Matrix

| Feature | Scalar Quantization | Product Quantization |
|---------|---------------------|----------------------|
| **Compression Ratio** | 4x (float32 → uint8) | 8-32x (configurable) |
| **Training Time** | Fast (seconds) | Medium (minutes) |
| **Encoding Speed** | Very Fast | Fast |
| **Search Speed** | Fast | Very Fast (with ADC) |
| **Accuracy** | 95-98% | 85-95% |
| **Memory Overhead** | Low | Medium |
| **Complexity** | Simple | Complex |
| **Best For** | Small-medium datasets | Large datasets |

---

## Product Quantization

### Concept

Product Quantization (PQ) divides each vector into **m subvectors** and quantizes each subvector independently using k-means clustering.

**Mathematical Foundation**:

```
Vector x ∈ ℝ^d split into m subvectors:
x = [x₁, x₂, ..., xₘ] where each xᵢ ∈ ℝ^(d/m)

Each subvector quantized to nearest centroid:
q(xᵢ) = argmin_j ||xᵢ - cⱼⁱ||

Storage: m × log₂(k) bits
Example: 8 subvectors × 8 bits = 64 bits (32x compression for 512-dim vectors)
```

### Configuration

```cpp
#include "vdb/quantization/product_quantizer.hpp"

using namespace vdb::quantization;

ProductQuantizerConfig config;
config.dimension = 512;              // Must match vector dimension
config.num_subquantizers = 8;       // Number of subvectors (must divide dimension)
config.num_centroids = 256;         // Centroids per subquantizer (256 = 8-bit codes)
config.num_iterations = 25;         // K-means iterations
config.num_threads = 0;             // 0 = auto-detect
config.metric = DistanceMetric::L2; // Distance metric
```

### Usage

```cpp
// Create quantizer
ProductQuantizer pq(config);

// Train on representative data
std::vector<Vector> training_data = get_training_vectors(10000);
auto result = pq.train(training_data);

if (!result) {
    std::cerr << "Training failed: " << result.error().message << "\n";
    return;
}

// Encode vectors
Vector original = get_vector();
auto encoded = pq.encode(original);

if (encoded) {
    std::cout << "Original size: " << original.size() * sizeof(float) << " bytes\n";
    std::cout << "Encoded size: " << encoded->size() << " bytes\n";
    std::cout << "Compression: " << pq.compression_ratio() << "x\n";
}

// Decode for reconstruction
auto reconstructed = pq.decode(*encoded);

// Compute distance (Asymmetric Distance Computation)
float distance = pq.compute_distance(query_vector, *encoded);
```

### Advanced: Distance Table Precomputation

For fast batch search, precompute distances:

```cpp
// Precompute distance table for query
auto distance_table = pq.precompute_distance_table(query_vector);

// Fast distance computation for many vectors
for (const auto& encoded_vector : database) {
    float dist = pq.compute_distance_precomputed(encoded_vector, distance_table);
    // Process results...
}
```

### Persistence

```cpp
// Save trained quantizer
pq.save("quantizer.pq");

// Load quantizer
auto loaded_pq = ProductQuantizer::load("quantizer.pq");
if (loaded_pq) {
    // Use loaded quantizer
}
```

---

## Scalar Quantization

### Concept

Scalar Quantization (SQ) quantizes each dimension independently by mapping float values to 8-bit integers.

**Mathematical Foundation**:

```
For each dimension i:
min_i = min(x_i) across training set
max_i = max(x_i) across training set

Quantization:
q(x_i) = round((x_i - min_i) / (max_i - min_i) * 255)

Dequantization:
x_i ≈ (q(x_i) / 255) * (max_i - min_i) + min_i

Storage: d × 8 bits (4x compression for float32)
```

### Configuration

```cpp
#include "vdb/quantization/scalar_quantizer.hpp"

using namespace vdb::quantization;

ScalarQuantizerConfig config;
config.dimension = 512;              // Vector dimension
config.per_dimension = true;         // Per-dimension vs global min/max
```

### Usage

```cpp
// Create quantizer
ScalarQuantizer sq(config);

// Train on representative data
std::vector<Vector> training_data = get_training_vectors(5000);
auto result = sq.train(training_data);

if (!result) {
    std::cerr << "Training failed\n";
    return;
}

// Encode vectors
Vector original = get_vector();
auto encoded = sq.encode(original);

if (encoded) {
    std::cout << "Compression: " << sq.compression_ratio() << "x\n";
}

// Decode
auto reconstructed = sq.decode(*encoded);

// Compute distance on quantized data
float distance = sq.compute_distance(query_vector, *encoded);
```

### Persistence

```cpp
// Save quantizer
sq.save("quantizer.sq");

// Load quantizer
auto loaded_sq = ScalarQuantizer::load("quantizer.sq");
```

---

## API Reference

### ProductQuantizer Class

#### Constructor

```cpp
explicit ProductQuantizer(const ProductQuantizerConfig& config = {});
```

#### Training

```cpp
[[nodiscard]] Result<void> train(std::span<const Vector> training_data);
[[nodiscard]] bool is_trained() const;
```

**Parameters**:
- `training_data`: Representative vectors (10,000+ recommended)

**Returns**: Result with success/error

#### Encoding

```cpp
[[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
[[nodiscard]] Result<std::vector<std::vector<uint8_t>>> encode_batch(
    std::span<const Vector> vectors) const;
```

#### Decoding

```cpp
[[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
```

#### Distance Computation

```cpp
[[nodiscard]] Distance compute_distance(VectorView query, 
    std::span<const uint8_t> codes) const;

[[nodiscard]] std::vector<Distance> precompute_distance_table(
    VectorView query) const;

[[nodiscard]] Distance compute_distance_precomputed(
    std::span<const uint8_t> codes,
    std::span<const Distance> distance_table) const;
```

#### Metrics

```cpp
[[nodiscard]] size_t code_size() const;
[[nodiscard]] float compression_ratio() const;
```

#### Persistence

```cpp
[[nodiscard]] Result<void> save(std::string_view path) const;
[[nodiscard]] static Result<ProductQuantizer> load(std::string_view path);
```

---

### ScalarQuantizer Class

#### Constructor

```cpp
explicit ScalarQuantizer(const ScalarQuantizerConfig& config = {});
```

#### Training

```cpp
[[nodiscard]] Result<void> train(std::span<const Vector> training_data);
[[nodiscard]] bool is_trained() const;
```

#### Encoding/Decoding

```cpp
[[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
[[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
```

#### Distance Computation

```cpp
[[nodiscard]] Distance compute_distance(VectorView query,
    std::span<const uint8_t> codes) const;
```

#### Metrics

```cpp
[[nodiscard]] size_t code_size() const;
[[nodiscard]] float compression_ratio() const;
```

#### Persistence

```cpp
[[nodiscard]] Result<void> save(std::string_view path) const;
[[nodiscard]] static Result<ScalarQuantizer> load(std::string_view path);
```

---

## Usage Examples

### Example 1: Basic Product Quantization

```cpp
#include "vdb/quantization/product_quantizer.hpp"
#include "vdb/database.hpp"

using namespace vdb;
using namespace vdb::quantization;

// Load database
VectorDatabase db("my_vectors");

// Get training data (sample from database)
std::vector<Vector> training_vectors;
for (size_t i = 0; i < 10000; ++i) {
    auto vec = db.get_vector(i);
    if (vec) {
        training_vectors.push_back(*vec);
    }
}

// Configure and train Product Quantizer
ProductQuantizerConfig config;
config.dimension = 512;
config.num_subquantizers = 8;
config.num_centroids = 256;

ProductQuantizer pq(config);
auto train_result = pq.train(training_vectors);

if (train_result) {
    std::cout << "Training successful!\n";
    std::cout << "Compression ratio: " << pq.compression_ratio() << "x\n";
    
    // Save for later use
    pq.save("database_pq.bin");
}

// Encode all vectors
for (size_t i = 0; i < db.size(); ++i) {
    auto vec = db.get_vector(i);
    auto encoded = pq.encode(*vec);
    // Store encoded vector...
}
```

---

### Example 2: Quantized Search

```cpp
#include "vdb/quantization/product_quantizer.hpp"

// Load quantizer
auto pq_result = ProductQuantizer::load("database_pq.bin");
ProductQuantizer& pq = *pq_result;

// Load encoded database
std::vector<std::vector<uint8_t>> encoded_database = load_encoded_vectors();

// Query
Vector query = get_query_vector();

// Precompute distance table for efficiency
auto distance_table = pq.precompute_distance_table(query);

// Search
std::vector<std::pair<size_t, float>> results;
for (size_t i = 0; i < encoded_database.size(); ++i) {
    float dist = pq.compute_distance_precomputed(
        encoded_database[i],
        distance_table
    );
    results.push_back({i, dist});
}

// Sort by distance
std::sort(results.begin(), results.end(),
    [](const auto& a, const auto& b) { return a.second < b.second; });

// Top-k results
for (size_t i = 0; i < 10; ++i) {
    std::cout << "ID: " << results[i].first 
              << " Distance: " << results[i].second << "\n";
}
```

---

### Example 3: Scalar Quantization with HNSW

```cpp
#include "vdb/quantization/scalar_quantizer.hpp"
#include "vdb/index/hnsw.hpp"

using namespace vdb;
using namespace vdb::quantization;

// Train Scalar Quantizer
ScalarQuantizer sq;
std::vector<Vector> training = sample_vectors(5000);
sq.train(training);

// Create HNSW index with quantized vectors
HNSWIndex index(config);

for (const auto& vec : all_vectors) {
    // Encode vector
    auto encoded = sq.encode(vec);
    
    // Decode for indexing (HNSW still uses floats internally)
    auto decoded = sq.decode(*encoded);
    
    // Add to index
    index.add(*decoded, metadata);
    
    // Store encoded version separately for space savings
    save_encoded(*encoded, id);
}

// Memory saved
size_t original_size = all_vectors.size() * 512 * sizeof(float);
size_t compressed_size = all_vectors.size() * 512;  // 1 byte per dim
std::cout << "Memory saved: " 
          << (original_size - compressed_size) / 1024 / 1024 << " MB\n";
```

---

### Example 4: Comparison Test

```cpp
#include "vdb/quantization/product_quantizer.hpp"
#include "vdb/quantization/scalar_quantizer.hpp"

// Prepare data
std::vector<Vector> training = sample_vectors(10000);
std::vector<Vector> test_set = sample_vectors(1000);
Vector query = get_query();

// Test Product Quantization
ProductQuantizer pq(config);
pq.train(training);

auto pq_encoded = pq.encode_batch(test_set);
float pq_recall = measure_recall(query, *pq_encoded, pq);
std::cout << "PQ Compression: " << pq.compression_ratio() << "x\n";
std::cout << "PQ Recall: " << pq_recall * 100 << "%\n";

// Test Scalar Quantization
ScalarQuantizer sq(sq_config);
sq.train(training);

std::vector<std::vector<uint8_t>> sq_encoded;
for (const auto& vec : test_set) {
    sq_encoded.push_back(*sq.encode(vec));
}

float sq_recall = measure_recall(query, sq_encoded, sq);
std::cout << "SQ Compression: " << sq.compression_ratio() << "x\n";
std::cout << "SQ Recall: " << sq_recall * 100 << "%\n";
```

---

## Performance Comparison

### Memory Usage

**1 Million 512-dim float32 vectors**:

| Method | Memory | Compression | Savings |
|--------|--------|-------------|---------|
| Uncompressed | 2 GB | 1x | - |
| Scalar Quantization | 512 MB | 4x | 1.5 GB |
| Product Quantization (8×256) | 64 MB | 32x | 1.94 GB |

### Accuracy (Recall@10)

| Method | Recall | Accuracy Loss |
|--------|--------|---------------|
| Uncompressed | 100% | 0% |
| Scalar Quantization | 96-98% | 2-4% |
| Product Quantization | 88-93% | 7-12% |

### Speed

**Search Performance (1M vectors, k=10)**:

| Method | Query Time | Throughput |
|--------|-----------|------------|
| Uncompressed HNSW | 3 ms | 333 qps |
| Scalar Quantized | 2.5 ms | 400 qps |
| Product Quantized (with ADC) | 1.8 ms | 555 qps |

---

## Best Practices

### Training Data Selection

```cpp
// Sample uniformly across dataset
std::vector<Vector> training_data;
size_t sample_rate = total_vectors / 10000;  // Target 10k samples

for (size_t i = 0; i < total_vectors; i += sample_rate) {
    training_data.push_back(get_vector(i));
}

// Or use random sampling
std::vector<size_t> indices = random_sample(total_vectors, 10000);
for (size_t idx : indices) {
    training_data.push_back(get_vector(idx));
}
```

### Configuration Guidelines

**Product Quantization**:
- `num_subquantizers`: 8 for 512-dim (d/m = 64 per subvector)
- `num_centroids`: 256 (8-bit codes, good tradeoff)
- Training samples: 10,000+ for good centroids
- More centroids = better accuracy but larger code size

**Scalar Quantization**:
- `per_dimension = true`: Better accuracy (recommended)
- `per_dimension = false`: Slightly faster, less accurate
- Training samples: 5,000+ sufficient

### When to Retrain

Retrain quantizers when:
- Data distribution changes significantly
- Adding new types of vectors
- Recall drops below acceptable threshold
- After every 100,000+ new vectors

---

## When to Use Quantization

### Use Scalar Quantization When:

- ✅ Memory is constrained but not critical
- ✅ You need high accuracy (95%+ recall)
- ✅ Dataset is small-medium (< 1M vectors)
- ✅ Fast encoding/decoding is important
- ✅ Simplicity is valued

### Use Product Quantization When:

- ✅ Memory is very constrained
- ✅ Dataset is large (> 1M vectors)
- ✅ You can tolerate 5-10% accuracy loss
- ✅ Search speed is critical
- ✅ You have time for training

### Don't Use Quantization When:

- ❌ Memory is abundant
- ❌ Dataset is tiny (< 10k vectors)
- ❌ You need perfect recall
- ❌ Vector distribution is highly non-uniform

---

## Troubleshooting

### Poor Recall After Quantization

**Solutions**:
- Increase `num_centroids` (PQ) or use `per_dimension = true` (SQ)
- Retrain with more diverse training data
- Try Scalar Quantization instead of Product Quantization
- Adjust HNSW parameters (increase ef_search)

### Out of Memory During Training

**Solutions**:
- Reduce training set size (minimum 5,000)
- Reduce `num_centroids`
- Use Scalar Quantization (simpler)

### Slow Encoding

**Solutions**:
- Use batch encoding (`encode_batch()`)
- Increase `num_threads`
- Precompute and cache quantizers

---

## Perceptual Quantization (System Snapshot) {#perceptual-quantization}

**System Snapshot (v4.0.0)**: Perceptual quantization is detailed in `docs/research/HEKTOR_ANALYSIS.md` and is part of the v4.0.0 system snapshot, focusing on image/video perceptual preservation.

### Overview

While Product and Scalar Quantization optimize for mathematical compression, **Perceptual Quantization** optimizes for **perceived quality**. This is especially important for:
- HDR images and videos
- Chart and graph visualizations  
- Image embeddings from CLIP/visual models
- Content displayed on varied devices (SDR/HDR)

### PQ Curve (SMPTE ST 2084)

The Perceptual Quantizer curve maps linear luminance to perceptually uniform space:

```cpp
#include "vdb/quantization/perceptual_curves.hpp"

using namespace vdb::quantization;

// HDR luminance values (0.1 to 10,000 nits)
std::vector<float> hdr_values = {0.1, 100.0, 1000.0, 10000.0};

// Encode to perceptual space
auto pq_encoded = PQCurve::encode_batch(hdr_values);

// Now quantize in perceptual space (equal steps = equal perceived differences)
ScalarQuantizer quantizer;
quantizer.train(perceptual_training_data);
auto compressed = quantizer.encode(pq_encoded);
```

**Benefits**:
- ✅ Perceptually uniform: Equal quantization steps = equal perceived differences
- ✅ Supports 0.001 to 10,000 nits (wide dynamic range)
- ✅ Industry standard (Dolby Vision, HDR10)
- ✅ Reduces banding and posterization artifacts

### Display-Aware Quantization

Optimize encoding for target display characteristics:

```cpp
#include "vdb/quantization/adaptive_quantizer.hpp"

using namespace vdb::quantization;

// Create quantizer for HDR display
DisplayAwareQuantizer quantizer(DisplayProfile::HDR1000_Standard());

// Train on representative data
quantizer.train(training_vectors);

// Encode optimized for HDR
auto encoded = quantizer.encode(vector);

// Re-quantize for different display
auto sdr_encoded = quantizer.requantize_for_display(
    encoded, 
    DisplayProfile::SDR_Standard()
);
```

**Supported Display Types**:
- SDR (Rec.709, 100 nits, 8-bit)
- HDR10 (ST.2084, 1000 nits, 10-bit)
- HDR4000 (ST.2084, 4000 nits, 10-bit)
- Dolby Vision (ST.2084, 10000 nits, 12-bit)

### Environment-Aware Quantization

Adapt quantization to viewing conditions:

```cpp
// Adapt to ambient lighting
EnvironmentAwareQuantizer quantizer(
    DisplayProfile::HDR1000_Standard(),
    EnvironmentProfile::DarkRoom()  // Cinema-like
);

// Runtime adaptation
void on_ambient_light_changed(float lux) {
    if (lux > 500) {
        quantizer.adapt_to_environment(EnvironmentProfile::Office());
    } else {
        quantizer.adapt_to_environment(EnvironmentProfile::HomeTheater());
    }
}
```

### Structured Quantization

Learn vector-level patterns instead of per-dimension quantization:

```cpp
#include "vdb/quantization/structured_quantizer.hpp"

StructuredQuantizerConfig config;
config.codebook_size = 65536;  // 16-bit codes
config.use_hierarchical = true;

StructuredQuantizer quantizer(config);
quantizer.train(large_dataset);  // 100k+ vectors

// Better quality than standard PQ
// Expected: +5% recall improvement
```

### Performance Impact

| Method | Memory | Recall@10 | Encoding Time | Use Case |
|--------|--------|-----------|---------------|----------|
| Standard SQ | 512 MB | 96.5% | 8 ms | General vectors |
| SQ + PQ Curve | 512 MB | 97.8% | 12 ms | Image embeddings |
| Display-Aware | 512 MB | 98.1% | 13 ms | HDR content |
| Structured PQ | 64 MB | 93.2% | 15 ms | Large-scale datasets |

**Key Insight**: 1-3% quality improvement with minimal overhead (20-50% encoding time).

### When to Use Perceptual Quantization

**✅ Use When:**
- Working with image/video embeddings
- Content will be displayed (not just searched)
- Quality perception matters more than mathematical accuracy
- Supporting multiple display types (SDR/HDR)
- Content has high dynamic range

**❌ Don't Use When:**
- Pure mathematical embeddings (text, abstract features)
- Display is irrelevant (backend processing only)
- Need maximum speed (perceptual overhead ~50%)

### Learn More

For comprehensive research, implementation details, and examples:
- **[Perceptual Quantization Research](24_PERCEPTUAL_QUANTIZATION_RESEARCH.md)** - In-depth analysis
- **[Usage Examples](PERCEPTUAL_QUANTIZATION_EXAMPLES.md)** - Practical code examples
- **[SMPTE ST 2084 Standard](https://www.smpte.org/)** - Official PQ specification

---

## See Also

- [Mathematical Foundations](09_VECTOR_OPERATIONS.md#quantization-and-compression) - Theory behind quantization
- [API Reference](20_API_REFERENCE.md) - Complete API documentation
- [Architecture](05_ARCHITECTURE.md) - System architecture

---

**Last Updated**: 2026-01-20  
**Version**: 3.0.0  
**Status**: Production Ready ✅
