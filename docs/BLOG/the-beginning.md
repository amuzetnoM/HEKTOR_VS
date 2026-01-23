# The Beginning
> Why I Started Building Hektor

*November 15, 2025*

---

Every meaningful project starts with frustration. Mine began on a cold November night, staring at benchmark results that made no sense.

I'd been evaluating vector databases for a project. Pinecone. Milvus. Qdrant. FAISS. Each one excellent in its own way. Each one missing something I needed.

## The Problem

What I wanted was simple:

1. **Sub-millisecond search** at scale (millions of vectors)
2. **High recall** (99%+ wasn't negotiable)
3. **Hybrid search** — vectors + full-text, unified, _enterprised_.
4. **Production-ready** — not a research prototype

No single solution hit all four. FAISS was fast but didn't do hybrid search. Milvus had features but latency was unpredictable. Qdrant was elegant but I needed more control over quantization.

## The Decision

So I did what engineers do when tools don't exist: I decided to build one.

The name came easily. **Hektor** — the defender. In the Iliad, Hektor was the Trojan hero who stood against impossible odds. My Hektor would defend against the chaos of unstructured data, the flood of embeddings that modern AI systems generate.

## What I'm Building

The architecture crystallized over a few whiteboard sessions:

- **C++23 core** — no compromises on performance
- **HNSW index** — the best algorithm for approximate nearest neighbor
- **Perceptual quantization** — compress intelligently, not blindly
- **Hybrid search engine** — BM25 + vectors, fused properly
- **Python bindings** — because the world runs on Python

I'm not trying to replace everything. I'm building something that does fewer things, but does them exceptionally well.

## What's Next

This blog will document the journey. The decisions that worked, the mistakes I made, the algorithms I implemented wrong three times before getting them right.

Building in public is terrifying. But it's also the only way to build something that matters.

Let's see where this goes.

---

*— Hektor*