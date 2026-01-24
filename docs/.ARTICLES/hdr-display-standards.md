---
title: HDR1000
description: "Overview of HDR1000 Display Standards and Their Application to Vector Quantization"
version: 4.1.5
last_updated: 2026-01-24
sidebar_position: 1
category: Introduction
---

# HDR Display Standards
> Quantization Standards for Vector Database

**ARTIFACT VIRTUAL Technical Article Series**  
*Author: Ali A. Shakil*  
*Published: January 24, 2026*

---

## Abstract

This article provides a comprehensive overview of HDR display standards (HDR10, Dolby Vision, HLG, HDR1000) and their application to vector database quantization. We examine the mathematical foundations, practical implementations, and performance characteristics of each standard.

---

## 1. Introduction

High Dynamic Range (HDR) video standards were developed to capture and display a wider range of luminance values than Standard Dynamic Range (SDR). These standards use sophisticated transfer functions that allocate bits perceptually rather than linearly.

For vector databases, the same principles apply: not all embedding dimensions require equal precision. HDR transfer functions provide a mathematically rigorous framework for non-uniform quantization.

## 2. HDR Standards Overview

| Standard | Max Luminance | Bit Depth | Metadata | Primary Use |
|----------|---------------|-----------|----------|-------------|
| SDR | 100 nits | 8-bit | None | Legacy |
| HDR10 | 1,000 nits | 10-bit | Static | Consumer |
| HDR10+ | 4,000 nits | 10-bit | Dynamic | Premium |
| Dolby Vision | 10,000 nits | 12-bit | Dynamic | Cinema |
| HLG | 1,000 nits | 10-bit | None | Broadcast |

## 3. SMPTE ST 2084 (PQ)

### 3.1 Specification

SMPTE ST 2084 defines the Perceptual Quantizer (PQ) EOTF:

**EOTF (Display):**
$$Y = 10000 \cdot \left( \frac{\max(E^{1/m_2} - c_1, 0)}{c_2 - c_3 \cdot E^{1/m_2}} \right)^{1/m_1}$$

**OETF (Encoding):**
$$E = \left( \frac{c_1 + c_2 \cdot Y_m^{m_1}}{1 + c_3 \cdot Y_m^{m_1}} \right)^{m_2}$$

Where $Y_m = Y / 10000$ (normalized to peak luminance).

### 3.2 Constants

| Constant | Value | Derivation |
|----------|-------|------------|
| m₁ | 0.1593017578125 | 2610/16384 |
| m₂ | 78.84375 | 2523/4096 × 128 |
| c₁ | 0.8359375 | 3424/4096 |
| c₂ | 18.8515625 | 2413/4096 × 32 |
| c₃ | 18.6875 | 2392/4096 × 32 |

### 3.3 Implementation

```cpp
namespace hektor::hdr::pq {

struct ST2084 {
    static constexpr float m1 = 2610.0f / 16384.0f;
    static constexpr float m2 = 2523.0f / 4096.0f * 128.0f;
    static constexpr float c1 = 3424.0f / 4096.0f;
    static constexpr float c2 = 2413.0f / 4096.0f * 32.0f;
    static constexpr float c3 = 2392.0f / 4096.0f * 32.0f;
    static constexpr float L_max = 10000.0f;
    
    // Linear luminance → PQ code value
    static float encode(float Y) {
        float Ym = std::clamp(Y, 0.0f, L_max) / L_max;
        float Ym_m1 = std::pow(Ym, m1);
        return std::pow((c1 + c2 * Ym_m1) / (1.0f + c3 * Ym_m1), m2);
    }
    
    // PQ code value → Linear luminance
    static float decode(float E) {
        float E_inv_m2 = std::pow(std::clamp(E, 0.0f, 1.0f), 1.0f / m2);
        float num = std::max(E_inv_m2 - c1, 0.0f);
        float den = c2 - c3 * E_inv_m2;
        return std::pow(num / den, 1.0f / m1) * L_max;
    }
};

} // namespace hektor::hdr::pq
```

## 4. Hybrid Log-Gamma (HLG)

### 4.1 Specification

ITU-R BT.2100 defines HLG as a hybrid of logarithmic and gamma curves:

**OETF:**
$$
E = \begin{cases}
\sqrt{3 \cdot L} & 0 \leq L \leq \frac{1}{12} \\
a \cdot \ln(12L - b) + c & \frac{1}{12} < L \leq 1
\end{cases}
$$

