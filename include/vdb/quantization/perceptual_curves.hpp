#pragma once
// ============================================================================
// VectorDB - Perceptual Transfer Functions (PQ, HLG, Gamma)
// Implements SMPTE ST 2084 (PQ) and Rec. 2100 HLG curves for HDR
// ============================================================================

#include "../core.hpp"
#include <cmath>
#include <vector>
#include <array>

namespace vdb {
namespace quantization {

// ============================================================================
// Perceptual Curve Types
// ============================================================================

enum class PerceptualCurve {
    Linear,          // No transform (identity)
    Gamma22,         // sRGB/Rec.709 gamma 2.2
    Gamma24,         // DCI-P3 gamma 2.4
    PQ_ST2084,       // SMPTE ST 2084 (Dolby Vision, HDR10)
    HLG_Rec2100,     // Hybrid Log-Gamma (BBC/NHK)
};

// ============================================================================
// PQ Curve (SMPTE ST 2084) - Perceptual Quantizer for HDR
// ============================================================================

class PQCurve {
public:
    // Constants from SMPTE ST 2084:2014
    static constexpr float M1 = 2610.0f / 16384.0f;      // 0.1593017578125
    static constexpr float M2 = 2523.0f / 4096.0f * 128.0f;  // 78.84375
    static constexpr float C1 = 3424.0f / 4096.0f;       // 0.8359375
    static constexpr float C2 = 2413.0f / 4096.0f * 32.0f;   // 18.8515625
    static constexpr float C3 = 2392.0f / 4096.0f * 32.0f;   // 18.6875
    
    static constexpr float INV_M1 = 1.0f / M1;
    static constexpr float INV_M2 = 1.0f / M2;
    
    static constexpr float PEAK_LUMINANCE = 10000.0f;  // 10,000 nits
    
    /**
     * Encode linear luminance to PQ (perceptual) space
     * 
     * @param linear_luminance Linear light value [0, 10000] nits
     * @return PQ-encoded value [0, 1]
     */
    [[nodiscard]] static inline float encode(float linear_luminance) noexcept {
        // Normalize to [0, 1]
        const float L = linear_luminance / PEAK_LUMINANCE;
        
        // Protect against invalid input
        if (L <= 0.0f) return 0.0f;
        if (L >= 1.0f) return 1.0f;
        
        // PQ encoding
        const float Lm1 = std::pow(L, M1);
        const float numerator = C1 + C2 * Lm1;
        const float denominator = 1.0f + C3 * Lm1;
        const float N = numerator / denominator;
        
        return std::pow(N, M2);
    }
    
    /**
     * Decode PQ (perceptual) space to linear luminance
     * 
     * @param pq_value PQ-encoded value [0, 1]
     * @return Linear luminance [0, 10000] nits
     */
    [[nodiscard]] static inline float decode(float pq_value) noexcept {
        // Protect against invalid input
        if (pq_value <= 0.0f) return 0.0f;
        if (pq_value >= 1.0f) return PEAK_LUMINANCE;
        
        // PQ decoding (inverse)
        const float Nm2 = std::pow(pq_value, INV_M2);
        const float numerator = std::max(Nm2 - C1, 0.0f);
        const float denominator = C2 - C3 * Nm2;
        
        if (denominator <= 0.0f) return PEAK_LUMINANCE;
        
        const float L = std::pow(numerator / denominator, INV_M1);
        
        return L * PEAK_LUMINANCE;
    }
    
    /**
     * Batch encode multiple values (optimized)
     */
    [[nodiscard]] static std::vector<float> encode_batch(std::span<const float> linear_values) {
        std::vector<float> encoded(linear_values.size());
        for (size_t i = 0; i < linear_values.size(); ++i) {
            encoded[i] = encode(linear_values[i]);
        }
        return encoded;
    }
    
    /**
     * Batch decode multiple values (optimized)
     */
    [[nodiscard]] static std::vector<float> decode_batch(std::span<const float> pq_values) {
        std::vector<float> decoded(pq_values.size());
        for (size_t i = 0; i < pq_values.size(); ++i) {
            decoded[i] = decode(pq_values[i]);
        }
        return decoded;
    }
};

// ============================================================================
// HLG Curve (Rec. 2100 Hybrid Log-Gamma) - BBC/NHK HDR
// ============================================================================

class HLGCurve {
public:
    static constexpr float A = 0.17883277f;
    static constexpr float B = 0.28466892f;  // 1 - 4*A
    static constexpr float C = 0.55991073f;  // 0.5 - A * ln(4*A)
    
    /**
     * Encode linear scene luminance to HLG
     * 
     * @param linear_value Linear scene light [0, 1]
     * @return HLG-encoded value [0, 1]
     */
    [[nodiscard]] static inline float encode(float linear_value) noexcept {
        if (linear_value <= 0.0f) return 0.0f;
        if (linear_value >= 1.0f) return 1.0f;
        
        if (linear_value <= 1.0f / 12.0f) {
            // Linear part
            return std::sqrt(3.0f * linear_value);
        } else {
            // Logarithmic part
            return A * std::log(12.0f * linear_value - B) + C;
        }
    }
    
