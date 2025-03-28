# order-matching-engine
A multi-threaded order matching engine with ZeroMQ integration
# trading-low-leatency
 

# Order Matching Engine

A multi-threaded order matching engine with ZeroMQ integration for real-time trading.

## Features
- Multi-threaded order matching.
- ZeroMQ for client-server communication.
- Lock-free queues for low-latency performance.
- Persistent storage for order book state.

## Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/JULIASIV/order-matching-engine.git
--file structure
   order-matching-engine/
├── src/
│   ├── client/
│   │   ├── ClientCode.cpp
│   │   └── UpdatedClientCode.cpp
│   ├── server/
│   │   ├── ServerCode.cpp
│   │   ├── UpdatedServerCode.cpp
│   │   └── AdvancedThreadManagement.cpp
│   ├── matching/
│   │   ├── OrderMatchingOptimization.cpp
│   │   └── OrderMatchingEngine.cpp
│   ├── zmq/
│   │   ├── EnhancedZeroMQIntegration.cpp
│   │   └── ZeroMQServer.cpp
│   └── utils/
│       ├── PersistentStorage.cpp
│       └── SystemMonitoring.cpp
├── include/
│   ├── Order.h
│   └── OrderBook.h
├── docs/
│   ├── notes.cpp
│   └── some_notes.cpp
├── CMakeLists.txt
└── README.md
