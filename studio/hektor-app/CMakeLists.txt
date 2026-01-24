# CMakeLists.txt for HEKTOR Native Addon
# This builds the Node.js native addon using cmake-js

cmake_minimum_required(VERSION 3.20)

# Project name
project(hektor_native LANGUAGES CXX)

# C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use dynamic runtime to match vdb_core.lib
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Find Node.js headers (provided by cmake-js)
include_directories(${CMAKE_JS_INC})

# Find node-addon-api
execute_process(
    COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
include_directories(${NODE_ADDON_API_DIR})

# Include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/native-addon/include
    ${CMAKE_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/../../build/_deps/json-src/include
    ${CMAKE_SOURCE_DIR}/../../build/_deps/fmt-src/include
)

# Source files
set(SOURCES
    native-addon/src/binding.cpp
    native-addon/src/database.cpp
    native-addon/src/async_operations.cpp
    native-addon/src/search.cpp
    native-addon/src/collections.cpp
    native-addon/src/ingestion.cpp
    native-addon/src/index_mgmt.cpp
    native-addon/src/quantization.cpp
    native-addon/src/utils.cpp
    native-addon/src/embeddings.cpp
    native-addon/src/storage.cpp
    native-addon/src/index.cpp
    native-addon/src/hybrid.cpp
    native-addon/src/rag.cpp
    native-addon/src/distributed.cpp
    native-addon/src/framework.cpp
    native-addon/src/telemetry.cpp
    native-addon/src/common.cpp
)

# Create the native addon as a shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${CMAKE_JS_SRC})

# Set output name and suffix
set_target_properties(${PROJECT_NAME} PROPERTIES 
    PREFIX "" 
    SUFFIX ".node"
)

# Link the core library
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/../../build/Release/vdb_core.lib
        ${CMAKE_SOURCE_DIR}/../../build/_deps/fmt-build/Release/fmt.lib
        ${CMAKE_JS_LIB}
        ws2_32
        advapi32
    )
else()
    target_link_libraries(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/../../build/libvdb_core.a
        ${CMAKE_SOURCE_DIR}/../../build/_deps/fmt-build/libfmt.a
        ${CMAKE_JS_LIB}
        pthread
        dl
    )
endif()

# Defines
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NAPI_VERSION=8
    NAPI_DISABLE_CPP_EXCEPTIONS
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _HAS_EXCEPTIONS=1
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /bigobj /Zc:__cplusplus)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -fexceptions -fPIC)
endif()

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
    # Generate node.lib for Windows
    execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()
