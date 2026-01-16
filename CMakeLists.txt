# ============================================================================
# VectorDB - High-Performance Vector Database for Gold Standard
# ============================================================================
cmake_minimum_required(VERSION 3.20)
project(VectorDB VERSION 3.0.0 LANGUAGES CXX)

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
option(VDB_USE_TENSORFLOW "Enable TensorFlow C++ API for embeddings" OFF)
option(VDB_USE_PYTORCH "Enable PyTorch C++ API (LibTorch) for embeddings" OFF)
option(VDB_USE_GRPC "Enable gRPC for distributed networking" OFF)
option(VDB_BUILD_DISTRIBUTED "Build distributed features (replication, sharding)" OFF)
option(VDB_USE_PROMETHEUS "Enable Prometheus metrics export" OFF)
option(VDB_USE_OPENTELEMETRY "Enable OpenTelemetry distributed tracing" OFF)

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
    # Release optimizations (LTO disabled for compatibility with GTest)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
endif()

# ============================================================================
# Dependencies (FetchContent for header-only libs)
# ============================================================================
include(FetchContent)

# Add custom Find modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

# Optional: Apache Arrow for Parquet support
find_package(Arrow CONFIG)
if(Arrow_FOUND)
    set(HAVE_ARROW TRUE)
    message(STATUS "Apache Arrow found - Full Parquet support enabled")
    add_compile_definitions(HAVE_ARROW)
    find_package(Parquet CONFIG REQUIRED)
