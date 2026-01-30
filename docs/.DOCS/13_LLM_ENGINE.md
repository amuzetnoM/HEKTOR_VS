---
title: "LLM Engine"
description: "Local text generation with llama.cpp"
version: "4.1.5"
last_updated: "2026-01-24"
sidebar_position: 13
category: "Feature"
---

![Version](https://img.shields.io/badge/version-4.1.5-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--24-green?style=flat-square)
![Type](https://img.shields.io/badge/type-advanced-red?style=flat-square)

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [Architecture](#architecture)
4. [Installation](#installation)
5. [Quick Start](#quick-start)
6. [Configuration](#configuration)
7. [API Reference](#api-reference)
8. [Usage Examples](#usage-examples)
9. [Supported Models](#supported-models)
10. [Performance](#performance)
11. [Best Practices](#best-practices)
12. [Troubleshooting](#troubleshooting)

---

## Overview

The LLM Engine provides **local text generation** capabilities through llama.cpp integration, enabling:

- **Local inference** without API calls or internet connectivity
- **GGUF model support** for Llama 2/3, Mistral, and other models
- **Streaming generation** with token-by-token callbacks
- **Chat completion** with conversation history
- **Flexible configuration** for temperature, sampling, and more

### Why Use LLM Engine?

| Benefit | Description |
|---------|-------------|
| **Privacy** | All processing happens locally, no data leaves your machine |
| **Cost** | No API fees, unlimited usage |
| **Speed** | Low latency for local inference |
| **Flexibility** | Support for multiple model formats and architectures |
| **Integration** | Seamless integration with Vector Studio's semantic search |

---

## Features

### Core Capabilities

- ✅ **Model Management**
  - Load/unload GGUF models
  - Auto-detect model metadata
  - Memory-mapped model loading for efficiency

- ✅ **Text Generation**
  - Configurable sampling (temperature, top_p, top_k)
  - Repetition penalty
  - Stop sequences
  - Max token limits

- ✅ **Streaming Support**
  - Token-by-token callbacks
  - Real-time generation feedback
  - Cancellation support

- ✅ **Chat Completion**
  - Multi-turn conversations
  - Message history management
  - Multiple chat templates (ChatML, Llama2, Llama3, Mistral)
  - System prompts

- ✅ **Token Operations**
  - Token counting
  - Context window management
  - Embeddings extraction (if model supports)

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    LLM Engine                            │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────┐      ┌─────────────────┐             │
│  │ LLMEngine    │─────▶│ llama.cpp       │             │
│  │ Interface    │      │ Integration     │             │
│  └──────────────┘      └─────────────────┘             │
│         │                                                │
│         ├──▶ Model Loading (GGUF)                      │
│         ├──▶ Text Generation                           │
│         ├──▶ Chat Completion                           │
│         ├──▶ Token Counting                            │
│         └──▶ Embeddings (optional)                     │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### Component Details

| Component | Technology | Purpose |
|-----------|------------|---------|
| **LLMEngine** | C++ Interface | Abstract interface for LLM operations |
| **llama.cpp** | C++ Library | Efficient CPU inference for LLaMA-like models |
| **GGUF** | File Format | Quantized model format (4-bit, 8-bit) |

---

## Installation

### Prerequisites

The LLM Engine is included in Vector Studio v2.0+. No additional installation required.

### Download Models

Download GGUF models from Hugging Face:

```bash
# Example: Download Llama 2 7B Chat (GGUF Q4_K_M)
mkdir -p ~/.cache/vector_studio/llm_models
cd ~/.cache/vector_studio/llm_models

# Download from Hugging Face (example)
wget https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF/resolve/main/llama-2-7b-chat.Q4_K_M.gguf
```

### Recommended Models

| Model | Size | RAM | Use Case |
|-------|------|-----|----------|
| Llama 2 7B Q4 | 4 GB | 6 GB | General chat, Q&A |
| Llama 2 13B Q4 | 8 GB | 10 GB | Better reasoning |
| Mistral 7B Q4 | 4 GB | 6 GB | Fast, efficient |
| Llama 3 8B Q4 | 5 GB | 7 GB | Latest, best quality |

---

## Quick Start

### C++ Example

```cpp
#include "vdb/llm/llm_engine.hpp"

using namespace vdb::llm;

// Create engine
auto engine = create_llm_engine();

// Configure model
LLMConfig config;
config.model_path = "~/.cache/vector_studio/llm_models/llama-2-7b-chat.Q4_K_M.gguf";
config.n_ctx = 4096;        // Context window
config.n_threads = 8;       // CPU threads
config.n_gpu_layers = 0;    // 0 = CPU only

// Load model
if (!engine->load(config)) {
    std::cerr << "Failed to load model\n";
    return;
}

// Generate text
GenerationParams params;
params.max_tokens = 512;
params.temperature = 0.7f;

std::string response = engine->generate(
    "Explain vector databases in simple terms:",
    params
);

std::cout << response << std::endl;

// Unload when done
engine->unload();
```

### Python Example

```python
import pyvdb.llm as llm

# Create and configure engine
engine = llm.create_engine()
config = llm.LLMConfig(
    model_path="~/.cache/vector_studio/llm_models/llama-2-7b-chat.Q4_K_M.gguf",
    n_ctx=4096,
    n_threads=8
)

# Load model
engine.load(config)

# Generate text
params = llm.GenerationParams(
    max_tokens=512,
    temperature=0.7
)

response = engine.generate(
    "Explain vector databases in simple terms:",
    params
)

print(response)
```

---

## Configuration

### LLMConfig Options

```cpp
struct LLMConfig {
    fs::path model_path;              // Path to GGUF model file
    int n_ctx = 4096;                 // Context window size (tokens)
    int n_batch = 512;                // Batch size for prompt processing
    int n_threads = 0;                // CPU threads (0 = auto-detect)
    int n_gpu_layers = 0;             // GPU layers (0 = CPU only, 32+ = most on GPU)
    bool use_mmap = true;             // Memory-map model (faster load)
    bool use_mlock = false;           // Lock model in RAM (prevents swapping)
    float rope_freq_base = 0.0f;      // RoPE frequency base (0 = model default)
    float rope_freq_scale = 0.0f;     // RoPE frequency scaling (0 = model default)
};
```

### GenerationParams Options

```cpp
struct GenerationParams {
    int max_tokens = 1024;            // Maximum tokens to generate
    float temperature = 0.7f;         // Sampling temperature (0 = greedy, higher = creative)
    float top_p = 0.9f;               // Nucleus sampling threshold
    int top_k = 40;                   // Top-k sampling
    float repeat_penalty = 1.1f;      // Repetition penalty (1.0 = no penalty)
    int repeat_last_n = 64;           // Tokens to consider for repetition
    
    // Stop sequences (generation stops if any matched)
    std::vector<std::string> stop_sequences;
    
    // Streaming callback (called for each token)
    std::function<bool(std::string_view token)> on_token;
};
```

---

## API Reference

### LLMEngine Interface

#### `load(const LLMConfig& config) -> bool`

Load a GGUF model into memory.

**Parameters**:
- `config`: Model configuration

**Returns**: `true` if successful, `false` otherwise

**Example**:
```cpp
LLMConfig config;
config.model_path = "model.gguf";
if (engine->load(config)) {
    std::cout << "Model loaded successfully\n";
}
```

---

#### `generate(std::string_view prompt, const GenerationParams& params) -> std::string`

Generate text from a prompt.

**Parameters**:
- `prompt`: Input text prompt
- `params`: Generation parameters (optional)

**Returns**: Generated text as string

**Example**:
```cpp
GenerationParams params;
params.max_tokens = 256;
params.temperature = 0.7f;

std::string response = engine->generate("Once upon a time", params);
```

---

#### `chat(const std::vector<Message>& messages, const GenerationParams& params) -> ChatCompletionResult`

Complete a chat conversation with message history.

**Parameters**:
- `messages`: Vector of messages with roles (System/User/Assistant)
- `params`: Generation parameters (optional)

**Returns**: ChatCompletionResult with response and metadata

**Example**:
```cpp
std::vector<Message> messages = {
    {Role::System, "You are a helpful AI assistant."},
    {Role::User, "What is a vector database?"}
};

auto result = engine->chat(messages, params);
std::cout << "Response: " << result.content << "\n";
std::cout << "Tokens generated: " << result.tokens_generated << "\n";
```

---

#### `count_tokens(std::string_view text) -> int`

Count the number of tokens in text.

**Parameters**:
- `text`: Input text

**Returns**: Token count

**Example**:
```cpp
int tokens = engine->count_tokens("Hello, world!");
std::cout << "Token count: " << tokens << "\n";
```

---

## Usage Examples

### Example 1: Basic Text Generation

```cpp
#include "vdb/llm/llm_engine.hpp"

auto engine = vdb::llm::create_llm_engine();

// Load model
vdb::llm::LLMConfig config;
config.model_path = "models/llama-2-7b-chat.gguf";
engine->load(config);

// Generate
vdb::llm::GenerationParams params;
params.max_tokens = 200;
params.temperature = 0.8f;

std::string story = engine->generate(
    "Write a short story about a brave knight:",
    params
);

std::cout << story << std::endl;
```

---

### Example 2: Streaming Generation

```cpp
#include "vdb/llm/llm_engine.hpp"
#include <iostream>

auto engine = vdb::llm::create_llm_engine();
engine->load(config);

vdb::llm::GenerationParams params;
params.max_tokens = 500;

// Set up streaming callback
params.on_token = [](std::string_view token) {
    std::cout << token << std::flush;  // Print each token as it's generated
    return true;  // Continue generation
};

engine->generate("Explain quantum computing:", params);
std::cout << "\n";
```

---

### Example 3: Multi-Turn Chat

```cpp
#include "vdb/llm/llm_engine.hpp"

auto engine = vdb::llm::create_llm_engine();
engine->load(config);

std::vector<vdb::llm::Message> conversation;

// System prompt
conversation.push_back({
    vdb::llm::Role::System,
    "You are a helpful assistant specialized in databases."
});

// First exchange
conversation.push_back({
    vdb::llm::Role::User,
    "What is HNSW?"
});

auto result1 = engine->chat(conversation, params);
std::cout << "Assistant: " << result1.content << "\n\n";

// Add to conversation
conversation.push_back({
    vdb::llm::Role::Assistant,
    result1.content
});

// Second exchange
conversation.push_back({
    vdb::llm::Role::User,
    "How does it compare to IVF?"
});

auto result2 = engine->chat(conversation, params);
std::cout << "Assistant: " << result2.content << "\n";
```

---

### Example 4: RAG (Retrieval-Augmented Generation)

Combine Vector Studio search with LLM generation:

```cpp
#include "vdb/database.hpp"
#include "vdb/llm/llm_engine.hpp"

// Search vector database
auto db = vdb::VectorDatabase(config);
auto search_results = db.search(user_query, 5);

// Build context from search results
std::string context = "Relevant information:\n";
for (const auto& result : search_results) {
    context += "- " + result.metadata.content + "\n";
}

// Generate answer with context
auto llm = vdb::llm::create_llm_engine();
llm->load(llm_config);

std::string prompt = context + "\n\nUser question: " + user_query + 
                     "\n\nAnswer based on the information above:";

std::string answer = llm->generate(prompt, params);
std::cout << answer << std::endl;
```

---

## Supported Models

### Model Families

| Family | Architectures | Quantization |
|--------|---------------|--------------|
| **LLaMA** | Llama 1, Llama 2, Llama 3 | Q4_K_M, Q5_K_M, Q8_0 |
| **Mistral** | Mistral 7B, Mixtral | Q4_K_M, Q5_K_M |
| **GPT** | GPT-2, GPT-J, GPT-NeoX | Q4_0, Q8_0 |
| **Other** | Falcon, MPT, Vicuna | Various |

### Chat Templates

The engine supports multiple chat templates:

```cpp
// Available templates
std::string template_name = "chatml";   // ChatML format (default)
std::string template_name = "llama2";   // Llama 2 format
std::string template_name = "llama3";   // Llama 3 format
std::string template_name = "mistral";  // Mistral format

// Apply template
std::string formatted = vdb::llm::apply_chat_template(messages, template_name);
```

---

## Performance

### Benchmarks

**Hardware**: Intel i7-12700H, 32GB RAM, CPU-only

| Model | Size | Load Time | Tokens/sec | Memory |
|-------|------|-----------|------------|--------|
| Llama 2 7B Q4_K_M | 4 GB | 2.5s | 25-30 | 6 GB |
| Llama 2 13B Q4_K_M | 8 GB | 4.8s | 15-20 | 10 GB |
| Mistral 7B Q4_K_M | 4 GB | 2.3s | 28-35 | 6 GB |

### Optimization Tips

1. **Use Q4_K_M quantization** for best quality/speed tradeoff
2. **Set n_threads** to number of physical cores
3. **Enable mmap** (use_mmap = true) for faster loading
4. **Adjust batch size** (n_batch) for memory constraints
5. **Use GPU layers** (n_gpu_layers) if CUDA available

---

## Best Practices

### Model Selection

- **7B models**: Good for general use, fast inference
- **13B models**: Better reasoning, more capable
- **Q4_K_M**: Best quality/size tradeoff
- **Q8_0**: Higher quality, larger size

### Memory Management

```cpp
// Unload model when not in use
engine->unload();

// Check if model is loaded
if (engine->is_loaded()) {
    // Safe to generate
}
```

### Error Handling

```cpp
if (!engine->load(config)) {
    // Check model path
    // Verify sufficient RAM
    // Check file permissions
    return;
}

// Validate token count
int tokens = engine->count_tokens(prompt);
if (tokens > config.n_ctx) {
    std::cerr << "Prompt too long: " << tokens << " > " << config.n_ctx << "\n";
}
```

---

## Troubleshooting

### Model Won't Load

**Problem**: `load()` returns false

**Solutions**:
- Verify model file exists and is readable
- Check available RAM (need 1.5x model size)
- Ensure GGUF format is compatible
- Try with smaller batch size

### Out of Memory

**Problem**: System runs out of RAM during generation

**Solutions**:
- Use smaller quantization (Q4 instead of Q8)
- Reduce `n_ctx` (context window)
- Reduce `n_batch`
- Enable `use_mmap` for model file

### Slow Generation

**Problem**: Low tokens per second

**Solutions**:
- Increase `n_threads` to match CPU cores
- Use Q4_K_M quantization
- Reduce `n_ctx` if you don't need large context
- Consider GPU acceleration (`n_gpu_layers > 0`)

### Repetitive Output

**Problem**: Model repeats itself

**Solutions**:
- Increase `repeat_penalty` (try 1.2-1.3)
- Increase `repeat_last_n` (try 128-256)
- Adjust `temperature` (try 0.8-1.0)
- Add stop sequences

---

## Integration with Vector Studio

### Example: Semantic Search + LLM

```cpp
// 1. Search vector database
auto search_results = vector_db->search(query, 5);

// 2. Extract context
std::string context;
for (const auto& result : search_results) {
    context += result.metadata.content + "\n\n";
}

// 3. Generate answer
std::string prompt = "Context:\n" + context + 
                     "\n\nQuestion: " + query +
                     "\n\nAnswer:";

std::string answer = llm_engine->generate(prompt, params);
```

---

## See Also

- [API Reference](20_API_REFERENCE.md) - Complete API documentation
- [Usage Examples](12_USAGE_EXAMPLES.md) - More code examples
- [Architecture](05_ARCHITECTURE.md) - System architecture overview

---

**Last Updated**: 2026-01-06  
**Version**: 3.0.0  
**Status**: Production Ready ✅
