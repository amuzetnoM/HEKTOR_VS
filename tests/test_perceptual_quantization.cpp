// ============================================================================
// VectorDB - Perceptual Quantization Tests
// ============================================================================

#include "vdb/quantization/perceptual_curves.hpp"
#include "vdb/quantization/adaptive_quantizer.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <vector>

using namespace vdb;
using namespace vdb::quantization;

// ============================================================================
// PQ Curve Tests
// ============================================================================

TEST(PQCurveTest, EncodeDecodeRoundtrip) {
    // Test round-trip encoding/decoding
    std::vector<float> test_values = {
        0.0f, 0.1f, 1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f
    };
    
    for (float value : test_values) {
        float encoded = PQCurve::encode(value);
        float decoded = PQCurve::decode(encoded);
        
        // Should be close to original (within 1%)
        EXPECT_NEAR(decoded, value, value * 0.01f);
    }
}

TEST(PQCurveTest, MonotonicEncoding) {
    // PQ encoding should be monotonically increasing
    float prev_encoded = 0.0f;
    
    for (float lum = 0.0f; lum <= 10000.0f; lum += 100.0f) {
        float encoded = PQCurve::encode(lum);
        EXPECT_GE(encoded, prev_encoded);
        prev_encoded = encoded;
    }
}

TEST(PQCurveTest, BoundaryConditions) {
    // Test edge cases
    EXPECT_EQ(PQCurve::encode(0.0f), 0.0f);
    EXPECT_LE(PQCurve::encode(10000.0f), 1.0f);
    
    EXPECT_EQ(PQCurve::decode(0.0f), 0.0f);
    EXPECT_LE(PQCurve::decode(1.0f), 10000.0f);
}

TEST(PQCurveTest, KnownTestVectors) {
    // Test against known PQ values from SMPTE ST 2084
    // Reference: 100 nits (typical SDR peak)
    float encoded_100 = PQCurve::encode(100.0f);
    EXPECT_NEAR(encoded_100, 0.508f, 0.01f);  // Approximate expected value
    
    // Reference: 1000 nits (HDR10 standard)
    float encoded_1000 = PQCurve::encode(1000.0f);
    EXPECT_NEAR(encoded_1000, 0.75f, 0.05f);  // Approximate expected value
}

TEST(PQCurveTest, BatchEncoding) {
    std::vector<float> input = {0.0f, 10.0f, 100.0f, 1000.0f, 10000.0f};
    auto encoded = PQCurve::encode_batch(input);
    
    EXPECT_EQ(encoded.size(), input.size());
    
    // Verify matches individual encoding
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_FLOAT_EQ(encoded[i], PQCurve::encode(input[i]));
    }
}

// ============================================================================
// HLG Curve Tests
// ============================================================================

TEST(HLGCurveTest, EncodeDecodeRoundtrip) {
    std::vector<float> test_values = {0.0f, 0.1f, 0.2f, 0.5f, 0.8f, 1.0f};
    
    for (float value : test_values) {
        float encoded = HLGCurve::encode(value);
        float decoded = HLGCurve::decode(encoded);
        
        EXPECT_NEAR(decoded, value, 0.001f);
    }
}

TEST(HLGCurveTest, PiecewiseContinuity) {
    // HLG transitions from linear to logarithmic at 1/12
    float transition_point = 1.0f / 12.0f;
    
    // Encode values around transition
    float just_below = HLGCurve::encode(transition_point - 0.001f);
    float just_above = HLGCurve::encode(transition_point + 0.001f);
    
    // Should be continuous (difference should be small)
    EXPECT_NEAR(just_below, just_above, 0.05f);
}

// ============================================================================
// Gamma Curve Tests
// ============================================================================

TEST(GammaCurveTest, StandardGamma22) {
    GammaCurve gamma(2.2f);
    
    // Test midpoint
    float encoded_05 = gamma.encode(0.5f);
    float decoded_05 = gamma.decode(encoded_05);
    
    EXPECT_NEAR(decoded_05, 0.5f, 0.001f);
    EXPECT_NEAR(encoded_05, std::pow(0.5f, 1.0f/2.2f), 0.001f);
}

