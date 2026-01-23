# Why HNSW Won: Choosing Our Index Algorithm

*December 3, 2025*

---

Choosing an index algorithm is like choosing a foundation for a house. Get it wrong, and everything built on top will be compromised.

We spent three weeks evaluating options. Here's what we learned.

## The Contenders

### KD-Trees
The classic. Beautiful in low dimensions (≤20). Falls apart spectacularly above that. We're dealing with 384-512 dimensional vectors. Next.

### LSH (Locality-Sensitive Hashing)
Probabilistic guarantees sound good in papers. In practice, you need multiple hash tables to get decent recall, which eats memory. The parameter tuning is also dark magic — `k` hash functions, `L` tables, and pray.

### IVF (Inverted File Index)
FAISS made this popular. You cluster your data, then search relevant clusters. Fast builds. Good for batch workloads. But for real-time search with frequent updates? The cluster assignments become stale. Rebalancing is expensive.

### HNSW
A graph where every node connects to its approximate nearest neighbors, organized in hierarchical layers. Like a skip list, but for metric spaces.

## Why HNSW Won

The numbers spoke for themselves:

| Algorithm | Recall@10 | QPS (1M vectors) | Memory Overhead |
|-----------|-----------|------------------|-----------------|
| IVF-PQ | 0.89 | 12,000 | Low |
| LSH | 0.85 | 8,000 | High |
| HNSW | 0.98 | 5,500 | Medium |

HNSW had the highest recall. Not the fastest raw QPS, but fast enough — and when you factor in that IVF needs reranking to match HNSW's recall, the gap closes.

But the real winner was **operational simplicity**.

## The Operational Argument

HNSW has two main parameters:
- `M`: connections per node (we use 16)
- `ef_search`: search queue size (we use 200)

That's it. No cluster count to tune. No hash function parameters. No periodic rebalancing.

When a new vector comes in, we insert it. Takes ~2ms. The graph stays balanced automatically because of how the layers are structured.

For a production system, this simplicity is gold.

## Our Implementation

We wrote HNSW from scratch in C++23. Using hnswlib was tempting, but we needed:

1. **SIMD distance functions** — AVX-512 makes a 4x difference
2. **Custom memory layout** — cache-friendly node storage
3. **Concurrent reads during writes** — no global locks

The implementation took six weeks. Worth every hour.

## What We'd Do Differently

If we were building for 10B+ vectors, we'd probably use a hybrid approach — IVF for coarse partitioning, HNSW within partitions. But for our target scale (millions to low billions), pure HNSW is the right call.

---

*Next week: The perceptual quantization rabbit hole. It goes deep.*