# Hektor Native NLP Engine - Complete Guide

## Overview

Hektor includes a fully native, bare-metal NLP (Natural Language Processing) engine built entirely in C++ for maximum performance. This end-to-end system handles text processing, embedding generation, semantic search, and language understanding without external dependencies.

## Architecture

### Component Stack

```
┌─────────────────────────────────────────────────────┐
│              Application Layer                       │
│  (CLI, API, Python Bindings, Web Interface)         │
└──────────────────────────┬──────────────────────────┘
                           │
┌──────────────────────────┴──────────────────────────┐
│              NLP Engine Layer                        │
│  ┌──────────┬──────────┬──────────┬──────────────┐ │
│  │ Query    │ Text     │ Semantic │ Entity       │ │
│  │ Analysis │ Preproc  │ Analysis │ Recognition  │ │
│  └──────────┴──────────┴──────────┴──────────────┘ │
└──────────────────────────┬──────────────────────────┘
                           │
┌──────────────────────────┴──────────────────────────┐
│         Embedding & Model Layer                      │
│  ┌─────────────┬────────────┬───────────────────┐  │
│  │ Transformer │ Tokenizer  │ Model Management  │  │
│  │ Models      │ (BPE/WP)   │ (Download/Cache)  │  │
│  └─────────────┴────────────┴───────────────────┘  │
└──────────────────────────┬──────────────────────────┘
                           │
┌──────────────────────────┴──────────────────────────┐
│           Vector Operations Layer                    │
│  ┌──────────┬───────────┬──────────┬─────────────┐ │
│  │ HNSW     │ Distance  │ Quan     │ Similarity  │ │
│  │ Index    │ Metrics   │ tization │ Search      │ │
│  └──────────┴───────────┴──────────┴─────────────┘ │
└──────────────────────────────────────────────────────┘
```

## Core Features

### 1. Text Preprocessing

**Capabilities:**
- Unicode normalization (NFD, NFC, NFKD, NFKC)
- Lowercasing with locale awareness
- Punctuation handling
- Stop word removal (custom lists)
- Stemming and lemmatization
- Whitespace normalization
- HTML/XML tag stripping
- Special character handling

**Usage:**
```cpp
#include <vdb/nlp/text_processor.hpp>

vdb::nlp::TextProcessor processor;
processor.setStopWords({"the", "a", "an", "is", "are"});
processor.enableStemming(true);
processor.enableLowercasing(true);

std::string text = "The <b>gold prices</b> are rising!";
std::string processed = processor.process(text);
// Result: "gold price rise"
```

**CLI Usage:**
```bash
# Process text with full preprocessing pipeline
hektor nlp preprocess --text "The gold prices are rising!" \
  --lowercase \
  --remove-stopwords \
  --stem

# Process file
hektor nlp preprocess --file input.txt --output processed.txt

# Custom stop words
hektor nlp preprocess --text "..." --stopwords custom_stopwords.txt
```

### 2. Tokenization

**Supported Tokenizers:**
- **Byte Pair Encoding (BPE)** - GPT-style
- **WordPiece** - BERT-style
- **SentencePiece** - Universal
- **Character-level** - Basic
- **Whitespace** - Simple splitting

**Usage:**
```cpp
#include <vdb/nlp/tokenizer.hpp>

// BPE Tokenizer
vdb::nlp::BPETokenizer tokenizer("vocab.json", "merges.txt");
auto tokens = tokenizer.encode("gold prices rising");
// Result: [8642, 7721, 12453]

auto text = tokenizer.decode(tokens);
// Result: "gold prices rising"

// Get token strings
auto token_strs = tokenizer.tokenize("gold prices rising");
// Result: ["gold", "Ġprices", "Ġrising"]
```

**CLI Usage:**
```bash
# Tokenize text
hektor nlp tokenize --text "gold prices rising" --model bpe --vocab vocab.json

# Encode to IDs
hektor nlp encode --text "..." --model bert --output ids.txt

# Decode from IDs
hektor nlp decode --ids "8642,7721,12453" --model bpe

# Train custom tokenizer
hektor nlp train-tokenizer --corpus corpus.txt --type bpe --vocab-size 32000
```

### 3. Embedding Generation

**Supported Models:**
- Sentence-BERT (all-MiniLM, all-mpnet, etc.)
- BERT (base, large, multilingual)
- GPT-2 (small, medium, large)
- RoBERTa
- DistilBERT
- Custom fine-tuned models