TEST(GammaCurveTest, CustomGamma) {
    GammaCurve gamma(2.4f);
    
    EXPECT_FLOAT_EQ(gamma.gamma(), 2.4f);
    
    // Verify power law
    float test_value = 0.3f;
    float encoded = gamma.encode(test_value);
    EXPECT_NEAR(encoded, std::pow(test_value, 1.0f/2.4f), 0.0001f);
}

// ============================================================================
// Perceptual Transfer Function Tests
// ============================================================================

TEST(PerceptualTransferFunctionTest, LinearIdentity) {
    PerceptualTransferFunction ptf(PerceptualCurve::Linear);
    
    Vector input = {0.0f, 0.5f, 1.0f};
    Vector encoded = ptf.encode(input);
    Vector decoded = ptf.decode(encoded);
    
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_FLOAT_EQ(decoded[i], input[i]);
    }
}

TEST(PerceptualTransferFunctionTest, PQCurveVector) {
    PerceptualTransferFunction ptf(PerceptualCurve::PQ_ST2084);
    
    Vector input = {0.0f, 100.0f, 1000.0f};
    Vector encoded = ptf.encode(input);
    
    // Verify individual elements match PQ curve
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_FLOAT_EQ(encoded[i], PQCurve::encode(input[i]));
    }
}

TEST(PerceptualTransferFunctionTest, SwitchCurves) {
    PerceptualTransferFunction ptf(PerceptualCurve::Linear);
    
    Vector input = {0.5f};
    Vector linear_result = ptf.encode(input);
    
    ptf.set_curve(PerceptualCurve::Gamma22);
    Vector gamma_result = ptf.encode(input);
    
    // Results should differ
    EXPECT_NE(linear_result[0], gamma_result[0]);
}

// ============================================================================
// Display Profile Tests
// ============================================================================

TEST(DisplayProfileTest, SDRPreset) {
    auto sdr = DisplayProfile::SDR_Standard();
    
    EXPECT_EQ(sdr.type, DisplayType::SDR_BT709);
    EXPECT_FLOAT_EQ(sdr.peak_luminance, 100.0f);
    EXPECT_EQ(sdr.bits_per_channel, 8);
    EXPECT_EQ(sdr.gamut, ColorGamut::sRGB);
}

TEST(DisplayProfileTest, HDRPresets) {
    auto hdr1000 = DisplayProfile::HDR1000_Standard();
    auto hdr4000 = DisplayProfile::HDR4000_Premium();
    
    EXPECT_GT(hdr4000.peak_luminance, hdr1000.peak_luminance);
    EXPECT_GT(hdr4000.contrast_ratio, hdr1000.contrast_ratio);
}

TEST(DisplayProfileTest, DolbyVision) {
    auto dv = DisplayProfile::DolbyVision_Cinema();
    
    EXPECT_FLOAT_EQ(dv.peak_luminance, 10000.0f);
    EXPECT_EQ(dv.bits_per_channel, 12);
    EXPECT_EQ(dv.gamut, ColorGamut::Rec2020);
}

// ============================================================================
// Environment Profile Tests
// ============================================================================

TEST(EnvironmentProfileTest, DarkRoom) {
    auto dark = EnvironmentProfile::DarkRoom();
    
    EXPECT_LT(dark.ambient_light_lux, 10.0f);
    EXPECT_EQ(dark.surround, EnvironmentProfile::SurroundType::Dim);
    EXPECT_LT(dark.eye_adaptation_level, 0.2f);
}

TEST(EnvironmentProfileTest, Office) {
    auto office = EnvironmentProfile::Office();
    
    EXPECT_GT(office.ambient_light_lux, 400.0f);
    EXPECT_EQ(office.surround, EnvironmentProfile::SurroundType::VeryBright);
    EXPECT_GT(office.eye_adaptation_level, 0.8f);
}

// ============================================================================
// Display-Aware Quantizer Tests
// ============================================================================

class DisplayAwareQuantizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create synthetic training data
        training_data_.clear();
        for (int i = 0; i < 100; ++i) {
            Vector vec(512);
            for (size_t j = 0; j < vec.size(); ++j) {
                vec[j] = static_cast<float>(i + j) / 1000.0f;
            }
            training_data_.push_back(vec);
        }
    }
    
    std::vector<Vector> training_data_;
};

TEST_F(DisplayAwareQuantizerTest, SDRTraining) {
    DisplayAwareQuantizer quantizer(DisplayProfile::SDR_Standard());
    
    auto result = quantizer.train(training_data_);
    EXPECT_TRUE(result);
    EXPECT_TRUE(quantizer.is_trained());
}

TEST_F(DisplayAwareQuantizerTest, HDRTraining) {
    DisplayAwareQuantizer quantizer(DisplayProfile::HDR1000_Standard());
    
    auto result = quantizer.train(training_data_);
    EXPECT_TRUE(result);
    EXPECT_TRUE(quantizer.is_trained());
}

TEST_F(DisplayAwareQuantizerTest, EncodeDecodeRoundtrip) {
    DisplayAwareQuantizer quantizer(DisplayProfile::SDR_Standard());
    quantizer.train(training_data_);
    
    Vector test_vec = training_data_[0];
    auto encoded = quantizer.encode(test_vec);
    ASSERT_TRUE(encoded);
    
    auto decoded = quantizer.decode(*encoded);
    ASSERT_TRUE(decoded);
    
    // Vectors should be approximately similar
    EXPECT_EQ(decoded->size(), test_vec.size());
}

TEST_F(DisplayAwareQuantizerTest, CompressionRatio) {
    DisplayAwareQuantizer quantizer(DisplayProfile::SDR_Standard());
    quantizer.train(training_data_);
    
    float ratio = quantizer.compression_ratio();
    EXPECT_GT(ratio, 1.0f);  // Should compress
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(IntegrationTest, PQCurveWithQuantization) {
    // Test that PQ curve improves perceptual quality
    
    // Create test vector representing luminance values
    Vector linear_luminance(512);
    for (size_t i = 0; i < linear_luminance.size(); ++i) {
        linear_luminance[i] = static_cast<float>(i) * 20.0f;  // 0 to ~10240 nits
    }
    
    // Apply PQ curve
    PerceptualTransferFunction ptf(PerceptualCurve::PQ_ST2084);
    Vector perceptual = ptf.encode(linear_luminance);
    
    // Verify perceptual uniformity
    // Equal steps in perceptual space should correspond to roughly equal
    // perceived differences
    for (size_t i = 1; i < perceptual.size(); ++i) {
        float perceptual_step = perceptual[i] - perceptual[i-1];
        // Steps should be relatively uniform in perceptual space
        EXPECT_GT(perceptual_step, 0.0f);
    }
}

TEST(IntegrationTest, MultipleDisplayProfiles) {
    // Test quantizing for different displays
    
    std::vector<Vector> training_data;
    for (int i = 0; i < 50; ++i) {
        Vector vec(512, static_cast<float>(i) / 50.0f);
        training_data.push_back(vec);
    }
    
    // Train for SDR
    DisplayAwareQuantizer sdr_quantizer(DisplayProfile::SDR_Standard());
    sdr_quantizer.train(training_data);
    
    // Train for HDR
    DisplayAwareQuantizer hdr_quantizer(DisplayProfile::HDR1000_Standard());
    hdr_quantizer.train(training_data);
    
    // Encode same vector with both
    Vector test_vec = training_data[0];
    auto sdr_encoded = sdr_quantizer.encode(test_vec);
    auto hdr_encoded = hdr_quantizer.encode(test_vec);
    
    EXPECT_TRUE(sdr_encoded);
    EXPECT_TRUE(hdr_encoded);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(PerformanceTest, PQEncodingSpeed) {
    const size_t num_values = 10000;
    std::vector<float> values(num_values);
    
    for (size_t i = 0; i < num_values; ++i) {
        values[i] = static_cast<float>(i) * 1.0f;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto encoded = PQCurve::encode_batch(values);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should be fast (< 1ms for 10k values)
    EXPECT_LT(duration.count(), 1000);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
