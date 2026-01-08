---
title: "AI Training Guide"
version: "3.0.0"
last_updated: "2026-01-06"
sidebar_position: 10
---

![Version](https://img.shields.io/badge/version-3.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--06-green?style=flat-square)
![Type](https://img.shields.io/badge/type-advanced-red?style=flat-square)

## Table of Contents

1. [Introduction](#introduction)
2. [Understanding Embedding Models](#understanding-embedding-models)
3. [Using Pretrained Models](#using-pretrained-models)
4. [Fine-Tuning for Your Domain](#fine-tuning-for-your-domain)
5. [Training from Scratch](#training-from-scratch)
6. [Contrastive Learning](#contrastive-learning)
7. [Multi-Modal Training](#multi-modal-training)
8. [Model Evaluation](#model-evaluation)
9. [Deployment and ONNX Export](#deployment-and-onnx-export)
10. [Advanced Topics](#advanced-topics)

---

## Introduction

This guide covers everything from using pretrained embedding models to training your own from scratch. Understanding these concepts enables:

- Better model selection for your use case
- Domain-specific fine-tuning for improved accuracy
- Training custom models for specialized applications
- Integration of self-trained models into Vector Studio

### When to Use What

| Scenario | Approach | Effort | Accuracy |
|----------|----------|--------|----------|
| General semantic search | Pretrained models | Low | Good |
| Domain-specific search | Fine-tuned models | Medium | Better |
| Novel domain / language | Train from scratch | High | Best (potentially) |
| Specialized task | Custom architecture | Very High | Task-dependent |

---

## Understanding Embedding Models

### What Embeddings Capture

Embedding models learn to encode semantic meaning into dense vectors:

```
"Gold prices surge on Fed expectations"
    │
    ▼
[0.023, -0.156, 0.089, ..., 0.234]  # 384 or 512 dimensions
```

Similar meanings → similar vectors → small distance.

### Model Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        EMBEDDING MODEL ARCHITECTURE                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Input: "Gold broke resistance at $4,200"                                   │
│         │                                                                   │
│         ▼                                                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                         TOKENIZER                                    │   │
│  │  WordPiece / BPE / SentencePiece                                    │   │
│  │  "Gold" "broke" "resistance" "at" "$" "4" "," "200"                 │   │
│  │  Token IDs: [2751, 3631, 5012, 2012, 1002, ...]                     │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│         │                                                                   │
│         ▼                                                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    TOKEN EMBEDDINGS                                  │   │
│  │  Lookup table: vocab_size × hidden_dim                              │   │
│  │  + Positional embeddings (absolute or relative)                     │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│         │                                                                   │
│         ▼                                                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                   TRANSFORMER LAYERS × N                             │   │
│  │  ┌───────────────┐    ┌───────────────┐    ┌───────────────┐        │   │
│  │  │ Multi-Head    │───▶│ Layer Norm    │───▶│ Feed Forward  │        │   │
│  │  │ Self-Attention│    │ + Residual    │    │ Network       │        │   │
│  │  └───────────────┘    └───────────────┘    └───────────────┘        │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│         │                                                                   │
│         ▼                                                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                         POOLING                                      │   │
│  │  • Mean pooling (average all tokens) ← most common                  │   │
│  │  • [CLS] token (first token)                                        │   │
│  │  • Max pooling (max per dimension)                                   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│         │                                                                   │
│         ▼                                                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      PROJECTION + NORMALIZE                          │   │
│  │  Optional linear layer → L2 normalization                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│         │                                                                   │
│         ▼                                                                   │
│  Output: [0.023, -0.156, ..., 0.234]  (unit vector)                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Key Components

1. **Tokenizer**: Splits text into subword tokens
2. **Token Embeddings**: Learnable lookup table
3. **Transformer Layers**: Self-attention + feed-forward
4. **Pooling**: Aggregate tokens into single vector
5. **Normalization**: Scale to unit sphere

---

## Using Pretrained Models

### Recommended Pretrained Models

| Model | Dimensions | Speed | Quality | Use Case |
|-------|------------|-------|---------|----------|
| `all-MiniLM-L6-v2` | 384 | ★★★★★ | ★★★☆☆ | General search, fast |
| `all-MiniLM-L12-v2` | 384 | ★★★★☆ | ★★★★☆ | Balanced |
| `all-mpnet-base-v2` | 768 | ★★☆☆☆ | ★★★★★ | High quality |
| `e5-base-v2` | 768 | ★★☆☆☆ | ★★★★★ | SOTA quality |
| `multilingual-e5-base` | 768 | ★★☆☆☆ | ★★★★☆ | Multi-language |

### Loading with Sentence-Transformers

```python
from sentence_transformers import SentenceTransformer

# Load pretrained model
model = SentenceTransformer('sentence-transformers/all-MiniLM-L6-v2')

# Generate embeddings
texts = [
    "Gold prices surge on Fed pivot expectations",
    "Silver breaks above key resistance level",
    "Bitcoin drops amid regulatory concerns"
]

embeddings = model.encode(texts, normalize_embeddings=True)
print(f"Shape: {embeddings.shape}")  # (3, 384)
```

### Loading with Transformers

```python
from transformers import AutoTokenizer, AutoModel
import torch

# Load tokenizer and model
tokenizer = AutoTokenizer.from_pretrained('sentence-transformers/all-MiniLM-L6-v2')
model = AutoModel.from_pretrained('sentence-transformers/all-MiniLM-L6-v2')

def mean_pooling(model_output, attention_mask):
    """Average token embeddings, weighted by attention mask."""
    token_embeddings = model_output[0]
    input_mask_expanded = attention_mask.unsqueeze(-1).expand(token_embeddings.size()).float()
    sum_embeddings = torch.sum(token_embeddings * input_mask_expanded, 1)
    sum_mask = torch.clamp(input_mask_expanded.sum(1), min=1e-9)
    return sum_embeddings / sum_mask

# Encode text
texts = ["Gold bullish breakout", "Silver resistance test"]
encoded = tokenizer(texts, padding=True, truncation=True, return_tensors='pt')

with torch.no_grad():
    outputs = model(**encoded)
    embeddings = mean_pooling(outputs, encoded['attention_mask'])
    embeddings = torch.nn.functional.normalize(embeddings, p=2, dim=1)

print(embeddings.shape)  # torch.Size([2, 384])
```

### Downloading for Vector Studio

```powershell
# Download ONNX models for local inference
python scripts/download_models.py

# This downloads:
# - all-MiniLM-L6-v2.onnx (text encoder)
# - clip-vit-base-patch32.onnx (image encoder)
```

---

## Fine-Tuning for Your Domain

### When to Fine-Tune

Fine-tune when:
- Domain-specific vocabulary (finance, medical, legal)
- Poor performance with pretrained models
- Have labeled pairs/triplets (>1000 examples)

### Preparing Training Data

#### Format 1: Sentence Pairs

```json
[
    {"query": "Gold price analysis", "positive": "XAU/USD bullish momentum"},
    {"query": "Fed interest rate decision", "positive": "FOMC policy announcement"},
    {"query": "Support level", "positive": "Price floor test"}
]
```

#### Format 2: Triplets (Harder Negatives)

```json
[
    {
        "anchor": "Gold breaking resistance at $4,200",
        "positive": "XAU/USD bullish breakout above key level",
        "negative": "Bitcoin drops below $50,000 support"
    }
]
```

#### Mining Hard Negatives

```python
def mine_hard_negatives(model, anchors, candidates, num_negatives=5):
    """Find challenging negatives using current model."""
    anchor_embs = model.encode(anchors)
    cand_embs = model.encode(candidates)
    
    hard_negatives = []
    for i, anchor_emb in enumerate(anchor_embs):
        # Find most similar non-matching candidates
        similarities = cosine_similarity([anchor_emb], cand_embs)[0]
        
        # Skip true positives, take top similar as hard negatives
        neg_indices = np.argsort(similarities)[::-1][:num_negatives]
        hard_negatives.append([candidates[j] for j in neg_indices])
    
    return hard_negatives
```

### Fine-Tuning with Sentence-Transformers

```python
from sentence_transformers import SentenceTransformer, InputExample, losses
from torch.utils.data import DataLoader

# Load base model
model = SentenceTransformer('sentence-transformers/all-MiniLM-L6-v2')

# Prepare training examples
train_examples = [
    InputExample(
        texts=["Gold analysis", "XAU/USD bullish momentum"],
        label=1.0  # Similarity score
    ),
    InputExample(
        texts=["Silver outlook", "SLV bearish reversal"],
        label=0.8
    ),
    # Add more examples...
]

train_dataloader = DataLoader(train_examples, shuffle=True, batch_size=16)

# Define loss function
train_loss = losses.CosineSimilarityLoss(model)

# Fine-tune
model.fit(
    train_objectives=[(train_dataloader, train_loss)],
    epochs=3,
    warmup_steps=100,
    output_path='./gold_standard_model'
)
```

### Contrastive Fine-Tuning

```python
from sentence_transformers import losses

# Multiple Negatives Ranking Loss (most effective for retrieval)
train_loss = losses.MultipleNegativesRankingLoss(model)

# Triplet loss with hard margin
train_loss = losses.TripletLoss(
    model=model,
    distance_metric=losses.TripletDistanceMetric.COSINE,
    triplet_margin=0.5
)
```

### Domain Adaptation Tips

1. **Start with good base model**: `all-mpnet-base-v2` or `e5-base-v2`
2. **Use domain corpus for MLM**: Masked language model pretraining
3. **Progressive fine-tuning**: General → Domain → Task
4. **Learning rate**: 1e-5 to 3e-5 (lower than pretraining)
5. **Batch size**: Larger is better for contrastive learning (32-128)

---

## Training from Scratch

### When to Train from Scratch

- Novel language not in pretrained models
- Highly specialized domain (code, chemistry)
- Research / maximum control
- Proprietary data concerns

### Architecture Design

```python
from transformers import BertConfig, BertModel

# Define architecture
config = BertConfig(
    vocab_size=30522,
    hidden_size=384,           # Embedding dimension
    num_hidden_layers=6,       # Transformer layers
    num_attention_heads=12,
    intermediate_size=1536,    # FFN hidden size (4x hidden)
    max_position_embeddings=512,
    hidden_dropout_prob=0.1,
    attention_probs_dropout_prob=0.1
)

# Initialize model with random weights
model = BertModel(config)
print(f"Parameters: {sum(p.numel() for p in model.parameters()):,}")
```

### Stage 1: Masked Language Model Pretraining

```python
from transformers import (
    BertForMaskedLM,
    DataCollatorForLanguageModeling,
    Trainer, TrainingArguments
)

# Create MLM model
mlm_model = BertForMaskedLM(config)

# Data collator handles masking
data_collator = DataCollatorForLanguageModeling(
    tokenizer=tokenizer,
    mlm=True,
    mlm_probability=0.15
)

# Training arguments
training_args = TrainingArguments(
    output_dir='./mlm_model',
    num_train_epochs=10,
    per_device_train_batch_size=32,
    learning_rate=1e-4,
    warmup_steps=10000,
    weight_decay=0.01,
    save_steps=5000,
    logging_steps=500
)

# Train
trainer = Trainer(
    model=mlm_model,
    args=training_args,
    data_collator=data_collator,
    train_dataset=train_dataset
)

trainer.train()
```

### Stage 2: Contrastive Learning

After MLM pretraining, add sentence-level objectives:

```python
import torch
import torch.nn as nn
import torch.nn.functional as F

class SentenceEmbeddingModel(nn.Module):
    def __init__(self, base_model, hidden_size=384, output_size=384):
        super().__init__()
        self.encoder = base_model
        self.projection = nn.Linear(hidden_size, output_size)
    
    def forward(self, input_ids, attention_mask):
        outputs = self.encoder(input_ids, attention_mask=attention_mask)
        
        # Mean pooling
        token_embeddings = outputs.last_hidden_state
        mask = attention_mask.unsqueeze(-1).expand(token_embeddings.size()).float()
        sum_embeddings = torch.sum(token_embeddings * mask, dim=1)
        sum_mask = torch.clamp(mask.sum(dim=1), min=1e-9)
        embeddings = sum_embeddings / sum_mask
        
        # Project and normalize
        embeddings = self.projection(embeddings)
        embeddings = F.normalize(embeddings, p=2, dim=1)
        
        return embeddings

class InfoNCELoss(nn.Module):
    """Contrastive loss with in-batch negatives."""
    def __init__(self, temperature=0.05):
        super().__init__()
        self.temperature = temperature
    
    def forward(self, embeddings_a, embeddings_b):
        # Compute similarity matrix
        similarity = torch.matmul(embeddings_a, embeddings_b.T) / self.temperature
        
        # Labels: diagonal entries are positives
        batch_size = embeddings_a.size(0)
        labels = torch.arange(batch_size, device=embeddings_a.device)
        
        # Cross-entropy with softmax
        loss_a = F.cross_entropy(similarity, labels)
        loss_b = F.cross_entropy(similarity.T, labels)
        
        return (loss_a + loss_b) / 2
```

### Stage 3: Hard Negative Mining

```python
class HardNegativeTrainer:
    def __init__(self, model, corpus, batch_size=32):
        self.model = model
        self.corpus = corpus
        self.batch_size = batch_size
        
    def mine_and_train(self, queries, positives, epochs=5):
        for epoch in range(epochs):
            # Re-encode corpus with current model
            corpus_embeddings = self.encode_corpus()
            
            # Mine hard negatives
            hard_negatives = []
            for query, positive in zip(queries, positives):
                query_emb = self.model.encode([query])[0]
                
                # Find similar but non-matching
                similarities = cosine_similarity([query_emb], corpus_embeddings)[0]
                hard_neg_idx = np.argsort(similarities)[::-1]
                
                # Skip if it's the actual positive
                for idx in hard_neg_idx:
                    if self.corpus[idx] != positive:
                        hard_negatives.append(self.corpus[idx])
                        break
            
            # Train with triplets
            self.train_step(queries, positives, hard_negatives)
            
            print(f"Epoch {epoch+1}: Hard negative mining complete")
```

### Training Data Preparation

#### For MLM Pretraining

```python
from datasets import Dataset
from transformers import AutoTokenizer

# Load your corpus
corpus = [
    "Gold prices surged 2% today amid Fed speculation.",
    "Technical analysis shows bullish divergence on daily chart.",
    # ... thousands/millions of domain documents
]

# Tokenize
tokenizer = AutoTokenizer.from_pretrained('bert-base-uncased')

def tokenize_function(examples):
    return tokenizer(
        examples['text'],
        padding='max_length',
        truncation=True,
        max_length=512
    )

dataset = Dataset.from_dict({'text': corpus})
tokenized_dataset = dataset.map(tokenize_function, batched=True)
```

#### For Contrastive Learning

```python
# Collect parallel pairs (automatic or manual)
pairs = [
    ("Query about gold", "Document about gold prices"),
    ("Support level question", "Technical analysis response"),
]

# Or use back-translation for augmentation
from transformers import MarianMTModel, MarianTokenizer

def back_translate(text, src='en', tgt='de'):
    """Create paraphrase via translation."""
    # en → de → en
    pass  # Implementation details
```

---

## Contrastive Learning

### Loss Functions Comparison

#### 1. InfoNCE / NT-Xent (Normalized Temperature-scaled Cross Entropy)

```python
def info_nce_loss(anchor, positive, temperature=0.07):
    """
    InfoNCE loss with in-batch negatives.
    
    For batch of N pairs, uses (N-1) negatives per sample.
    """
    # Similarity matrix [N, N]
    sim = torch.mm(anchor, positive.T) / temperature
    
    # Positives on diagonal
    labels = torch.arange(sim.size(0), device=sim.device)
    
    # Cross-entropy: maximize diagonal, minimize off-diagonal
    loss = F.cross_entropy(sim, labels)
    
    return loss
```

**Pros**: Simple, effective with large batches
**Cons**: Needs large batch sizes (>256)

#### 2. Triplet Loss

```python
def triplet_loss(anchor, positive, negative, margin=0.5):
    """
    max(0, d(a,p) - d(a,n) + margin)
    
    Push positive closer than negative by margin.
    """
    pos_dist = torch.norm(anchor - positive, dim=1)
    neg_dist = torch.norm(anchor - negative, dim=1)
    
    loss = F.relu(pos_dist - neg_dist + margin)
    
    return loss.mean()
```

**Pros**: Explicit negative control
**Cons**: Needs hard negative mining

#### 3. Multiple Negatives Ranking Loss

```python
def mnrl_loss(query, positive, negatives, scale=20.0):
    """
    Score positive higher than all negatives.
    
    Similar to InfoNCE but with additional hard negatives.
    """
    # Positive score
    pos_score = torch.sum(query * positive, dim=1) * scale
    
    # Negative scores
    neg_scores = torch.mm(query, negatives.T) * scale
    
    # Log-softmax over [positive, negatives]
    all_scores = torch.cat([pos_score.unsqueeze(1), neg_scores], dim=1)
    labels = torch.zeros(all_scores.size(0), dtype=torch.long, device=all_scores.device)
    
    loss = F.cross_entropy(all_scores, labels)
    
    return loss
```

**Pros**: Best for retrieval tasks
**Cons**: Needs curated negatives

### Batch Construction Strategies

#### In-Batch Negatives

Each sample's positives become other samples' negatives:

```
Batch:
  Sample 1: (query_1, positive_1)
  Sample 2: (query_2, positive_2)
  Sample 3: (query_3, positive_3)

For query_1:
  - Positive: positive_1
  - Negatives: positive_2, positive_3
```

#### Hard Negative Mining

```python
def select_hard_negatives(query_emb, all_embeddings, k=5, exclude_idx=None):
    """
    Select k most similar non-matching embeddings.
    """
    similarities = cosine_similarity([query_emb], all_embeddings)[0]
    
    # Sort by similarity descending
    sorted_indices = np.argsort(similarities)[::-1]
    
    # Skip excluded (true positives)
    hard_negatives = []
    for idx in sorted_indices:
        if exclude_idx is None or idx not in exclude_idx:
            hard_negatives.append(idx)
            if len(hard_negatives) >= k:
                break
    
    return hard_negatives
```

---

## Multi-Modal Training

### CLIP-Style Training

Train text and image encoders together:

```python
class CLIPModel(nn.Module):
    def __init__(self, text_encoder, image_encoder, embed_dim=512):
        super().__init__()
        self.text_encoder = text_encoder
        self.image_encoder = image_encoder
        
        # Projection heads
        self.text_proj = nn.Linear(text_encoder.config.hidden_size, embed_dim)
        self.image_proj = nn.Linear(image_encoder.config.hidden_size, embed_dim)
        
        # Learnable temperature
        self.logit_scale = nn.Parameter(torch.ones([]) * np.log(1 / 0.07))
    
    def forward(self, text_inputs, image_inputs):
        # Encode text
        text_features = self.text_encoder(**text_inputs).last_hidden_state[:, 0]
        text_features = self.text_proj(text_features)
        text_features = F.normalize(text_features, dim=-1)
        
        # Encode images
        image_features = self.image_encoder(**image_inputs).last_hidden_state[:, 0]
        image_features = self.image_proj(image_features)
        image_features = F.normalize(image_features, dim=-1)
        
        return text_features, image_features
    
    def compute_loss(self, text_features, image_features):
        # Scaled cosine similarity
        logit_scale = self.logit_scale.exp()
        logits_per_image = logit_scale * image_features @ text_features.T
        logits_per_text = logits_per_image.T
        
        # Symmetric cross-entropy
        batch_size = text_features.size(0)
        labels = torch.arange(batch_size, device=text_features.device)
        
        loss_i2t = F.cross_entropy(logits_per_image, labels)
        loss_t2i = F.cross_entropy(logits_per_text, labels)
        
        return (loss_i2t + loss_t2i) / 2
```

### Data Preparation for Multi-Modal

```python
# Image-text pairs
pairs = [
    {
        "image": "charts/gold_daily.png",
        "text": "Gold daily chart showing bullish breakout"
    },
    {
        "image": "charts/silver_weekly.png", 
        "text": "Silver weekly resistance test"
    }
]

# Data augmentation
from torchvision import transforms

image_transforms = transforms.Compose([
    transforms.Resize(256),
    transforms.RandomCrop(224),
    transforms.RandomHorizontalFlip(),
    transforms.ColorJitter(0.1, 0.1, 0.1),
    transforms.ToTensor(),
    transforms.Normalize(mean=[0.485, 0.456, 0.406], 
                        std=[0.229, 0.224, 0.225])
])
```

---

## Model Evaluation

### Semantic Similarity Benchmarks

```python
from sentence_transformers import SentenceTransformer, evaluation

model = SentenceTransformer('./my_model')

# STS Benchmark
sts_evaluator = evaluation.EmbeddingSimilarityEvaluator(
    sentences1=['sentence 1', ...],
    sentences2=['sentence 2', ...],
    scores=[0.8, ...]  # Human similarity scores
)

result = sts_evaluator(model)
print(f"Spearman correlation: {result}")
```

### Information Retrieval Metrics

```python
def evaluate_retrieval(model, queries, corpus, relevance):
    """
    Evaluate retrieval performance.
    
    Args:
        queries: List of query texts
        corpus: List of document texts
        relevance: Dict[query_idx, List[relevant_doc_idx]]
    """
    query_embeddings = model.encode(queries)
    corpus_embeddings = model.encode(corpus)
    
    metrics = {'mrr': [], 'recall@10': [], 'ndcg@10': []}
    
    for i, query_emb in enumerate(query_embeddings):
        # Compute similarities
        similarities = cosine_similarity([query_emb], corpus_embeddings)[0]
        
        # Rank documents
        ranked_indices = np.argsort(similarities)[::-1]
        
        # Get relevant documents
        relevant = set(relevance.get(i, []))
        
        # MRR (Mean Reciprocal Rank)
        for rank, idx in enumerate(ranked_indices, 1):
            if idx in relevant:
                metrics['mrr'].append(1.0 / rank)
                break
        else:
            metrics['mrr'].append(0)
        
        # Recall@10
        retrieved_10 = set(ranked_indices[:10])
        metrics['recall@10'].append(len(retrieved_10 & relevant) / len(relevant))
        
        # nDCG@10
        dcg = sum(1/np.log2(rank+2) for rank, idx in enumerate(ranked_indices[:10]) 
                 if idx in relevant)
        idcg = sum(1/np.log2(i+2) for i in range(min(10, len(relevant))))
        metrics['ndcg@10'].append(dcg / idcg if idcg > 0 else 0)
    
    return {k: np.mean(v) for k, v in metrics.items()}
```

### Visualization

```python
import umap
import matplotlib.pyplot as plt

def visualize_embeddings(embeddings, labels=None):
    """Reduce to 2D and plot."""
    reducer = umap.UMAP(n_components=2, random_state=42)
    embeddings_2d = reducer.fit_transform(embeddings)
    
    plt.figure(figsize=(12, 8))
    scatter = plt.scatter(
        embeddings_2d[:, 0],
        embeddings_2d[:, 1],
        c=labels if labels is not None else None,
        cmap='tab10',
        alpha=0.6
    )
    if labels is not None:
        plt.colorbar(scatter)
    plt.title('Embedding Space Visualization')
    plt.savefig('embeddings_viz.png', dpi=150)
```

---

## Deployment and ONNX Export

### Converting to ONNX

```python
import torch
from transformers import AutoModel, AutoTokenizer
import onnx

# Load model
model = AutoModel.from_pretrained('./my_model')
tokenizer = AutoTokenizer.from_pretrained('./my_model')

model.eval()

# Dummy input
dummy_input = tokenizer(
    "Sample text for export",
    return_tensors='pt',
    padding='max_length',
    max_length=128,
    truncation=True
)

# Export to ONNX
torch.onnx.export(
    model,
    (dummy_input['input_ids'], dummy_input['attention_mask']),
    'model.onnx',
    input_names=['input_ids', 'attention_mask'],
    output_names=['last_hidden_state'],
    dynamic_axes={
        'input_ids': {0: 'batch_size', 1: 'sequence'},
        'attention_mask': {0: 'batch_size', 1: 'sequence'},
        'last_hidden_state': {0: 'batch_size', 1: 'sequence'}
    },
    opset_version=14
)

# Verify
onnx_model = onnx.load('model.onnx')
onnx.checker.check_model(onnx_model)
print("ONNX export successful!")
```

### ONNX Optimization

```python
from onnxruntime.transformers import optimizer
from onnxruntime.transformers.fusion_options import FusionOptions

# Optimize for inference
options = FusionOptions('bert')
options.enable_skip_layer_norm = True
options.enable_attention = True

optimized_model = optimizer.optimize_model(
    'model.onnx',
    model_type='bert',
    num_heads=12,
    hidden_size=384,
    optimization_options=options
)

optimized_model.save_model_to_file('model_optimized.onnx')
```

### Integration with Vector Studio

```powershell
# Place model in cache directory
Copy-Item model_optimized.onnx ~/.cache/vector_studio/models/custom_model.onnx

# Update config
@"
{
    "embedding": {
        "text_model": "custom_model.onnx"
    }
}
"@ | Out-File -FilePath ./my_db/config.json
```

---

## Advanced Topics

### Knowledge Distillation

Train smaller model from larger teacher:

```python
class DistillationTrainer:
    def __init__(self, teacher_model, student_model, temperature=2.0):
        self.teacher = teacher_model
        self.student = student_model
        self.temperature = temperature
        
        self.teacher.eval()
        for param in self.teacher.parameters():
            param.requires_grad = False
    
    def distillation_loss(self, texts, labels):
        # Teacher predictions
        with torch.no_grad():
            teacher_emb = self.teacher.encode(texts)
        
        # Student predictions
        student_emb = self.student.encode(texts)
        
        # Soft target loss (MSE between embeddings)
        mse_loss = F.mse_loss(student_emb, teacher_emb)
        
        # Hard target loss (task-specific)
        task_loss = self.compute_task_loss(student_emb, labels)
        
        # Combined
        return 0.5 * mse_loss + 0.5 * task_loss
```

### Retrieval-Augmented Training

Use retrieval to enhance training:

```python
def retrieval_augmented_batch(query, db, model, tokenizer):
    """
    Retrieve relevant documents to augment training.
    """
    # Retrieve similar documents
    results = db.search(query, k=5)
    
    # Use as additional context
    context = " ".join([r.text for r in results])
    augmented = f"{context}\n\n{query}"
    
    return tokenizer(augmented, ...)
```

### Curriculum Learning

Progressive difficulty during training:

```python
def curriculum_sampling(dataset, epoch, num_epochs):
    """
    Start with easy examples, gradually add harder ones.
    """
    # Sort by difficulty (similarity of positive pair)
    difficulties = [compute_difficulty(ex) for ex in dataset]
    sorted_indices = np.argsort(difficulties)
    
    # Take fraction based on epoch
    fraction = 0.3 + 0.7 * (epoch / num_epochs)  # 30% → 100%
    num_samples = int(len(dataset) * fraction)
    
    return [dataset[i] for i in sorted_indices[:num_samples]]
```

### Continual Learning

Update model without forgetting:

```python
class EWC:
    """Elastic Weight Consolidation for continual learning."""
    
    def __init__(self, model, previous_dataset, lambda_ewc=0.4):
        self.model = model
        self.lambda_ewc = lambda_ewc
        
        # Compute Fisher information
        self.fisher = self.compute_fisher(previous_dataset)
        
        # Store previous parameters
        self.previous_params = {
            name: param.clone() 
            for name, param in model.named_parameters()
        }
    
    def penalty(self):
        """Regularization to prevent catastrophic forgetting."""
        loss = 0
        for name, param in self.model.named_parameters():
            fisher = self.fisher[name]
            prev = self.previous_params[name]
            loss += (fisher * (param - prev) ** 2).sum()
        
        return self.lambda_ewc * loss
```

---

## Training Checklist

### Before Training

- [ ] Collect and clean training data
- [ ] Choose base model or architecture
- [ ] Set up evaluation benchmarks
- [ ] Configure hardware (GPU/TPU)
- [ ] Set random seeds for reproducibility

### During Training

- [ ] Monitor loss curves
- [ ] Track evaluation metrics
- [ ] Save checkpoints regularly
- [ ] Watch for overfitting (eval > train)
- [ ] Adjust learning rate if needed

### After Training

- [ ] Evaluate on held-out test set
- [ ] Compare to pretrained baseline
- [ ] Export to ONNX
- [ ] Optimize for inference
- [ ] Test integration with Vector Studio
- [ ] Document model card

---

## Resources

### Papers
- BERT: Devlin et al. (2018) - "BERT: Pre-training of Deep Bidirectional Transformers"
- Sentence-BERT: Reimers & Gurevych (2019) - "Sentence-BERT: Sentence Embeddings using Siamese BERT-Networks"
- SimCSE: Gao et al. (2021) - "SimCSE: Simple Contrastive Learning of Sentence Embeddings"
- E5: Wang et al. (2022) - "Text Embeddings by Weakly-Supervised Contrastive Pre-training"

### Libraries
- [Sentence-Transformers](https://www.sbert.net/)
- [Hugging Face Transformers](https://huggingface.co/transformers/)
- [ONNX Runtime](https://onnxruntime.ai/)

### Datasets
- [MTEB Benchmark](https://huggingface.co/spaces/mteb/leaderboard)
- [STS Benchmark](http://ixa2.si.ehu.eus/stswiki/index.php/STSbenchmark)
- [MS MARCO](https://microsoft.github.io/msmarco/)
