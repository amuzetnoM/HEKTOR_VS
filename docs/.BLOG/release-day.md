# Release Day: Hektor 4.0 is Live

*January 22, 2026*

---

I shipped.

After four months of architecture, implementation, debugging, and more debugging — Hektor 4.0 is live.

## What's in the Box

### Core Engine (C++23)
- **HNSW Index**: Sub-millisecond search at 99%+ recall
- **SIMD Optimized**: AVX-512 distance functions (8x speedup)
- **Memory-Mapped Storage**: Handles datasets larger than RAM

### Hybrid Search
- **BM25 Engine**: Full-text search with proper linguistics
- **RRF Fusion**: Rank-based score combination
- **Query Rewriting**: Synonym expansion, spell correction

### Quantization System
- **Product Quantizer**: 8-32x compression
- **Perceptual Curves**: PQ, HLG, adaptive gamma
- **Display-Aware Encoding**: Downstream-optimized precision

### Integrations
- **Python Bindings**: First-class pyvdb module
- **REST API**: FastAPI server with JWT auth
- **OpenTelemetry**: Distributed tracing built-in

### Extras
- **LLM Engine**: Local inference via llama.cpp
- **RAG Toolkit**: Chunking, context building, reranking
- **Multi-format Ingest**: CSV, JSON, PDF, Excel, Parquet, and more

## The Numbers

Benchmarked on SIFT-1M (1 million 128-dim vectors):

| Metric | Hektor 4.0 |
|--------|------------|
| Recall@10 | 99.2% |
| Query Latency (p99) | 2.8ms |
| Build Time | 45 seconds |
| Memory | 1.2 GB |

Hektor is competitive with the best open-source options and beats several commercial offerings.

## What I Learned

**The hard parts weren't where I expected.**

Memory management? Straightforward.
SIMD optimization? Tedious but tractable.
HNSW implementation? Well-documented.

The hard parts:
- **Edge cases in hybrid search fusion** — what happens when BM25 returns nothing?
- **Unicode handling in tokenization** — three weeks of pain
- **Cross-platform builds** — CMake is powerful and frustrating

**Tests save you.** I have 200+ test cases. Every one caught a real bug.

**Docs are a feature.** I spent 20% of my time on documentation. It shows.

## What's Next

This is 4.0, not 1.0 that I'm calling 4.0 for marketing. It's genuinely the fourth major architecture.

For 4.1:
- GPU acceleration for embedding generation
- Distributed mode (sharding + replication)
- Improved adaptive quantization

For 5.0 (someday):
- Neural index structures
- End-to-end differentiable search
- Things I haven't imagined yet

## Thank You

To everyone who gave feedback, reported bugs, asked hard questions — thank you. Building in public is only possible because people engage.

The code is MIT licensed. The research is open. Use it, break it, improve it.

---

*This is just the beginning.*

```
pip install pyvdb
```

*Let's build something great.*

— *Hektor*