**Usage:**
```cpp
#include <vdb/nlp/embedder.hpp>

// Initialize embedder
vdb::nlp::Embedder embedder("all-MiniLM-L6-v2");

// Generate embedding
std::string text = "gold prices rising";
std::vector<float> embedding = embedder.embed(text);
// Result: 384-dimensional vector

// Batch embeddings
std::vector<std::string> texts = {"text1", "text2", "text3"};
std::vector<std::vector<float>> embeddings = embedder.embedBatch(texts);

// Pooling strategies
embedder.setPooling(vdb::nlp::PoolingStrategy::MEAN);  // Mean pooling
embedder.setPooling(vdb::nlp::PoolingStrategy::CLS);   // CLS token
embedder.setPooling(vdb::nlp::PoolingStrategy::MAX);   // Max pooling
```

**CLI Usage:**
```bash
# Generate embedding
hektor nlp embed --text "gold prices rising" --model all-MiniLM-L6-v2

# Batch embedding from file
hektor nlp embed --file texts.txt --model bert-base --output embeddings.npy

# Embedding with pooling
hektor nlp embed --text "..." --pooling mean --normalize
```

### 4. Semantic Similarity

**Distance Metrics:**
- Cosine similarity
- Euclidean distance
- Dot product
- Manhattan distance
- Jaccard similarity

**Usage:**
```cpp
#include <vdb/nlp/similarity.hpp>

vdb::nlp::SimilarityCalculator sim;

// Cosine similarity
float score = sim.cosine(embedding1, embedding2);  // 0.0 to 1.0

// Semantic textual similarity
float sts_score = sim.semanticSimilarity(
    embedder, 
    "gold prices rising", 
    "gold market increasing"
);  // 0.934

// Pairwise similarity matrix
auto matrix = sim.pairwiseMatrix(embeddings);
```

**CLI Usage:**
```bash
# Compare two texts
hektor nlp similarity \
  --text1 "gold prices rising" \
  --text2 "gold market increasing" \
  --metric cosine

# Pairwise similarity
hektor nlp pairwise --file texts.txt --metric cosine --output matrix.csv
```

### 5. Named Entity Recognition (NER)

**Entity Types:**
- PERSON - People, characters
- ORGANIZATION - Companies, agencies
- LOCATION - Cities, countries, regions
- DATE - Dates, times, periods
- MONEY - Monetary values
- PERCENT - Percentages
- PRODUCT - Products, objects
- EVENT - Events, occurrences

**Usage:**
```cpp
#include <vdb/nlp/ner.hpp>

vdb::nlp::NERTagger ner("bert-base-ner");

std::string text = "Gold prices in London reached $2000 on Monday.";
auto entities = ner.extract(text);

for (const auto& entity : entities) {
    std::cout << entity.text << " (" << entity.type << "): " 
              << entity.confidence << std::endl;
}
// Output:
// Gold (PRODUCT): 0.95
// London (LOCATION): 0.98
// $2000 (MONEY): 0.99
// Monday (DATE): 0.97
```

**CLI Usage:**
```bash
# Extract entities
hektor nlp ner --text "Gold prices in London reached $2000 on Monday."

# NER on file
hektor nlp ner --file article.txt --output entities.json

# Filter by entity type
hektor nlp ner --text "..." --types PERSON,ORGANIZATION,LOCATION
```

### 6. Semantic Search & Query Understanding

**Query Analysis:**
- Intent detection
- Entity extraction from queries
- Query expansion
- Synonym generation
- Query rewriting

**Usage:**
```cpp
#include <vdb/nlp/query_analyzer.hpp>

vdb::nlp::QueryAnalyzer analyzer;

// Analyze query
auto analysis = analyzer.analyze("gold price forecast next quarter");

std::cout << "Intent: " << analysis.intent << std::endl;
// Output: Intent: INFORMATION_SEEKING

std::cout << "Entities: ";
for (const auto& entity : analysis.entities) {
    std::cout << entity.text << " ";
}
// Output: Entities: gold price forecast quarter

// Query expansion
auto expanded = analyzer.expand("gold");
// Result: ["gold", "au", "precious metal", "bullion", "gold prices"]
```

**CLI Usage:**
```bash
# Analyze query
hektor nlp query --text "gold price forecast next quarter" --analyze

# Expand query
hektor nlp query --text "gold" --expand --synonyms

# Rewrite query
hektor nlp query --text "gld prc" --rewrite --correct-spelling
```

### 7. Text Classification

**Classification Types:**
- Sentiment analysis
- Topic classification
- Intent classification
- Language detection
- Spam detection

**Usage:**
```cpp
#include <vdb/nlp/classifier.hpp>

// Sentiment analysis
vdb::nlp::SentimentClassifier sentiment;
auto result = sentiment.classify("Gold prices are soaring! Great news!");

std::cout << "Sentiment: " << result.label << std::endl;
std::cout << "Confidence: " << result.confidence << std::endl;
// Output:
// Sentiment: POSITIVE
// Confidence: 0.92

// Topic classification
vdb::nlp::TopicClassifier topics({"finance", "sports", "tech", "politics"});
auto topic = topics.classify("Gold prices rising on inflation fears");
// Result: finance (0.94)
```

