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
# Note: gcc-multilib/g++-multilib are x86-only, install conditionally based on architecture
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
    # Python 3.12 (default in Ubuntu 24.04)
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Install multilib packages only on x86 (for SIMD/AVX2 support with llama.cpp)
RUN if [ "$(dpkg --print-architecture)" = "amd64" ]; then \
        apt-get update && apt-get install -y --no-install-recommends gcc-multilib g++-multilib && \
        rm -rf /var/lib/apt/lists/*; \
    fi

# Upgrade pip - use --ignore-installed to avoid conflicts with Debian-managed packages
RUN python3 -m pip install --ignore-installed --break-system-packages pip setuptools wheel

# Copy source code
WORKDIR /build
COPY . .

# Install Python dependencies
RUN pip3 install --no-cache-dir --break-system-packages -r requirements.txt && \
    pip3 install --no-cache-dir --break-system-packages -r api/requirements.txt

# Download ONNX models (create directory even if download fails)
RUN mkdir -p models && \
    (python3 scripts/download_models.py --all || echo "Model download failed, continuing...")

# Configure and build C++ backend
# Use architecture-specific flags (AVX2 for x86, NEON for ARM64)
RUN if [ "$(dpkg --print-architecture)" = "amd64" ]; then \
        export CFLAGS="-mavx2 -mfma -mf16c" && \
        export CXXFLAGS="-mavx2 -mfma -mf16c" && \
        cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_C_FLAGS="-mavx2 -mfma -mf16c" \
        -DCMAKE_CXX_FLAGS="-mavx2 -mfma -mf16c" \
        -DVDB_BUILD_PYTHON=ON \
        -DVDB_BUILD_TESTS=OFF \
        -DVDB_USE_AVX2=ON \
        -DVDB_USE_LLAMA_CPP=ON; \
    else \
        cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DVDB_BUILD_PYTHON=ON \
        -DVDB_BUILD_TESTS=OFF \
        -DVDB_USE_AVX2=OFF \
        -DVDB_USE_LLAMA_CPP=ON; \
    fi && \
    cmake --build build --target install -j$(nproc)

# Install Python bindings from source root
RUN pip3 install --no-cache-dir --break-system-packages /build

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

# Create app user (non-root) - use -o to allow non-unique UID if it exists
RUN useradd -m -o -u 1000 -s /bin/bash appuser 2>/dev/null || \
    (getent passwd 1000 && usermod -l appuser $(id -nu 1000) && usermod -d /home/appuser -m appuser) || \
    useradd -m -s /bin/bash appuser && \
    mkdir -p /data /app && \
    chown -R appuser:appuser /data /app

# Set working directory
WORKDIR /app

# Copy API server
COPY --from=builder /build/api /app/api

# Install API dependencies as appuser (use --break-system-packages for Ubuntu 24.04 PEP 668)
USER appuser
RUN pip3 install --user --no-cache-dir --break-system-packages -r api/requirements.txt

# Add user pip bin to PATH
ENV PATH="/home/appuser/.local/bin:$PATH"

# Expose API port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=40s --retries=3 \
    CMD python3 -c "import requests; requests.get('http://localhost:8080/health', timeout=5)" || exit 1

# Default command: Run API server
CMD ["python3", "-m", "uvicorn", "api.main:app", "--host", "0.0.0.0", "--port", "8080", "--workers", "4"]

