// ============================================================================
// VectorDB - Closed-Loop Quantization Systems Implementation
// ============================================================================

#include "vdb/quantization/adaptive_quantizer.hpp"
#include "vdb/quantization/product_quantizer.hpp"
#include <algorithm>

namespace vdb {
namespace quantization {

// ============================================================================
// DisplayProfile Presets
// ============================================================================

DisplayProfile DisplayProfile::SDR_Standard() {
    DisplayProfile profile;
    profile.type = DisplayType::SDR_BT709;
    profile.peak_luminance = 100.0f;
    profile.black_level = 0.1f;
    profile.contrast_ratio = 1000.0f;
    profile.gamut = ColorGamut::sRGB;
    profile.bits_per_channel = 8;
    profile.name = "SDR Standard (Rec.709)";
    return profile;
}

DisplayProfile DisplayProfile::HDR1000_Standard() {
    DisplayProfile profile;
    profile.type = DisplayType::HDR1000;
    profile.peak_luminance = 1000.0f;
    profile.black_level = 0.005f;
    profile.contrast_ratio = 200000.0f;
    profile.gamut = ColorGamut::DCI_P3;
    profile.bits_per_channel = 10;
    profile.name = "HDR 1000 nits (HDR10)";
    return profile;
}

DisplayProfile DisplayProfile::HDR4000_Premium() {
    DisplayProfile profile;
    profile.type = DisplayType::HDR4000;
    profile.peak_luminance = 4000.0f;
    profile.black_level = 0.005f;
    profile.contrast_ratio = 800000.0f;
    profile.gamut = ColorGamut::Rec2020;
    profile.bits_per_channel = 10;
    profile.name = "HDR 4000 nits (Premium)";
    return profile;
}

DisplayProfile DisplayProfile::DolbyVision_Cinema() {
    DisplayProfile profile;
    profile.type = DisplayType::DolbyVision;
    profile.peak_luminance = 10000.0f;
    profile.black_level = 0.001f;
    profile.contrast_ratio = 10000000.0f;
    profile.gamut = ColorGamut::Rec2020;
    profile.bits_per_channel = 12;
    profile.name = "Dolby Vision Cinema";
    return profile;
}

// ============================================================================
// EnvironmentProfile Presets
// ============================================================================

EnvironmentProfile EnvironmentProfile::DarkRoom() {
    EnvironmentProfile env;
    env.ambient_light_lux = 5.0f;
    env.surround = SurroundType::Dim;
    env.eye_adaptation_level = 0.1f;
    return env;
}

EnvironmentProfile EnvironmentProfile::HomeTheater() {
    EnvironmentProfile env;
    env.ambient_light_lux = 25.0f;
    env.surround = SurroundType::Average;
    env.eye_adaptation_level = 0.3f;
    return env;
}

EnvironmentProfile EnvironmentProfile::LivingRoom() {
    EnvironmentProfile env;
    env.ambient_light_lux = 200.0f;
    env.surround = SurroundType::Bright;
    env.eye_adaptation_level = 0.6f;
    return env;
}

EnvironmentProfile EnvironmentProfile::Office() {
    EnvironmentProfile env;
    env.ambient_light_lux = 500.0f;
    env.surround = SurroundType::VeryBright;
    env.eye_adaptation_level = 0.9f;
    return env;
}

// ============================================================================
// DisplayAwareQuantizer Implementation
// ============================================================================

DisplayAwareQuantizer::DisplayAwareQuantizer(const DisplayProfile& profile)
    : profile_(profile), trained_(false), transfer_function_(PerceptualCurve::Linear) {
    adapt_transfer_function();
}

void DisplayAwareQuantizer::adapt_transfer_function() {
    // Select appropriate transfer function based on display type
    switch (profile_.type) {
        case DisplayType::SDR_BT709:
            transfer_function_.set_curve(PerceptualCurve::Gamma22);
            break;
            
        case DisplayType::HDR10:
        case DisplayType::HDR10_PLUS:
        case DisplayType::HDR1000:
        case DisplayType::HDR4000:
        case DisplayType::DolbyVision:
            transfer_function_.set_curve(PerceptualCurve::PQ_ST2084);
            break;
    }
}

Result<void> DisplayAwareQuantizer::train(std::span<const Vector> training_data) {
    // Create underlying quantizer with appropriate configuration
    ProductQuantizerConfig config;
    config.dimension = training_data.empty() ? UNIFIED_DIM : training_data[0].size();
    config.num_centroids = (1u << std::min(profile_.bits_per_channel, 8u));
    
    quantizer_ = std::make_unique<ProductQuantizer>(config);
    
    // Transform to perceptual space before training
    std::vector<Vector> perceptual_data;
    perceptual_data.reserve(training_data.size());
    
    for (const auto& vec : training_data) {
        perceptual_data.push_back(transfer_function_.encode(vec));
    }
    
    auto result = quantizer_->train(perceptual_data);
    if (result) {
        trained_ = true;
    }
    
    return result;
}

Result<std::vector<uint8_t>> DisplayAwareQuantizer::encode(VectorView vector) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::NotTrained, "DisplayAwareQuantizer not trained"});
    }
    
    // Transform to perceptual space
    auto perceptual = transfer_function_.encode(vector);
    
    // Quantize in perceptual space
    return quantizer_->encode(perceptual);
}