**CLI Usage:**
```bash
# Sentiment analysis
hektor nlp sentiment --text "Gold prices are soaring! Great news!"

# Topic classification
hektor nlp classify --text "..." --topics finance,sports,tech,politics

# Multi-label classification
hektor nlp classify --text "..." --multi-label --threshold 0.7
```

### 8. Language Models & Text Generation

**Capabilities:**
- Next token prediction
- Text completion
- Masked language modeling
- Question answering
- Text summarization

**Usage:**
```cpp
#include <vdb/nlp/language_model.hpp>

vdb::nlp::LanguageModel lm("gpt2-medium");

// Text completion
std::string prompt = "Gold prices are";
std::string completion = lm.complete(prompt, /*max_tokens=*/50);
// Result: "Gold prices are rising steadily amid economic uncertainty..."

// Generate with parameters
auto result = lm.generate(prompt, {
    .max_tokens = 100,
    .temperature = 0.8,
    .top_p = 0.95,
    .top_k = 50
});
```

**CLI Usage:**
```bash
# Text completion
hektor nlp complete --text "Gold prices are" --max-tokens 50

# Generate with sampling
hektor nlp generate --prompt "..." --temperature 0.8 --top-p 0.95

# Question answering
hektor nlp qa --context "..." --question "What are gold prices?"
```

## End-to-End NLP Pipeline

### Complete Workflow Example

```cpp
#include <vdb/nlp/pipeline.hpp>

// Initialize full NLP pipeline
vdb::nlp::Pipeline pipeline;
pipeline
    .addStage<vdb::nlp::TextProcessor>()
    .addStage<vdb::nlp::BPETokenizer>("vocab.json")
    .addStage<vdb::nlp::Embedder>("all-MiniLM-L6-v2")
    .addStage<vdb::nlp::NERTagger>("bert-base-ner");

// Process text end-to-end
std::string text = "Gold prices in London reached $2000 on Monday.";
auto result = pipeline.process(text);

// Access results
auto processed_text = result.get<std::string>("processed_text");
auto tokens = result.get<std::vector<int>>("tokens");
auto embedding = result.get<std::vector<float>>("embedding");
auto entities = result.get<std::vector<Entity>>("entities");
```

### CLI Pipeline

```bash
# Complete NLP pipeline
hektor nlp pipeline \
  --text "Gold prices in London reached $2000 on Monday." \
  --stages preprocess,tokenize,embed,ner \
  --model all-MiniLM-L6-v2 \
  --output result.json

# Batch processing
hektor nlp pipeline \
  --file documents.txt \
  --stages preprocess,embed \
  --model bert-base \
  --workers 8 \
  --output embeddings/
```

## Model Management

### Downloading Models

```bash
# List available models
hektor model list --category nlp

# Download model
hektor model download all-MiniLM-L6-v2

# Download with progress
hektor model download bert-base-uncased --show-progress

# Set default model
hektor model set-default all-MiniLM-L6-v2
```

### Model Information

```bash
# Show model details
hektor model info all-MiniLM-L6-v2

# List downloaded models
hektor model list --installed

# Model benchmarks
hektor model benchmark all-MiniLM-L6-v2 --texts 1000
```

### Model Quantization

```bash
# Quantize model for faster inference
hektor model quantize bert-base --type int8 --output bert-base-int8

# Benchmark quantized vs original
hektor model benchmark bert-base --compare bert-base-int8
```

## Performance Optimization

### Batch Processing

```cpp
// Batch size optimization
embedder.setBatchSize(32);  // Process 32 texts at once

// Parallel processing
embedder.setNumThreads(8);  // Use 8 threads

// GPU acceleration (if available)
embedder.setDevice("cuda:0");
```

### Caching

```cpp
// Enable embedding cache
embedder.enableCache(true);
embedder.setCacheSize(10000);  // Cache 10k embeddings

// Preload models
embedder.preload();  // Load model into memory upfront
```

### Memory Management

```cpp
// Low memory mode
embedder.setLowMemoryMode(true);

// Quantization for reduced memory
embedder.setQuantization(vdb::nlp::Quantization::INT8);
```

## Integration with Vector Database

### Automatic Embedding Generation

```bash
# Add document with auto-embedding
hektor add ./mydb --text "Gold prices rising" --auto-embed

# Ingest with embedding generation
hektor ingest ./mydb documents.txt --auto-embed --model all-MiniLM-L6-v2

# Batch with NLP pipeline
hektor batch ./mydb data.jsonl \
  --nlp-pipeline preprocess,embed,ner \
  --model bert-base
```

### Semantic Search

