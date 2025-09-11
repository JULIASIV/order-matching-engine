# Dockerfile
FROM ubuntu:20.04 as builder

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    cmake \
    libzmq3-dev \
    pkg-config \
    git \
    && rm -rf /var/lib/apt/lists/*

# Copy source
WORKDIR /src
COPY . .

# Build
RUN mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DUSE_CONAN=OFF .. && \
    make -j$(nproc)

# Runtime image
FROM ubuntu:20.04

# Install runtime dependencies
RUN apt-get update && \
    apt-get install -y \
    libzmq5 \
    && rm -rf /var/lib/apt/lists/*

# Copy built binary
COPY --from=builder /src/build/order-matching-engine /usr/local/bin/
COPY config/config.yaml /etc/order-matching-engine/

# Create user
RUN useradd -m -s /bin/bash engine

# Switch to non-root user
USER engine

# Expose ports
EXPOSE 5555 5556

# Run the application
CMD ["order-matching-engine", "-c", "/etc/order-matching-engine/config.yaml"]