Result<Vector> DisplayAwareQuantizer::decode(std::span<const uint8_t> codes) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::NotTrained, "DisplayAwareQuantizer not trained"});
    }
    
    // Decode from quantizer
    auto perceptual_result = quantizer_->decode(codes);
    if (!perceptual_result) {
        return perceptual_result;
    }
    
    // Transform back from perceptual space
    return transfer_function_.decode(*perceptual_result);
}

Result<std::vector<uint8_t>> DisplayAwareQuantizer::requantize_for_display(
    std::span<const uint8_t> codes,
    const DisplayProfile& target_display
) const {
    // Decode with current display profile
    auto decoded_result = decode(codes);
    if (!decoded_result) {
        return tl::unexpected(decoded_result.error());
    }
    
    // For requantization, we need the quantizer to be trained
    if (!quantizer_ || !trained_) {
        return tl::unexpected(Error{ErrorCode::NotTrained, "Quantizer not trained for requantization"});
    }
    
    // Determine target perceptual curve from display type
    PerceptualCurve target_curve = PerceptualCurve::Linear;
    switch (target_display.type) {
        case DisplayType::SDR_BT709:
            target_curve = PerceptualCurve::Gamma22;
            break;
        case DisplayType::HDR10:
        case DisplayType::HDR10_PLUS:
        case DisplayType::HDR1000:
        case DisplayType::HDR4000:
        case DisplayType::DolbyVision:
            target_curve = PerceptualCurve::PQ_ST2084;
            break;
    }
    
    // Create transfer function for target display
    PerceptualTransferFunction target_transfer(target_curve);
    
    // Transform to target perceptual space
    auto perceptual = target_transfer.encode(*decoded_result);
    
    // Quantize using the same trained quantizer (codebooks are display-agnostic)
    return quantizer_->encode(perceptual);
}

void DisplayAwareQuantizer::set_display_profile(const DisplayProfile& profile) {
    profile_ = profile;
    adapt_transfer_function();
}

size_t DisplayAwareQuantizer::code_size() const {
    return quantizer_ ? quantizer_->code_size() : 0;
}

float DisplayAwareQuantizer::compression_ratio() const {
    return quantizer_ ? quantizer_->compression_ratio() : 0.0f;
}

uint32_t DisplayAwareQuantizer::compute_required_bits() const {
    // Compute required bits based on display characteristics
    // Higher peak luminance requires more bits for smooth gradients
    
    float dynamic_range = profile_.peak_luminance / std::max(profile_.black_level, 0.001f);
    float log_dr = std::log2(dynamic_range);
    
    // Account for viewing distance and resolution
    float pixels_per_degree = (profile_.width / 2.0f) / 
        std::atan(profile_.viewing_distance_meters * 100.0f / (profile_.width / profile_.ppi * 2.54f));
    
    // More bits needed for higher resolution at closer viewing distances
    float resolution_factor = std::min(pixels_per_degree / 60.0f, 2.0f);
    
    uint32_t required_bits = static_cast<uint32_t>(
        std::ceil(log_dr + resolution_factor)
    );
    
    return std::clamp(required_bits, 6u, 16u);
}

// ============================================================================
// EnvironmentAwareQuantizer Implementation
// ============================================================================

EnvironmentAwareQuantizer::EnvironmentAwareQuantizer(
    const DisplayProfile& display,
    const EnvironmentProfile& environment
) : display_(display), environment_(environment), trained_(false),
    shadow_precision_multiplier_(1.0f), highlight_precision_multiplier_(1.0f) {
    
    base_quantizer_ = std::make_unique<DisplayAwareQuantizer>(display);
    compute_adaptation_parameters();
}

