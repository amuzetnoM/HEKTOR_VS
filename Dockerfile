# Multi-stage Dockerfile for building Vector Studio (CMake project)
# Builder stage
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git ca-certificates \
        libsqlite3-dev libpq-dev libarrow-dev pkg-config python3 python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . /src

RUN mkdir -p build && cd build \
    && cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. \
    && cmake --build . --target install -j$(nproc)

# Runtime stage
FROM ubuntu:22.04
RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local /usr/local
ENV PATH="/usr/local/bin:$PATH"

# Default to an interactive shell; users may override CMD in docker-compose or when running container
CMD ["/bin/bash"]