```bash
# Search with query analysis
hektor search ./mydb "gold forecast" --nlp-analyze --expand

# Search with entity extraction
hektor search ./mydb "gold in London" --extract-entities --filter-by-entities

# Hybrid search with NLP
hektor hybrid:search ./mydb "market analysis" \
  --nlp-preprocess \
  --query-expansion \
  --fusion rrf
```

## Advanced Features

### Custom Model Fine-tuning

```bash
# Fine-tune on custom data
hektor nlp train \
  --base-model bert-base \
  --train-data training.jsonl \
  --task classification \
  --labels finance,sports,tech \
  --epochs 3 \
  --output custom-bert

# Evaluate model
hektor nlp evaluate --model custom-bert --test-data test.jsonl
```

### Multi-language Support

```bash
# Use multilingual model
hektor nlp embed --text "金価格が上昇" --model bert-multilingual

# Language detection
hektor nlp detect-lang --text "金価格が上昇"
# Output: Japanese (ja) - confidence: 0.99

# Translate query (if translation model available)
hektor nlp translate --text "金価格が上昇" --target en
```

### Domain-specific Models

```bash
# Use finance-specific model
hektor nlp embed --text "gold prices" --model finbert

# Legal domain
hektor nlp embed --text "..." --model legal-bert

# Medical domain
hektor nlp embed --text "..." --model biobert
```

## Best Practices

### 1. Model Selection

- **General purpose**: `all-MiniLM-L6-v2` (fast, 384-dim)
- **High quality**: `all-mpnet-base-v2` (slower, 768-dim)
- **Multilingual**: `paraphrase-multilingual-mpnet-base-v2`
- **Domain-specific**: Use specialized models when available

### 2. Preprocessing

- Always normalize unicode
- Remove HTML/XML tags
- Handle special characters appropriately
- Use domain-specific stop words
- Apply stemming/lemmatization cautiously

### 3. Performance

- Use batch processing for multiple documents
- Enable caching for repeated queries
- Quantize models for faster inference
- Use GPU if available
- Preload models at startup

### 4. Accuracy

- Use query expansion for better recall
- Apply NER for entity-based filtering
- Combine vector and lexical search
- Fine-tune models on domain data
- Validate results with test sets

## API Reference

### C++ API

```cpp
namespace vdb::nlp {
    class TextProcessor;
    class Tokenizer;
    class Embedder;
    class SimilarityCalculator;
    class NERTagger;
    class QueryAnalyzer;
    class Classifier;
    class LanguageModel;
    class Pipeline;
}
```

### CLI Commands

```bash
hektor nlp preprocess     # Text preprocessing
hektor nlp tokenize       # Tokenization
hektor nlp encode         # Encode to token IDs
hektor nlp decode         # Decode from token IDs
hektor nlp embed          # Generate embeddings
hektor nlp similarity     # Similarity calculation
hektor nlp ner            # Named entity recognition
hektor nlp query          # Query analysis
hektor nlp sentiment      # Sentiment analysis
hektor nlp classify       # Text classification
hektor nlp complete       # Text completion
hektor nlp generate       # Text generation
hektor nlp pipeline       # End-to-end pipeline
```

## Troubleshooting

### Common Issues

**Model not found:**
```bash
# Download the model first
hektor model download all-MiniLM-L6-v2
```

**Out of memory:**
```bash
# Use quantization or smaller batch size
hektor nlp embed --file large.txt --batch-size 8 --quantize int8
```

**Slow inference:**
```bash
# Enable GPU or use smaller model
hektor nlp embed --text "..." --device cuda --model all-MiniLM-L6-v2
```

## Examples

### Complete Application

See `examples/nlp_complete_workflow.cpp` for a full example integrating all NLP components.

### Tutorials

- `examples/nlp_basic.cpp` - Basic NLP operations
- `examples/nlp_semantic_search.cpp` - Semantic search with NLP
- `examples/nlp_classification.cpp` - Text classification
- `examples/nlp_ner.cpp` - Named entity recognition
- `examples/nlp_custom_model.cpp` - Custom model integration

## Performance Benchmarks

| Operation | Speed | Memory |
|-----------|-------|--------|
| Text preprocessing | 50K docs/s | 100 MB |
| Tokenization (BPE) | 20K docs/s | 200 MB |
| Embedding (MiniLM) | 500 docs/s | 1.5 GB |
| Embedding (MPNet) | 200 docs/s | 2.5 GB |
| NER | 100 docs/s | 2 GB |
| Classification | 1K docs/s | 1 GB |

*Benchmarks on Intel i9-12900K, 32GB RAM, no GPU*

## Conclusion

The Hektor native NLP engine provides a complete, high-performance solution for natural language processing entirely in C++. It integrates seamlessly with the vector database for end-to-end semantic search applications.

For more information, see:
- API documentation: `/docs/20_API_REFERENCE.md`
- Examples: `/examples/nlp_*.cpp`
- Architecture: `/docs/05_ARCHITECTURE.md`
