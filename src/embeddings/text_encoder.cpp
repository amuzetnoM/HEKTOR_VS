// ============================================================================
// VectorDB - Text Encoder Implementation
// Uses sentence-transformers/all-MiniLM-L6-v2 for semantic text embeddings
// ============================================================================

#include "vdb/embeddings/text.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace vdb::embeddings {

// ============================================================================
// TextEncoder Implementation
// ============================================================================

TextEncoder::~TextEncoder() = default;

TextEncoder::TextEncoder(TextEncoder&& other) noexcept
    : config_(std::move(other.config_))
    , session_(std::move(other.session_))
    , tokenizer_(std::move(other.tokenizer_))
    , ready_(other.ready_)
{
    other.ready_ = false;
}

TextEncoder& TextEncoder::operator=(TextEncoder&& other) noexcept {
    if (this != &other) {
        config_ = std::move(other.config_);
        session_ = std::move(other.session_);
        tokenizer_ = std::move(other.tokenizer_);
        ready_ = other.ready_;
        other.ready_ = false;
    }
    return *this;
}

Result<void> TextEncoder::init(const TextEncoderConfig& config) {
    config_ = config;
    
    // Validate paths
    if (!fs::exists(config_.model_path)) {
        return Error{ErrorCode::IoError, 
                    "Text encoder model not found: " + config_.model_path.string()};
    }
    
    if (!fs::exists(config_.vocab_path)) {
        return Error{ErrorCode::IoError,
                    "Vocabulary file not found: " + config_.vocab_path.string()};
    }
    
    // Load tokenizer
    try {
        tokenizer_ = std::make_unique<Tokenizer>(config_.vocab_path);
    } catch (const std::exception& e) {
        return Error{ErrorCode::InvalidData, 
                    std::string("Failed to load tokenizer: ") + e.what()};
    }
    
    // Load ONNX model
    try {
        session_ = std::make_unique<OnnxSession>(config_.model_path, config_.device);
    } catch (const std::exception& e) {
        return Error{ErrorCode::InvalidData,
                    std::string("Failed to load ONNX model: ") + e.what()};
    }
    
    ready_ = true;
    return {};
}

Result<std::vector<float>> TextEncoder::encode(std::string_view text) {
    if (!ready_) {
        return Error{ErrorCode::InvalidState, "TextEncoder not initialized"};
    }
    
    // Tokenize
    auto input_ids = tokenizer_->encode(text, config_.max_seq_length, true);
    
    // Create attention mask (1 for real tokens, 0 for padding)
    std::vector<int64_t> attention_mask(input_ids.size());
    for (size_t i = 0; i < input_ids.size(); ++i) {
        attention_mask[i] = (input_ids[i] != 0) ? 1 : 0;  // PAD token is 0
    }
    
    // Token type IDs (all zeros for single sequence)
    std::vector<int64_t> token_type_ids(input_ids.size(), 0);
    
    try {
        // Create input tensors
        auto& mem_info = session_->memory_info();
        std::vector<int64_t> shape = {1, static_cast<int64_t>(input_ids.size())};
        
        std::vector<Ort::Value> inputs;
        inputs.reserve(3);
        
        inputs.push_back(Ort::Value::CreateTensor<int64_t>(
            mem_info,
            input_ids.data(),
            input_ids.size(),
            shape.data(),
            shape.size()
        ));
        
        inputs.push_back(Ort::Value::CreateTensor<int64_t>(
            mem_info,
            attention_mask.data(),
            attention_mask.size(),
            shape.data(),
            shape.size()
        ));
        
        inputs.push_back(Ort::Value::CreateTensor<int64_t>(
            mem_info,
            token_type_ids.data(),
            token_type_ids.size(),
            shape.data(),
            shape.size()
        ));
        
        // Run inference
        auto outputs = session_->run(inputs);
        
        if (outputs.empty()) {
            return Error{ErrorCode::InvalidData, "Model returned no outputs"};
        }
        
        // Get output tensor info
        auto& output = outputs[0];
        auto type_info = output.GetTensorTypeAndShapeInfo();
        auto output_shape = type_info.GetShape();
        
        // Output shape should be [1, seq_len, hidden_dim]
        if (output_shape.size() != 3) {
            return Error{ErrorCode::InvalidData, "Unexpected output shape"};
        }
        
        size_t seq_len = static_cast<size_t>(output_shape[1]);
        Dim hidden_dim = static_cast<Dim>(output_shape[2]);
        
        const float* output_data = output.GetTensorData<float>();
        
        // Mean pooling
        auto embedding = mean_pooling(output_data, attention_mask, seq_len, hidden_dim);
        
        // Normalize if configured
        if (config_.normalize_embeddings) {
            normalize(embedding);
        }
        
        return embedding;
        
    } catch (const Ort::Exception& e) {
        return Error{ErrorCode::InvalidData, 
                    std::string("ONNX inference failed: ") + e.what()};
    }
}

Result<std::vector<std::vector<float>>> TextEncoder::encode_batch(
    const std::vector<std::string>& texts) {
    
    std::vector<std::vector<float>> results;
    results.reserve(texts.size());
    
    // For simplicity, process one at a time
    // A more optimized version would batch multiple texts together
    for (const auto& text : texts) {
        auto result = encode(text);
        if (!result) {
            return result.error();
        }
        results.push_back(std::move(*result));
    }
    
    return results;
}

std::vector<float> TextEncoder::mean_pooling(
    const float* token_embeddings,
    const std::vector<int64_t>& attention_mask,
    size_t seq_length,
    Dim hidden_dim) {
    
    std::vector<float> result(hidden_dim, 0.0f);
    float mask_sum = 0.0f;
    
    // Sum embeddings weighted by attention mask
    for (size_t t = 0; t < seq_length; ++t) {
        float mask = static_cast<float>(attention_mask[t]);
        mask_sum += mask;
        
        for (Dim d = 0; d < hidden_dim; ++d) {
            result[d] += token_embeddings[t * hidden_dim + d] * mask;
        }
    }
    
    // Divide by sum of mask (number of real tokens)
    if (mask_sum > 0.0f) {
        for (Dim d = 0; d < hidden_dim; ++d) {
            result[d] /= mask_sum;
        }
    }
    
    return result;
}

void TextEncoder::normalize(std::vector<float>& vec) {
    float norm = 0.0f;
    for (float v : vec) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    
    if (norm > 1e-12f) {
        for (float& v : vec) {
            v /= norm;
        }
    }
}

} // namespace vdb::embeddings
