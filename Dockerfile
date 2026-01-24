# ============================================================================
# Vector Studio Production Dockerfile
# Multi-stage build for optimized production deployment
# Requires C++23 compiler (GCC 13+) - Ubuntu 24.04 provides GCC 14
# ============================================================================

# ============================================================================
# Stage 1: Builder - Build C++ backend and Python bindings
# ============================================================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    wget \
    curl \
    pkg-config \
    # C++ dependencies
    libsqlite3-dev \
    libpq-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    libssl-dev \
    # SIMD/AVX2 support for llama.cpp
    gcc-multilib \
    g++-multilib \
    # Python
    python3.11 \
    python3.11-dev \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Set Python 3.11 as default
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.11 1 && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3.11 1

# Upgrade pip
RUN python3 -m pip install --upgrade pip setuptools wheel

# Copy source code
WORKDIR /build
COPY . .

# Install Python dependencies
RUN pip3 install --no-cache-dir -r requirements.txt && \
    pip3 install --no-cache-dir -r api/requirements.txt

# Download ONNX models
RUN python3 scripts/download_models.py --all || echo "Model download failed, continuing..."

# Configure and build C++ backend with explicit AVX2 support
RUN export CFLAGS="-mavx2 -mfma -mf16c" && \
    export CXXFLAGS="-mavx2 -mfma -mf16c" && \
    cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_C_FLAGS="-mavx2 -mfma -mf16c" \
    -DCMAKE_CXX_FLAGS="-mavx2 -mfma -mf16c" \
    -DVDB_BUILD_PYTHON=ON \
    -DVDB_BUILD_TESTS=OFF \
    -DVDB_USE_AVX2=ON \
    -DVDB_USE_LLAMA_CPP=ON \
    && cmake --build build --target install -j$(nproc)

# Install Python bindings
RUN cd build && \
    pip3 install --no-cache-dir .

# ============================================================================
# Stage 2: Runtime - Minimal production image
# ============================================================================
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1
ENV PATH="/usr/local/bin:$PATH"

# Install runtime dependencies only
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libstdc++6 \
    libgomp1 \
    libsqlite3-0 \
    libpq5 \
    libcurl4 \
    zlib1g \
    libssl3 \
    python3.12 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Set Python 3.12 as default
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.12 1 && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3.12 1

# Copy built artifacts from builder
COPY --from=builder /usr/local /usr/local
COPY --from=builder /build/models /app/models
COPY --from=builder /build/api /app/api
COPY --from=builder /usr/local/lib/python3.12/dist-packages /usr/local/lib/python3.12/dist-packages

# Create app user (non-root)
RUN useradd -m -u 1000 -s /bin/bash appuser && \
    mkdir -p /data /app && \
    chown -R appuser:appuser /data /app

# Set working directory
WORKDIR /app

# Copy API server
COPY --from=builder /build/api /app/api

# Install API dependencies as appuser
USER appuser
RUN pip3 install --user --no-cache-dir -r api/requirements.txt

# Add user pip bin to PATH
ENV PATH="/home/appuser/.local/bin:$PATH"

# Expose API port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=40s --retries=3 \
    CMD python3 -c "import requests; requests.get('http://localhost:8080/health', timeout=5)" || exit 1

# Default command: Run API server
CMD ["python3", "-m", "uvicorn", "api.main:app", "--host", "0.0.0.0", "--port", "8080", "--workers", "4"]

