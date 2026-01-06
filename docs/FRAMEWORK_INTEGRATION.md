# Framework Integration Guide

## Overview

Vector Studio v2.2.0 provides integration with TensorFlow and PyTorch for custom embedding models. This guide explains how to enable and use these features.

## TensorFlow Integration

### Installation

#### Option 1: pip (Recommended)
```bash
# Install TensorFlow
pip install tensorflow>=2.15.0

# Find TensorFlow C++ headers location
python3 -c "import tensorflow as tf; print(tf.sysconfig.get_include())"

# Set environment variable
export TensorFlow_ROOT=$(python3 -c "import tensorflow as tf; import os; print(os.path.dirname(tf.sysconfig.get_include()))")
```

#### Option 2: From Source
```bash
# Clone TensorFlow
git clone https://github.com/tensorflow/tensorflow.git
cd tensorflow

# Build C++ library
bazel build //tensorflow:libtensorflow_cc.so

# Install
sudo cp bazel-bin/tensorflow/libtensorflow_cc.so* /usr/local/lib/
sudo cp -r tensorflow/cc /usr/local/include/tensorflow/
sudo ldconfig

# Set path
export TensorFlow_ROOT=/usr/local
```

#### Option 3: Pre-built Binaries
Download from: https://www.tensorflow.org/install/lang_c

### Build with TensorFlow Support

```bash
cmake -DVDB_USE_TENSORFLOW=ON \
      -DTensorFlow_ROOT=/path/to/tensorflow \
      -DCMAKE_BUILD_TYPE=Release \
      ..
make -j$(nproc)
```

### Usage

```cpp
#include "vdb/framework_integration.hpp"

using namespace vdb::framework;

// Configure TensorFlow embedder
TensorFlowConfig config;
config.model_path = "/path/to/saved_model";
config.use_gpu = true;
config.num_threads = 4;

// Create embedder
TensorFlowEmbedder embedder(config);

// Generate embeddings
auto result = embedder.embed("sample text");
if (result.is_ok()) {
    Vector embedding = result.unwrap();
    // Use embedding for search
}

// Batch processing
std::vector<std::string> texts = {"text1", "text2", "text3"};
auto batch_result = embedder.embed_batch(texts);
```

### Export for Training

```cpp
// Export vectors for TensorFlow training
std::vector<Vector> vectors = {...};
std::vector<std::string> labels = {"class1", "class2", ...};

TensorFlowEmbedder::export_for_training(
    vectors,
    labels,
    "training_data.tfrecord"
);
```

### Python Example

```python
import tensorflow as tf
import pyvdb

# Load model
model = tf.saved_model.load("my_model")

# Create Vector Studio database
db = pyvdb.VectorDatabase("vectors.db", dim=768)

# Generate and store embeddings
texts = ["document 1", "document 2", ...]
for i, text in enumerate(texts):
    # Get embedding from TensorFlow
    embedding = model(text).numpy()
    
    # Add to Vector Studio
    db.add(i, embedding, {"text": text})

# Search
query_embedding = model("query text").numpy()
results = db.search(query_embedding, k=10)
```

## PyTorch Integration

### Installation

#### Option 1: Download LibTorch
```bash
# Download from: https://pytorch.org/get-started/locally/
# Select C++ / LibTorch

# Linux (CPU)
wget https://download.pytorch.org/libtorch/cpu/libtorch-shared-with-deps-latest.zip
unzip libtorch-shared-with-deps-latest.zip

# Linux (CUDA 11.8)
wget https://download.pytorch.org/libtorch/cu118/libtorch-shared-with-deps-latest.zip
unzip libtorch-shared-with-deps-latest.zip

# Set path
export Torch_ROOT=/path/to/libtorch
export LD_LIBRARY_PATH=$Torch_ROOT/lib:$LD_LIBRARY_PATH
```

#### Option 2: Build from Source
```bash
git clone --recursive https://github.com/pytorch/pytorch.git
cd pytorch

# Build
python setup.py install

# LibTorch will be in: pytorch/torch/lib
export Torch_ROOT=/path/to/pytorch/torch
```

### Build with PyTorch Support

```bash
cmake -DVDB_USE_TORCH=ON \
      -DTorch_ROOT=/path/to/libtorch \
      -DCMAKE_BUILD_TYPE=Release \
      ..
make -j$(nproc)
```

### Usage

```cpp
#include "vdb/framework_integration.hpp"

using namespace vdb::framework;

// Configure PyTorch embedder
PyTorchConfig config;
config.model_path = "/path/to/model.pt";  // TorchScript model
config.device = "cuda:0";  // or "cpu"
config.use_half_precision = true;  // FP16 for faster inference

// Create embedder
PyTorchEmbedder embedder(config);

// Generate embeddings
auto result = embedder.embed("sample text");
if (result.is_ok()) {
    Vector embedding = result.unwrap();
    // Use embedding for search
}

// Batch processing (efficient on GPU)
std::vector<std::string> texts = {"text1", "text2", "text3"};
auto batch_results = embedder.embed_batch(texts);
```

### Export TorchScript Model

Convert your PyTorch model to TorchScript format:

