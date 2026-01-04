// ============================================================================
// VectorDB - LLM Engine Implementation
// llama.cpp integration for local text generation
// ============================================================================

#include "vdb/llm/llm_engine.hpp"

// llama.cpp headers (will be available after adding as dependency)
#ifdef VDB_USE_LLAMA_CPP
#include "llama.h"
#include "common.h"
#endif

#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <thread>

namespace vdb::llm
{

    // ============================================================================
    // Chat Templates
    // ============================================================================

    std::string apply_chat_template(
        const std::vector<Message> &messages,
        std::string_view template_name)
    {
        std::ostringstream result;

        if (template_name == "chatml")
        {
            // ChatML format (used by many models including Mistral, Phi-3)
            for (const auto &msg : messages)
            {
                switch (msg.role)
                {
                case Role::System:
                    result << "<|im_start|>system\n"
                           << msg.content << "<|im_end|>\n";
                    break;
                case Role::User:
                    result << "<|im_start|>user\n"
                           << msg.content << "<|im_end|>\n";
                    break;
                case Role::Assistant:
                    result << "<|im_start|>assistant\n"
                           << msg.content << "<|im_end|>\n";
                    break;
                }
            }
            result << "<|im_start|>assistant\n";
        }
        else if (template_name == "llama3")
        {
            // Llama 3 format
            result << "<|begin_of_text|>";
            for (const auto &msg : messages)
            {
                switch (msg.role)
                {
                case Role::System:
                    result << "<|start_header_id|>system<|end_header_id|>\n\n"
                           << msg.content << "<|eot_id|>";
                    break;
                case Role::User:
                    result << "<|start_header_id|>user<|end_header_id|>\n\n"
                           << msg.content << "<|eot_id|>";
                    break;
                case Role::Assistant:
                    result << "<|start_header_id|>assistant<|end_header_id|>\n\n"
                           << msg.content << "<|eot_id|>";
                    break;
                }
            }
            result << "<|start_header_id|>assistant<|end_header_id|>\n\n";
        }
        else if (template_name == "llama2")
        {
            // Llama 2 format
            bool has_system = false;
            for (const auto &msg : messages)
            {
                if (msg.role == Role::System)
                {
                    result << "[INST] <<SYS>>\n"
                           << msg.content << "\n<</SYS>>\n\n";
                    has_system = true;
                    break;
                }
            }

            bool first_user = true;
            for (const auto &msg : messages)
            {
                if (msg.role == Role::System)
                    continue;

                switch (msg.role)
                {
                case Role::User:
                    if (first_user && has_system)
                    {
                        result << msg.content << " [/INST] ";
                        first_user = false;
                    }
                    else
                    {
                        result << "[INST] " << msg.content << " [/INST] ";
                    }
                    break;
                case Role::Assistant:
                    result << msg.content << " </s>";
                    break;
                default:
                    break;
                }
            }
        }
        else if (template_name == "mistral")
        {
            // Mistral Instruct format
            for (const auto &msg : messages)
            {
                switch (msg.role)
                {
                case Role::System:
                    result << "[INST] " << msg.content << "\n\n";
                    break;
                case Role::User:
                    result << "[INST] " << msg.content << " [/INST]";
                    break;
                case Role::Assistant:
                    result << msg.content << "</s>";
                    break;
                }
            }
        }
        else
        {
            // Default: simple newline-separated
            for (const auto &msg : messages)
            {
                switch (msg.role)
                {
                case Role::System:
                    result << "System: " << msg.content << "\n";
                    break;
                case Role::User:
                    result << "User: " << msg.content << "\n";
                    break;
                case Role::Assistant:
                    result << "Assistant: " << msg.content << "\n";
                    break;
                }
            }
            result << "Assistant: ";
        }

        return result.str();
    }

    // ============================================================================
    // GGUF Metadata Reader
    // ============================================================================

    std::optional<GGUFMetadata> read_gguf_metadata(const fs::path &model_path)
    {
        if (!fs::exists(model_path))
        {
            return std::nullopt;
        }

        std::ifstream file(model_path, std::ios::binary);
        if (!file)
        {
            return std::nullopt;
        }

        // Read GGUF magic number
        char magic[4];
        file.read(magic, 4);
        if (std::strncmp(magic, "GGUF", 4) != 0)
        {
            return std::nullopt;
        }

        GGUFMetadata meta;
        meta.file_size = fs::file_size(model_path);
        meta.name = model_path.stem().string();

        // Basic heuristics from filename for now
        // Full GGUF parsing would require reading the key-value pairs
        std::string filename = model_path.filename().string();
        std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

        if (filename.find("q4_k_m") != std::string::npos)
            meta.quantization = "Q4_K_M";
        else if (filename.find("q4_k_s") != std::string::npos)
            meta.quantization = "Q4_K_S";
        else if (filename.find("q5_k_m") != std::string::npos)
            meta.quantization = "Q5_K_M";
        else if (filename.find("q5_k_s") != std::string::npos)
            meta.quantization = "Q5_K_S";
        else if (filename.find("q8_0") != std::string::npos)
            meta.quantization = "Q8_0";
        else if (filename.find("f16") != std::string::npos)
            meta.quantization = "F16";
        else
            meta.quantization = "Unknown";

        if (filename.find("llama") != std::string::npos)
            meta.architecture = "llama";
        else if (filename.find("mistral") != std::string::npos)
            meta.architecture = "mistral";
        else if (filename.find("phi") != std::string::npos)
            meta.architecture = "phi";
        else if (filename.find("qwen") != std::string::npos)
            meta.architecture = "qwen";
        else
            meta.architecture = "unknown";

        // Default context lengths (will be overridden by actual model metadata)
        meta.context_length = 4096;
        meta.embedding_length = 4096;
        meta.vocab_size = 32000;

        return meta;
    }

