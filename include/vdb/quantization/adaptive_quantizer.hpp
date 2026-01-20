#pragma once
// ============================================================================
// VectorDB - Closed-Loop Quantization Systems
// Display-aware, environment-aware, and feedback-based quantization
// ============================================================================

#include "../core.hpp"
#include "../distance.hpp"
#include "perceptual_curves.hpp"
#include "product_quantizer.hpp"
#include <vector>
#include <span>
#include <memory>

namespace vdb {
namespace quantization {

// ============================================================================
// Display Profile - Target display characteristics
// ============================================================================

enum class DisplayType {
    SDR_BT709,      // Standard Dynamic Range (sRGB, Rec.709)
    HDR10,          // HDR10 (ST.2084, 1000 nits)
    HDR10_PLUS,     // HDR10+ (ST.2084, 1000-4000 nits, dynamic metadata)
    HDR1000,        // Generic HDR 1000 nits
    HDR4000,        // Generic HDR 4000 nits
    DolbyVision,    // Dolby Vision (ST.2084, 10000 nits, dynamic metadata)
};

enum class ColorGamut {
    sRGB,           // Standard RGB (Rec.709)
    DCI_P3,         // Digital Cinema (Apple displays)
    Rec2020,        // Ultra HD (future-proof)
};

struct DisplayProfile {
    DisplayType type = DisplayType::SDR_BT709;
    
    // Luminance characteristics
    float peak_luminance = 100.0f;      // nits (cd/m²)
    float black_level = 0.1f;           // nits
    float contrast_ratio = 1000.0f;     // peak/black
    
    // Color characteristics
    ColorGamut gamut = ColorGamut::sRGB;
    
    // Resolution (affects quantization visibility)
    uint32_t width = 1920;
    uint32_t height = 1080;
    float ppi = 96.0f;                  // Pixels per inch
    
    // Viewing distance (affects perceptual requirements)
    float viewing_distance_meters = 2.0f;
    
    // Bit depth
    uint32_t bits_per_channel = 8;
    
    // Name for debugging
    std::string name;
    
    // Common presets
    static DisplayProfile SDR_Standard();
    static DisplayProfile HDR1000_Standard();
    static DisplayProfile HDR4000_Premium();
    static DisplayProfile DolbyVision_Cinema();
};

// ============================================================================
// Environment Profile - Viewing environment characteristics
// ============================================================================

struct EnvironmentProfile {
    // Ambient lighting
    float ambient_light_lux = 200.0f;   // lux (typical indoor: 100-500)
    
    // Surround characteristics
    enum class SurroundType {
        Dim,        // Cinema-like (< 5 lux)
        Average,    // Home theater (5-50 lux)
        Bright,     // Living room (50-500 lux)
        VeryBright, // Office/outdoor (> 500 lux)
    };
    SurroundType surround = SurroundType::Average;
    
    // Adaptation level
    float eye_adaptation_level = 0.5f;  // [0, 1], 0=dark adapted, 1=light adapted
    
    // Common presets
    static EnvironmentProfile DarkRoom();
    static EnvironmentProfile HomeTheater();
    static EnvironmentProfile LivingRoom();
    static EnvironmentProfile Office();
};

// ============================================================================
// Display-Aware Quantizer
// ============================================================================

class DisplayAwareQuantizer {
public:
    explicit DisplayAwareQuantizer(const DisplayProfile& profile = DisplayProfile::SDR_Standard());
    
    // Training with display awareness
    [[nodiscard]] Result<void> train(std::span<const Vector> training_data);
    [[nodiscard]] bool is_trained() const { return trained_; }
    
