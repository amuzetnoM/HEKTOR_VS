# Installation Guide

## Important: Package Name

This vector database project is named **hektor** but the Python package is published as **hektor-vdb** to avoid conflict with an existing unrelated package on PyPI.

When installing, use:
```bash
pip install hektor-vdb
```

NOT:
```bash
pip install hektor  # ⚠️ This installs a different package (QTI-XML converter)
```

## Quick Install

### From Source (Recommended for Development)

```bash
# Clone the repository
git clone https://github.com/amuzetnoM/hektor.git
cd hektor

# Install with pip
pip install .

# Or install in editable mode for development
pip install -e .
```

### Requirements

- **Python:** 3.10 or higher
- **CMake:** 3.20 or higher  
- **C++ Compiler:** 
  - GCC 11+ (Linux)
  - Clang 14+ (macOS)
  - MSVC 2022+ (Windows)

### System Dependencies (Linux)

On Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libsqlite3-dev zlib1g-dev \
    libicu-dev libpq-dev
```

On macOS:
```bash
brew install cmake
```

On Windows:
```powershell
# Using winget
winget install Kitware.CMake
winget install Microsoft.VisualStudio.2022.BuildTools
```

## Installation with Optional Features

```bash
# Install with ML dependencies (ONNX, transformers, etc.)
pip install hektor-vdb[ml]

# Install with data processing tools (pandas, openpyxl, etc.)
pip install hektor-vdb[data]

# Install with database integrations (PostgreSQL, pgvector)
pip install hektor-vdb[db]

# Install with all optional dependencies
pip install hektor-vdb[all]

# Install with development tools
pip install hektor-vdb[dev]
```

## Verify Installation

```python
import pyvdb
print(f"pyvdb imported successfully")

# Create a test database
db = pyvdb.create_gold_standard_db("./test_db")
print(f"Database created successfully")
```

## Troubleshooting

### Build Errors

If you encounter build errors:

1. **Missing CMake:** Install CMake 3.20 or higher
2. **Missing C++ Compiler:** Install a compatible C++ compiler
3. **Missing System Libraries:** Install required system dependencies (see above)

### Import Errors

If `import pyvdb` fails:

1. Make sure you installed `hektor-vdb`, not `hektor`
2. Check that the installation completed successfully
3. Verify your Python version is 3.10 or higher

### Wrong Package Installed

If you accidentally installed the wrong `hektor` package:

```bash
# Uninstall the wrong package
pip uninstall hektor

# Install the correct package
pip install hektor-vdb
```

## Next Steps

- See [docs/02_INSTALLATION.md](docs/02_INSTALLATION.md) for detailed installation instructions
- See [docs/22_PYTHON_BINDINGS.md](docs/22_PYTHON_BINDINGS.md) for Python API documentation
- See [README.md](README.md) for project overview and usage examples