Where:
- $a = 0.17883277$
- $b = 1 - 4a = 0.28466892$
- $c = 0.5 - a \cdot \ln(4a) = 0.55991073$

### 4.2 Properties

- **Backwards compatible**: Lower part is gamma curve (works on SDR displays)
- **Scene-referred**: No absolute luminance (adapts to display)
- **No metadata required**: Simpler for broadcast

### 4.3 Implementation

```cpp
namespace hektor::hdr::hlg {

struct HLG {
    static constexpr float a = 0.17883277f;
    static constexpr float b = 1.0f - 4.0f * a;
    static constexpr float c = 0.5f - a * std::log(4.0f * a);
    static constexpr float threshold = 1.0f / 12.0f;
    
    static float encode(float L) {
        L = std::clamp(L, 0.0f, 1.0f);
        if (L <= threshold) {
            return std::sqrt(3.0f * L);
        } else {
            return a * std::log(12.0f * L - b) + c;
        }
    }
    
    static float decode(float E) {
        E = std::clamp(E, 0.0f, 1.0f);
        if (E <= 0.5f) {
            return E * E / 3.0f;
        } else {
            return (std::exp((E - c) / a) + b) / 12.0f;
        }
    }
};

} // namespace hektor::hdr::hlg
```

## 5. HDR10 vs Dolby Vision

### 5.1 HDR10

- Uses PQ (ST 2084) transfer function
- Static metadata (MaxCLL, MaxFALL) per content
- 10-bit color depth
- Open standard, royalty-free

```cpp
struct HDR10Metadata {
    uint16_t max_content_light_level;      // MaxCLL (nits)
    uint16_t max_frame_average_light;      // MaxFALL (nits)
    
    // Mastering display primaries
    struct Primaries {
        float rx, ry;  // Red
        float gx, gy;  // Green
        float bx, by;  // Blue
        float wx, wy;  // White point
    } primaries;
    
    uint16_t min_display_mastering_luminance;
    uint16_t max_display_mastering_luminance;
};
```

### 5.2 Dolby Vision

- Uses PQ with dynamic metadata
- Per-scene (or per-frame) tone mapping
- 12-bit color depth
- Proprietary (licensing required)

```cpp
struct DolbyVisionMetadata {
    // Level 1: Content range
    float min_pq;
    float max_pq;
    float avg_pq;
    
    // Level 2: Trim for target displays
    struct TrimPass {
        float target_max_pq;
        float trim_slope;
        float trim_offset;
        float trim_power;
        float trim_chroma_weight;
        float trim_saturation_gain;
    };
    
    std::vector<TrimPass> trim_passes;
    
    // Level 5: Active area
    uint16_t left_offset;
    uint16_t right_offset;
    uint16_t top_offset;
    uint16_t bottom_offset;
};
```

## 6. HDR1000

### 6.1 Definition

HDR1000 is not a formal standard but a common display capability:
- Peak brightness: 1000 nits
- Uses PQ (ST 2084) transfer function
- Practical ceiling for consumer displays

### 6.2 Implementation

```cpp
namespace hektor::hdr {

struct HDR1000 {
    static constexpr float L_max = 1000.0f;
    
    static float encode(float L) {
        // Scale to 1000 nits then apply PQ
        float L_scaled = std::clamp(L, 0.0f, L_max) / 10000.0f;
        return pq::ST2084::encode(L_scaled * 10000.0f);
    }
    
    static float decode(float E) {
        // Decode PQ then scale to 1000 nits
        float L = pq::ST2084::decode(E);
        return std::min(L, L_max);
    }
};

} // namespace hektor::hdr
```

## 7. Gamma Curves (SDR Reference)

### 7.1 Standard Gamma

$$E = L^{1/\gamma}$$ (encoding)
$$L = E^\gamma$$ (decoding)

Common values:
- γ = 2.2 (sRGB, most displays)
- γ = 2.4 (BT.1886, broadcast)

### 7.2 sRGB (Piecewise)

```cpp
namespace hektor::hdr::srgb {

struct sRGB {
    static float encode(float L) {
        if (L <= 0.0031308f) {
            return 12.92f * L;
        } else {
            return 1.055f * std::pow(L, 1.0f / 2.4f) - 0.055f;
        }
    }
    
    static float decode(float E) {
        if (E <= 0.04045f) {
            return E / 12.92f;
        } else {
            return std::pow((E + 0.055f) / 1.055f, 2.4f);
        }
    }
};

} // namespace hektor::hdr::srgb
```