    // Encoding optimized for target display
    [[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
    [[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
    
    // Re-quantize for different display
    [[nodiscard]] Result<std::vector<uint8_t>> requantize_for_display(
        std::span<const uint8_t> codes,
        const DisplayProfile& target_display
    ) const;
    
    // Display profile management
    void set_display_profile(const DisplayProfile& profile);
    [[nodiscard]] const DisplayProfile& display_profile() const { return profile_; }
    
    // Stats
    [[nodiscard]] size_t code_size() const;
    [[nodiscard]] float compression_ratio() const;

private:
    DisplayProfile profile_;
    bool trained_;
    
    PerceptualTransferFunction transfer_function_;
    std::unique_ptr<ProductQuantizer> quantizer_;
    
    // Adapt transfer function to display
    void adapt_transfer_function();
    
    // Compute required precision
    [[nodiscard]] uint32_t compute_required_bits() const;
};

// ============================================================================
// Environment-Aware Quantizer
// ============================================================================

class EnvironmentAwareQuantizer {
public:
    explicit EnvironmentAwareQuantizer(
        const DisplayProfile& display = DisplayProfile::SDR_Standard(),
        const EnvironmentProfile& environment = EnvironmentProfile::LivingRoom()
    );
    
    // Training with environment awareness
    [[nodiscard]] Result<void> train(std::span<const Vector> training_data);
    [[nodiscard]] bool is_trained() const { return trained_; }
    
    // Encoding adapted to environment
    [[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
    [[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
    
    // Adapt to changing environment
    void adapt_to_environment(const EnvironmentProfile& environment);
    
    // Profile management
    void set_display_profile(const DisplayProfile& profile);
    void set_environment_profile(const EnvironmentProfile& environment);
    
    [[nodiscard]] const DisplayProfile& display_profile() const { return display_; }
    [[nodiscard]] const EnvironmentProfile& environment_profile() const { return environment_; }

private:
    DisplayProfile display_;
    EnvironmentProfile environment_;
    bool trained_;
    
    std::unique_ptr<DisplayAwareQuantizer> base_quantizer_;
    
    // Adaptation parameters
    float shadow_precision_multiplier_;
    float highlight_precision_multiplier_;
    
    void compute_adaptation_parameters();
};

// ============================================================================
// Saliency-Based Quantizer
// ============================================================================

struct SaliencyMap {
    std::vector<float> importance;  // [0, 1] per dimension
    Dim dimension;
};

class SaliencyQuantizer {
public:
    explicit SaliencyQuantizer(uint32_t total_bit_budget = 512 * 8);
    
    // Training
    [[nodiscard]] Result<void> train(std::span<const Vector> training_data);
    [[nodiscard]] bool is_trained() const { return trained_; }
    
    // Encoding with saliency-based bit allocation
    [[nodiscard]] Result<std::vector<uint8_t>> encode(
        VectorView vector,
        const SaliencyMap& saliency
    ) const;
    
    [[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
    
    // Saliency detection
    [[nodiscard]] SaliencyMap detect_saliency(VectorView vector) const;
    
    // Stats
    [[nodiscard]] size_t code_size() const { return total_bit_budget_ / 8; }

private:
    uint32_t total_bit_budget_;
    bool trained_;
    
    std::vector<uint32_t> base_bit_allocation_;
    
    // Saliency-based bit allocation
    [[nodiscard]] std::vector<uint32_t> allocate_bits(const SaliencyMap& saliency) const;
    
    // Variable-precision quantization
    [[nodiscard]] uint32_t quantize_value(float value, uint32_t bits) const;
    [[nodiscard]] float dequantize_value(uint32_t code, uint32_t bits) const;
};

// ============================================================================
// Feedback-Loop Quantizer (Iterative Refinement)
// ============================================================================

struct FeedbackConfig {
    uint32_t max_iterations = 5;
    float perceptual_error_threshold = 0.01f;
    
    // Error metric
    enum class ErrorMetric {
        L2,             // Euclidean distance
        SSIM,           // Structural similarity
        Perceptual,     // Perceptual distance
    };
    ErrorMetric error_metric = ErrorMetric::Perceptual;
    
    // Refinement strategy
    bool adaptive_bit_allocation = true;
    bool use_residual_coding = true;
};

class FeedbackLoopQuantizer {
public:
    explicit FeedbackLoopQuantizer(const FeedbackConfig& config = {});
    
    // Offline training with feedback
    [[nodiscard]] Result<void> train(std::span<const Vector> training_data);
    [[nodiscard]] bool is_trained() const { return trained_; }
    
    // Iterative encoding with quality feedback
    [[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
    [[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
    
    // Get convergence statistics
    struct ConvergenceStats {
        uint32_t iterations;
        float final_error;
        std::vector<float> error_history;
    };
    [[nodiscard]] ConvergenceStats get_last_convergence_stats() const {
        return last_stats_;
    }

private:
    FeedbackConfig config_;
    bool trained_;
    mutable ConvergenceStats last_stats_;
    
    std::unique_ptr<ProductQuantizer> base_quantizer_;
    std::vector<std::unique_ptr<ProductQuantizer>> residual_quantizers_;
    
    // Error computation
    [[nodiscard]] float compute_error(VectorView original, VectorView reconstructed) const;
    
    // Iterative refinement
    [[nodiscard]] Result<std::vector<uint8_t>> iterative_encode(VectorView vector) const;
    
    // Error-based bit reallocation
    [[nodiscard]] std::vector<uint32_t> compute_error_based_allocation(
        VectorView error_map
    ) const;
};

// ============================================================================
// Adaptive Quantization System (Unified)
// ============================================================================

struct AdaptiveQuantizerConfig {
    // Display and environment
    DisplayProfile display = DisplayProfile::SDR_Standard();
    EnvironmentProfile environment = EnvironmentProfile::LivingRoom();
    
    // Features
    bool enable_display_awareness = true;
    bool enable_environment_awareness = true;
    bool enable_saliency = false;           // Requires saliency maps
    bool enable_feedback_loop = false;      // Expensive, offline only
    
    // Underlying quantizer
    ProductQuantizerConfig base_config;
};

class AdaptiveQuantizer {
public:
    explicit AdaptiveQuantizer(const AdaptiveQuantizerConfig& config = {});
    
    // Training
    [[nodiscard]] Result<void> train(std::span<const Vector> training_data);
    [[nodiscard]] bool is_trained() const;
    
    // Encoding (adapts based on enabled features)
    [[nodiscard]] Result<std::vector<uint8_t>> encode(VectorView vector) const;
    [[nodiscard]] Result<std::vector<uint8_t>> encode_with_saliency(
        VectorView vector,
        const SaliencyMap& saliency
    ) const;
    
    [[nodiscard]] Result<Vector> decode(std::span<const uint8_t> codes) const;
    
    // Runtime adaptation
    void adapt_to_display(const DisplayProfile& display);
    void adapt_to_environment(const EnvironmentProfile& environment);
    
    // Stats
    [[nodiscard]] size_t code_size() const;
    [[nodiscard]] float compression_ratio() const;
    
    [[nodiscard]] const AdaptiveQuantizerConfig& config() const { return config_; }

private:
    AdaptiveQuantizerConfig config_;
    
    std::unique_ptr<EnvironmentAwareQuantizer> env_quantizer_;
    std::unique_ptr<SaliencyQuantizer> saliency_quantizer_;
    std::unique_ptr<FeedbackLoopQuantizer> feedback_quantizer_;
};

}} // namespace vdb::quantization
