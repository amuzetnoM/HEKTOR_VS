# Hektor CLI - Phase 1 Implementation

This is the Phase 1 implementation of the Hektor CLI, providing foundation and core commands.

## Features Implemented

### Foundation
- ✅ Argument parser
- ✅ Command router
- ✅ Output formatters (Table, JSON, CSV)
- ✅ Error handling
- ✅ Help system

### Core Commands

#### Database Management
- `hektor init <path>` - Initialize a new database
- `hektor info <path>` - Show database information

#### Data Operations
- `hektor add <db>` - Add a document
- `hektor get <db> <id>` - Get document by ID
- `hektor delete <db> <id>` - Delete a document

#### Search
- `hektor search <db> <query>` - Search the database
- `hektor s <db> <query>` - Short alias

## Usage Examples

```bash
# Initialize a database
hektor init ./mydb

# Add documents
hektor add ./mydb --text "Gold prices rising on inflation fears"
hektor add ./mydb --file document.txt --type journal

# Search
hektor search ./mydb "gold outlook" -k 10
hektor s ./mydb "analysis" --type journal

# Get database info
hektor info ./mydb

# Delete a document
hektor delete ./mydb 12345
```

## Building

```bash
cmake -B build -G Ninja
cmake --build build
./build/hektor help
```

## Architecture

```
src/cli/
├── main.cpp                  # Entry point
├── cli.cpp                   # Main CLI class
├── output_formatter.cpp       # Output formatting
└── commands/
    ├── db_commands.cpp       # Database commands
    ├── data_commands.cpp     # Data operations
    └── search_commands.cpp   # Search commands

include/vdb/cli/
├── cli.hpp
├── command_base.hpp
├── output_formatter.hpp
└── commands/
    ├── db_commands.hpp
    ├── data_commands.hpp
    └── search_commands.hpp
```

## Next Steps (Phase 2)

- Hybrid search implementation
- Full database engine integration
- Advanced ingestion adapters
- Index management commands
- Collection management
