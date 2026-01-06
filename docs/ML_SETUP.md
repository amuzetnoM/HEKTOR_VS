# ML Framework Setup Guide

Quick guide for enabling TensorFlow and PyTorch embedding support in Vector Studio.

## TensorFlow Integration

### Install TensorFlow C++ API

```bash
# Install TensorFlow Python package
pip install tensorflow>=2.15.0

# Build Vector Studio with TensorFlow support
export TensorFlow_ROOT=$(python3 -c "import tensorflow as tf; import os; print(os.path.dirname(tf.sysconfig.get_include()))")
cmake -DVDB_USE_TENSORFLOW=ON -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Usage

```cpp
#include "vdb/framework_integration.hpp"

TensorFlowConfig config;
config.model_path = "model.savedmodel";
config.device = "CPU";  // or "GPU"

TensorFlowEmbedder embedder(config);
auto result = embedder.embed("Hello world");
```

## PyTorch Integration

### Install LibTorch

```bash
# Download LibTorch
wget https://download.pytorch.org/libtorch/cpu/libtorch-shared-with-deps-latest.zip
unzip libtorch-shared-with-deps-latest.zip

# Build Vector Studio with PyTorch support
export Torch_ROOT=/path/to/libtorch
cmake -DVDB_USE_TORCH=ON -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Usage

```cpp
#include "vdb/framework_integration.hpp"

PyTorchConfig config;
config.model_path = "model.pt";
config.device = "cpu";  // or "cuda"

PyTorchEmbedder embedder(config);
auto result = embedder.embed("Hello world");
```

## Verification

```bash
# Check if TensorFlow is enabled
cmake . | grep TensorFlow

# Check if PyTorch is enabled
cmake . | grep Torch

# Run tests
make test
```

## Troubleshooting

**Issue:** `TensorFlowEmbedder requires TensorFlow C++ API`
- **Solution:** Install TensorFlow and rebuild with `-DVDB_USE_TENSORFLOW=ON`

**Issue:** `PyTorchEmbedder requires LibTorch C++ API`
- **Solution:** Download LibTorch and rebuild with `-DVDB_USE_TORCH=ON`

**Issue:** CMake cannot find TensorFlow/PyTorch
- **Solution:** Set `TensorFlow_ROOT` or `Torch_ROOT` environment variables

## Full Documentation

See `docs/FRAMEWORK_INTEGRATION.md` for complete installation instructions, performance tuning, and advanced usage.
