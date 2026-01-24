# CMakeLists.txt for HEKTOR Benchmark Suite
cmake_minimum_required(VERSION 3.20)
project(HEKTORBenchmarks VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /O2 /arch:AVX2)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
endif()

# Find dependencies
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../../include)

# Benchmark executables

# 1M Vector Stress Test
add_executable(test_1m_vectors
    database/stress_tests/test_1m_vectors.cpp
)
target_link_libraries(test_1m_vectors PRIVATE Threads::Threads)

# Add more benchmarks here as they are implemented
# add_executable(test_simd_performance database/simd/test_simd_performance.cpp)
# add_executable(test_hnsw_cpp database/hnsw/test_hnsw_performance.cpp)

# Install targets
install(TARGETS test_1m_vectors DESTINATION bin)

# Custom targets for running benchmarks
add_custom_target(run_benchmarks
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_1m_vectors --vectors 1000000 --dimension 512 --threads 8
    DEPENDS test_1m_vectors
    COMMENT "Running benchmark suite..."
)

# Display configuration
message(STATUS "=== HEKTOR Benchmark Suite Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "=========================================")