    // ============================================================================
    // Find GGUF models in directory
    // ============================================================================

    std::vector<fs::path> find_gguf_models(const fs::path &directory)
    {
        std::vector<fs::path> models;

        if (!fs::exists(directory) || !fs::is_directory(directory))
        {
            return models;
        }

        for (const auto &entry : fs::directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".gguf")
                {
                    models.push_back(entry.path());
                }
            }
        }

        // Sort by filename
        std::sort(models.begin(), models.end());
        return models;
    }

    // ============================================================================
    // LLaMA.cpp Engine Implementation
    // ============================================================================

#ifdef VDB_USE_LLAMA_CPP

    class LlamaEngine : public LLMEngine
    {
    public:
        LlamaEngine() = default;
        ~LlamaEngine() override
        {
            unload();
        }

        bool load(const LLMConfig &config) override
        {
            unload();

            config_ = config;

            // Initialize llama backend
            llama_backend_init();

            // Model parameters
            llama_model_params model_params = llama_model_default_params();
            model_params.n_gpu_layers = config.n_gpu_layers;
            model_params.use_mmap = config.use_mmap;
            model_params.use_mlock = config.use_mlock;

            // Load model
            model_ = llama_load_model_from_file(config.model_path.string().c_str(), model_params);
            if (!model_)
            {
                return false;
            }

            // Context parameters
            llama_context_params ctx_params = llama_context_default_params();
            ctx_params.n_ctx = config.n_ctx;
            ctx_params.n_batch = config.n_batch;
            ctx_params.n_threads = config.n_threads > 0 ? config.n_threads : std::thread::hardware_concurrency();
            ctx_params.n_threads_batch = ctx_params.n_threads;

            if (config.rope_freq_base > 0)
            {
                ctx_params.rope_freq_base = config.rope_freq_base;
            }
            if (config.rope_freq_scale > 0)
            {
                ctx_params.rope_freq_scale = config.rope_freq_scale;
            }

            // Create context
            ctx_ = llama_new_context_with_model(model_, ctx_params);
            if (!ctx_)
            {
                llama_free_model(model_);
                model_ = nullptr;
                return false;
            }

            return true;
        }

        bool is_loaded() const override
        {
            return model_ != nullptr && ctx_ != nullptr;
        }

        void unload() override
        {
            if (ctx_)
            {
                llama_free(ctx_);
                ctx_ = nullptr;
            }
            if (model_)
            {
                llama_free_model(model_);
                model_ = nullptr;
            }
        }

        std::string model_name() const override
        {
            if (!model_)
                return "";
            return config_.model_path.stem().string();
        }

        int context_size() const override
        {
            return ctx_ ? llama_n_ctx(ctx_) : 0;
        }

        int vocab_size() const override
        {
            return model_ ? llama_n_vocab(model_) : 0;
        }

        std::string generate(std::string_view prompt, const GenerationParams &params) override
        {
            if (!is_loaded())
                return "";

            [[maybe_unused]] auto start_time = std::chrono::high_resolution_clock::now();

            // Tokenize prompt
            std::vector<llama_token> tokens(prompt.size() + 32);
            int n_tokens = llama_tokenize(model_, prompt.data(), prompt.size(),
                                          tokens.data(), tokens.size(), true, false);
            if (n_tokens < 0)
            {
                tokens.resize(-n_tokens);
                n_tokens = llama_tokenize(model_, prompt.data(), prompt.size(),
                                          tokens.data(), tokens.size(), true, false);
            }
            tokens.resize(n_tokens);

            // Process prompt using batch_get_one for simplicity
            llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);

            if (llama_decode(ctx_, batch) != 0)
            {
                return "";
            }

            // Create sampler for this generation
            llama_sampler *smpl = create_sampler(params);

            // Generate tokens
            std::string result;
            int n_generated = 0;
            int n_cur = n_tokens;

            while (n_generated < params.max_tokens)
            {
                // Sample next token
                llama_token new_token = sample_token(smpl, params, n_cur);

                // Check for EOS
                if (llama_token_is_eog(model_, new_token))
                {
                    break;
                }

                // Decode token to text
                char buf[256];
                int len = llama_token_to_piece(model_, new_token, buf, sizeof(buf), 0, true);
                if (len > 0)
                {
                    std::string_view piece(buf, len);
                    result += piece;

                    // Check stop sequences
                    bool should_stop = false;
                    for (const auto &stop : params.stop_sequences)
                    {
                        if (result.size() >= stop.size() &&
                            result.substr(result.size() - stop.size()) == stop)
                        {
                            result.resize(result.size() - stop.size());
                            should_stop = true;
                            break;
                        }
                    }

                    if (should_stop)
                        break;

                    // Streaming callback
                    if (params.on_token && !params.on_token(piece))
                    {
                        break;
                    }
                }

                // Prepare next token for decode
                llama_batch next_batch = llama_batch_get_one(&new_token, 1);

                if (llama_decode(ctx_, next_batch) != 0)
                {
                    break;
                }

                ++n_cur;
                ++n_generated;
            }

            llama_sampler_free(smpl);
            return result;
        }

        ChatCompletionResult chat(const std::vector<Message> &messages,
                                  const GenerationParams &params) override
        {
            ChatCompletionResult result;

            auto start_time = std::chrono::high_resolution_clock::now();

            // Detect chat template from model architecture
            std::string template_name = "chatml"; // Default
            std::string model_lower = model_name();
            std::transform(model_lower.begin(), model_lower.end(), model_lower.begin(), ::tolower);

            if (model_lower.find("llama-3") != std::string::npos ||
                model_lower.find("llama3") != std::string::npos)
            {
                template_name = "llama3";
            }
            else if (model_lower.find("llama-2") != std::string::npos ||
                     model_lower.find("llama2") != std::string::npos)
            {
                template_name = "llama2";
            }
            else if (model_lower.find("mistral") != std::string::npos)
            {
                template_name = "mistral";
            }

            // Apply chat template
            std::string prompt = apply_chat_template(messages, template_name);

            // Count prompt tokens
            result.tokens_prompt = count_tokens(prompt);

            // Generate
            result.content = generate(prompt, params);
            result.tokens_generated = count_tokens(result.content);

            auto end_time = std::chrono::high_resolution_clock::now();
            result.generation_time_ms = std::chrono::duration<double, std::milli>(
                                            end_time - start_time)
                                            .count();

            return result;
        }

        int count_tokens(std::string_view text) const override
        {
            if (!model_)
                return 0;

            std::vector<llama_token> tokens(text.size() + 32);
            int n = llama_tokenize(model_, text.data(), text.size(),
                                   tokens.data(), tokens.size(), false, false);
            return n > 0 ? n : 0;
        }

        std::optional<std::vector<float>> embed(std::string_view text) const override
        {
            // TODO: Implement embeddings if model supports
            return std::nullopt;
        }

    private:
        llama_sampler *create_sampler(const GenerationParams &params)
        {
            auto sparams = llama_sampler_chain_default_params();
            llama_sampler *smpl = llama_sampler_chain_init(sparams);

            if (params.temperature <= 0)
            {
                // Greedy sampling
                llama_sampler_chain_add(smpl, llama_sampler_init_greedy());
            }
            else
            {
                // Top-k -> Top-p -> Temperature -> Distribution sampling
                llama_sampler_chain_add(smpl, llama_sampler_init_top_k(params.top_k));
                llama_sampler_chain_add(smpl, llama_sampler_init_top_p(params.top_p, 1));
                llama_sampler_chain_add(smpl, llama_sampler_init_temp(params.temperature));
                llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
            }

            return smpl;
        }

        llama_token sample_token(llama_sampler *smpl, [[maybe_unused]] const GenerationParams &params, [[maybe_unused]] int pos)
        {
            llama_token token = llama_sampler_sample(smpl, ctx_, -1);
            llama_sampler_accept(smpl, token);
            return token;
        }

        LLMConfig config_;
        llama_model *model_ = nullptr;
        llama_context *ctx_ = nullptr;
        llama_sampler *sampler_ = nullptr;
    };

    std::unique_ptr<LLMEngine> create_llm_engine()
    {
        return std::make_unique<LlamaEngine>();
    }

#else

    // Stub implementation when llama.cpp is not available
    class StubLLMEngine : public LLMEngine
    {
    public:
        bool load(const LLMConfig &) override { return false; }
        bool is_loaded() const override { return false; }
        void unload() override {}
        std::string model_name() const override { return ""; }
        int context_size() const override { return 0; }
        int vocab_size() const override { return 0; }
        std::string generate(std::string_view, const GenerationParams &) override { return ""; }
        ChatCompletionResult chat(const std::vector<Message> &, const GenerationParams &) override
        {
            return {};
        }
        int count_tokens(std::string_view) const override { return 0; }
        std::optional<std::vector<float>> embed(std::string_view) const override { return std::nullopt; }
    };

    std::unique_ptr<LLMEngine> create_llm_engine()
    {
        return std::make_unique<StubLLMEngine>();
    }

#endif

} // namespace vdb::llm
