# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(OrderMatchingEngine VERSION 1.0.0 LANGUAGES CXX)

# Set high warning levels
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(USE_CONAN "Use Conan package manager" ON)
option(USE_SANITIZERS "Enable sanitizers" OFF)
option(USE_LTO "Enable link-time optimization" ON)

# Include directories
include_directories(include)

# Find dependencies
find_package(Threads REQUIRED)

if(USE_CONAN)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
else()
    # Find system packages
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZMQ REQUIRED libzmq)
    
    include_directories(${ZMQ_INCLUDE_DIRS})
    link_directories(${ZMQ_LIBRARY_DIRS})
endif()

# Add compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
    
    if(USE_SANITIZERS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
    endif()
    
    if(USE_LTO)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    endif()
endif()

# Add executable
add_executable(order-matching-engine
    src/engine/main.cpp
    src/engine/MatchingEngine.cpp
    src/engine/OrderBook.cpp
    src/networking/ZmqInterface.cpp
    src/utils/Logger.cpp
    src/utils/Config.cpp
)

target_include_directories(order-matching-engine PRIVATE include)

if(USE_CONAN)
    target_link_libraries(order-matching-engine ${CONAN_LIBS})
else()
    target_link_libraries(order-matching-engine
        ${ZMQ_LIBRARIES}
        Threads::Threads
    )
endif()

# Add tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS order-matching-engine DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
install(FILES config/config.yaml DESTINATION etc)

# Packaging
set(CPACK_PACKAGE_VENDOR "YourCompany")
set(CPACK_PACKAGE_CONTACT "engineering@yourcompany.com")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
include(CPack)