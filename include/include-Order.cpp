cmake_minimum_required(VERSION 3.10)
project(OrderMatchingEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add ZeroMQ and pthread libraries
find_package(ZeroMQ REQUIRED)
find_package(Threads REQUIRED)

# Add server executable
add_executable(server src/server/ServerCode.cpp)
target_link_libraries(server ${ZEROMQ_LIBRARIES} Threads::Threads)

# Add client executable
add_executable(client src/client/ClientCode.cpp)
target_link_libraries(client ${ZEROMQ_LIBRARIES})
