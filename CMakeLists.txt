# ============================================================================
# VectorDB - High-Performance Vector Database for Gold Standard
# ============================================================================
cmake_minimum_required(VERSION 3.20)
project(VectorDB VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Build Configuration
# ============================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable position independent code globally for shared library support
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================================
# Options
# ============================================================================
option(VDB_BUILD_TESTS "Build unit tests" ON)
option(VDB_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(VDB_BUILD_PYTHON "Build Python bindings" ON)
option(VDB_USE_AVX2 "Enable AVX2 SIMD optimizations" ON)
option(VDB_USE_AVX512 "Enable AVX-512 SIMD optimizations" OFF)
option(VDB_ENABLE_GPU "Enable GPU acceleration via ONNX DirectML/CUDA" OFF)
option(VDB_USE_LLAMA_CPP "Enable llama.cpp for local LLM inference" ON)
option(VDB_USE_ONNX_RUNTIME "Enable ONNX Runtime for text/image encoders (requires MSVC)" OFF)

# ============================================================================
# Compiler Flags
# ============================================================================
if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(VDB_USE_AVX2)
        add_compile_options(/arch:AVX2)
    endif()
    if(VDB_USE_AVX512)
        add_compile_options(/arch:AVX512)
    endif()
    # Release optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /Ot /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(VDB_USE_AVX2)
        add_compile_options(-mavx2 -mfma)
    endif()
    if(VDB_USE_AVX512)
        add_compile_options(-mavx512f -mavx512dq)
    endif()
    # Release optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -flto")
endif()

# ============================================================================
# Dependencies (FetchContent for header-only libs)
# ============================================================================
include(FetchContent)

# nlohmann/json - JSON parsing
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# {fmt} - Fast formatting
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

# ONNX Runtime - Download pre-built binaries (MSVC only)
# libcurl - HTTP client
find_package(CURL)
if(CURL_FOUND)
    set(HAVE_CURL TRUE)
    message(STATUS "CURL found - HTTP client enabled")
else()
    message(STATUS "CURL not found - HTTP client disabled")
endif()

# SQLite3 - Metadata and caching
find_package(SQLite3)
if(SQLite3_FOUND)
    set(HAVE_SQLITE3 TRUE)
    message(STATUS "SQLite3 found - Metadata and caching enabled")
else()
    message(STATUS "SQLite3 not found - Metadata and caching disabled")
endif()

# ZLIB - For PNG decompression in stb_image
find_package(ZLIB)
if(ZLIB_FOUND)
    set(HAVE_ZLIB TRUE)
    message(STATUS "ZLIB found - PNG decompression enabled")
else()
    message(STATUS "ZLIB not found - PNG decompression disabled")
endif()

# Optional: Poppler for PDF support
find_package(Poppler COMPONENTS cpp)
if(Poppler_FOUND)
    set(HAVE_POPPLER TRUE)
    message(STATUS "Poppler found - PDF support enabled")
    add_compile_definitions(HAVE_POPPLER)
endif()

# Optional: ICU for text encoding detection
find_package(ICU COMPONENTS uc i18n)
if(ICU_FOUND)
    set(HAVE_ICU TRUE)
    message(STATUS "ICU found - Advanced text encoding support enabled")
    add_compile_definitions(HAVE_ICU)
endif()

# Optional: libxlsxwriter for Excel support  
find_library(XLSXWRITER_LIB xlsxwriter)
if(XLSXWRITER_LIB)
    set(HAVE_XLSXWRITER TRUE)
    message(STATUS "libxlsxwriter found - Excel support enabled")
    add_compile_definitions(HAVE_XLSXWRITER)
endif()

# Optional: Poppler for PDF support
find_package(Poppler COMPONENTS cpp)
if(Poppler_FOUND)
    set(HAVE_POPPLER TRUE)
    message(STATUS "Poppler found - PDF support enabled")
    add_compile_definitions(HAVE_POPPLER)
endif()

# Optional: PostgreSQL for pgvector support
find_package(PostgreSQL)
if(PostgreSQL_FOUND)
    set(HAVE_POSTGRESQL TRUE)
    message(STATUS "PostgreSQL found - pgvector support enabled")
    add_compile_definitions(HAVE_POSTGRESQL)
    include_directories(${PostgreSQL_INCLUDE_DIRS})
else()
    message(STATUS "PostgreSQL not found - pgvector support disabled")
    message(STATUS "  Install: apt-get install libpq-dev (Ubuntu)")
    message(STATUS "           brew install postgresql (macOS)")
    message(STATUS "           vcpkg install libpq (Windows)")
endif()

# ONNX Runtime - Download pre-built binaries
set(ONNX_VERSION "1.16.3")
if(VDB_USE_ONNX_RUNTIME)
    if(NOT MSVC)
        message(WARNING "ONNX Runtime pre-built binaries require MSVC. Disabling ONNX Runtime.")
        set(VDB_USE_ONNX_RUNTIME OFF CACHE BOOL "" FORCE)
    else()
        if(WIN32)
            set(ONNX_PLATFORM "win-x64")
            set(ONNX_EXT "zip")
        else()
            set(ONNX_PLATFORM "linux-x64")
            set(ONNX_EXT "tgz")
        endif()

        set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-${ONNX_PLATFORM}-${ONNX_VERSION}.${ONNX_EXT}")
        set(ONNX_DIR "${CMAKE_BINARY_DIR}/onnxruntime")

        if(NOT EXISTS "${ONNX_DIR}")
            message(STATUS "Downloading ONNX Runtime ${ONNX_VERSION}...")
            FetchContent_Declare(
                onnxruntime
                URL ${ONNX_URL}
                SOURCE_DIR ${ONNX_DIR}
            )
            FetchContent_MakeAvailable(onnxruntime)
        endif()

        # Find ONNX Runtime
        find_library(ONNX_RUNTIME_LIB 
            NAMES onnxruntime
            PATHS "${ONNX_DIR}/lib"
            NO_DEFAULT_PATH
        )
        set(ONNX_RUNTIME_INCLUDE "${ONNX_DIR}/include")
    endif()
endif()

# ============================================================================
# llama.cpp - Local LLM Inference
# ============================================================================
if(VDB_USE_LLAMA_CPP)
    message(STATUS "Fetching llama.cpp...")
    FetchContent_Declare(
        llama_cpp
        GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
        GIT_TAG b4399  # Use proper tag format
        GIT_SHALLOW TRUE
    )
    
    # Configure llama.cpp options
    set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
    set(LLAMA_NATIVE ON CACHE BOOL "" FORCE)
    
    # GPU acceleration options
    if(VDB_ENABLE_GPU)
        if(WIN32)
            # Use DirectML on Windows for broad GPU support
            # Alternatively use CUDA if available
            find_package(CUDAToolkit QUIET)
            if(CUDAToolkit_FOUND)
                set(GGML_CUDA ON CACHE BOOL "" FORCE)
                message(STATUS "llama.cpp: Using CUDA acceleration")
            else()
                # DirectML not directly supported in llama.cpp yet
                # Fall back to CPU with optimizations
                message(STATUS "llama.cpp: GPU not available, using CPU")
            endif()
        else()
            find_package(CUDAToolkit QUIET)
            if(CUDAToolkit_FOUND)
                set(GGML_CUDA ON CACHE BOOL "" FORCE)
                message(STATUS "llama.cpp: Using CUDA acceleration")
            endif()
        endif()
    endif()
    
    FetchContent_MakeAvailable(llama_cpp)
endif()

# ============================================================================
# Core Library
# ============================================================================
set(VDB_CORE_SOURCES
    src/core/vector_ops.cpp
    src/core/distance.cpp
    src/core/thread_pool.cpp
    src/index/hnsw.cpp
    src/index/flat.cpp
    src/index/metadata_index.cpp
    src/storage/mmap_store.cpp
    src/storage/metadata.cpp
    src/database.cpp
    src/ingest/markdown_parser.cpp
    src/ingest/gold_standard_ingest.cpp
    src/llm/llama_engine.cpp
    src/adapters/data_adapter.cpp
    src/adapters/csv_adapter.cpp
    src/adapters/json_adapter.cpp
    src/adapters/fred_adapter.cpp
    src/adapters/http_client.cpp
    src/adapters/pdf_adapter.cpp
    src/adapters/text_adapter.cpp
    src/adapters/excel_adapter.cpp
    src/storage/sqlite_store.cpp
    src/quantization/product_quantizer.cpp
    src/quantization/scalar_quantizer.cpp
)

# Add ONNX-dependent sources only if ONNX Runtime is enabled
if(VDB_USE_ONNX_RUNTIME)
    list(APPEND VDB_CORE_SOURCES
        src/embeddings/onnx_runtime.cpp
        src/embeddings/text_encoder.cpp
        src/embeddings/image_encoder.cpp
    )
endif()

add_library(vdb_core STATIC ${VDB_CORE_SOURCES})

target_include_directories(vdb_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(vdb_core PUBLIC
    nlohmann_json::nlohmann_json
    fmt::fmt
)

# ONNX Runtime linking
if(VDB_USE_ONNX_RUNTIME)
    target_include_directories(vdb_core PUBLIC ${ONNX_RUNTIME_INCLUDE})
    target_link_libraries(vdb_core PUBLIC ${ONNX_RUNTIME_LIB})
    target_compile_definitions(vdb_core PUBLIC VDB_USE_ONNX_RUNTIME)
endif()

# Link llama.cpp if enabled
if(VDB_USE_LLAMA_CPP)
    target_link_libraries(vdb_core PUBLIC llama common)
    target_compile_definitions(vdb_core PUBLIC VDB_USE_LLAMA_CPP)
    target_include_directories(vdb_core PUBLIC 
        ${llama_cpp_SOURCE_DIR}/include
        ${llama_cpp_SOURCE_DIR}/common
    )
endif()

# Link core dependencies
if(CURL_FOUND)
    target_link_libraries(vdb_core PUBLIC CURL::libcurl)
endif()

if(SQLite3_FOUND)
    target_link_libraries(vdb_core PUBLIC SQLite::SQLite3)
endif()

if(ZLIB_FOUND)
    target_link_libraries(vdb_core PUBLIC ZLIB::ZLIB)
endif()

# Optional libraries for adapters
if(ICU_FOUND)
    target_link_libraries(vdb_core PUBLIC ICU::uc ICU::i18n)
endif()

if(XLSXWRITER_LIB)
    target_link_libraries(vdb_core PUBLIC ${XLSXWRITER_LIB})
endif()

if(POPPLER_FOUND)
    target_link_libraries(vdb_core PUBLIC poppler-cpp)
endif()

if(PostgreSQL_FOUND)
    target_link_libraries(vdb_core PUBLIC PostgreSQL::PostgreSQL)
endif()

# Platform-specific threading
if(WIN32)
    target_compile_definitions(vdb_core PRIVATE VDB_PLATFORM_WINDOWS)
else()
    target_link_libraries(vdb_core PUBLIC pthread)
    target_compile_definitions(vdb_core PRIVATE VDB_PLATFORM_UNIX)
endif()

# GPU support
if(VDB_ENABLE_GPU)
    target_compile_definitions(vdb_core PRIVATE VDB_GPU_ENABLED)
endif()

# ============================================================================
# CLI Tool
# ============================================================================
add_executable(vdb_cli
    src/cli/main.cpp
)

target_link_libraries(vdb_cli PRIVATE vdb_core)

# ============================================================================
# Python Bindings
# ============================================================================
if(VDB_BUILD_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.11.1
    )
    FetchContent_MakeAvailable(pybind11)
    
    pybind11_add_module(pyvdb
        bindings/python/pyvdb.cpp
    )
    
    target_link_libraries(pyvdb PRIVATE vdb_core)
    
    # Install Python module
    install(TARGETS pyvdb
        LIBRARY DESTINATION ${Python3_SITELIB}
    )
endif()

# ============================================================================
# Tests
# ============================================================================
if(VDB_BUILD_TESTS)
    enable_testing()
    
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    add_executable(vdb_tests
        tests/test_vector_ops.cpp
        tests/test_distance.cpp
        tests/test_hnsw.cpp
        tests/test_storage.cpp
        tests/test_embeddings.cpp
        tests/test_ingest.cpp
    )
    
    target_link_libraries(vdb_tests PRIVATE
        vdb_core
        GTest::gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(vdb_tests)
endif()

# ============================================================================
# Benchmarks
# ============================================================================
if(VDB_BUILD_BENCHMARKS)
    add_executable(vdb_benchmark
        tools/benchmark.cpp
    )
    target_link_libraries(vdb_benchmark PRIVATE vdb_core)
endif()

# ============================================================================
# Installation
# ============================================================================
install(TARGETS vdb_core vdb_cli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/vdb
    DESTINATION include
)

# ============================================================================
# Summary
# ============================================================================
message(STATUS "")
message(STATUS "VectorDB Configuration:")
message(STATUS "  Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "  AVX2:           ${VDB_USE_AVX2}")
message(STATUS "  AVX-512:        ${VDB_USE_AVX512}")
message(STATUS "  GPU Support:    ${VDB_ENABLE_GPU}")
message(STATUS "  Python Bindings: ${VDB_BUILD_PYTHON}")
message(STATUS "  Tests:          ${VDB_BUILD_TESTS}")
message(STATUS "  Benchmarks:     ${VDB_BUILD_BENCHMARKS}")
message(STATUS "")
