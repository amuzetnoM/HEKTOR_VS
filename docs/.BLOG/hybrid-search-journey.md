# Hybrid Search: Where Vectors Meet Words

*January 5, 2026*

---

A user searched for "Apple M3 chip performance benchmarks" and got results about fruit orchards.

This is the semantic search failure mode. The embedding model understood "Apple" as a general concept but missed that in this context, it's a company name. It understood "chip" but associated it with potato chips in some training examples.

Pure semantic search is amazing...
>  Until it isn't.

## The Keyword Problem

Meanwhile, traditional search has the opposite problem. Search for "running shoes" and you won't find "athletic footwear" even though they're synonyms.

Keywords are precise but brittle.
Semantics are flexible but imprecise.

The answer, obviously, is both.

## Building BM25 From Scratch

BM25 (Best Match 25) is the algorithm behind Elasticsearch, Lucene, and most production search systems. The formula looks scary:

```
score(D, Q) = Σ IDF(qi) · (f(qi, D) · (k₁ + 1)) / (f(qi, D) + k₁ · (1 - b + b · |D|/avgdl))
```

But it's really just three intuitions:
1. **IDF**: Rare terms matter more ("the" appears everywhere, ignore it)
2. **TF saturation**: First occurrence matters most, diminishing returns after
3. **Length normalization**: Don't bias toward longer documents

We implemented BM25 in C++ with:
- Porter stemming
- Stop word removal  
- Inverted index with posting lists

Search over 1M documents: 8ms. Fast enough.

## The Fusion Problem

Now you have two lists of results:
1. Vector search: results ranked by cosine similarity
2. BM25 search: results ranked by relevance score

How do you combine them?

### Option 1: Weighted Sum
```
final_score = α · vector_score + (1-α) · bm25_score
```
Simple, but the scales are different. BM25 scores might be 0-20, while cosine similarity is 0-1.

### Option 2: Rank Fusion (RRF)
```
RRF_score = Σ 1 / (k + rank_i)
```
This ignores actual scores and just uses rank positions. Surprisingly effective.

### Option 3: Learned Fusion
Train a model to combine scores. More complex, potentially better, requires training data.

Well, I implemented all three. **RRF won for simplicity**. With k=60, it handles most cases well. For power users, we expose the weighted sum with configurable α.

## When Hybrid Shines

Hybrid search crushes pure approaches when queries contain:

- **Named entities**: "NVIDIA 4090 vs AMD 7900 XTX"
- **Product codes**: "iPhone 15 Pro Max A17"
- **Technical terms**: "HNSW ef_construction parameter"
- **Mixed intent**: "best practices kubernetes secrets" (concept + keyword)

For pure conceptual queries ("what is the meaning of life"), vector search alone is fine. For pure keyword queries ("error code 0x8007007E"), BM25 alone is fine.

Hybrid handles everything in between.

## The API

```
results = db.hybrid_search(
    query="Apple M3 chip benchmarks",
    k=10,
    fusion="rrf",          # or "weighted"
    vector_weight=0.7,     # for weighted fusion
    lexical_weight=0.3
)
```

Clean and simple. The complexity is hidden where it should be.

## Lessons Learned

1. **Don't choose between paradigms** — combine them
2. **RRF is underrated** — rank-based fusion is robust and simple
3. **Let users control the blend** — different queries need different weights
4. **Keyword matching isn't dead** — it's a feature, not a bug

---

*Two weeks until v4.0 release. The finish line is in sight.*