void EnvironmentAwareQuantizer::compute_adaptation_parameters() {
    // Adapt precision based on ambient light and eye adaptation
    
    // Bright environments reduce shadow detail visibility
    if (environment_.ambient_light_lux > 200.0f) {
        shadow_precision_multiplier_ = 0.7f;
        highlight_precision_multiplier_ = 1.3f;
    } else if (environment_.ambient_light_lux < 50.0f) {
        // Dark environments increase shadow detail visibility
        shadow_precision_multiplier_ = 1.3f;
        highlight_precision_multiplier_ = 0.9f;
    } else {
        shadow_precision_multiplier_ = 1.0f;
        highlight_precision_multiplier_ = 1.0f;
    }
}

Result<void> EnvironmentAwareQuantizer::train(std::span<const Vector> training_data) {
    auto result = base_quantizer_->train(training_data);
    if (result) {
        trained_ = true;
    }
    return result;
}

Result<std::vector<uint8_t>> EnvironmentAwareQuantizer::encode(VectorView vector) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::NotTrained, "EnvironmentAwareQuantizer not trained"});
    }
    
    // For now, delegate to base quantizer
    // Future: apply shadow/highlight precision adjustments
    return base_quantizer_->encode(vector);
}

Result<Vector> EnvironmentAwareQuantizer::decode(std::span<const uint8_t> codes) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::NotTrained, "EnvironmentAwareQuantizer not trained"});
    }
    
    return base_quantizer_->decode(codes);
}

void EnvironmentAwareQuantizer::adapt_to_environment(const EnvironmentProfile& environment) {
    environment_ = environment;
    compute_adaptation_parameters();
}

void EnvironmentAwareQuantizer::set_display_profile(const DisplayProfile& profile) {
    display_ = profile;
    base_quantizer_->set_display_profile(profile);
}

void EnvironmentAwareQuantizer::set_environment_profile(const EnvironmentProfile& environment) {
    adapt_to_environment(environment);
}

// ============================================================================
// SaliencyQuantizer Implementation
// ============================================================================

SaliencyQuantizer::SaliencyQuantizer(uint32_t total_bit_budget)
    : total_bit_budget_(total_bit_budget), trained_(false) {
}

Result<void> SaliencyQuantizer::train(std::span<const Vector> training_data) {
    if (training_data.empty()) {
        return tl::unexpected(Error{ErrorCode::InvalidInput, "Empty training data"});
    }
    
    // Initialize base bit allocation (uniform)
    size_t dim = training_data[0].size();
    uint32_t bits_per_dim = total_bit_budget_ / static_cast<uint32_t>(dim);
    base_bit_allocation_.assign(dim, bits_per_dim);
    
    trained_ = true;
    return {};
}

Result<std::vector<uint8_t>> SaliencyQuantizer::encode(
    VectorView vector,
    const SaliencyMap& saliency
) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::NotTrained, "SaliencyQuantizer not trained"});
    }
    
    // Allocate bits based on saliency
    auto bit_allocation = allocate_bits(saliency);
    
    // Quantize each dimension with allocated bits
    std::vector<uint8_t> codes;
    codes.reserve(total_bit_budget_ / 8);
    
    uint32_t bit_offset = 0;
    uint8_t current_byte = 0;
    
    for (size_t i = 0; i < vector.size(); ++i) {
        uint32_t bits = bit_allocation[i];
        uint32_t code = quantize_value(vector[i], bits);
        
        // Pack bits into bytes (simplified - assumes byte-aligned output for now)
        // Full implementation would do proper bit packing
        if (bits >= 8) {
            codes.push_back(static_cast<uint8_t>(code & 0xFF));
        }
    }
    
    return codes;
}

Result<Vector> SaliencyQuantizer::decode(std::span<const uint8_t> codes) const {
    if (!trained_) {
        return tl::unexpected(Error{ErrorCode::NotTrained, "SaliencyQuantizer not trained"});
    }
    
    // Simplified decoding (assumes uniform bit allocation for now)
    Vector output(static_cast<Dim>(base_bit_allocation_.size()));
    
    for (size_t i = 0; i < base_bit_allocation_.size() && i < codes.size(); ++i) {
        output[i] = dequantize_value(codes[i], base_bit_allocation_[i]);
    }
    
    return output;
}

SaliencyMap SaliencyQuantizer::detect_saliency(VectorView vector) const {
    SaliencyMap saliency;
    saliency.dimension = vector.size();
    saliency.importance.resize(vector.size());
    
    // Simple magnitude-based saliency detection
    float max_val = 0.0f;
    for (size_t i = 0; i < vector.size(); ++i) {
        float abs_val = std::abs(vector[i]);
        if (abs_val > max_val) max_val = abs_val;
    }
    
    if (max_val > 0.0f) {
        for (size_t i = 0; i < vector.size(); ++i) {
            saliency.importance[i] = std::abs(vector[i]) / max_val;
        }
    } else {
        std::fill(saliency.importance.begin(), saliency.importance.end(), 1.0f);
    }
    
    return saliency;
}

