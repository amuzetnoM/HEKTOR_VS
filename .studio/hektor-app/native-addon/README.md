# Native Addon Build

This directory contains the C++ native addon for HEKTOR's core vector database operations.

## Structure

```
native-addon/
├── include/              # Header files
│   ├── database.h       # Database operations
│   └── async_operations.h  # Async query workers
├── src/                 # Implementation files
│   ├── binding.cpp      # N-API module initialization
│   ├── database.cpp     # Database implementation
│   └── async_operations.cpp  # Async operations
```

## Building

### Prerequisites

- Node.js 20.x LTS
- C++23 compiler (GCC 11+, Clang 14+, MSVC 2022+)
- CMake 3.20+ (for HEKTOR core)
- Python 3.x (for node-gyp)

### Build Commands

```bash
# Build native addon
npm run build:native

# Or manually
node-gyp rebuild
```

## Integration Status

- ✅ N-API module structure
- ✅ Basic database operations (open, close, add, query)
- ✅ Async operations with AsyncWorker
- ⏳ HEKTOR core integration (TODO)
- ⏳ Zero-copy data transfer via SharedArrayBuffer
- ⏳ Advanced PQ features

## API

### Synchronous Operations

```javascript
const native = require('./build/Release/hektor_native.node');

// Open database
const result = native.openDatabase('/path/to/db');

// Add vector
const addResult = native.addVector([0.1, 0.2, 0.3], { label: 'test' });

// Query vectors
const results = native.queryVectors([0.1, 0.2, 0.3], 10);

// Close database
native.closeDatabase();
```

### Asynchronous Operations

```javascript
native.queryVectorsAsync([0.1, 0.2, 0.3], 10, (err, results) => {
  if (err) {
    console.error('Query failed:', err);
  } else {
    console.log('Results:', results);
  }
});
```

## Next Steps

1. **Link HEKTOR Core**: Integrate with actual VDB implementation
2. **SharedArrayBuffer**: Implement zero-copy data transfer
3. **Advanced Features**: PQ, hybrid search, RAG integration
4. **Performance**: SIMD optimization, batch operations
5. **Testing**: Unit tests for native code

## Troubleshooting

### Build Errors

**Missing node-addon-api:**
```bash
npm install node-addon-api --save-dev
```

**C++23 not supported:**
- Ensure compiler supports C++23
- Update compiler or use C++20 fallback

**Link errors:**
- Verify HEKTOR core is built
- Check library paths in binding.gyp

### Runtime Errors

**Addon not loading:**
- Check build output in `build/Release/`
- Verify native module matches Node.js version
- Rebuild: `node-gyp rebuild`

## License

MIT
