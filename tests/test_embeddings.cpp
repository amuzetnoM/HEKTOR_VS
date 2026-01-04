// ============================================================================
// VectorDB Tests - Embeddings
// ============================================================================

#include <gtest/gtest.h>
#ifdef VDB_USE_ONNX_RUNTIME
#include "vdb/embeddings/onnx_runtime.hpp"
#include "vdb/embeddings/text.hpp"
#include "vdb/embeddings/image.hpp"
#endif

namespace vdb::test {

#ifdef VDB_USE_ONNX_RUNTIME
using namespace vdb::embeddings;

// ============================================================================
// Tokenizer Tests
// ============================================================================

class TokenizerTest : public ::testing::Test {
protected:
    // These tests require vocabulary file - skip if not available
    bool vocab_available() const {
        // Check if vocab.txt exists in expected location
        return false;  // Placeholder - adjust for your setup
    }
};

TEST_F(TokenizerTest, DISABLED_BasicTokenization) {
    // Disabled until vocab file is available
    // Tokenizer tok("path/to/vocab.txt");
    // auto tokens = tok.encode("Hello world", 32);
    // EXPECT_GT(tokens.size(), 0);
}

// ============================================================================
// ImagePreprocessor Tests
// ============================================================================

TEST(ImagePreprocessorTest, OutputSize) {
    ImagePreprocessor prep(Size{224, 224});
    EXPECT_EQ(prep.output_size(), 3 * 224 * 224);
}

TEST(ImagePreprocessorTest, ProcessSimpleImage) {
    ImagePreprocessor prep(Size{4, 4});
    
    // Create a simple 8x8 RGB image
    std::vector<uint8_t> image(8 * 8 * 3, 128);
    
    auto result = prep.process(image.data(), 8, 8);
    
    EXPECT_EQ(result.size(), 3 * 4 * 4);
}

// ============================================================================
// Device Detection Tests
// ============================================================================

TEST(DeviceTest, DetectDevice) {
    Device device = detect_best_device();
    
    // Should at least return CPU
    EXPECT_TRUE(device == Device::CPU || 
                device == Device::CUDA || 
                device == Device::DirectML);
}

TEST(DeviceTest, DeviceName) {
    EXPECT_EQ(device_name(Device::CPU), "CPU");
    EXPECT_EQ(device_name(Device::CUDA), "CUDA");
    EXPECT_EQ(device_name(Device::DirectML), "DirectML");
}

#else
// Dummy test when ONNX Runtime is not available
TEST(EmbeddingsTest, OnnxRuntimeNotAvailable) {
    EXPECT_TRUE(true) << "ONNX Runtime tests disabled (VDB_USE_ONNX_RUNTIME not defined)";
}
#endif

} // namespace vdb::test