else()
    message(STATUS "Apache Arrow not found - Parquet adapter will use placeholder implementation")
    message(STATUS "  To enable full Parquet support:")
    message(STATUS "    Ubuntu: sudo apt-get install libarrow-dev libparquet-dev")
    message(STATUS "    macOS: brew install apache-arrow")
    message(STATUS "    Windows: vcpkg install arrow:x64-windows")
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
        GIT_TAG b7716  # Latest stable version with GCC 15 fixes
        GIT_SHALLOW TRUE
    )
    
    # Configure llama.cpp options
    set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_COMMON ON CACHE BOOL "" FORCE)
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
    
    # Populate the content first to apply patches
    FetchContent_GetProperties(llama_cpp)
    if(NOT llama_cpp_POPULATED)
        FetchContent_Populate(llama_cpp)
        
        # Patch ggml-impl.h to include immintrin.h for AVX2
        set(GGML_IMPL_H "${llama_cpp_SOURCE_DIR}/ggml/src/ggml-impl.h")
        if(EXISTS "${GGML_IMPL_H}")
            file(READ "${GGML_IMPL_H}" GGML_IMPL_CONTENT)
            # Check if patch is needed (not already applied)
            string(FIND "${GGML_IMPL_CONTENT}" "#if defined(__AVX2__) || defined(__AVX__) || defined(__AVX512F__)" AVX2_PATCH_FOUND)
            if(AVX2_PATCH_FOUND EQUAL -1)
                message(STATUS "Patching ggml-impl.h to include AVX2 intrinsics header")
                # Store original content to verify replacement succeeded
                set(ORIGINAL_CONTENT "${GGML_IMPL_CONTENT}")
                # Insert AVX2 include block before __cplusplus block
                # This is a robust location that should exist in all versions
                string(REGEX REPLACE 
                    "(#endif[^\n]*\n*)(\n*#ifdef __cplusplus)"
                    "\\1\n#if defined(__AVX2__) || defined(__AVX__) || defined(__AVX512F__)\n#include <immintrin.h>\n#endif\n\\2"
                    GGML_IMPL_CONTENT "${GGML_IMPL_CONTENT}")
                # Verify the replacement was successful
                if("${GGML_IMPL_CONTENT}" STREQUAL "${ORIGINAL_CONTENT}")
                    message(FATAL_ERROR "Failed to patch ggml-impl.h: Could not find pattern '#endif' followed by '#ifdef __cplusplus'. This may indicate an incompatible llama.cpp version. Please report this issue with your llama.cpp version.")
                endif()
                # Verify the patch was actually applied
                string(FIND "${GGML_IMPL_CONTENT}" "#if defined(__AVX2__) || defined(__AVX__) || defined(__AVX512F__)" VERIFY_PATCH)
                if(VERIFY_PATCH EQUAL -1)
                    message(FATAL_ERROR "Failed to verify AVX2 patch in ggml-impl.h")
                endif()
                file(WRITE "${GGML_IMPL_H}" "${GGML_IMPL_CONTENT}")
                message(STATUS "Successfully patched ggml-impl.h")
            else()
                message(STATUS "ggml-impl.h already patched, skipping")
            endif()
        endif()
        
        # Add the subdirectories
        add_subdirectory(${llama_cpp_SOURCE_DIR} ${llama_cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif()

# ============================================================================
# TensorFlow C++ API (Optional)
# ============================================================================
if(VDB_USE_TENSORFLOW)
    message(STATUS "Searching for TensorFlow C++ API...")
    find_package(TensorFlow)
    if(TensorFlow_FOUND)
        message(STATUS "TensorFlow found - ML embeddings enabled")
        message(STATUS "  Include: ${TensorFlow_INCLUDE_DIRS}")
        message(STATUS "  Library: ${TensorFlow_LIBRARIES}")
        set(HAVE_TENSORFLOW TRUE)
    else()
        message(WARNING "TensorFlow C++ API not found - ML embeddings disabled")
        message(WARNING "  Install TensorFlow C++ API:")
        message(WARNING "    NOTE: pip install tensorflow only provides Python bindings")
        message(WARNING "    For C++ API, see docs/FRAMEWORK_INTEGRATION.md")
        message(WARNING "    Then set: export TensorFlow_ROOT=/path/to/tensorflow")
        set(VDB_USE_TENSORFLOW OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ============================================================================
# PyTorch C++ API / LibTorch (Optional)
# ============================================================================
if(VDB_USE_TORCH)
    message(STATUS "Searching for LibTorch...")
    find_package(Torch)
    if(Torch_FOUND)
        message(STATUS "LibTorch found - PyTorch embeddings enabled")
        message(STATUS "  Include: ${Torch_INCLUDE_DIRS}")
        message(STATUS "  Libraries: ${Torch_LIBRARIES}")
        set(HAVE_TORCH TRUE)
    else()
        message(WARNING "LibTorch not found - PyTorch embeddings disabled")
        message(WARNING "  NOTE: pip install torch only provides Python bindings")
        message(WARNING "  Download LibTorch C++ API from:")
        message(WARNING "    https://pytorch.org/get-started/locally/")
        message(WARNING "  See docs/FRAMEWORK_INTEGRATION.md for details")
        message(WARNING "  Then set: export Torch_ROOT=/path/to/libtorch")
        set(VDB_USE_TORCH OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ============================================================================
# gRPC - Distributed Networking
# ============================================================================
if(VDB_USE_GRPC)
    message(STATUS "Searching for gRPC...")
    find_package(gRPC CONFIG)
    find_package(Protobuf CONFIG)
    
    if(gRPC_FOUND AND Protobuf_FOUND)
        message(STATUS "gRPC found - Distributed networking enabled")
        message(STATUS "  gRPC version: ${gRPC_VERSION}")
        message(STATUS "  Protobuf version: ${Protobuf_VERSION}")
        set(HAVE_GRPC TRUE)
        
        # Find gRPC C++ plugin
        find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
        if(GRPC_CPP_PLUGIN)
            message(STATUS "  gRPC C++ plugin: ${GRPC_CPP_PLUGIN}")
        else()
            message(WARNING "gRPC C++ plugin not found")
        endif()
    else()
        message(WARNING "gRPC or Protobuf not found - Distributed features disabled")
        message(WARNING "  Install: vcpkg install grpc:x64-windows protobuf:x64-windows")
        set(VDB_USE_GRPC OFF CACHE BOOL "" FORCE)
        set(VDB_BUILD_DISTRIBUTED OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ============================================================================
# Prometheus C++ Client - Metrics Export
# ============================================================================
if(VDB_USE_PROMETHEUS)
    message(STATUS "Searching for Prometheus C++ client...")
    find_package(prometheus-cpp CONFIG)
    
    if(prometheus-cpp_FOUND)
        message(STATUS "Prometheus C++ found - Metrics export enabled")
        set(HAVE_PROMETHEUS TRUE)
    else()
        message(WARNING "Prometheus C++ not found - Metrics export disabled")
        message(WARNING "  Install: vcpkg install prometheus-cpp:x64-windows")
        set(VDB_USE_PROMETHEUS OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ============================================================================
# OpenTelemetry C++ SDK - Distributed Tracing
# ============================================================================
if(VDB_USE_OPENTELEMETRY)
    message(STATUS "Searching for OpenTelemetry C++ SDK...")
    find_package(opentelemetry-cpp CONFIG)
    
    if(opentelemetry-cpp_FOUND)
        message(STATUS "OpenTelemetry C++ SDK found - Distributed tracing enabled")
        set(HAVE_OPENTELEMETRY TRUE)
        add_compile_definitions(VDB_USE_OPENTELEMETRY)
    else()
        message(WARNING "OpenTelemetry C++ SDK not found - Distributed tracing disabled")
        message(WARNING "  Install: vcpkg install opentelemetry-cpp")
        message(WARNING "  Or build from source: https://github.com/open-telemetry/opentelemetry-cpp")
        set(VDB_USE_OPENTELEMETRY OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ============================================================================
# Core Library
# ============================================================================
set(VDB_CORE_SOURCES
    src/core/vector_ops.cpp
    src/core/distance.cpp
    src/core/thread_pool.cpp
    src/core/telemetry.cpp
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
    src/adapters/http_adapter.cpp
    src/adapters/pdf_adapter.cpp
    src/adapters/text_adapter.cpp
    src/adapters/excel_adapter.cpp
    src/adapters/xml_adapter.cpp
    src/adapters/parquet_adapter.cpp
    src/adapters/sqlite_adapter.cpp
    src/adapters/pgvector_adapter.cpp
    src/quantization/product_quantizer.cpp
    src/quantization/scalar_quantizer.cpp
    src/hybrid/bm25_engine.cpp
    src/hybrid/hybrid_search_engine.cpp
    src/framework/rag_engine.cpp
    src/framework/adapters.cpp
)

# Add SQLite-dependent sources only if SQLite3 is available
if(SQLite3_FOUND)
    list(APPEND VDB_CORE_SOURCES src/storage/sqlite_store.cpp)
endif()

# Add CURL-dependent sources only if CURL is available
if(CURL_FOUND)
    list(APPEND VDB_CORE_SOURCES src/adapters/http_client.cpp)
endif()

# Add ONNX-dependent sources only if ONNX Runtime is enabled
if(VDB_USE_ONNX_RUNTIME)
    list(APPEND VDB_CORE_SOURCES
        src/embeddings/onnx_runtime.cpp
        src/embeddings/text_encoder.cpp
        src/embeddings/image_encoder.cpp
    )
endif()

# Add TensorFlow-dependent sources only if TensorFlow is enabled
if(HAVE_TENSORFLOW)
    list(APPEND VDB_CORE_SOURCES
        src/framework/tensorflow_embedder.cpp
    )
endif()

# Add PyTorch-dependent sources only if LibTorch is enabled
if(HAVE_TORCH)
    list(APPEND VDB_CORE_SOURCES
        src/framework/pytorch_embedder.cpp
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
    target_compile_definitions(vdb_core PUBLIC HAVE_SQLITE3)
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
    target_compile_definitions(vdb_core PUBLIC HAVE_LIBPQ)
endif()

# Link Apache Arrow if available
if(HAVE_ARROW)
    target_link_libraries(vdb_core PUBLIC Arrow::arrow_shared Parquet::parquet_shared)
endif()

# Link TensorFlow if available
if(HAVE_TENSORFLOW)
    target_link_libraries(vdb_core PUBLIC TensorFlow::TensorFlow)
    target_compile_definitions(vdb_core PUBLIC VDB_USE_TENSORFLOW)
    target_include_directories(vdb_core PUBLIC ${TensorFlow_INCLUDE_DIRS})
endif()

# Link LibTorch if available
if(HAVE_TORCH)
    target_link_libraries(vdb_core PUBLIC Torch::Torch)
    target_compile_definitions(vdb_core PUBLIC VDB_USE_TORCH)
    target_include_directories(vdb_core PUBLIC ${Torch_INCLUDE_DIRS})
endif()

# Link OpenTelemetry if available
if(HAVE_OPENTELEMETRY)
    target_link_libraries(vdb_core PUBLIC
        opentelemetry-cpp::trace
        opentelemetry-cpp::metrics
        opentelemetry-cpp::logs
        opentelemetry-cpp::otlp_grpc_exporter
        opentelemetry-cpp::ostream_span_exporter
        opentelemetry-cpp::prometheus_exporter
    )
    target_compile_definitions(vdb_core PUBLIC VDB_USE_OPENTELEMETRY)
    message(STATUS "Linked OpenTelemetry libraries")
endif()

# Platform-specific threading
if(WIN32)
    target_compile_definitions(vdb_core PUBLIC VDB_PLATFORM_WINDOWS)
else()
    target_link_libraries(vdb_core PUBLIC pthread)
    target_compile_definitions(vdb_core PUBLIC VDB_PLATFORM_UNIX)
endif()

# GPU support
if(VDB_ENABLE_GPU)
    target_compile_definitions(vdb_core PRIVATE VDB_GPU_ENABLED)
endif()

# ============================================================================
# CLI Tool
# ============================================================================
if(VDB_BUILD_CLI)
    # New modular CLI (hektor)
    add_executable(hektor
        src/cli/main.cpp
        src/cli/cli.cpp
        src/cli/output_formatter.cpp
        src/cli/colors.cpp
        src/cli/interactive_shell.cpp
        src/cli/commands/db_commands.cpp
        src/cli/commands/data_commands.cpp
        src/cli/commands/search_commands.cpp
        src/cli/commands/hybrid_commands.cpp
        src/cli/commands/ingest_commands.cpp
        src/cli/commands/index_commands.cpp
        src/cli/commands/collection_commands.cpp
        src/cli/commands/export_commands.cpp
    )

    target_include_directories(hektor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(hektor PRIVATE
        vdb_core
        fmt::fmt
    )

    # Legacy CLI (vdb_cli)
    add_executable(vdb_cli
        src/cli/main.cpp.old
    )

    target_link_libraries(vdb_cli PRIVATE vdb_core)
endif()

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
    # Note: scikit-build-core handles the installation path automatically
    install(TARGETS pyvdb
        LIBRARY DESTINATION .
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
        tests/test_flat_index.cpp
        tests/test_storage.cpp
        tests/test_embeddings.cpp
        tests/test_ingest.cpp
        tests/test_bm25.cpp
        tests/test_hybrid_search.cpp
        tests/test_rag.cpp
    )
    
    target_link_libraries(vdb_tests PRIVATE
        vdb_core
        GTest::gtest_main
    )
    
    include(GoogleTest)
    # Use DISCOVERY_MODE PRE_TEST to avoid running executable at configure time
    # This prevents issues with MinGW DLLs not being in PATH during cmake configure
    gtest_discover_tests(vdb_tests
        DISCOVERY_MODE PRE_TEST
    )
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
set(INSTALL_TARGETS vdb_core)
if(VDB_BUILD_CLI)
    list(APPEND INSTALL_TARGETS hektor vdb_cli)
endif()

install(TARGETS ${INSTALL_TARGETS}
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
message(STATUS "  Build Type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:       ${CMAKE_CXX_STANDARD}")
message(STATUS "  AVX2:               ${VDB_USE_AVX2}")
message(STATUS "  AVX-512:            ${VDB_USE_AVX512}")
message(STATUS "  GPU Support:        ${VDB_ENABLE_GPU}")
message(STATUS "  Python Bindings:    ${VDB_BUILD_PYTHON}")
message(STATUS "  Tests:              ${VDB_BUILD_TESTS}")
message(STATUS "  Benchmarks:         ${VDB_BUILD_BENCHMARKS}")
message(STATUS "")
message(STATUS "Optional Features:")
message(STATUS "  llama.cpp:          ${VDB_USE_LLAMA_CPP}")
message(STATUS "  ONNX Runtime:       ${VDB_USE_ONNX_RUNTIME}")
message(STATUS "  TensorFlow:         ${VDB_USE_TENSORFLOW}")
message(STATUS "  PyTorch (LibTorch): ${VDB_USE_TORCH}")
message(STATUS "  Distributed:        ${VDB_ENABLE_DISTRIBUTED}")
message(STATUS "")
