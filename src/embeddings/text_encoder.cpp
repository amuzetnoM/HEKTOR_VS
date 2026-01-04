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
        return std::unexpected(Error{ErrorCode::IoError, 
                    "Text encoder model not found: " + config_.model_path.string()});
    }
    
    if (!fs::exists(config_.vocab_path)) {
        return std::unexpected(Error{ErrorCode::IoError,
                    "Vocabulary file not found: " + config_.vocab_path.string()});
    }
    
    // Load tokenizer
    try {
        tokenizer_ = std::make_unique<Tokenizer>(config_.vocab_path);
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, 
                    std::string("Failed to load tokenizer: ") + e.what()});
    }
    
    // Load ONNX model
    try {
        session_ = std::make_unique<OnnxSession>(config_.model_path, config_.device);
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                    std::string("Failed to load ONNX model: ") + e.what()});
    }
    
    ready_ = true;
    return {};
}

Result<std::vector<float>> TextEncoder::encode(std::string_view text) {
    if (!ready_) {
        return std::unexpected(Error{ErrorCode::InvalidState, "TextEncoder not initialized"});
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
            return std::unexpected(Error{ErrorCode::InvalidData, "Model returned no outputs"});
        }
        
        // Get output tensor info
        auto& output = outputs[0];
        auto type_info = output.GetTensorTypeAndShapeInfo();
        auto output_shape = type_info.GetShape();
        
        // Output shape should be [1, seq_len, hidden_dim]
        if (output_shape.size() != 3) {
            return std::unexpected(Error{ErrorCode::InvalidData, "Unexpected output shape"});
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
        return std::unexpected(Error{ErrorCode::InvalidData, 
                    std::string("ONNX inference failed: ") + e.what()});
    }
}

Result<std::vector<std::vector<float>>> TextEncoder::encode_batch(
    const std::vector<std::string>& texts) {
    
    if (!ready_) {
        return std::unexpected(Error{ErrorCode::InvalidState, "TextEncoder not initialized"});
    }
    
    if (texts.empty()) {
        return std::vector<std::vector<float>>{};
    }
    
    // Tokenize all texts and find max length for padding
    std::vector<std::vector<int64_t>> all_input_ids;
    std::vector<std::vector<int64_t>> all_attention_masks;
    std::vector<std::vector<int64_t>> all_token_type_ids;
    
    size_t max_length = 0;
    all_input_ids.reserve(texts.size());
    all_attention_masks.reserve(texts.size());
    all_token_type_ids.reserve(texts.size());
    
    for (const auto& text : texts) {
        auto input_ids = tokenizer_->encode(text, config_.max_seq_length, true);
        max_length = std::max(max_length, input_ids.size());
        
        std::vector<int64_t> attention_mask;
        attention_mask.reserve(input_ids.size());
        for (size_t i = 0; i < input_ids.size(); ++i) {
            attention_mask.push_back((input_ids[i] != 0) ? 1 : 0);
        }
        
        std::vector<int64_t> token_type_ids(input_ids.size(), 0);
        
        all_input_ids.push_back(std::move(input_ids));
        all_attention_masks.push_back(std::move(attention_mask));
        all_token_type_ids.push_back(std::move(token_type_ids));
    }
    
    // Pad all sequences to max_length
    for (size_t i = 0; i < texts.size(); ++i) {
        all_input_ids[i].resize(max_length, 0);
        all_attention_masks[i].resize(max_length, 0);
        all_token_type_ids[i].resize(max_length, 0);
    }
    
    // Flatten to batch format [batch_size, max_length]
    std::vector<int64_t> batch_input_ids;
    std::vector<int64_t> batch_attention_masks;
    std::vector<int64_t> batch_token_type_ids;
    
    batch_input_ids.reserve(texts.size() * max_length);
    batch_attention_masks.reserve(texts.size() * max_length);
    batch_token_type_ids.reserve(texts.size() * max_length);
    
    for (size_t i = 0; i < texts.size(); ++i) {
        batch_input_ids.insert(batch_input_ids.end(), 
            all_input_ids[i].begin(), all_input_ids[i].end());
        batch_attention_masks.insert(batch_attention_masks.end(),
            all_attention_masks[i].begin(), all_attention_masks[i].end());
        batch_token_type_ids.insert(batch_token_type_ids.end(),
            all_token_type_ids[i].begin(), all_token_type_ids[i].end());
    }
    
    try {
        // Create batch input tensors
        auto& mem_info = session_->memory_info();
        std::vector<int64_t> shape = {
            static_cast<int64_t>(texts.size()), 
            static_cast<int64_t>(max_length)
        };
        
        std::vector<Ort::Value> inputs;
        inputs.reserve(3);
        
        inputs.push_back(Ort::Value::CreateTensor<int64_t>(
            mem_info,
            batch_input_ids.data(),
            batch_input_ids.size(),
            shape.data(),
            shape.size()
        ));
        
        inputs.push_back(Ort::Value::CreateTensor<int64_t>(
            mem_info,
            batch_attention_masks.data(),
            batch_attention_masks.size(),
            shape.data(),
            shape.size()
        ));
        
        inputs.push_back(Ort::Value::CreateTensor<int64_t>(
            mem_info,
            batch_token_type_ids.data(),
            batch_token_type_ids.size(),
            shape.data(),
            shape.size()
        ));
        
        // Run batch inference
        auto outputs = session_->run(inputs);
        
        if (outputs.empty()) {
            return std::unexpected(Error{ErrorCode::InvalidData, "Model returned no outputs"});
        }
        
        // Get output tensor info
        auto& output = outputs[0];
        auto type_info = output.GetTensorTypeAndShapeInfo();
        auto output_shape = type_info.GetShape();
        
        // Output shape should be [batch_size, seq_len, hidden_dim]
        if (output_shape.size() != 3) {
            return std::unexpected(Error{ErrorCode::InvalidData, "Unexpected output shape"});
        }
        
        size_t batch_size = static_cast<size_t>(output_shape[0]);
        size_t seq_len = static_cast<size_t>(output_shape[1]);
        Dim hidden_dim = static_cast<Dim>(output_shape[2]);
        
        const float* output_data = output.GetTensorData<float>();
        
        // Process each item in batch
        std::vector<std::vector<float>> results;
        results.reserve(batch_size);
        
        for (size_t b = 0; b < batch_size; ++b) {
            const float* batch_data = output_data + (b * seq_len * hidden_dim);
            auto embedding = mean_pooling(batch_data, all_attention_masks[b], seq_len, hidden_dim);
            
            if (config_.normalize_embeddings) {
                normalize(embedding);
            }
            
            results.push_back(std::move(embedding));
        }
        
        return results;
        
    } catch (const Ort::Exception& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, 
                    std::string("ONNX batch inference failed: ") + e.what()});
    }
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
