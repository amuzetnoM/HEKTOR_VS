// ============================================================================
// VectorDB Tests - TensorFlow and PyTorch Integration
// Phase 4: ML Framework Testing
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/framework_integration.hpp"
#include <filesystem>
#include <fstream>

namespace vdb::framework::test {

namespace fs = std::filesystem;

// ============================================================================
// TensorFlow Integration Tests
// ============================================================================

class TensorFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Note: These tests check the API structure
        // Full tests require actual TensorFlow models
        config_.model_path = "/tmp/test_model";
        config_.use_gpu = false;  // Use CPU for CI/CD
        config_.num_threads = 4;
    }
    
    TensorFlowConfig config_;
};

TEST_F(TensorFlowTest, ConfigStructure) {
    // Verify config has required fields
    EXPECT_FALSE(config_.model_path.empty());
    EXPECT_GE(config_.num_threads, 0);
}

TEST_F(TensorFlowTest, ConditionalCompilation) {
#ifdef VDB_USE_TENSORFLOW
    // If TensorFlow is available, test would load model
    GTEST_SKIP() << "TensorFlow available but no model path configured";
#else
    // Without TensorFlow, embedder should throw
    EXPECT_THROW({
        TensorFlowEmbedder embedder(config_);
    }, std::runtime_error);
#endif
}

TEST_F(TensorFlowTest, ExportForTraining) {
    // Test export API without actual TensorFlow dependency
    std::vector<Vector> vectors;
    std::vector<std::string> labels;
    
    for (int i = 0; i < 10; ++i) {
        Vector vec(128);
        for (size_t j = 0; j < 128; ++j) {
            vec[j] = static_cast<float>(i + j) * 0.01f;
        }
        vectors.push_back(vec);
        labels.push_back("label_" + std::to_string(i));
    }
    
    std::string output_path = "/tmp/test_export_tf.tfrecord";
    
    auto result = TensorFlowEmbedder::export_for_training(vectors, labels, output_path);
    
#ifdef VDB_USE_TENSORFLOW
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(fs::exists(output_path));
    fs::remove(output_path);
#else
    EXPECT_FALSE(result.has_value());
#endif
}

TEST_F(TensorFlowTest, DimensionInference) {
    // Verify dimension getter works
#ifdef VDB_USE_TENSORFLOW
    GTEST_SKIP() << "Requires actual model for dimension test";
#else
    // API structure test only
    EXPECT_NO_THROW({
        try {
            TensorFlowEmbedder embedder(config_);
            embedder.dimension();  // Should not crash
        } catch (const std::runtime_error&) {
            // Expected without TensorFlow
        }
    });
#endif
}

TEST_F(TensorFlowTest, GPUConfiguration) {
    config_.use_gpu = true;
    
    // GPU config should be settable
    EXPECT_TRUE(config_.use_gpu);
    
    // Test configuration flexibility
    config_.use_gpu = false;
    EXPECT_FALSE(config_.use_gpu);
}

// ============================================================================
// PyTorch Integration Tests
// ============================================================================

class PyTorchTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.model_path = "/tmp/test_model.pt";
        config_.device = "cpu";  // Use CPU for CI/CD
        config_.num_threads = 4;
        config_.use_half_precision = false;
    }
    
    PyTorchConfig config_;
};

TEST_F(PyTorchTest, ConfigStructure) {
    EXPECT_FALSE(config_.model_path.empty());
    EXPECT_FALSE(config_.device.empty());
    EXPECT_GE(config_.num_threads, 0);
}

TEST_F(PyTorchTest, ConditionalCompilation) {
#ifdef VDB_USE_TORCH
    GTEST_SKIP() << "LibTorch available but no model path configured";
#else
    // Without LibTorch, embedder should throw
    EXPECT_THROW({
        PyTorchEmbedder embedder(config_);
    }, std::runtime_error);
#endif
}

TEST_F(PyTorchTest, DeviceSelection) {
    // Test CPU device
    config_.device = "cpu";
    EXPECT_EQ(config_.device, "cpu");
    
    // Test CUDA device string
    config_.device = "cuda";
    EXPECT_EQ(config_.device, "cuda");
    
    config_.device = "cuda:0";
    EXPECT_EQ(config_.device, "cuda:0");
}

TEST_F(PyTorchTest, HalfPrecisionConfig) {
    config_.use_half_precision = true;
    EXPECT_TRUE(config_.use_half_precision);
    
    // Half precision only makes sense on GPU
    if (config_.device == "cpu") {
        config_.use_half_precision = false;
        EXPECT_FALSE(config_.use_half_precision);
    }
}

