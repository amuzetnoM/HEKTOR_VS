# The Quantization Rabbit Hole

*December 18, 2025*

---

"I just need to compress the vectors a bit."

Famous last words.

What started as a simple optimization became a three-week journey through perceptual science, color theory, and the mathematics of human vision, also arg/sofmax but lets not go there. Here's what I learned.

## The Naive Approach

Standard vector quantization is straightforward:

1. Take your 512-dimensional float32 vectors (2KB each)
2. Quantize to int8 using scalar quantization (512 bytes each)
3. Save 75% memory

This works. Recall drops from 99.7% to ~95%. Acceptable for some use cases.

But I think we can do better.

## Enter Product Quantization

The key insight: divide your vector into subvectors, quantize each independently.

```
512-dim vector → 8 subvectors of 64 dims each
Each subvector → mapped to one of 256 centroids
Result: 8 bytes instead of 2048 bytes (256x compression!)
```

The magic is in training the centroids. You cluster your subvectors using k-means, and the centroids capture the structure of your data.

I implemented this. Recall: 97%. Memory: 8 bytes per vector.

But then someone asked: *"What if perception matters?"*

## The Perceptual Revelation

Here's a fact that broke my brain: 

> Human perception of _intensity_ is **logarithmic**, not linear. <br>
> Duhh.

The difference between 0 and 1 (luminance) looks the same as 10 and 11 to a meter. But to your eyes, 0→1 is a massive change while 10→11 is barely noticeable.

This is the Weber-Fechner law. And it means: **uniform quantization wastes bits**.

If you allocate equal precision across the range, you're over-representing changes humans can't perceive and under-representing changes they can.

## PQ Curves and HDR

The video industry solved this decades ago with Perceptual Quantizer (PQ) curves — SMPTE ST.2084.

The curve maps linear luminance to a perceptually uniform space.

```
PQ(Y) = ((c₁ + c₂·Y^m₁) / (1 + c₃·Y^m₁))^m₂
```

Where:


Instead of luminance, we apply perceptual curves to embedding magnitudes. Dimensions with high variance get more precision. Dimensions near zero get less.


Result: 98.5% recall at the same 8 bytes per vector.

## Display-Aware Encoding

This is getting wild.

Different displays have different capabilities:
- SDR monitors: 100 nits peak
- HDR10: 1000 nits
-HDR4000: 4000 nits

We built a `DisplayAwareQuantizer` that adapts encoding based on target display characteristics. For vector databases, "display" means "downstream model" — what precision do we really need?

A lightweight retrieval model doesn't need the same precision as a fine-grained reranker.

My final quantization system:

Our final quantization system:

```cpp
namespace vdb::quantization {
    class ProductQuantizer;       // Standard PQ
    class ScalarQuantizer;        // Simple 8-bit
    class DisplayAwareQuantizer;  // Perceptual curves
    class AdaptiveQuantizer;      // Learns from data
}
```

Each has its place:
- **ProductQuantizer**: Maximum compression, good recall
- **ScalarQuantizer**: Fast, simple, baseline
- **DisplayAwareQuantizer**: When perception matters
## What I Learnedzer**: When you have training data

## What We Learned

Compression isn't about throwing away data. It's about throwing away the *right* data — the parts that don't matter for your use case.

In vectors, that's... well, I'm still figuring that out.
In audio, that's inaudible frequencies.
But the journey was worth it. I'm shipping a quantization system that's 10-15% better than standard approaches, with zero additional latency.

But the journey was worth it. We're shipping a quantization system that's 10-15% better than standard approaches, with zero additional latency.

---

*Sometimes the rabbit hole is the destination.*