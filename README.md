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

### structer 
order-matching-engine/
├── .github/
│   ├── workflows/
│   │   ├── ci-cd.yml
│   │   └── benchmarks.yml
│   └── ISSUE_TEMPLATE/
├── cmake/
│   ├── FindZeroMQ.cmake
│   ├── FindCppRedis.cmake
│   └── Conan.cmake
├── config/
│   ├── config.yaml
│   ├── logging.yaml
│   └── instruments.csv
├── docs/
│   ├── API.md
│   ├── DEPLOYMENT.md
│   ├── PERFORMANCE.md
│   └── ARCHITECTURE.md
├── include/
│   ├── engine/
│   │   ├── OrderBook.hpp
│   │   ├── MatchingEngine.hpp
│   │   ├── Order.hpp
│   │   ├── Types.hpp
│   │   ├── Constants.hpp
│   │   └── Strategies/
│   ├── networking/
│   │   ├── ZmqInterface.hpp
│   │   ├── Protocol.hpp
│   │   ├── FixAdapter.hpp
│   │   └── WebSocketInterface.hpp
│   ├── persistence/
│   │   ├── StorageInterface.hpp
│   │   ├── RedisStorage.hpp
│   │   ├── ChronicleQueue.hpp
│   │   └── SnapshotManager.hpp
│   ├── risk/
│   │   ├── RiskEngine.hpp
│   │   ├── PositionManager.hpp
│   │   └── LimitsChecker.hpp
│   ├── monitoring/
│   │   ├── Metrics.hpp
│   │   ├── Telemetry.hpp
│   │   └── HealthCheck.hpp
│   ├── utils/
│   │   ├── LockFreeQueue.hpp
│   │   ├── ThreadPool.hpp
│   │   ├── Logger.hpp
│   │   ├── Config.hpp
│   │   ├── Clock.hpp
│   │   └── Statistics.hpp
│   └── api/
│       ├── RestApi.hpp
│       └── AdminInterface.hpp
├── src/
│   ├── engine/
│   │   ├── OrderBook.cpp
│   │   ├── MatchingEngine.cpp
│   │   ├── main.cpp
│   │   └── Strategies/
│   ├── networking/
│   │   ├── ZmqInterface.cpp
│   │   ├── Protocol.cpp
│   │   ├── FixAdapter.cpp
│   │   └── WebSocketInterface.cpp
│   ├── persistence/
│   │   ├── RedisStorage.cpp
│   │   ├── ChronicleQueue.cpp
│   │   └── SnapshotManager.cpp
│   ├── risk/
│   │   ├── RiskEngine.cpp
│   │   ├── PositionManager.cpp
│   │   └── LimitsChecker.cpp
│   ├── monitoring/
│   │   ├── Metrics.cpp
│   │   ├── Telemetry.cpp
│   │   └── HealthCheck.cpp
│   ├── utils/
│   │   ├── LockFreeQueue.cpp
│   │   ├── ThreadPool.cpp
│   │   ├── Logger.cpp
│   │   ├── Config.cpp
│   │   ├── Clock.cpp
│   │   └── Statistics.cpp
│   └── api/
│       ├── RestApi.cpp
│       └── AdminInterface.cpp
├── tests/
│   ├── unit/
│   │   ├── engine/
│   │   ├── networking/
│   │   ├── risk/
│   │   └── utils/
│   ├── integration/
│   │   ├── engine_network/
│   │   ├── persistence/
│   │   └── risk_engine/
│   ├── performance/
│   │   ├── latency/
│   │   ├── throughput/
│   │   └── memory/
│   └── fuzz/
├── scripts/
│   ├── deployment/
│   ├── monitoring/
│   ├── benchmarks/
│   └── provisioning/
├── third_party/
│   ├── conanfile.txt
│   └── patches/
├── Dockerfile
├── docker-compose.yml
├── CMakeLists.txt
└── README.md


### Build Steps

```bash
git clone https://github.com/JULIASIV/order-matching-engine.git
cd order-matching-engine
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
