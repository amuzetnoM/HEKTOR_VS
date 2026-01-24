# HEKTOR Build System Issues: Research, Solution & Implementation

## Executive Summary

This document provides comprehensive research into the build failures encountered in the HEKTOR Vector Database project, covering root cause analysis, solution development, and implementation details for both the core C++ library and the Electron/Node.js native addon.

## Table of Contents

1. [Problem Statement](#problem-statement)
2. [Root Cause Analysis](#root-cause-analysis)
3. [Solution Development](#solution-development)
4. [Implementation Details](#implementation-details)
5. [Testing & Validation](#testing--validation)
6. [Platform-Specific Notes](#platform-specific-notes)

---

## Problem Statement

### Symptoms Observed

1. **C++ Core Library Build Failures:**
   - `std::expected` not found errors
   - Template instantiation failures
   - Linker errors for `std::unexpected`

2. **Native Addon Build Failures:**
   - node-gyp failing to compile C++23 code
   - Library linking failures (`vdb_core.lib` not found)
   - Header include path issues

3. **CI/CD Pipeline Failures:**
   - GitHub Actions failing on all platforms
   - Compiler version mismatches
   - Missing C++23 standard library features

### Impact Assessment

| Component | Severity | User Impact |
|-----------|----------|-------------|
| Core Library | Critical | No pip install |
| Native Addon | Critical | No Studio app |
| CI Pipeline | High | No automated releases |

---

## Root Cause Analysis

### Issue 1: std::expected Compiler Support

**The Core Problem:**

The project uses C++23's `std::expected<T, E>` throughout the codebase:

```cpp
// include/vdb/core.hpp
template<typename T>
using Result = std::expected<T, Error>;

// Usage throughout
Result<VectorId> add_vector(VectorView vec, const Metadata& meta);
```

**Root Cause:**

The CI/CD pipeline was using outdated compiler versions:

```yaml
# BEFORE (Broken)
jobs:
  build-linux:
    runs-on: ubuntu-22.04  # GCC 11 - NO std::expected
```

GCC 11 and earlier do not support `std::expected`. The feature requires:
- GCC 13+ 
- Clang 16+
- MSVC 19.33+ (Visual Studio 2022 17.3+)

### Issue 2: Native Addon Library Linking

**The Core Problem:**

The binding.gyp was configured with incorrect library paths:

```javascript
// BEFORE (Broken)
"libraries": [
  "../../build/src/Release/vdb_core.lib"  // Wrong path!
]
```

**Root Cause:**

CMake's output structure differs by generator:
- **Ninja/Make:** `build/libvdb_core.a`
- **MSVC:** `build/Release/vdb_core.lib`

The binding.gyp assumed a `src/` subdirectory that doesn't exist.

### Issue 3: C++23 Flag Override

**The Core Problem:**

node-gyp's default flags were overriding C++23:

```bash
# What node-gyp was doing:
cl.exe /std:c++17 ... /std:c++23 ...
# First flag wins in MSVC!
```

**Root Cause:**

node-gyp applies default C++ standard flags before user flags, and the first `/std:` flag takes precedence in MSVC.

---

## Solution Development

### Solution 1: Upgrade CI Compiler Versions

**Approach:** Update GitHub Actions to use modern runners with C++23 support.

```yaml
# .github/workflows/ci.yml - FIXED

jobs:
  build-windows:
    runs-on: windows-2022
    steps:
      - uses: ilammy/msvc-dev-cmd@v1
        with:
          arch: x64
          toolset: 14.36  # VS 2022 17.6+

  build-linux:
    runs-on: ubuntu-24.04
    steps:
      - name: Install GCC 13
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc-13 g++-13
          sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
          sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

  build-macos:
    runs-on: macos-14  # M1 with Xcode 15+
```

### Solution 2: Fix Library Paths in binding.gyp

**Approach:** Use correct paths for each platform's build output.

```javascript
// binding.gyp - FIXED
{
  "targets": [{
    "target_name": "hektor_native",
    "include_dirs": [
      "<!@(node -p \"require('node-addon-api').include\")",
      "native-addon/include",
      "../../include",
      "../../build/_deps/json-src/include",
      "../../build/_deps/fmt-src/include"
    ],
    "conditions": [
      ["OS=='win'", {
        "libraries": [
          "../../build/Release/vdb_core.lib",
          "../../build/_deps/fmt-build/Release/fmt.lib"
        ]
      }],
      ["OS=='linux'", {
        "libraries": [
          "<(module_root_dir)/../../build/libvdb_core.a",
          "<(module_root_dir)/../../build/_deps/fmt-build/libfmt.a"
        ]
      }]
    ]
  }]
}
```

### Solution 3: Proper C++23 Flag Configuration

**Approach:** Override node-gyp defaults and ensure C++23 is used.

```javascript
// binding.gyp - C++23 Configuration
{
  "cflags!": ["-fno-exceptions"],      // Remove these defaults
  "cflags_cc!": ["-fno-exceptions"],
  
  "conditions": [
    ["OS=='win'", {
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1,
          "AdditionalOptions": ["/std:c++23", "/bigobj", "/EHsc"]
        }
      }
    }],
    ["OS=='linux'", {
      "cflags_cc": ["-std=c++23", "-fexceptions", "-fPIC"]
    }],
    ["OS=='mac'", {
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LANGUAGE_STANDARD": "c++23"
      }
    }]
  ]
}
```

---

## Implementation Details

### Step 1: Update CMakeLists.txt

Ensure the core library is configured for C++23:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(VectorDB VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

### Step 2: Update GitHub Actions Workflow

```yaml
# .github/workflows/ci.yml

name: CI

on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-2022
    steps:
      - uses: actions/checkout@v4
      
      - name: Setup MSVC
        uses: ilammy/msvc-dev-cmd@v1
        with:
          arch: x64
          
      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release
        
      - name: Build
        run: cmake --build build --config Release -j 8
        
      - name: Test
        run: ctest --test-dir build -C Release --output-on-failure

  build-linux:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      
      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc-13 g++-13 ninja-build
          sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
          sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
          
      - name: Configure
        run: cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
        
      - name: Build
        run: cmake --build build -j $(nproc)
        
      - name: Test
        run: ctest --test-dir build --output-on-failure
```

### Step 3: Update Native Addon Build

```javascript
// .studio/hektor-app/binding.gyp - Complete Configuration
{
  "targets": [
    {
      "target_name": "hektor_native",
      "sources": [
        "native-addon/src/binding.cpp",
        "native-addon/src/database.cpp",
        "native-addon/src/search.cpp",
        "native-addon/src/quantization.cpp",
        "native-addon/src/hybrid.cpp",
        // ... other sources
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "native-addon/include",
        "../../include",
        "../../build/_deps/json-src/include",
        "../../build/_deps/fmt-src/include"
      ],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS", "NAPI_VERSION=8"],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "conditions": [
        ["OS=='win'", {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": ["/std:c++23", "/bigobj", "/EHsc"]
            },
            "VCLinkerTool": {
              "AdditionalLibraryDirectories": [
                "../../build/Release",
                "../../build/_deps/fmt-build/Release"
              ]
            }
          },
          "libraries": [
            "../../build/Release/vdb_core.lib",
            "../../build/_deps/fmt-build/Release/fmt.lib",
            "ws2_32.lib",
            "advapi32.lib"
          ]
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++23",
            "MACOSX_DEPLOYMENT_TARGET": "10.15"
          }
        }],
        ["OS=='linux'", {
          "cflags_cc": ["-std=c++23", "-fexceptions", "-fPIC"],
          "libraries": [
            "<(module_root_dir)/../../build/libvdb_core.a",
            "<(module_root_dir)/../../build/_deps/fmt-build/libfmt.a",
            "-lpthread",
            "-ldl"
          ]
        }]
      ]
    }
  ]
}
```

---

## Testing & Validation

### Test Matrix

| Platform | Compiler | C++ Standard | Build | Tests | Status |
|----------|----------|--------------|-------|-------|--------|
| Windows x64 | MSVC 19.44 | C++23 | ✓ | 107/107 | PASS |
| Linux x64 | GCC 13.2 | C++23 | ✓ | 107/107 | PASS |
| macOS arm64 | Clang 16 | C++23 | ✓ | 107/107 | PASS |

### Validation Commands

```bash
# Windows (PowerShell)
cmake -B build -DVDB_BUILD_TESTS=ON
cmake --build build --config Release -j 8
ctest --test-dir build -C Release --output-on-failure

# Linux/macOS
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DVDB_BUILD_TESTS=ON
cmake --build build -j $(nproc)
ctest --test-dir build --output-on-failure

# Native Addon
cd .studio/hektor-app
npm install
npm run build:native
npm run dev
```

### Test Output (Windows)

```
100% tests passed, 0 tests failed out of 107

Test categories:
- Vector operations: 12 tests
- Distance metrics: 8 tests
- HNSW index: 15 tests
- Flat index: 10 tests
- Storage: 12 tests
- BM25 search: 10 tests
- Hybrid search: 8 tests
- RAG engine: 6 tests
- Perceptual quantization: 26 tests

Total Test time (real) = 2.21 sec
```

---

## Platform-Specific Notes

### Windows

- **Visual Studio 2022 17.3+** required for `std::expected`
- Use `/bigobj` flag for large template instantiations
- Link against `ws2_32.lib` for networking, `advapi32.lib` for registry

### Linux

- **GCC 13+** or **Clang 16+** required
- Install with: `sudo apt-get install gcc-13 g++-13`
- Use `-fPIC` for shared library compatibility

### macOS

- **Xcode 15+** recommended (Apple Clang 16+)
- Set `MACOSX_DEPLOYMENT_TARGET` to 10.15 minimum
- Use `-stdlib=libc++` for modern C++ features

### Raspberry Pi (ARM)

- Cross-compile with GCC 13 for ARM
- Disable AVX2/AVX512 flags (x86 only)
- Use NEON for SIMD if available

---

## Conclusion

The build issues were resolved by:

1. **Upgrading CI compilers** to versions with full C++23 support
2. **Fixing library paths** in binding.gyp to match CMake output
3. **Properly configuring C++23 flags** to avoid node-gyp overrides

All 107 tests pass on Windows, Linux, and macOS. The native addon now compiles with full C++23 features including `std::expected`.

---

*Document Version: 1.0*
*Last Updated: 2026-01-24*
*Author: HEKTOR Development Team*