```python
import torch

# Load your model
model = MyEmbeddingModel()
model.load_state_dict(torch.load("weights.pth"))
model.eval()

# Convert to TorchScript
example_input = torch.randn(1, 512)  # Adjust size
traced_model = torch.jit.trace(model, example_input)

# Save
traced_model.save("model.pt")
```

### Export for Training

```cpp
// Export vectors for PyTorch training
std::vector<Vector> vectors = {...};
std::vector<std::string> labels = {"class1", "class2", ...};

PyTorchEmbedder::export_for_training(
    vectors,
    labels,
    "training_data.pt"
);
```

### Python Example

```python
import torch
import pyvdb

# Load TorchScript model
model = torch.jit.load("model.pt")
model.eval()

# Create Vector Studio database
db = pyvdb.VectorDatabase("vectors.db", dim=768)

# Generate and store embeddings
texts = ["document 1", "document 2", ...]
for i, text in enumerate(texts):
    # Tokenize and get embedding from PyTorch
    with torch.no_grad():
        embedding = model(text).numpy()
    
    # Add to Vector Studio
    db.add(i, embedding, {"text": text})

# Search
with torch.no_grad():
    query_embedding = model("query text").numpy()
results = db.search(query_embedding, k=10)
```

## System Requirements

### Minimum
- **CPU:** x86-64 with AVX2 support
- **RAM:** 8GB (16GB recommended for large models)
- **Disk:** 
  - TensorFlow: ~150MB
  - LibTorch: ~200MB
  - Models: 100MB-10GB depending on size

### Recommended
- **CPU:** Multi-core processor (8+ cores)
- **GPU:** NVIDIA GPU with CUDA 11.8+ (optional)
- **RAM:** 16GB+
- **Disk:** SSD for faster model loading

## Performance Considerations

### Batch Processing
Always use batch operations when processing multiple inputs:

```cpp
// Bad: One at a time
for (const auto& text : texts) {
    auto embedding = embedder.embed(text);
    // Process...
}

// Good: Batch processing
auto embeddings = embedder.embed_batch(texts);
```

### GPU Acceleration
Enable GPU for significant speedup:

```cpp
// TensorFlow
TensorFlowConfig config;
config.use_gpu = true;

// PyTorch
PyTorchConfig config;
config.device = "cuda:0";  // Use first GPU
config.use_half_precision = true;  // FP16 for 2x speedup
```

### Thread Configuration
Set appropriate thread count:

```cpp
config.num_threads = std::thread::hardware_concurrency();
```

## Troubleshooting

### CMake Cannot Find TensorFlow/PyTorch

```bash
# Verify installation
ls $TensorFlow_ROOT/include/tensorflow
ls $Torch_ROOT/include/torch

# If missing, reinstall or set correct path
export TensorFlow_ROOT=/correct/path
export Torch_ROOT=/correct/path
```

### Linking Errors

```bash
# Add to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$TensorFlow_ROOT/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$Torch_ROOT/lib:$LD_LIBRARY_PATH

# Or use rpath during build
cmake -DCMAKE_INSTALL_RPATH="$TensorFlow_ROOT/lib;$Torch_ROOT/lib" ..
```

### Runtime Errors

```bash
# Verify libraries can be loaded
ldd ./vdb_cli | grep tensorflow
ldd ./vdb_cli | grep torch

# Check GPU availability (if using)
nvidia-smi
```

## Current Status (v2.2.0)

### Implemented
- Complete API definitions
- CMake build system integration
- Find modules for TensorFlow and PyTorch
- Stub implementations with clear requirements
- Documentation and examples

### Requires Installation
Both TensorFlow and PyTorch require external C++ libraries:
- **TensorFlow C++ API:** ~150MB, complex build process
- **LibTorch:** ~200MB, straightforward installation

### To Enable
1. Install TensorFlow or LibTorch (see above)
2. Build Vector Studio with appropriate flags:
   ```bash
   cmake -DVDB_USE_TENSORFLOW=ON -DVDB_USE_TORCH=ON ..
   ```
3. Both features are optional and can be enabled independently

### Recommended Approach
- **For Python users:** Use Python bindings with TensorFlow/PyTorch directly (easier)
- **For C++ users:** Install LibTorch (simpler than TensorFlow C++)
- **For production:** Consider using ONNX Runtime (already supported, lighter weight)

## Alternative: ONNX Runtime

Vector Studio already supports ONNX Runtime which is lighter weight:

```cpp
// Convert models to ONNX format
// TensorFlow → ONNX: tf2onnx
// PyTorch → ONNX: torch.onnx.export()

// Then use ONNX Runtime (already in Vector Studio)
#include "vdb/embeddings.hpp"

ONNXEmbedder embedder("model.onnx");
auto embedding = embedder.embed(text);
```

**Benefits:**
- Smaller dependency (~50MB vs 150-200MB)
- Faster inference
- Better cross-platform support
- Already integrated in Vector Studio

## Summary

TensorFlow and PyTorch integrations provide maximum flexibility for custom models, but require large external dependencies. For most use cases, ONNX Runtime (already supported) provides an excellent balance of features and simplicity.

**v2.2.0 Status:**
- APIs defined and ready
- Build system configured
- Requires opt-in installation of external libraries
- Full implementations available once dependencies are installed