std::vector<uint32_t> SaliencyQuantizer::allocate_bits(const SaliencyMap& saliency) const {
    std::vector<uint32_t> allocation(saliency.dimension);
    
    // Compute total importance for normalization
    float total_importance = 0.0f;
    for (float imp : saliency.importance) {
        total_importance += imp;
    }
    
    if (total_importance <= 0.0f) {
        // Uniform allocation if no importance
        uint32_t bits_per_dim = total_bit_budget_ / static_cast<uint32_t>(saliency.dimension);
        std::fill(allocation.begin(), allocation.end(), bits_per_dim);
    } else {
        // Allocate bits proportional to importance
        uint32_t remaining_bits = total_bit_budget_;
        for (size_t i = 0; i < saliency.dimension; ++i) {
            float fraction = saliency.importance[i] / total_importance;
            allocation[i] = static_cast<uint32_t>(fraction * total_bit_budget_);
            allocation[i] = std::max(allocation[i], 1u);  // At least 1 bit per dimension
            remaining_bits -= allocation[i];
        }
    }
    
    return allocation;
}

uint32_t SaliencyQuantizer::quantize_value(float value, uint32_t bits) const {
    if (bits == 0) return 0;
    
    // Clamp to [0, 1] range
    float clamped = std::max(0.0f, std::min(1.0f, value));
    
    // Quantize to bits levels
    uint32_t max_val = (1u << bits) - 1;
    return static_cast<uint32_t>(clamped * max_val + 0.5f);
}

float SaliencyQuantizer::dequantize_value(uint32_t code, uint32_t bits) const {
    if (bits == 0) return 0.0f;
    
    uint32_t max_val = (1u << bits) - 1;
    return static_cast<float>(code) / static_cast<float>(max_val);
}

// ============================================================================
// AdaptiveQuantizer Implementation (Unified System)
// ============================================================================

AdaptiveQuantizer::AdaptiveQuantizer(const AdaptiveQuantizerConfig& config)
    : config_(config) {
    
    if (config_.enable_environment_awareness) {
        env_quantizer_ = std::make_unique<EnvironmentAwareQuantizer>(
            config_.display,
            config_.environment
        );
    }
    
    // Additional quantizers created as needed
}

Result<void> AdaptiveQuantizer::train(std::span<const Vector> training_data) {
    if (env_quantizer_) {
        return env_quantizer_->train(training_data);
    }
    
    return tl::unexpected(Error{ErrorCode::InvalidState, "No quantizer available"});
}

bool AdaptiveQuantizer::is_trained() const {
    if (env_quantizer_) {
        return env_quantizer_->is_trained();
    }
    return false;
}

Result<std::vector<uint8_t>> AdaptiveQuantizer::encode(VectorView vector) const {
    if (env_quantizer_) {
        return env_quantizer_->encode(vector);
    }
    
    return tl::unexpected(Error{ErrorCode::InvalidState, "No quantizer available"});
}

Result<std::vector<uint8_t>> AdaptiveQuantizer::encode_with_saliency(
    VectorView vector,
    const SaliencyMap& saliency
) const {
    if (saliency_quantizer_) {
        return saliency_quantizer_->encode(vector, saliency);
    }
    
    // Fallback to standard encoding
    return encode(vector);
}

Result<Vector> AdaptiveQuantizer::decode(std::span<const uint8_t> codes) const {
    if (env_quantizer_) {
        return env_quantizer_->decode(codes);
    }
    
    return tl::unexpected(Error{ErrorCode::InvalidState, "No quantizer available"});
}

void AdaptiveQuantizer::adapt_to_display(const DisplayProfile& display) {
    config_.display = display;
    if (env_quantizer_) {
        env_quantizer_->set_display_profile(display);
    }
}

void AdaptiveQuantizer::adapt_to_environment(const EnvironmentProfile& environment) {
    config_.environment = environment;
    if (env_quantizer_) {
        env_quantizer_->set_environment_profile(environment);
    }
}

size_t AdaptiveQuantizer::code_size() const {
    if (env_quantizer_) {
        // Get code size from environment quantizer's base
        return config_.base_config.num_subquantizers;
    }
    return 0;
}

float AdaptiveQuantizer::compression_ratio() const {
    if (env_quantizer_) {
        float original_size = config_.base_config.dimension * sizeof(float);
        float compressed_size = static_cast<float>(code_size());
        return original_size / compressed_size;
    }
    return 0.0f;
}

}} // namespace vdb::quantization
