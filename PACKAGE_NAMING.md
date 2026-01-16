# Package Naming - Important Information

## Why "hektor-vdb" and not "hektor"?

### The Problem

There is already a package on PyPI called [`hektor`](https://pypi.org/project/hektor/) which is a completely unrelated project:
- **Existing package**: QTI-XML/XLS to JSON converter
- **This project**: High-performance vector database

When users tried to install this vector database with `pip install hektor`, they would:
1. Get the wrong package
2. Encounter build failures due to the old `lxml` dependency

### The Solution

This project is now available as **`hektor-vdb`** on PyPI to avoid naming conflicts.

## Installation Commands

### ✅ Correct
```bash
pip install hektor-vdb       # Installs THIS vector database project
```

### ❌ Incorrect  
```bash
pip install hektor           # Installs unrelated QTI-XML converter
```

## In Your Code

The package name for pip is `hektor-vdb`, but you still import it as `pyvdb`:

```python
import pyvdb  # ✅ Correct import

# NOT:
import hektor_vdb  # ❌ Wrong
import hektor      # ❌ Wrong
```

## Quick Reference

| Context | Name |
|---------|------|
| **Project name** | Hektor / Vector Studio |
| **Repository** | `amuzetnoM/hektor` |
| **PyPI package** | `hektor-vdb` |
| **Python import** | `pyvdb` |

## Examples

```bash
# Installation
pip install hektor-vdb

# Usage
python3 << 'PYTHON'
import pyvdb

# Create database
db = pyvdb.create_gold_standard_db("./my_db")

# Add data
db.add_text("Sample text", pyvdb.DocumentType.Journal, "2026-01-16")

# Search
results = db.search("sample query", k=10)
PYTHON
```

## If You Installed the Wrong Package

If you accidentally installed the wrong `hektor` package:

```bash
# Uninstall the wrong package
pip uninstall hektor

# Install the correct package
pip install hektor-vdb
```

## Questions?

- See [INSTALL.md](INSTALL.md) for detailed installation instructions
- See [README.md](README.md) for project overview
- See [docs/22_PYTHON_BINDINGS.md](docs/22_PYTHON_BINDINGS.md) for API documentation