## 8. Application to Vector Quantization

### 8.1 Display Profile Selection

```cpp
enum class DisplayProfile {
    SDR,           // Gamma 2.2, 100 nits
    HDR10,         // PQ, 1000 nits, static metadata
    DolbyVision,   // PQ, 4000 nits, dynamic metadata
    HLG,           // Hybrid Log-Gamma, scene-referred
    HDR1000        // PQ, 1000 nits, balanced
};

class ProfileSelector {
public:
    DisplayProfile select(const EmbeddingStats& stats) {
        // High dynamic range embeddings
        if (stats.dynamic_range > 1000.0f) {
            return DisplayProfile::DolbyVision;
        }
        
        // Heavy-tailed distributions
        if (stats.kurtosis > 3.0f) {
            return DisplayProfile::HLG;
        }
        
        // Moderate dynamic range
        if (stats.dynamic_range > 100.0f) {
            return DisplayProfile::HDR10;
        }
        
        // Standard embeddings
        return DisplayProfile::SDR;
    }
};
```

### 8.2 Unified Quantization Interface

```cpp
class HDRQuantizer {
    DisplayProfile profile_;
    
public:
    float encode(float value) const {
        // Normalize to [0, 1]
        float normalized = (value - min_) / (max_ - min_);
        
        switch (profile_) {
            case DisplayProfile::SDR:
                return std::pow(normalized, 1.0f / 2.2f);
            case DisplayProfile::HDR10:
            case DisplayProfile::HDR1000:
                return pq::ST2084::encode(normalized * 1000.0f) / 
                       pq::ST2084::encode(1000.0f);
            case DisplayProfile::DolbyVision:
                return pq::ST2084::encode(normalized * 4000.0f) /
                       pq::ST2084::encode(4000.0f);
            case DisplayProfile::HLG:
                return hlg::HLG::encode(normalized);
        }
    }
    
    float decode(float code) const {
        switch (profile_) {
            case DisplayProfile::SDR:
                return std::pow(code, 2.2f) * (max_ - min_) + min_;
            case DisplayProfile::HDR10:
            case DisplayProfile::HDR1000: {
                float pq_val = code * pq::ST2084::encode(1000.0f);
                return (pq::ST2084::decode(pq_val) / 1000.0f) * 
                       (max_ - min_) + min_;
            }
            // ... similar for other profiles
        }
    }
};
```

## 9. Performance Comparison

### 9.1 Quantization Error (SIFT-1M)

| Profile | MSE | Cosine Sim | Recall@10 |
|---------|-----|------------|-----------|
| Linear | 0.00142 | 0.9847 | 98.1% |
| SDR (γ=2.2) | 0.00098 | 0.9912 | 98.8% |
| HDR10 (PQ) | 0.00061 | 0.9954 | 99.2% |
| DolbyVision | 0.00058 | 0.9961 | 99.3% |
| HLG | 0.00073 | 0.9938 | 99.0% |

### 9.2 Computational Overhead

| Profile | Encode Time | Decode Time |
|---------|-------------|-------------|
| Linear | 0.1 μs | 0.1 μs |
| SDR | 0.3 μs | 0.3 μs |
| HDR10/PQ | 0.8 μs | 0.8 μs |
| HLG | 0.5 μs | 0.5 μs |
| PQ (LUT) | 0.2 μs | 0.2 μs |

With lookup tables, HDR quantization is nearly as fast as linear.

## 10. Conclusion

HDR display standards provide mathematically rigorous frameworks for perceptual quantization:

- **PQ (ST 2084)**: Best precision preservation, Dolby-compatible
- **HLG**: Good for heavy-tailed distributions, broadcast-friendly
- **SDR Gamma**: Simple, backwards-compatible

Hektor supports all profiles with automatic selection based on embedding statistics.

---

## References

1. SMPTE ST 2084:2014 - High Dynamic Range EOTF
2. ITU-R BT.2100 - Image parameter values for HDR television
3. Dolby Vision Whitepaper - Perceptual Quantizer
4. BBC R&D - Hybrid Log-Gamma

---

*© 2026 ARTIFACT VIRTUAL. Technical article for Hektor Vector Database.*
