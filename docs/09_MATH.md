---
title: "Mathematical Foundations"
version: "2.0.0"
last_updated: "2026-01-06"
sidebar_position: 9
---

# SCIENCE

![Version](https://img.shields.io/badge/version-2.2.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--06-green?style=flat-square)
![Type](https://img.shields.io/badge/type-theory-purple?style=flat-square)

## Table of Contents

1. [Introduction](#introduction)
2. [Vector Spaces and Embeddings](#vector-spaces-and-embeddings)
3. [Distance and Similarity Metrics](#distance-and-similarity-metrics)
4. [Approximate Nearest Neighbor Search](#approximate-nearest-neighbor-search)
5. [HNSW Algorithm Deep Dive](#hnsw-algorithm-deep-dive)
6. [Embedding Models](#embedding-models)
7. [Dimensionality Reduction](#dimensionality-reduction)
8. [Quantization and Compression](#quantization-and-compression)
9. [Information Retrieval Theory](#information-retrieval-theory)
10. [Numerical Stability](#numerical-stability)

---

## Introduction

This document provides the mathematical foundations underlying Vector Studio's vector database and similarity search capabilities. Understanding these concepts enables better configuration, debugging, and extension of the system.

### Notation

| Symbol | Meaning |
|--------|---------|
| $\mathbf{x}, \mathbf{y}, \mathbf{q}$ | Vectors in $\mathbb{R}^d$ |
| $d$ | Vector dimensionality (default: 512) |
| $n$ | Number of vectors in database |
| $k$ | Number of nearest neighbors to return |
| $\|\mathbf{x}\|_2$ | Euclidean (L2) norm of $\mathbf{x}$ |
| $\langle \mathbf{x}, \mathbf{y} \rangle$ | Dot product of $\mathbf{x}$ and $\mathbf{y}$ |
| $\cos(\theta)$ | Cosine similarity |
| $\mathcal{O}(\cdot)$ | Big-O complexity notation |

---

## Vector Spaces and Embeddings

### Definition: Embedding

An **embedding** is a mapping from discrete objects (words, sentences, images) to continuous vector representations:

$$
\phi: \mathcal{X} \rightarrow \mathbb{R}^d
$$

where $\mathcal{X}$ is the input space (e.g., text, images) and $d$ is the embedding dimension.

### Properties of Good Embeddings

1. **Semantic Similarity**: Similar inputs map to nearby vectors
   $$
   \text{similar}(x_1, x_2) \implies \|\phi(x_1) - \phi(x_2)\|_2 \text{ is small}
   $$

2. **Compositionality**: Relationships are preserved
   $$
   \phi(\text{"king"}) - \phi(\text{"man"}) + \phi(\text{"woman"}) \approx \phi(\text{"queen"})
   $$

3. **Discrimination**: Different concepts are separated
   $$
   \text{different}(x_1, x_2) \implies \|\phi(x_1) - \phi(x_2)\|_2 \text{ is large}
   $$

### Embedding Space Geometry

The embedding space $\mathbb{R}^d$ has the following structure:

- **Origin**: Not semantically meaningful (arbitrary)
- **Direction**: Encodes semantic meaning
- **Magnitude**: Often normalized to unit sphere

After L2 normalization:
$$
\hat{\mathbf{x}} = \frac{\mathbf{x}}{\|\mathbf{x}\|_2}
$$

All vectors lie on the unit hypersphere $S^{d-1}$.

---

## Distance and Similarity Metrics

### Euclidean Distance (L2)

The most intuitive distance measure:

$$
d_{\text{euclidean}}(\mathbf{x}, \mathbf{y}) = \|\mathbf{x} - \mathbf{y}\|_2 = \sqrt{\sum_{i=1}^{d} (x_i - y_i)^2}
$$

**Properties**:
- Always non-negative: $d(\mathbf{x}, \mathbf{y}) \geq 0$
- Symmetric: $d(\mathbf{x}, \mathbf{y}) = d(\mathbf{y}, \mathbf{x})$
- Triangle inequality: $d(\mathbf{x}, \mathbf{z}) \leq d(\mathbf{x}, \mathbf{y}) + d(\mathbf{y}, \mathbf{z})$

**Squared Euclidean** (faster, avoids sqrt):
$$
d_{\text{sq}}(\mathbf{x}, \mathbf{y}) = \sum_{i=1}^{d} (x_i - y_i)^2
$$

### Cosine Similarity

Measures the angle between vectors, ignoring magnitude:

$$
\text{cos\_sim}(\mathbf{x}, \mathbf{y}) = \frac{\langle \mathbf{x}, \mathbf{y} \rangle}{\|\mathbf{x}\|_2 \cdot \|\mathbf{y}\|_2} = \frac{\sum_{i=1}^{d} x_i y_i}{\sqrt{\sum_{i=1}^{d} x_i^2} \cdot \sqrt{\sum_{i=1}^{d} y_i^2}}
$$

**Range**: $[-1, 1]$ where:
- $1$: Identical direction (most similar)
- $0$: Orthogonal (unrelated)
- $-1$: Opposite direction (most dissimilar)

**Cosine Distance** (for use in nearest neighbor search):
$$
d_{\text{cosine}}(\mathbf{x}, \mathbf{y}) = 1 - \text{cos\_sim}(\mathbf{x}, \mathbf{y})
$$

### Dot Product (Inner Product)

For L2-normalized vectors, equivalent to cosine similarity:

$$
\langle \mathbf{x}, \mathbf{y} \rangle = \sum_{i=1}^{d} x_i y_i
$$

When $\|\mathbf{x}\|_2 = \|\mathbf{y}\|_2 = 1$:
$$
\langle \hat{\mathbf{x}}, \hat{\mathbf{y}} \rangle = \cos(\theta)
$$

**Negative dot product** for distance:
$$
d_{\text{dot}}(\mathbf{x}, \mathbf{y}) = -\langle \mathbf{x}, \mathbf{y} \rangle
$$

### Relationship Between Metrics

For L2-normalized vectors:

$$
\|\hat{\mathbf{x}} - \hat{\mathbf{y}}\|_2^2 = 2(1 - \langle \hat{\mathbf{x}}, \hat{\mathbf{y}} \rangle) = 2 \cdot d_{\text{cosine}}(\hat{\mathbf{x}}, \hat{\mathbf{y}})
$$

This means that for normalized vectors, minimizing Euclidean distance is equivalent to maximizing cosine similarity.

### Manhattan Distance (L1)

$$
d_{\text{manhattan}}(\mathbf{x}, \mathbf{y}) = \|\mathbf{x} - \mathbf{y}\|_1 = \sum_{i=1}^{d} |x_i - y_i|
$$

Less commonly used for embeddings but more robust to outliers.

---

## Approximate Nearest Neighbor Search

### The Curse of Dimensionality

In high dimensions, exact nearest neighbor search becomes intractable:

1. **Distance Concentration**: All points become approximately equidistant
   $$
   \lim_{d \to \infty} \frac{d_{\max} - d_{\min}}{d_{\min}} \to 0
   $$

2. **Volume Growth**: The volume of a hypersphere grows as $r^d$

3. **Computational Cost**: Exact search requires $\mathcal{O}(n \cdot d)$ per query

### Approximate vs. Exact

Trade-off between:
- **Recall**: Fraction of true nearest neighbors found
- **Speed**: Query time
- **Space**: Index memory usage

$$
\text{Recall@}k = \frac{|\text{ANN}(q, k) \cap \text{NN}(q, k)|}{k}
$$

where $\text{ANN}$ is approximate and $\text{NN}$ is exact.

### Index Structures

| Method | Build Time | Query Time | Space | Recall |
|--------|------------|------------|-------|--------|
| Brute Force | $\mathcal{O}(1)$ | $\mathcal{O}(nd)$ | $\mathcal{O}(nd)$ | 100% |
| KD-Tree | $\mathcal{O}(n \log n)$ | $\mathcal{O}(\log n)$* | $\mathcal{O}(nd)$ | 100%* |
| LSH | $\mathcal{O}(n)$ | $\mathcal{O}(n^{\rho})$ | $\mathcal{O}(n)$ | ~90% |
| IVF | $\mathcal{O}(n)$ | $\mathcal{O}(\sqrt{n})$ | $\mathcal{O}(n)$ | ~95% |
| **HNSW** | $\mathcal{O}(n \log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(n)$ | **~99%** |

*KD-Tree degrades to $\mathcal{O}(n)$ in high dimensions

---

## HNSW Algorithm Deep Dive

### Navigable Small World Graphs

A **Navigable Small World (NSW)** graph has the property that greedy routing finds near-optimal paths:

$$
d(v_i, v_{i+1}) < d(v_i, q) \implies \text{progress toward } q
$$

where $q$ is the query point and $v_i$ are nodes on the search path.

### Hierarchical Structure

HNSW extends NSW with multiple layers:

$$
P(\text{node at layer } l) = \exp(-l \cdot m_L)
$$

where $m_L = 1 / \ln(M)$ and $M$ is the max connections parameter.

**Layer Distribution**:
- Layer 0: All $n$ nodes
- Layer 1: $\approx n / M$ nodes
- Layer 2: $\approx n / M^2$ nodes
- Layer $l$: $\approx n / M^l$ nodes

Expected max layer:
$$
L_{\max} = \lfloor \log_M(n) \rfloor
$$

### Graph Construction

**Algorithm: INSERT(q)**

```
1. l ← ⌊-ln(uniform(0,1)) × mL⌋  # Random layer assignment
2. ep ← entry_point              # Start from top
3. for lc = L down to l+1:       # Navigate to layer l
      W ← SEARCH-LAYER(q, ep, ef=1, lc)
      ep ← nearest in W
4. for lc = l down to 0:         # Insert with connections
      W ← SEARCH-LAYER(q, ep, efConstruction, lc)
      neighbors ← SELECT-NEIGHBORS(q, W, M)
      Add bidirectional edges q ↔ neighbors
      for each neighbor:
          Prune if degree > Mmax
      ep ← nearest in W
5. if l > L: entry_point ← q
```

### Neighbor Selection

**SELECT-NEIGHBORS-SIMPLE** (greedy):
Return the $M$ nearest neighbors.

**SELECT-NEIGHBORS-HEURISTIC** (improved):
Consider diversity – avoid neighbors that are too close to each other:

```
R ← ∅
W_copy ← copy of W
while |W_copy| > 0 and |R| < M:
    e ← extract nearest from W_copy
    if e is closer to q than to all in R:
        R ← R ∪ {e}
return R
```

### Search Algorithm

**Algorithm: K-NN-SEARCH(q, k, ef)**

```
1. ep ← entry_point
2. L ← max layer
3. for lc = L down to 1:           # Coarse search
      W ← SEARCH-LAYER(q, ep, ef=1, lc)
      ep ← nearest in W
4. W ← SEARCH-LAYER(q, ep, ef, lc=0)  # Fine search
5. return k nearest from W
```

**SEARCH-LAYER(q, ep, ef, layer)**:

```
visited ← {ep}
candidates ← MinHeap(ep)      # Closest first
results ← MaxHeap(ep)         # Farthest first

while |candidates| > 0:
    c ← candidates.pop_min()
    f ← results.top()
    if d(c, q) > d(f, q): break    # No improvement possible
    
    for e in neighbors(c, layer):
        if e ∉ visited:
            visited ← visited ∪ {e}
            f ← results.top()
            if d(e, q) < d(f, q) or |results| < ef:
                candidates.push(e)
                results.push(e)
                if |results| > ef:
                    results.pop_max()

return results
```

### Complexity Analysis

**Construction**:
- Per insertion: $\mathcal{O}(M \cdot \log n)$ expected
- Total: $\mathcal{O}(n \cdot M \cdot \log n)$

**Query**:
- Expected: $\mathcal{O}(\log n)$ distance computations
- Each distance: $\mathcal{O}(d)$
- Total: $\mathcal{O}(d \cdot \log n)$

**Space**:
- Vectors: $\mathcal{O}(n \cdot d)$
- Graph: $\mathcal{O}(n \cdot M)$ edges per layer
- Total: $\mathcal{O}(n \cdot (d + M \cdot L))$

### Parameter Tuning

| Parameter | Effect of Increase |
|-----------|-------------------|
| $M$ | Better recall, more memory, slower build |
| $ef_{\text{construction}}$ | Better graph quality, slower build |
| $ef_{\text{search}}$ | Better recall, slower query |

**Recommended starting values**:
$$
M = 16, \quad ef_{\text{construction}} = 200, \quad ef_{\text{search}} = 50
$$

**Recall vs. ef_search** (empirical):
$$
\text{Recall} \approx 1 - \exp(-\alpha \cdot ef_{\text{search}})
$$

where $\alpha$ depends on data distribution.

---

## Embedding Models

### Transformer Architecture

Modern embedding models use the Transformer architecture:

$$
\text{Attention}(Q, K, V) = \text{softmax}\left(\frac{QK^T}{\sqrt{d_k}}\right) V
$$

where:
- $Q = XW_Q$: Query projections
- $K = XW_K$: Key projections
- $V = XW_V$: Value projections
- $d_k$: Key dimension (for scaling)

### Multi-Head Attention

$$
\text{MultiHead}(Q, K, V) = \text{Concat}(\text{head}_1, ..., \text{head}_h) W_O
$$

where each head:
$$
\text{head}_i = \text{Attention}(QW_i^Q, KW_i^K, VW_i^V)
$$

### Sentence Embeddings (Mean Pooling)

Given token embeddings $\mathbf{h}_1, ..., \mathbf{h}_n$ and attention mask $m_1, ..., m_n$:

$$
\mathbf{e} = \frac{\sum_{i=1}^{n} m_i \cdot \mathbf{h}_i}{\sum_{i=1}^{n} m_i}
$$

This averages token embeddings, excluding padding tokens.

### MiniLM Architecture

**all-MiniLM-L6-v2** (used in Vector Studio):

| Component | Value |
|-----------|-------|
| Layers | 6 |
| Hidden size | 384 |
| Attention heads | 12 |
| Max tokens | 256 |
| Output dimension | 384 |

**Knowledge Distillation**:
MiniLM is trained to mimic a larger teacher model:

$$
\mathcal{L} = \mathcal{L}_{\text{task}} + \lambda \cdot D_{KL}(p_{\text{student}} \| p_{\text{teacher}})
$$

### CLIP Vision Encoder

**CLIP ViT-B/32** (used for images):

| Component | Value |
|-----------|-------|
| Architecture | Vision Transformer |
| Patch size | 32×32 |
| Input size | 224×224 |
| Patches | 7×7 = 49 |
| Hidden size | 768 |
| Output dimension | 512 |

**Contrastive Learning**:
CLIP is trained on (image, text) pairs:

$$
\mathcal{L} = -\frac{1}{N} \sum_{i=1}^{N} \left[ \log \frac{\exp(\mathbf{v}_i^T \mathbf{t}_i / \tau)}{\sum_{j=1}^{N} \exp(\mathbf{v}_i^T \mathbf{t}_j / \tau)} \right]
$$

where $\tau$ is temperature parameter.

### Projection Layer

To align text (384-dim) and image (512-dim) embeddings:

$$
\mathbf{e}_{\text{projected}} = W \mathbf{e}_{\text{text}} + \mathbf{b}
$$

where $W \in \mathbb{R}^{512 \times 384}$ and $\mathbf{b} \in \mathbb{R}^{512}$.

---

## Dimensionality Reduction

### Principal Component Analysis (PCA)

Find orthogonal directions of maximum variance:

$$
\mathbf{x}_{\text{reduced}} = W_{\text{PCA}}^T (\mathbf{x} - \boldsymbol{\mu})
$$

where $W_{\text{PCA}} \in \mathbb{R}^{d \times k}$ contains top-$k$ eigenvectors of covariance matrix.

**Variance Preserved**:
$$
\frac{\sum_{i=1}^{k} \lambda_i}{\sum_{i=1}^{d} \lambda_i}
$$

### Random Projection (Johnson-Lindenstrauss)

**Lemma**: For $\epsilon \in (0, 1)$, any $n$ points in $\mathbb{R}^d$ can be projected to $k = \mathcal{O}(\epsilon^{-2} \log n)$ dimensions while preserving pairwise distances within factor $(1 \pm \epsilon)$.

$$
\mathbf{x}_{\text{projected}} = \frac{1}{\sqrt{k}} R \mathbf{x}
$$

where $R_{ij} \sim \mathcal{N}(0, 1)$.

### UMAP for Visualization

UMAP preserves local structure for 2D/3D visualization:

$$
\mathcal{L}_{\text{UMAP}} = \sum_{i,j} w_{ij} \log\left(\frac{w_{ij}}{p_{ij}}\right) + (1 - w_{ij}) \log\left(\frac{1 - w_{ij}}{1 - p_{ij}}\right)
$$

where $w_{ij}$ are high-dim weights and $p_{ij}$ are low-dim probabilities.

---

## Quantization and Compression

### Scalar Quantization

Map float32 to int8:

$$
q(x) = \text{round}\left(\frac{x - x_{\min}}{x_{\max} - x_{\min}} \times 255\right)
$$

**Dequantization**:
$$
\hat{x} = \frac{q}{255} \times (x_{\max} - x_{\min}) + x_{\min}
$$

**Compression**: 4× (32 bits → 8 bits)

### Product Quantization (PQ)

Split vector into $m$ subvectors, quantize each:

$$
\mathbf{x} = [\mathbf{x}^1, \mathbf{x}^2, ..., \mathbf{x}^m]
$$

Each subvector $\mathbf{x}^i \in \mathbb{R}^{d/m}$ is quantized to one of $k$ centroids:

$$
q^i(\mathbf{x}^i) = \arg\min_{j \in \{1,...,k\}} \|\mathbf{x}^i - \mathbf{c}_j^i\|_2
$$

**Storage**: $m \times \lceil \log_2 k \rceil$ bits per vector

**Distance Approximation** (Asymmetric):
$$
\hat{d}(\mathbf{x}, \mathbf{y}) = \sqrt{\sum_{i=1}^{m} \|{\mathbf{x}}^i - \mathbf{c}_{q^i(\mathbf{y})}^i\|_2^2}
$$

Precompute distances: $\mathcal{O}(m \cdot k)$ per query.

### Binary Quantization

Extreme compression - 1 bit per dimension:

$$
b_i = \begin{cases} 1 & \text{if } x_i > 0 \\ 0 & \text{otherwise} \end{cases}
$$

**Hamming Distance**:
$$
d_H(\mathbf{b}_1, \mathbf{b}_2) = \text{popcount}(\mathbf{b}_1 \oplus \mathbf{b}_2)
$$

**Compression**: 32× (float32 → 1 bit)

---

## Information Retrieval Theory

### Relevance and Similarity

In semantic search, we assume:

$$
P(\text{relevant} | q, d) \propto \text{similarity}(\phi(q), \phi(d))
$$

where $\phi$ is the embedding function.

### Evaluation Metrics

**Precision@k**:
$$
P@k = \frac{|\text{relevant} \cap \text{retrieved}_k|}{k}
$$

**Recall@k**:
$$
R@k = \frac{|\text{relevant} \cap \text{retrieved}_k|}{|\text{relevant}|}
$$

**Mean Average Precision (MAP)**:
$$
\text{MAP} = \frac{1}{|Q|} \sum_{q \in Q} \frac{1}{|\text{relevant}_q|} \sum_{k=1}^{n} P@k \cdot \text{rel}(k)
$$

**Normalized Discounted Cumulative Gain (nDCG)**:
$$
\text{DCG}_k = \sum_{i=1}^{k} \frac{2^{\text{rel}_i} - 1}{\log_2(i + 1)}
$$

$$
\text{nDCG}_k = \frac{\text{DCG}_k}{\text{IDCG}_k}
$$

### Bias-Variance Tradeoff

**High ef_search** (low bias, high variance):
- More exploration
- Higher recall
- Slower

**Low ef_search** (high bias, low variance):
- Less exploration
- Faster
- May miss relevant results

Optimal $ef_{\text{search}}$ balances:
$$
\text{Error} = \text{Bias}^2 + \text{Variance}
$$

---

## Numerical Stability

### Floating Point Considerations

**Float32 precision**: ~7 decimal digits

For normalized vectors:
- Expected dot product: $[-1, 1]$
- Accumulated error: $\mathcal{O}(\sqrt{d} \cdot \epsilon_{\text{machine}})$

### Kahan Summation

Reduces accumulated error in dot product:

```python
def kahan_dot(a, b):
    sum = 0.0
    c = 0.0  # Compensation
    for i in range(len(a)):
        y = a[i] * b[i] - c
        t = sum + y
        c = (t - sum) - y
        sum = t
    return sum
```

### Normalization Stability

Avoid division by zero:
$$
\hat{\mathbf{x}} = \frac{\mathbf{x}}{\max(\|\mathbf{x}\|_2, \epsilon)}
$$

where $\epsilon = 10^{-8}$.

### SIMD Accuracy

Fused multiply-add (FMA) is more accurate:
$$
\text{FMA}(a, b, c) = a \times b + c \quad \text{(single rounding)}
$$

vs. separate operations:
$$
(a \times b) + c \quad \text{(two roundings)}
$$

---

## Appendix: Derivations

### A.1: Cosine Distance and Euclidean Distance Relationship

For unit vectors $\hat{\mathbf{x}}$ and $\hat{\mathbf{y}}$:

$$
\begin{align}
\|\hat{\mathbf{x}} - \hat{\mathbf{y}}\|_2^2 &= (\hat{\mathbf{x}} - \hat{\mathbf{y}})^T (\hat{\mathbf{x}} - \hat{\mathbf{y}}) \\
&= \hat{\mathbf{x}}^T \hat{\mathbf{x}} - 2\hat{\mathbf{x}}^T \hat{\mathbf{y}} + \hat{\mathbf{y}}^T \hat{\mathbf{y}} \\
&= 1 - 2\langle \hat{\mathbf{x}}, \hat{\mathbf{y}} \rangle + 1 \\
&= 2(1 - \cos\theta) \\
&= 2 \cdot d_{\text{cosine}}(\hat{\mathbf{x}}, \hat{\mathbf{y}})
\end{align}
$$

### A.2: Expected Number of HNSW Layers

Let $p = \exp(-m_L)$ be the probability of being promoted to next layer.

$$
E[L] = \sum_{l=0}^{\infty} P(L > l) = \sum_{l=0}^{\infty} p^l = \frac{1}{1-p} = \frac{1}{1 - e^{-m_L}}
$$

For $m_L = 1/\ln(M)$:
$$
E[L] = \frac{1}{1 - 1/M} = \frac{M}{M-1}
$$

### A.3: Johnson-Lindenstrauss Lemma Proof Sketch

For random projection $R \in \mathbb{R}^{k \times d}$ with $R_{ij} \sim \mathcal{N}(0, 1/k)$:

$$
E[\|R\mathbf{x}\|_2^2] = \|\mathbf{x}\|_2^2
$$

By Chernoff bounds, with probability $1 - \delta$:
$$
(1 - \epsilon)\|\mathbf{x}\|_2^2 \leq \|R\mathbf{x}\|_2^2 \leq (1 + \epsilon)\|\mathbf{x}\|_2^2
$$

when $k \geq \frac{4 + 2\epsilon}{\epsilon^2/2 - \epsilon^3/3} \ln(n/\delta)$.

---

## References

1. Malkov, Y. A., & Yashunin, D. A. (2018). *Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs*. IEEE TPAMI.

2. Johnson, J., Douze, M., & Jégou, H. (2019). *Billion-scale similarity search with GPUs*. IEEE TBD.

3. Reimers, N., & Gurevych, I. (2019). *Sentence-BERT: Sentence Embeddings using Siamese BERT-Networks*. EMNLP.

4. Radford, A., et al. (2021). *Learning Transferable Visual Models From Natural Language Supervision*. ICML.

5. Wang, W., et al. (2020). *MiniLM: Deep Self-Attention Distillation for Task-Agnostic Compression of Pre-Trained Transformers*. NeurIPS.
