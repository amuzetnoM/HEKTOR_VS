---
title: "Embeddings & Models"
description: "Text and image encoders, model specifications"
version: "4.1.5"
last_updated: "2026-01-24"
sidebar_position: 8
category: "Technical"
---

![Version](https://img.shields.io/badge/version-4.1.5-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--24-green?style=flat-square)
![Models](https://img.shields.io/badge/models-ONNX-blueviolet?style=flat-square)

## Table of Contents

1. [Overview](#overview)
2. [Text Embedding Models](#text-embedding-models)
3. [Image Embedding Models](#image-embedding-models)
4. [Model Comparison](#model-comparison)
5. [Model Cards](#model-cards)
6. [Custom Model Integration](#custom-model-integration)

---

## Overview

Vector Studio uses ONNX-format models for local inference. This document provides detailed specifications and benchmarks for all supported models.

### Model Directory Structure

```
~/.cache/vector_studio/models/
├── all-MiniLM-L6-v2.onnx         # Default text encoder
├── clip-vit-base-patch32.onnx    # Default image encoder
├── all-mpnet-base-v2.onnx        # High-quality text (optional)
├── multilingual-e5-base.onnx     # Multi-language (optional)
└── custom/
    └── my_model.onnx             # User models
```

---

## Text Embedding Models

### all-MiniLM-L6-v2 (Default)

The default text embedding model, optimized for speed and quality balance.

| Property | Value |
|----------|-------|
| **Full Name** | sentence-transformers/all-MiniLM-L6-v2 |
| **Architecture** | BERT (MiniLM distilled) |
| **Layers** | 6 |
| **Hidden Size** | 384 |
| **Attention Heads** | 12 |
| **Parameters** | 22.7M |
| **Output Dimension** | 384 |
| **Max Tokens** | 256 |
| **File Size** | ~23 MB (ONNX) |
| **License** | Apache 2.0 |

#### Performance

| Metric | Value |
|--------|-------|
| Inference Speed (CPU) | ~2ms per sentence |
| Inference Speed (GPU) | ~0.3ms per sentence |
| STS Benchmark | 0.783 (Spearman) |
| Memory (Inference) | ~150 MB |

#### Training Details

- **Base Model**: microsoft/MiniLM-L6-v2
- **Training Data**: 1B+ sentence pairs
- **Training Objective**: Contrastive learning (MultipleNegativesRankingLoss)
- **Distillation**: From larger teacher models

#### Usage

```python
# Automatic in Vector Studio
db.add_text("Your text here")

# Manual ONNX inference
import onnxruntime as ort
import numpy as np

session = ort.InferenceSession("all-MiniLM-L6-v2.onnx")

# Prepare inputs (tokenized)
inputs = {
    "input_ids": input_ids,
    "attention_mask": attention_mask
}

# Run inference
outputs = session.run(None, inputs)
embeddings = mean_pooling(outputs[0], attention_mask)
embeddings = embeddings / np.linalg.norm(embeddings, axis=1, keepdims=True)
```

---

### all-MiniLM-L12-v2

Larger variant with improved quality.

| Property | Value |
|----------|-------|
| **Architecture** | BERT (MiniLM distilled) |
| **Layers** | 12 |
| **Hidden Size** | 384 |
| **Parameters** | 33.4M |
| **Output Dimension** | 384 |
| **File Size** | ~45 MB (ONNX) |
| **STS Benchmark** | 0.789 |

#### When to Use

- Need better accuracy than L6
- Can accept ~1.5x slower inference
- Memory not constrained

---

### all-mpnet-base-v2

High-quality model for maximum accuracy.

| Property | Value |
|----------|-------|
| **Architecture** | MPNet |
| **Layers** | 12 |
| **Hidden Size** | 768 |
| **Parameters** | 109M |
| **Output Dimension** | 768 |
| **Max Tokens** | 384 |
| **File Size** | ~420 MB (ONNX) |
| **STS Benchmark** | 0.831 |

#### Advantages

- Best quality among general-purpose models
- Better long-text handling (384 tokens)
- More nuanced semantic understanding

#### Trade-offs

- 5x larger than MiniLM-L6
- ~3x slower inference
- Requires projection to 512-dim for Vector Studio

---

### e5-base-v2

State-of-the-art retrieval model.

| Property | Value |
|----------|-------|
| **Architecture** | BERT-base |
| **Layers** | 12 |
| **Hidden Size** | 768 |
| **Parameters** | 109M |
| **Output Dimension** | 768 |
| **Max Tokens** | 512 |
| **File Size** | ~440 MB (ONNX) |

#### Special Usage

E5 models require query/passage prefixes:

```python
# For queries
query = "query: What is the gold price forecast?"

# For documents
doc = "passage: Gold is expected to reach $5,000 by 2026."
```

#### Performance (MTEB Benchmark)

| Task | Score |
|------|-------|
| Retrieval | 0.521 |
| STS | 0.843 |
| Classification | 0.795 |
| Clustering | 0.465 |

---

### multilingual-e5-base

Multi-language support for non-English content.

| Property | Value |
|----------|-------|
| **Languages** | 100+ |
| **Layers** | 12 |
| **Hidden Size** | 768 |
| **Parameters** | 278M |
| **Output Dimension** | 768 |
| **File Size** | ~1.1 GB (ONNX) |

#### Supported Languages

English, Spanish, French, German, Chinese, Japanese, Korean, Arabic, Russian, Portuguese, Italian, Dutch, Polish, Turkish, Vietnamese, Thai, Indonesian, Hindi, and 80+ more.

#### Cross-Lingual Search

```python
# English query finds Chinese documents
results = db.search("gold price analysis")  # Finds: 黄金价格分析
```

---

## Image Embedding Models

### CLIP ViT-B/32 (Default)

Default image encoder using CLIP's vision transformer.

| Property | Value |
|----------|-------|
| **Full Name** | openai/clip-vit-base-patch32 |
| **Architecture** | Vision Transformer |
| **Patch Size** | 32×32 |
| **Input Size** | 224×224 |
| **Layers** | 12 |
| **Hidden Size** | 768 |
| **Parameters** | 86M |
| **Output Dimension** | 512 |
| **File Size** | ~340 MB (ONNX) |

#### Preprocessing

```python
from PIL import Image
import numpy as np

def preprocess_image(image_path):
    """Prepare image for CLIP inference."""
    img = Image.open(image_path).convert('RGB')
    
    # Resize to 224x224
    img = img.resize((224, 224), Image.BICUBIC)
    
    # To numpy
    img = np.array(img).astype(np.float32) / 255.0
    
    # Normalize with CLIP stats
    mean = np.array([0.48145466, 0.4578275, 0.40821073])
    std = np.array([0.26862954, 0.26130258, 0.27577711])
    img = (img - mean) / std
    
    # CHW format, add batch dim
    img = img.transpose(2, 0, 1)
    img = np.expand_dims(img, 0)
    
    return img
```

#### Image-Text Alignment

CLIP's unique strength is cross-modal search:

```python
# Add image
db.add_image("gold_chart.png", metadata)

# Search with text
results = db.search("bullish flag pattern")  # Finds matching charts!
```

---

### CLIP ViT-B/16

Higher resolution variant for better detail.

| Property | Value |
|----------|-------|
| **Patch Size** | 16×16 |
| **Input Size** | 224×224 |
| **Patches** | 14×14 = 196 |
| **Parameters** | 86M |
| **File Size** | ~340 MB |

#### When to Use

- Charts with fine details
- Small text in images
- Pattern recognition requiring precision

---

### CLIP ViT-L/14

Large variant for maximum accuracy.

| Property | Value |
|----------|-------|
| **Patch Size** | 14×14 |
| **Layers** | 24 |
| **Hidden Size** | 1024 |
| **Parameters** | 304M |
| **Output Dimension** | 768 |
| **File Size** | ~1.2 GB |

#### Trade-offs

- Highest image quality
- Requires dimension projection (768→512)
- ~4x slower than ViT-B/32

---

## Model Comparison

### Text Models

| Model | Dim | Params | Speed | Quality | Size |
|-------|-----|--------|-------|---------|------|
| MiniLM-L6-v2 | 384 | 22.7M | ★★★★★ | ★★★☆☆ | 23 MB |
| MiniLM-L12-v2 | 384 | 33.4M | ★★★★☆ | ★★★★☆ | 45 MB |
| mpnet-base-v2 | 768 | 109M | ★★☆☆☆ | ★★★★★ | 420 MB |
| e5-base-v2 | 768 | 109M | ★★☆☆☆ | ★★★★★ | 440 MB |
| multilingual-e5 | 768 | 278M | ★☆☆☆☆ | ★★★★☆ | 1.1 GB |

### Image Models

| Model | Input | Patches | Speed | Quality | Size |
|-------|-------|---------|-------|---------|------|
| ViT-B/32 | 224 | 49 | ★★★★★ | ★★★☆☆ | 340 MB |
| ViT-B/16 | 224 | 196 | ★★★★☆ | ★★★★☆ | 340 MB |
| ViT-L/14 | 224 | 256 | ★★☆☆☆ | ★★★★★ | 1.2 GB |

### Recommendation Matrix

| Use Case | Recommended Model |
|----------|-------------------|
| General search, fast | MiniLM-L6-v2 |
| Balanced quality/speed | MiniLM-L12-v2 |
| Maximum accuracy | mpnet-base-v2 |
| Retrieval-focused | e5-base-v2 |
| Multi-language | multilingual-e5-base |
| Chart/image search | ViT-B/32 |
| Detailed charts | ViT-B/16 |

---

## Model Cards

### MiniLM-L6-v2 Model Card

```yaml
---
Model: all-MiniLM-L6-v2
Version: 2.0
Date: 2022-01

Intended Use:
  - Semantic search
  - Sentence similarity
  - Paraphrase detection
  - Clustering

Limitations:
  - Max 256 tokens
  - English-focused
  - May struggle with:
    - Very technical jargon
    - Extremely short queries (<3 words)
    - Negation and sarcasm

Training Data:
  - Wikipedia (English)
  - Reddit (filtered)
  - Stack Exchange
  - S2ORC (academic)

Ethical Considerations:
  - May reflect biases in training data
  - Not suitable for high-stakes decisions
  - Review embeddings for fairness

Citation:
  @article{reimers2019sentence,
    title={Sentence-BERT},
    author={Reimers, Nils and Gurevych, Iryna},
    journal={EMNLP},
    year={2019}
  }
---
```

### CLIP ViT-B/32 Model Card

```yaml
---
Model: clip-vit-base-patch32
Version: 1.0
Date: 2021-01

Intended Use:
  - Image search
  - Cross-modal retrieval
  - Zero-shot classification
  - Chart/diagram understanding

Limitations:
  - 224×224 resolution
  - May miss fine details
  - Limited OCR capability
  - Struggles with:
    - Abstract art
    - Medical imagery
    - Text-heavy images

Training Data:
  - 400M image-text pairs
  - Web-scraped (filtered)

Ethical Considerations:
  - May exhibit social biases
  - Western image bias
  - Not for facial recognition

Citation:
  @article{radford2021learning,
    title={Learning Transferable Visual Models},
    author={Radford, Alec and others},
    journal={ICML},
    year={2021}
  }
---
```

---

## Custom Model Integration

### Requirements

Custom models must:
1. Be in ONNX format
2. Accept tokenized input (text) or preprocessed image
3. Output embeddings of consistent dimension

### Adding a Custom Text Model

```python
# 1. Export your model to ONNX
import torch

model = YourModel.from_pretrained("your-model")
model.eval()

dummy = torch.randint(0, 1000, (1, 128))

torch.onnx.export(
    model,
    (dummy,),
    "custom_text.onnx",
    input_names=["input_ids"],
    output_names=["embeddings"],
    dynamic_axes={"input_ids": {0: "batch", 1: "seq"}}
)

# 2. Copy to model directory
# Copy-Item custom_text.onnx ~/.cache/vector_studio/models/

# 3. Update config
config = {
    "embedding": {
        "text_model": "custom_text.onnx",
        "text_dim": 512  # Your model's output dimension
    }
}
```

### Dimension Alignment

If your model outputs different dimensions than 512:

```python
# Option 1: Add projection layer before export
class ProjectedModel(nn.Module):
    def __init__(self, base_model, input_dim, output_dim=512):
        super().__init__()
        self.base = base_model
        self.proj = nn.Linear(input_dim, output_dim)
    
    def forward(self, x):
        emb = self.base(x)
        return self.proj(emb)

# Option 2: Configure Vector Studio to project
config = {
    "embedding": {
        "text_model": "custom_768dim.onnx",
        "text_dim": 768,
        "project_to": 512
    }
}
```

### Validation

```powershell
# Test custom model
python -c "
import onnxruntime as ort
import numpy as np

sess = ort.InferenceSession('~/.cache/vector_studio/models/custom_text.onnx')
print('Inputs:', [i.name for i in sess.get_inputs()])
print('Outputs:', [o.name for o in sess.get_outputs()])

# Test inference
dummy = np.random.randint(0, 1000, (1, 64)).astype(np.int64)
result = sess.run(None, {'input_ids': dummy})
print('Output shape:', result[0].shape)
"
```

---

## Appendix: ONNX Operators

### Supported Operators (ONNX Runtime 1.19+)

| Operator | Use | GPU |
|----------|-----|-----|
| MatMul | Attention, FFN | ✓ |
| Add | Residuals | ✓ |
| LayerNormalization | Normalization | ✓ |
| Softmax | Attention | ✓ |
| Gather | Embeddings | ✓ |
| Reshape | View changes | ✓ |
| Transpose | Attention heads | ✓ |
| Mul | Scaling | ✓ |
| Div | Attention scaling | ✓ |
| Sqrt | Normalization | ✓ |
| Erf | GELU activation | ✓ |
| Tanh | Activations | ✓ |

### Execution Providers

| Provider | Platform | Speed |
|----------|----------|-------|
| CPU | All | Baseline |
| CUDA | NVIDIA | 10-50x |
| DirectML | Windows | 5-20x |
| CoreML | macOS | 5-15x |
| TensorRT | NVIDIA | 20-100x |

Configure in Vector Studio:

```python
config = {
    "embedding": {
        "execution_provider": "CUDAExecutionProvider",
        "device_id": 0
    }
}
```