    /**
     * Decode HLG to linear scene luminance
     * 
     * @param hlg_value HLG-encoded value [0, 1]
     * @return Linear scene light [0, 1]
     */
    [[nodiscard]] static inline float decode(float hlg_value) noexcept {
        if (hlg_value <= 0.0f) return 0.0f;
        if (hlg_value >= 1.0f) return 1.0f;
        
        if (hlg_value <= 0.5f) {
            // Linear part inverse
            return (hlg_value * hlg_value) / 3.0f;
        } else {
            // Logarithmic part inverse
            return (std::exp((hlg_value - C) / A) + B) / 12.0f;
        }
    }
    
    [[nodiscard]] static std::vector<float> encode_batch(std::span<const float> linear_values) {
        std::vector<float> encoded(linear_values.size());
        for (size_t i = 0; i < linear_values.size(); ++i) {
            encoded[i] = encode(linear_values[i]);
        }
        return encoded;
    }
    
    [[nodiscard]] static std::vector<float> decode_batch(std::span<const float> hlg_values) {
        std::vector<float> decoded(hlg_values.size());
        for (size_t i = 0; i < hlg_values.size(); ++i) {
            decoded[i] = decode(hlg_values[i]);
        }
        return decoded;
    }
};

// ============================================================================
// Gamma Curve (sRGB, Rec.709, DCI-P3)
// ============================================================================

class GammaCurve {
public:
    explicit GammaCurve(float gamma = 2.2f) : gamma_(gamma), inv_gamma_(1.0f / gamma) {}
    
    [[nodiscard]] inline float encode(float linear_value) const noexcept {
        if (linear_value <= 0.0f) return 0.0f;
        if (linear_value >= 1.0f) return 1.0f;
        return std::pow(linear_value, inv_gamma_);
    }
    
    [[nodiscard]] inline float decode(float gamma_value) const noexcept {
        if (gamma_value <= 0.0f) return 0.0f;
        if (gamma_value >= 1.0f) return 1.0f;
        return std::pow(gamma_value, gamma_);
    }
    
    [[nodiscard]] std::vector<float> encode_batch(std::span<const float> linear_values) const {
        std::vector<float> encoded(linear_values.size());
        for (size_t i = 0; i < linear_values.size(); ++i) {
            encoded[i] = encode(linear_values[i]);
        }
        return encoded;
    }
    
    [[nodiscard]] std::vector<float> decode_batch(std::span<const float> gamma_values) const {
        std::vector<float> decoded(gamma_values.size());
        for (size_t i = 0; i < gamma_values.size(); ++i) {
            decoded[i] = decode(gamma_values[i]);
        }
        return decoded;
    }
    
    [[nodiscard]] float gamma() const noexcept { return gamma_; }

private:
    float gamma_;
    float inv_gamma_;
};

// ============================================================================
// Perceptual Transfer Function - Unified Interface
// ============================================================================

class PerceptualTransferFunction {
public:
    explicit PerceptualTransferFunction(PerceptualCurve curve = PerceptualCurve::Linear)
        : curve_(curve), gamma_curve_(2.2f) {}
    
    /**
     * Apply perceptual encoding to vector
     */
    [[nodiscard]] Vector encode(VectorView input) const {
        Vector output(input.size());
        
        switch (curve_) {
            case PerceptualCurve::Linear:
                std::copy(input.begin(), input.end(), output.begin());
                break;
                
            case PerceptualCurve::Gamma22:
                for (size_t i = 0; i < input.size(); ++i) {
                    output[i] = gamma_curve_.encode(input[i]);
                }
                break;
                
            case PerceptualCurve::Gamma24: {
                GammaCurve gamma24(2.4f);
                for (size_t i = 0; i < input.size(); ++i) {
                    output[i] = gamma24.encode(input[i]);
                }
                break;
            }
                
            case PerceptualCurve::PQ_ST2084:
                for (size_t i = 0; i < input.size(); ++i) {
                    output[i] = PQCurve::encode(input[i]);
                }
                break;
                
            case PerceptualCurve::HLG_Rec2100:
                for (size_t i = 0; i < input.size(); ++i) {
                    output[i] = HLGCurve::encode(input[i]);
                }
                break;
        }
        
        return output;
    }
    
    /**
     * Apply perceptual decoding to vector
     */
    [[nodiscard]] Vector decode(VectorView input) const {
        Vector output(input.size());
        
        switch (curve_) {
            case PerceptualCurve::Linear:
                std::copy(input.begin(), input.end(), output.begin());
                break;
                
            case PerceptualCurve::Gamma22:
                for (size_t i = 0; i < input.size(); ++i) {
                    output[i] = gamma_curve_.decode(input[i]);
                }
                break;
                
            case PerceptualCurve::Gamma24: {
                GammaCurve gamma24(2.4f);
                for (size_t i = 0; i < input.size(); ++i) {
                    output[i] = gamma24.decode(input[i]);
                }
                break;
            }
                
            case PerceptualCurve::PQ_ST2084:
                for (size_t i = 0; i < input.size(); ++i) {
                    output[i] = PQCurve::decode(input[i]);
                }
                break;
                
            case PerceptualCurve::HLG_Rec2100:
                for (size_t i = 0; i < input.size(); ++i) {
                    output[i] = HLGCurve::decode(input[i]);
                }
                break;
        }
        
        return output;
    }
    
    [[nodiscard]] PerceptualCurve curve() const noexcept { return curve_; }
    void set_curve(PerceptualCurve curve) noexcept { curve_ = curve; }

private:
    PerceptualCurve curve_;
    GammaCurve gamma_curve_;
};

}} // namespace vdb::quantization