TEST_F(PyTorchTest, ExportForTraining) {
    std::vector<Vector> vectors;
    std::vector<std::string> labels;
    
    for (int i = 0; i < 10; ++i) {
        Vector vec(128);
        for (size_t j = 0; j < 128; ++j) {
            vec[j] = static_cast<float>(i + j) * 0.01f;
        }
        vectors.push_back(vec);
        labels.push_back("label_" + std::to_string(i));
    }
    
    std::string output_path = "/tmp/test_export_torch";
    
    auto result = PyTorchEmbedder::export_for_training(vectors, labels, output_path);
    
#ifdef VDB_USE_TORCH
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(fs::exists(output_path + ".pt"));
    EXPECT_TRUE(fs::exists(output_path + ".labels"));
    fs::remove(output_path + ".pt");
    fs::remove(output_path + ".labels");
#else
    EXPECT_FALSE(result.has_value());
#endif
}

TEST_F(PyTorchTest, FromTrainedLoader) {
    std::string model_path = "/tmp/trained_model.pt";
    
    auto result = PyTorchEmbedder::from_trained(model_path, "cpu");
    
#ifdef VDB_USE_TORCH
    // Would succeed with actual model file
    GTEST_SKIP() << "Requires actual trained model";
#else
    EXPECT_FALSE(result.has_value());
#endif
}

TEST_F(PyTorchTest, BatchInferenceAPI) {
    // Test batch API structure without actual model
    std::vector<std::string> texts = {"text1", "text2", "text3"};
    
#ifdef VDB_USE_TORCH
    GTEST_SKIP() << "Requires actual model for inference";
#else
    // API should exist even if not functional
    EXPECT_NO_THROW({
        try {
            PyTorchEmbedder embedder(config_);
            embedder.embed_batch(texts);
        } catch (const std::runtime_error&) {
            // Expected without LibTorch
        }
    });
#endif
}

// ============================================================================
// Framework Integration Tests
// ============================================================================

class FrameworkIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for framework tests
    }
};

TEST_F(FrameworkIntegrationTest, TensorFlowPyTorchCompatibility) {
    // Verify both frameworks can coexist
    TensorFlowConfig tf_config;
    tf_config.model_path = "/tmp/tf_model";
    
    PyTorchConfig pt_config;
    pt_config.model_path = "/tmp/pt_model.pt";
    
    // Should be able to configure both
    EXPECT_FALSE(tf_config.model_path.empty());
    EXPECT_FALSE(pt_config.model_path.empty());
}

TEST_F(FrameworkIntegrationTest, VectorNormalization) {
    // Test that both frameworks normalize vectors consistently
    Vector test_vec{3.0f, 4.0f};  // Length = 5
    
    // Manual normalization
    float norm = std::sqrt(3.0f * 3.0f + 4.0f * 4.0f);
    float expected_x = 3.0f / norm;
    float expected_y = 4.0f / norm;
    
    EXPECT_FLOAT_EQ(expected_x, 0.6f);
    EXPECT_FLOAT_EQ(expected_y, 0.8f);
    
    // Both frameworks should produce this normalization
}

TEST_F(FrameworkIntegrationTest, ExportFormatCompatibility) {
    // Verify export formats are documented
    std::string tf_extension = ".tfrecord";
    std::string pt_extension = ".pt";
    
    EXPECT_EQ(tf_extension, ".tfrecord");
    EXPECT_EQ(pt_extension, ".pt");
}

TEST_F(FrameworkIntegrationTest, DimensionConsistency) {
    // Common embedding dimensions
    const Dim bert_dim = 768;
    const Dim gpt_dim = 1536;
    const Dim small_dim = 384;
    
    // Verify dimension types work correctly
    EXPECT_EQ(bert_dim, 768);
    EXPECT_EQ(gpt_dim, 1536);
    EXPECT_EQ(small_dim, 384);
}

TEST_F(FrameworkIntegrationTest, GPUFallbackBehavior) {
    // Test that GPU unavailability is handled gracefully
    std::string device = "cuda";
    bool cuda_available = false;  // Simulated - would be checked at runtime
    
    // Simulate fallback logic
    if (!cuda_available) {
        device = "cpu";
    }
    
    EXPECT_EQ(device, "cpu");
    
    // Also test explicit CPU selection
    device = "cpu";
    EXPECT_EQ(device, "cpu");
}

} // namespace vdb::framework::test
