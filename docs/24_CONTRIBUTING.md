---
title: "Contributing Guide"
description: "How to contribute to Vector Studio"
version: "4.0.0"
last_updated: "2026-01-22"
sidebar_position: 24
category: "Guide"
---

![Version](https://img.shields.io/badge/version-4.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--22-green?style=flat-square)
![Type](https://img.shields.io/badge/type-operations-yellow?style=flat-square)

# Contributing to Vector Studio

Thank you for your interest in contributing to Vector Studio. This document provides guidelines and instructions for contributing.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Making Changes](#making-changes)
- [Code Standards](#code-standards)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)
- [Release Process](#release-process)

---

## Code of Conduct

This project adheres to a code of conduct. By participating, you are expected to:

- Be respectful and inclusive
- Accept constructive criticism gracefully
- Focus on what is best for the project
- Show empathy towards other contributors

---

## Getting Started

### Prerequisites

- C++20 compatible compiler (MSVC 19.30+, GCC 11+, Clang 14+)
- CMake 3.20+
- Python 3.10+ (for bindings and scripts)
- Git

### Fork and Clone

```bash
# Fork the repository on GitHub, then:
git clone https://github.com/YOUR_USERNAME/hektor.git
cd hektor
git remote add upstream https://github.com/amuzetnoM/hektor.git
```

---

## Development Setup

> **For basic installation instructions, see [02_INSTALLATION.md](02_INSTALLATION.md).**

### Development Build

**Windows:**

```powershell
# Run automated setup with development dependencies
.\scripts\setup.ps1

# Build in debug mode for development
.\scripts\build.ps1 -Debug
```

**Unix/Linux/macOS:**

```bash
# Run automated setup
./scripts/setup.sh

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### IDE Setup

**Visual Studio Code** (recommended):
```json
// .vscode/settings.json
{
    "cmake.configureSettings": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VDB_BUILD_TESTS": "ON"
    },
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

**Visual Studio 2022**:
- Open folder as CMake project
- Select Debug configuration

---

## Making Changes

### Branch Naming

Use descriptive branch names:

```
feature/add-quantization
bugfix/fix-memory-leak
docs/update-readme
perf/optimize-distance
refactor/cleanup-storage
```

### Commit Messages

Follow conventional commits format:

```
type(scope): description

[optional body]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `perf`: Performance improvement
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `build`: Build system changes
- `ci`: CI/CD changes

Examples:
```
feat(index): add IVF-PQ quantization support

Implements Inverted File with Product Quantization for
memory-efficient storage of large vector collections.

Closes #123
```

```
fix(storage): resolve memory leak in mmap resize

The previous implementation didn't properly unmap
the old region before extending the file.
```

---

## Code Standards

### C++ Style

Follow the project's existing style:

```cpp
// Use snake_case for functions and variables
void calculate_distance(const float* a, const float* b);

// Use PascalCase for classes and structs
class VectorDatabase;
struct QueryOptions;

// Use UPPER_CASE for constants and macros
constexpr size_t MAX_DIMENSION = 4096;
#define VDB_VERSION_MAJOR 1

// Use descriptive names
size_t vector_count;  // Good
size_t n;             // Avoid (unless loop variable)

// Braces on same line for functions
void example() {
    if (condition) {
        // ...
    }
}

// Document public APIs
/**
 * @brief Search for k nearest neighbors.
 * @param query Query vector (must be normalized)
 * @param k Number of neighbors to return
 * @param options Search configuration
 * @return Vector of (id, distance) pairs
 */
std::vector<Result> search(VectorView query, size_t k, const QueryOptions& options);
```

### Python Style

Follow PEP 8 with these additions:

```python
# Use type hints
def search(query: str, k: int = 10) -> list[SearchResult]:
    ...

# Docstrings for public functions
def add_text(text: str, metadata: dict | None = None) -> int:
    """
    Add a text document to the database.

    Args:
        text: The text content to embed and store.
        metadata: Optional metadata dictionary.

    Returns:
        The ID of the newly added document.

    Raises:
        ValueError: If text is empty.
    """
    ...
```

### Formatting

- **C++**: clang-format (config in `.clang-format`)
- **Python**: black + isort

Run formatters before committing:

```bash
# C++
clang-format -i src/**/*.cpp include/**/*.hpp

# Python
black scripts/ bindings/python/
isort scripts/ bindings/python/
```

---

## Testing

### Running Tests

```bash
# C++ tests
cd build
ctest --output-on-failure

# Python tests
pytest tests/ -v

# With coverage
pytest tests/ --cov=pyvdb --cov-report=html
```

### Writing Tests

**C++ (Google Test)**:
```cpp
TEST(DistanceTest, CosineNormalized) {
    std::vector<float> a = {1.0f, 0.0f, 0.0f};
    std::vector<float> b = {0.0f, 1.0f, 0.0f};

    float dist = cosine_distance(a.data(), b.data(), 3);
    EXPECT_NEAR(dist, 1.0f, 1e-6f);  // Orthogonal = max distance
}
```

**Python (pytest)**:
```python
def test_add_and_search():
    db = pyvdb.create_database(":memory:")

    doc_id = db.add_text("test document")
    assert doc_id >= 0

    results = db.search("test", k=1)
    assert len(results) == 1
    assert results[0].id == doc_id
```

### Test Coverage Requirements

- New features must include tests
- Bug fixes should include regression tests
- Maintain >80% coverage for core modules

---

## Submitting Changes

### Pull Request Process

1. **Update from upstream**:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Run all checks**:
   ```bash
   # Build and test
   .\scripts\build.ps1 -Debug
   ctest --output-on-failure
   pytest tests/

   # Lint
   clang-format --dry-run -Werror src/**/*.cpp
   black --check scripts/
   ```

3. **Push and create PR**:
   ```bash
   git push origin feature/your-feature
   ```

4. **Fill out PR template**:
   - Description of changes
   - Related issues
   - Testing performed
   - Breaking changes (if any)

### PR Requirements

- [ ] All tests pass
- [ ] Code is formatted
- [ ] Documentation updated (if applicable)
- [ ] No merge conflicts
- [ ] Commits are clean (squash if needed)

### Review Process

1. Automated checks run (CI)
2. Code review by maintainers
3. Address feedback
4. Approval and merge

---

## Release Process

Releases follow semantic versioning (MAJOR.MINOR.PATCH):

- **MAJOR**: Breaking API changes
- **MINOR**: New features (backwards compatible)
- **PATCH**: Bug fixes

### Release Checklist

1. Update version in `CMakeLists.txt`
2. Update `CHANGELOG.md`
3. Create release tag: `git tag -a v1.2.0 -m "Release v1.2.0"`
4. Push tag: `git push origin v1.2.0`
5. GitHub Actions builds and publishes release

---

## Questions?

- Open a [Discussion](https://github.com/amuzetnoM/gold_standard/discussions) for questions
- Open an [Issue](https://github.com/amuzetnoM/gold_standard/issues) for bugs
- Contact maintainers for security issues

Thank you for contributing!
