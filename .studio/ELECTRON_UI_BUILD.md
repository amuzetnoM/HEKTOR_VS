# HEKTOR Perceptual Quantization Studio
## Build Documentation

**Version**: 1.0.0  
**Date**: 2026-01-23  
**Platform**: Electron + Native C++ Addons  
**Target**: Windows, macOS, Linux

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Architecture Overview](#architecture-overview)
3. [Project Structure](#project-structure)
4. [Development Environment Setup](#development-environment-setup)
5. [Building Native Addons](#building-native-addons)
6. [Electron Application Build](#electron-application-build)
7. [Testing](#testing)
8. [Packaging & Distribution](#packaging--distribution)
9. [Continuous Integration](#continuous-integration)
10. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Software

| Tool | Version | Purpose |
|------|---------|---------|
| **Node.js** | 20.x LTS | Runtime environment |
| **npm** or **pnpm** | 9.x / 8.x | Package manager |
| **CMake** | 3.20+ | Native addon build |
| **C++ Compiler** | See below | Native compilation |
| **Python** | 3.10+ | Node-gyp dependency |
| **Git** | 2.x | Version control |

### C++ Compiler Requirements

**Windows**:
- Visual Studio 2022 (MSVC 19.30+)
- Windows SDK 10.0.22000+
- C++23 support enabled

**macOS**:
- Xcode 14.3+ (AppleClang 14.0+)
- Command Line Tools installed
- macOS 12.0+ SDK

**Linux**:
- GCC 11+ or Clang 14+
- build-essential package
- libx11-dev, libxext-dev (for GUI)

### Optional Tools

| Tool | Purpose |
|------|---------|
| **VS Code** | Recommended IDE |
| **CUDA Toolkit** | GPU acceleration (optional) |
| **Vulkan SDK** | Graphics API (optional) |
| **Docker** | Containerized builds |

---

## Architecture Overview

```
HEKTOR-UI/
├── electron-app/           # Electron main & renderer process
│   ├── main/               # Main process (Node.js)
│   │   ├── index.ts        # Entry point
│   │   ├── ipc/            # IPC handlers
│   │   └── native/         # Native addon loader
│   ├── renderer/           # Renderer process (React)
│   │   ├── src/            # React components
│   │   ├── assets/         # Static assets
│   │   └── styles/         # CSS/Tailwind
│   └── preload/            # Preload scripts
├── native-addon/           # C++ Native Addon (N-API)
│   ├── src/                # C++ source files
│   │   ├── binding.cpp     # N-API bindings
│   │   ├── hektor_wrapper.cpp  # C++ wrapper
│   │   └── vector_ops.cpp  # Vector operations
│   ├── include/            # Header files
│   ├── binding.gyp         # Node-gyp config
│   └── CMakeLists.txt      # CMake config (alternative)
├── hektor-core/            # HEKTOR C++ engine (submodule)
│   ├── src/                # Core implementation
│   ├── include/            # Public headers
│   └── CMakeLists.txt      # Core build config
├── scripts/                # Build automation scripts
│   ├── build-native.sh     # Native addon build
│   ├── build-electron.sh   # Electron build
│   └── package.sh          # Distribution packaging
├── tests/                  # Test suites
│   ├── unit/               # Unit tests
│   ├── integration/        # Integration tests
│   └── e2e/                # End-to-end tests
├── docs/                   # Additional documentation
├── package.json            # NPM dependencies
├── tsconfig.json           # TypeScript config
└── README.md               # Project overview
```

---

## Project Structure

### Directory Descriptions

**electron-app/**:
- Contains all Electron-specific code
- Separated into main, renderer, and preload processes
- Uses TypeScript throughout for type safety

**native-addon/**:
- C++ code that wraps HEKTOR core
- Compiled as Node.js native addon using N-API
- Provides JavaScript bindings to C++ functions

**hektor-core/**:
- Git submodule pointing to main HEKTOR repository
- Compiled as static library linked into native addon
- Unchanged from original C++23 codebase

**scripts/**:
- Automation scripts for building and packaging
- Cross-platform support (sh for Unix, ps1 for Windows)
- Handles complex build orchestration

---

## Development Environment Setup

### Step 1: Clone Repository

```bash
# Clone with submodules
git clone --recursive https://github.com/amuzetnoM/hektor-ui.git
cd hektor-ui

# If already cloned without --recursive
git submodule update --init --recursive
```

### Step 2: Install Node.js Dependencies

```bash
# Using npm
npm install

# OR using pnpm (faster, recommended)
pnpm install
```

This will install:
- Electron
- React & related libraries
- TypeScript compiler
- Build tools (electron-builder, node-gyp)
- Testing frameworks
- Development utilities

### Step 3: Install Native Dependencies

**Windows**:
```powershell
# Install Visual Studio Build Tools if not already installed
winget install Microsoft.VisualStudio.2022.BuildTools

# Install Python for node-gyp
winget install Python.Python.3.11

# Set npm configuration
npm config set msvs_version 2022
npm config set python python3
```

**macOS**:
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install additional dependencies via Homebrew
brew install cmake ninja
```

**Linux (Ubuntu/Debian)**:
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build \
  libx11-dev libxext-dev libxfixes-dev libxi-dev \
  libxcursor-dev libxrandr-dev libxinerama-dev \
  libgl1-mesa-dev libglu1-mesa-dev
```

### Step 4: Verify Environment

```bash
# Run verification script
npm run verify-env

# Expected output:
# ✓ Node.js 20.x detected
# ✓ npm 9.x detected
# ✓ CMake 3.20+ detected
# ✓ C++ compiler detected
# ✓ Python 3.10+ detected
# ✓ Git submodules initialized
```

---

## Building Native Addons

### Overview

The native addon bridges JavaScript and C++, providing:
- Zero-copy access to vector data
- SIMD-optimized operations
- Direct HNSW index access
- Async operations with thread pool

### Build Process

#### Step 1: Build HEKTOR Core

```bash
# Navigate to hektor-core submodule
cd hektor-core

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DVDB_BUILD_PYTHON=OFF \
  -DVDB_BUILD_CLI=OFF \
  -DVDB_BUILD_SHARED=OFF \
  -DVDB_USE_AVX2=ON \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON

# Build
ninja

# Result: libhektor.a static library
cd ../..
```

**Key CMake Options**:
- `CMAKE_BUILD_TYPE=Release`: Optimized build
- `VDB_BUILD_SHARED=OFF`: Static library for linking
- `CMAKE_POSITION_INDEPENDENT_CODE=ON`: Required for shared objects
- `VDB_USE_AVX2=ON`: Enable SIMD optimizations

#### Step 2: Build Native Addon

**Using node-gyp** (default):
```bash
cd native-addon

# Configure and build
npx node-gyp configure
npx node-gyp build

# Result: build/Release/hektor_native.node
```

**Using CMake-JS** (alternative, recommended for complex projects):
```bash
cd native-addon

# Install cmake-js globally
npm install -g cmake-js

# Build
cmake-js compile

# Result: build/Release/hektor_native.node
```

#### Step 3: Link Native Addon to Electron

```bash
# Copy built addon to electron-app
cp native-addon/build/Release/hektor_native.node \
   electron-app/main/native/

# OR use npm script
npm run build:native
```

### binding.gyp Configuration

```python
{
  "targets": [
    {
      "target_name": "hektor_native",
      "sources": [
        "src/binding.cpp",
        "src/hektor_wrapper.cpp",
        "src/vector_ops.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "include",
        "../hektor-core/include"
      ],
      "libraries": [
        "../hektor-core/build/libhektor.a"
      ],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "cflags_cc": [
        "-std=c++23",
        "-O3",
        "-march=native",
        "-ffast-math"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "conditions": [
        ["OS=='win'", {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": ["/std:c++latest", "/O2", "/arch:AVX2"]
            }
          }
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++23",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "12.0",
            "OTHER_CPLUSPLUSFLAGS": ["-O3", "-march=native"]
          }
        }],
        ["OS=='linux'", {
          "cflags_cc": ["-std=c++23", "-O3", "-march=native"]
        }]
      ]
    }
  ]
}
```

### Native Addon API

**JavaScript Interface**:
```typescript
// hektor_native.node exports
interface HektorNative {
  // Database operations
  openDatabase(path: string): DatabaseHandle;
  closeDatabase(handle: DatabaseHandle): void;
  
  // Vector operations (zero-copy via SharedArrayBuffer)
  queryVectors(handle: DatabaseHandle, query: Float32Array, k: number): VectorResult[];
  addVector(handle: DatabaseHandle, vector: Float32Array, metadata: object): number;
  
  // Index operations
  getHNSWGraph(handle: DatabaseHandle, layer: number): GraphData;
  optimizeIndex(handle: DatabaseHandle): void;
  
  // Async operations
  queryVectorsAsync(
    handle: DatabaseHandle,
    query: Float32Array,
    k: number,
    callback: (err: Error | null, results: VectorResult[]) => void
  ): void;
}
```

**C++ Implementation Example**:
```cpp
// src/binding.cpp
#include <napi.h>
#include "hektor_wrapper.h"

// Sync function: Query vectors
Napi::Value QueryVectors(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // Extract arguments
    HektorWrapper* wrapper = Napi::ObjectWrap<HektorWrapper>::Unwrap(
        info[0].As<Napi::Object>()
    );
    Napi::Float32Array query = info[1].As<Napi::Float32Array>();
    int k = info[2].As<Napi::Number>().Int32Value();
    
    // Call HEKTOR core
    std::vector<vdb::SearchResult> results = 
        wrapper->database()->search(query.Data(), query.ElementLength(), k);
    
    // Convert to JavaScript array
    Napi::Array jsResults = Napi::Array::New(env, results.size());
    for (size_t i = 0; i < results.size(); i++) {
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("id", Napi::Number::New(env, results[i].id));
        obj.Set("similarity", Napi::Number::New(env, results[i].similarity));
        // ... add metadata
        jsResults[i] = obj;
    }
    
    return jsResults;
}

// Async function: Query vectors in background thread
class QueryAsyncWorker : public Napi::AsyncWorker {
public:
    QueryAsyncWorker(
        Napi::Function& callback,
        HektorWrapper* wrapper,
        std::vector<float> query,
        int k
    ) : Napi::AsyncWorker(callback),
        wrapper_(wrapper),
        query_(std::move(query)),
        k_(k) {}
    
    void Execute() override {
        // Runs in worker thread
        results_ = wrapper_->database()->search(query_.data(), query_.size(), k_);
    }
    
    void OnOK() override {
        // Runs in main thread
        Napi::HandleScope scope(Env());
        
        Napi::Array jsResults = Napi::Array::New(Env(), results_.size());
        for (size_t i = 0; i < results_.size(); i++) {
            Napi::Object obj = Napi::Object::New(Env());
            obj.Set("id", Napi::Number::New(Env(), results_[i].id));
            obj.Set("similarity", Napi::Number::New(Env(), results_[i].similarity));
            jsResults[i] = obj;
        }
        
        Callback().Call({Env().Null(), jsResults});
    }
    
private:
    HektorWrapper* wrapper_;
    std::vector<float> query_;
    int k_;
    std::vector<vdb::SearchResult> results_;
};

Napi::Value QueryVectorsAsync(const Napi::CallbackInfo& info) {
    // ... extract arguments
    
    QueryAsyncWorker* worker = new QueryAsyncWorker(callback, wrapper, query, k);
    worker->Queue();
    
    return info.Env().Undefined();
}

// Module initialization
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("queryVectors", Napi::Function::New(env, QueryVectors));
    exports.Set("queryVectorsAsync", Napi::Function::New(env, QueryVectorsAsync));
    // ... register other functions
    
    return exports;
}

NODE_API_MODULE(hektor_native, Init)
```

---

## Electron Application Build

### Development Build

```bash
# Start development server (hot reload enabled)
npm run dev

# This runs:
# - Vite dev server for renderer process
# - TypeScript watch for main process
# - Electron with --inspect flag
```

**Development Features**:
- Hot Module Replacement (HMR) for React components
- DevTools automatically opened
- Source maps enabled
- Debug logging to console

### Production Build

```bash
# Build all components
npm run build

# This runs in sequence:
# 1. npm run build:native     # Build native addon
# 2. npm run build:renderer   # Build React app (Vite)
# 3. npm run build:main       # Build main process (TypeScript)
# 4. npm run build:preload    # Build preload scripts
```

**Build Output**:
```
dist/
├── main/               # Compiled main process
│   ├── index.js
│   └── ipc/
├── renderer/           # Compiled renderer (Vite output)
│   ├── index.html
│   ├── assets/
│   └── *.js, *.css
├── preload/            # Compiled preload scripts
│   └── index.js
└── native/             # Native addon
    └── hektor_native.node
```

### Vite Configuration (renderer)

```typescript
// electron-app/renderer/vite.config.ts
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';

export default defineConfig({
  plugins: [react()],
  base: './',
  build: {
    outDir: '../../dist/renderer',
    emptyOutDir: true,
    rollupOptions: {
      output: {
        manualChunks: {
          'three': ['three'],
          'vendor': ['react', 'react-dom'],
        }
      }
    }
  },
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
      '@components': path.resolve(__dirname, './src/components'),
      '@lib': path.resolve(__dirname, './src/lib'),
    }
  },
  optimizeDeps: {
    exclude: ['hektor-native'] // Don't bundle native addon
  }
});
```

### TypeScript Configuration

```json
// tsconfig.json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "ESNext",
    "lib": ["ES2022", "DOM", "DOM.Iterable"],
    "jsx": "react-jsx",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "moduleResolution": "bundler",
    "resolveJsonModule": true,
    "isolatedModules": true,
    "noEmit": false,
    "outDir": "./dist",
    "baseUrl": ".",
    "paths": {
      "@/*": ["./electron-app/renderer/src/*"],
      "@components/*": ["./electron-app/renderer/src/components/*"]
    }
  },
  "include": [
    "electron-app/**/*.ts",
    "electron-app/**/*.tsx"
  ],
  "exclude": [
    "node_modules",
    "dist",
    "build"
  ]
}
```

---

## Testing

### Unit Tests (Jest)

```bash
# Run all unit tests
npm test

# Run with coverage
npm run test:coverage

# Watch mode
npm run test:watch
```

**Test Structure**:
```
tests/unit/
├── main/               # Main process tests
│   ├── ipc.test.ts
│   └── native.test.ts
├── renderer/           # Renderer tests
│   ├── components/     # Component tests
│   └── lib/            # Utility tests
└── native/             # Native addon tests
    └── bindings.test.ts
```

**Example Component Test**:
```typescript
// tests/unit/renderer/components/Button.test.tsx
import { render, screen, fireEvent } from '@testing-library/react';
import { Button } from '@/components/Button';

describe('Button', () => {
  it('renders with text', () => {
    render(<Button>Click me</Button>);
    expect(screen.getByText('Click me')).toBeInTheDocument();
  });
  
  it('calls onClick when clicked', () => {
    const handleClick = jest.fn();
    render(<Button onClick={handleClick}>Click me</Button>);
    
    fireEvent.click(screen.getByText('Click me'));
    expect(handleClick).toHaveBeenCalledTimes(1);
  });
  
  it('disables button when loading', () => {
    render(<Button loading>Click me</Button>);
    expect(screen.getByRole('button')).toBeDisabled();
  });
});
```

### Integration Tests

```bash
# Run integration tests
npm run test:integration
```

**Tests**:
- IPC communication between main and renderer
- Native addon function calls
- Database operations
- File system operations

### End-to-End Tests (Playwright)

```bash
# Run e2e tests
npm run test:e2e

# Run in headed mode (see browser)
npm run test:e2e:headed

# Debug mode
npm run test:e2e:debug
```

**Example E2E Test**:
```typescript
// tests/e2e/database.spec.ts
import { test, expect, _electron as electron } from '@playwright/test';

test('opens database and displays vectors', async () => {
  // Launch Electron app
  const app = await electron.launch({ args: ['.'] });
  const window = await app.firstWindow();
  
  // Open database
  await window.click('button:has-text("Open Database")');
  await window.setInputFiles('input[type="file"]', 'tests/fixtures/sample.db');
  
  // Wait for vectors to load
  await window.waitForSelector('.vector-count');
  const count = await window.textContent('.vector-count');
  expect(count).toMatch(/\d+ vectors/);
  
  // Check 3D viewport is rendered
  await window.waitForSelector('canvas');
  const canvas = await window.locator('canvas');
  expect(canvas).toBeVisible();
  
  await app.close();
});
```

---

## Packaging & Distribution

### Electron Builder Configuration

```json
// electron-builder.json
{
  "appId": "com.hektor.quantization-studio",
  "productName": "HEKTOR Perceptual Quantization Studio",
  "copyright": "Copyright © 2026 HEKTOR",
  "directories": {
    "buildResources": "build",
    "output": "release"
  },
  "files": [
    "dist/**/*",
    "package.json"
  ],
  "extraResources": [
    {
      "from": "hektor-core/models",
      "to": "models",
      "filter": ["*.onnx"]
    }
  ],
  "mac": {
    "target": ["dmg", "zip"],
    "category": "public.app-category.developer-tools",
    "icon": "build/icon.icns",
    "hardenedRuntime": true,
    "gatekeeperAssess": false,
    "entitlements": "build/entitlements.mac.plist",
    "entitlementsInherit": "build/entitlements.mac.plist",
    "minimumSystemVersion": "12.0"
  },
  "win": {
    "target": ["nsis", "portable"],
    "icon": "build/icon.ico",
    "certificateSubjectName": "HEKTOR Inc.",
    "signingHashAlgorithms": ["sha256"],
    "verifyUpdateCodeSignature": false
  },
  "linux": {
    "target": ["AppImage", "deb", "rpm"],
    "icon": "build/icons",
    "category": "Development",
    "maintainer": "HEKTOR Team <team@hektor.dev>"
  },
  "nsis": {
    "oneClick": false,
    "perMachine": false,
    "allowToChangeInstallationDirectory": true,
    "createDesktopShortcut": true,
    "createStartMenuShortcut": true
  },
  "publish": {
    "provider": "github",
    "owner": "amuzetnoM",
    "repo": "hektor-ui"
  }
}
```

### Build Packages

```bash
# Build for current platform
npm run package

# Build for all platforms (requires appropriate host OS)
npm run package:all

# Build for specific platform
npm run package:win    # Windows
npm run package:mac    # macOS
npm run package:linux  # Linux
```

**Output**:
```
release/
├── HEKTOR-Perceptual-Quantization-Studio-1.0.0.dmg       # macOS
├── HEKTOR-Perceptual-Quantization-Studio-1.0.0.exe       # Windows
├── HEKTOR-Perceptual-Quantization-Studio-1.0.0.AppImage  # Linux
└── latest*.yml                                            # Auto-update manifests
```

### Code Signing

**Windows** (requires certificate):
```bash
# Set environment variables
export CSC_LINK=/path/to/certificate.pfx
export CSC_KEY_PASSWORD=your_password

# Build with signing
npm run package:win
```

**macOS** (requires Apple Developer account):
```bash
# Sign and notarize
export APPLEID=your@email.com
export APPLEID_PASSWORD=app-specific-password
export APPLE_TEAM_ID=TEAM123456

# Build with signing
npm run package:mac
```

---

## Continuous Integration

### GitHub Actions Workflow

```yaml
# .github/workflows/build.yml
name: Build and Release

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]
  release:
    types: [created]

jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-22.04, windows-2022, macos-13]
        node-version: [20.x]
    
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Setup Node.js
        uses: actions/setup-node@v4
        with:
          node-version: ${{ matrix.node-version }}
          cache: 'npm'
      
      - name: Install dependencies
        run: npm ci
      
      - name: Setup build tools (Linux)
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y build-essential cmake ninja-build
      
      - name: Setup build tools (Windows)
        if: runner.os == 'Windows'
        uses: microsoft/setup-msbuild@v1.1
      
      - name: Build native addon
        run: npm run build:native
      
      - name: Run tests
        run: npm test
      
      - name: Build Electron app
        run: npm run build
      
      - name: Package
        run: npm run package
      
      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: ${{ matrix.os }}-build
          path: release/*
  
  release:
    needs: build
    runs-on: ubuntu-latest
    if: github.event_name == 'release'
    steps:
      - uses: actions/download-artifact@v3
      
      - name: Upload release assets
        uses: softprops/action-gh-release@v1
        with:
          files: |
            *-build/*
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

---

## Troubleshooting

### Common Issues

#### Native Addon Build Fails

**Problem**: `node-gyp rebuild` fails with compilation errors

**Solution**:
```bash
# Clean build cache
npm run clean
rm -rf node_modules
npm install

# Rebuild with verbose output
npm run build:native -- --verbose

# Check compiler version
gcc --version  # Linux
clang --version  # macOS
cl.exe  # Windows (in VS Developer Command Prompt)
```

#### Electron Window is Blank

**Problem**: Renderer process not loading

**Solution**:
```bash
# Check DevTools console for errors
# Main process logs:
npm run dev  # Look for errors in terminal

# Renderer process logs:
# Open DevTools (F12) in Electron window

# Common causes:
# 1. Incorrect `base` in vite.config.ts (should be './')
# 2. CSP blocking resources
# 3. Wrong path to index.html in main process
```

#### Native Addon Not Found

**Problem**: `Error: Cannot find module 'hektor_native.node'`

**Solution**:
```bash
# Verify addon exists
ls electron-app/main/native/hektor_native.node

# Rebuild if missing
npm run build:native

# Check require path in code
# Should be relative to main process:
const addon = require('./native/hektor_native.node');
```

#### Performance Issues in 3D Viewport

**Problem**: Low FPS with large datasets

**Solution**:
```typescript
// Enable WebGL extensions
const renderer = new THREE.WebGLRenderer({
  antialias: true,
  powerPreference: 'high-performance',
  precision: 'highp'
});

// Implement LOD
const lod = new THREE.LOD();
lod.addLevel(highDetailMesh, 0);
lod.addLevel(mediumDetailMesh, 50);
lod.addLevel(lowDetailMesh, 200);

// Use instancing
const geometry = new THREE.InstancedBufferGeometry();
const mesh = new THREE.InstancedMesh(geometry, material, count);

// Frustum culling
scene.autoUpdate = false;
camera.updateMatrixWorld();
frustum.setFromProjectionMatrix(
  new THREE.Matrix4().multiplyMatrices(
    camera.projectionMatrix,
    camera.matrixWorldInverse
  )
);
```

### Debug Flags

**Main Process**:
```bash
# Enable Electron debugging
npm run dev -- --inspect=5858

# Attach Chrome DevTools:
# Open chrome://inspect in Chrome browser
```

**Renderer Process**:
```bash
# Automatically opens DevTools in dev mode
npm run dev

# Production debugging
ELECTRON_ENABLE_LOGGING=1 npm start
```

**Native Addon**:
```bash
# Enable C++ debug symbols
export CFLAGS="-g"
export CXXFLAGS="-g"
npm run build:native

# Use lldb (macOS/Linux) or WinDbg (Windows)
lldb -- electron .
```

---

## Performance Optimization

### Build Time Optimization

```bash
# Use pnpm instead of npm (faster installs)
pnpm install

# Parallel builds
npm run build:native &
npm run build:renderer &
wait

# Incremental builds
npm run build -- --incremental

# Cache native builds
ccache -M 5G  # Linux/macOS
```

### Runtime Optimization

**Renderer Process**:
- Code splitting (dynamic imports)
- Tree shaking (unused code elimination)
- Asset optimization (image compression, lazy loading)
- Web Workers for heavy computations

**Main Process**:
- Lazy require (load modules on demand)
- IPC batching (combine multiple messages)
- Background threads for I/O operations

**Native Addon**:
- SIMD optimizations enabled
- Memory pooling for allocations
- Async operations for long-running tasks
- SharedArrayBuffer for zero-copy transfers

---

## Additional Resources

### Documentation
- [Electron Docs](https://www.electronjs.org/docs/latest)
- [Node.js N-API](https://nodejs.org/api/n-api.html)
- [Three.js Docs](https://threejs.org/docs/)
- [React Docs](https://react.dev/)

### Tools
- [Electron Forge](https://www.electronforge.io/) - Alternative build tool
- [Electron Fiddle](https://www.electronjs.org/fiddle) - Rapid prototyping
- [Spectron](https://www.electronjs.org/spectron) - E2E testing (deprecated, use Playwright)

### Community
- [Electron Discord](https://discord.com/invite/electron)
- [Stack Overflow - Electron](https://stackoverflow.com/questions/tagged/electron)
- [GitHub Discussions](https://github.com/amuzetnoM/hektor-ui/discussions)

---

**Document Version**: 1.0.0  
**Last Updated**: 2026-01-23  
**Maintainer**: HEKTOR Development Team
