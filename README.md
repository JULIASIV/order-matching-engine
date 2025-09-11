# High-Performance C++ Order Matching Engine

A low-latency, multi-threaded order matching engine designed for electronic trading systems.

## Features

- **Ultra-Low Latency**: Lock-free data structures and efficient algorithms
- **Multi-threaded Architecture**: Scales across CPU cores
- **ZeroMQ Integration**: High-throughput messaging
- **Multiple Order Types**: Limit, market, iceberg, and more
- **Real-time Data**: Order book depth and trade streaming
- **Persistence**: Redis integration for fault tolerance
- **Monitoring**: Performance metrics and health checks

## Building

### Prerequisites

- C++20 compatible compiler (GCC 11+, Clang 12+, MSVC 2019+)
- CMake 3.15+
- ZeroMQ
- Redis++ (C++ Redis client)
- Boost (system, thread)

### Build Steps

```bash
git clone https://github.com/JULIASIV/order-matching-engine.git
cd order-matching-engine
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)