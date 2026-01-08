---
title: "CLI Quick Reference & Examples"
description: "Quick reference guide with practical examples for all commands"
version: "3.0.0"
last_updated: "2026-01-07"
status: "Design"
---

# Hektor CLI - Quick Reference & Examples

## Quick Start

### 5-Minute Tutorial

```bash
# 1. Initialize database
hektor init ./mydb --preset gold-standard

# 2. Add a document
hektor add ./mydb --text "Gold prices rising on inflation fears"

# 3. Search
hektor search ./mydb "gold outlook" -k 5

# 4. Get stats
hektor info ./mydb
```

## Command Reference by Task

### Working with Databases

#### Create New Database
```bash
# Basic initialization
hektor db init ./mydb

# With custom configuration
hektor db init ./mydb \
  --dimension 384 \
  --metric cosine \
  --hnsw-m 32 \
  --hnsw-ef 200

# Use preset for Gold Standard
hektor db init ./mydb --preset gold-standard
```

#### Database Information
```bash
# Show database stats
hektor db info ./mydb
hektor info ./mydb              # short form

# Health check
hektor db health ./mydb

# List all databases
hektor db list
hektor ls                        # short form
```

#### Maintenance
```bash
# Optimize database
hektor db optimize ./mydb
hektor optimize ./mydb          # short form

# Sync to disk
hektor db sync ./mydb
hektor sync ./mydb

# Backup
hektor db backup ./mydb ./backup/mydb-2025-01-07.tar.gz --compress

# Restore
hektor db restore ./backup/mydb.tar.gz ./mydb-restored
```

### Adding Data

#### Single Documents
```bash
# Add text
hektor add ./mydb --text "Market analysis shows bullish trend"

# Add from file
hektor add ./mydb --file document.txt

# Add with metadata
hektor add ./mydb --text "Gold analysis" \
  --type journal \
  --date 2025-01-07 \
  --asset GOLD \
  --tags "analysis,bullish,2025"

# Add with JSON metadata
hektor add ./mydb --text "Report content" \
  --metadata '{"author":"John","priority":"high"}'

# Add image
hektor add ./mydb --image chart.png \
  --type chart \
  --asset GOLD
```

#### Batch Operations
```bash
# Batch insert from JSONL
hektor add ./mydb --batch documents.jsonl

# Batch with multiple workers
hektor batch ./mydb data.jsonl --workers 8

# Skip errors during batch
hektor batch ./mydb data.csv --skip-errors
```

#### Update & Delete
```bash
# Update document
hektor update ./mydb 12345 \
  --text "Updated content" \
  --tags "updated,reviewed"

# Delete document
hektor delete ./mydb 12345
hektor rm ./mydb 12345          # short form

# Delete with force (no confirmation)
hektor rm ./mydb 12345 --force
```

### Searching

#### Basic Search
```bash
# Simple search
hektor search ./mydb "gold price forecast"
hektor s ./mydb "gold"          # short alias

# Search with top-k
hektor search ./mydb "analysis" -k 20

# Search with threshold
hektor search ./mydb "outlook" --threshold 0.7
```

#### Filtered Search
```bash
# Filter by type
hektor search ./mydb "analysis" --type journal

# Filter by date range
hektor search ./mydb "gold" \
  --date-from 2025-01-01 \
  --date-to 2025-01-31

# Filter by asset
hektor search ./mydb "forecast" --asset GOLD

# Filter by tags
hektor search ./mydb "technical" --tags "analysis,chart"

# Multiple filters
hektor search ./mydb "outlook" \
  --type journal \
  --date-from 2025-01-01 \
  --asset GOLD \
  --tags "bullish" \
  -k 10
```

#### Advanced Queries
```bash
# Query with filter expression
hektor query ./mydb \
  -q "market analysis" \
  --filter "gold_price > 4000 AND date > '2025-01-01'"

# Query with sorting
hektor query ./mydb \
  -q "forecast" \
  --sort-by date \
  --order desc

# Query with pagination
hektor query ./mydb \
  -q "analysis" \
  --limit 50 \
  --offset 100
```

#### Find Similar
```bash
# Find documents similar to ID 12345
hektor similar ./mydb 12345 -k 10

# Exclude the query document
hektor similar ./mydb 12345 -k 10 --exclude-self
```

#### Explain Search
```bash
# Show search details
hektor search ./mydb "gold" --explain
```

### Hybrid Search

#### Combined Vector + BM25
```bash
# Hybrid search with RRF fusion (recommended)
hektor hybrid search ./mydb "gold price analysis" -k 20
hektor hs ./mydb "gold"         # short alias

# Specify fusion method
hektor hs ./mydb "analysis" --fusion rrf

# Weighted fusion
hektor hs ./mydb "forecast" \
  --fusion weighted \
  --vector-weight 0.7 \
  --lexical-weight 0.3

# CombSUM fusion
hektor hs ./mydb "outlook" --fusion combsum
```

#### BM25 Only
```bash
# Full-text search with BM25
hektor hybrid bm25 ./mydb "gold prices surge"
```

#### Reranking
```bash
# Rerank existing results
hektor search ./mydb "analysis" --format json > results.json
hektor hybrid rerank ./mydb results.json --method rrf
```

### Data Ingestion

#### From Files
```bash
# Ingest CSV
hektor ingest ./mydb data.csv

# Ingest with format detection
hektor ingest ./mydb documents.json

# Force specific format
hektor ingest ./mydb data.txt --format csv

# Ingest directory recursively
hektor ingest ./mydb ./documents --recursive

# Ingest with pattern
hektor ingest ./mydb ./reports --pattern "*.pdf" --recursive
```

#### Chunking Strategies
```bash
# Fixed size chunking
hektor ingest ./mydb document.pdf \
  --chunk-strategy fixed \
  --chunk-size 512

# Sentence-based chunking
hektor ingest ./mydb paper.pdf \
  --chunk-strategy sentence

# Semantic chunking
hektor ingest ./mydb book.pdf \
  --chunk-strategy semantic \
  --chunk-size 1024 \
  --chunk-overlap 100

# Paragraph chunking
hektor ingest ./mydb article.txt \
  --chunk-strategy paragraph

# Recursive chunking
hektor ingest ./mydb large_doc.pdf \
  --chunk-strategy recursive
```

#### From APIs
```bash
# Ingest from HTTP endpoint
hektor ingest ./mydb http://api.example.com/data --format json

# Ingest from database
hektor ingest ./mydb postgres://host/db --adapter pgvector
hektor ingest ./mydb sqlite://path/to/db.sqlite --adapter sqlite
```

#### Gold Standard Integration
```bash
# Ingest all Gold Standard outputs
hektor ingest gold-standard ./mydb ../gold_standard/output
hektor ingest gs ./mydb ../output              # short alias

# Selective ingestion
hektor ingest gs ./mydb ../output --journals-only
hektor ingest gs ./mydb ../output --charts-only
hektor ingest gs ./mydb ../output --reports-only

# Incremental ingestion (skip existing)
hektor ingest gs ./mydb ../output --incremental
```

#### Batch Processing
```bash
# Parallel ingestion
hektor ingest ./mydb ./docs --workers 8 --recursive

# Watch for changes
hektor ingest ./mydb ./docs --watch --incremental
```

#### Preview Data
```bash
# Scan and preview data source
hektor ingest scan ./data --format csv --show-preview
```

### Index Management

#### Build & Rebuild
```bash
# Build index
hektor index build ./mydb

# Rebuild from scratch
hektor index rebuild ./mydb

# Build with custom parameters
hektor index build ./mydb \
  --type hnsw \
  --hnsw-m 32 \
  --hnsw-ef 400 \
  --workers 8
```

#### Optimization
```bash
# Optimize index
hektor index optimize ./mydb
hektor index compact ./mydb     # alias
```

#### Index Information
```bash
# Show index stats
hektor index stats ./mydb
hektor index info ./mydb

# Validate index
hektor index validate ./mydb
hektor index check ./mydb
```

#### Benchmarking
```bash
# Benchmark index performance
hektor index benchmark ./mydb
hektor index benchmark ./mydb \
  --queries 10000 \
  --k 10
```

### Collections

#### Manage Collections
```bash
# List collections
hektor collection list ./mydb
hektor col ls ./mydb            # short form

# Create collection
hektor collection create ./mydb journals \
  --dimension 512 \
  --metric cosine \
  --description "Journal entries"

# Rename collection
hektor col rename ./mydb old_name new_name

# Delete collection
hektor col delete ./mydb old_collection --force

# Clone collection
hektor col clone ./mydb src_coll dest_coll

# Collection info
hektor col info ./mydb journals
```

### Metadata Operations

#### Tagging
```bash
# Add tags
hektor tag ./mydb 12345 analysis,gold,2025
hektor meta tag ./mydb 12345 analysis,gold,2025

# Remove tags
hektor untag ./mydb 12345 draft
hektor meta untag ./mydb 12345 draft

# List all tags
hektor meta tags ./mydb
```

#### Attributes
```bash
# Set attribute
hektor meta set ./mydb 12345 priority high
hektor meta set ./mydb 12345 reviewed true

# Get attribute
hektor meta get ./mydb 12345 priority

# Get all attributes
hektor meta get ./mydb 12345

# Delete attribute
hektor meta delete ./mydb 12345 draft
```

#### Filtering
```bash
# Filter by metadata
hektor meta filter ./mydb "type=journal AND date>2025-01-01"
hektor meta filter ./mydb "gold_price>4000 AND bias='BULLISH'"
```

### Export & Import

#### Export Data
```bash
# Export to JSONL
hektor export ./mydb data.jsonl

# Export with filters
hektor export ./mydb journals.jsonl \
  --filter "type=journal"

# Export with vectors
hektor export ./mydb full_data.jsonl --include-vectors

# Export as CSV
hektor export ./mydb data.csv --format csv

# Export compressed
hektor export ./mydb data.jsonl.gz --compress
```

#### Training Data Export
```bash
# Export positive pairs
hektor export pairs ./mydb pairs.jsonl \
  --min-similarity 0.8

# Export triplets (anchor, positive, negative)
hektor export triplets ./mydb triplets.jsonl \
  --negative-samples 5

# Export for contrastive learning
hektor export training ./mydb train.jsonl \
  --type contrastive
```

#### Export Embeddings
```bash
# Export as numpy
hektor export embeddings ./mydb embeddings.npy

# Export as HDF5
hektor export embeddings ./mydb embeddings.h5 --format hdf5
```

#### Import Data
```bash
# Import from file
hektor import ./mydb data.jsonl

# Import with merge
hektor import ./mydb data.jsonl --merge

# Import with overwrite
hektor import ./mydb data.jsonl --overwrite
```

### Model Management

#### List Models
```bash
# List local models
hektor model list --local
hektor model ls                 # alias

# List remote (available) models
hektor model list --remote

# List all
hektor model list
```

#### Download Models
```bash
# Download specific model
hektor model download all-MiniLM-L6-v2
hektor model get all-MiniLM-L6-v2    # alias

# Download with quantization
hektor model download clip-vit-b32 --quantization fp16

# Download to custom location
hektor model download model-name --cache-dir /path/to/cache
```

#### Model Information
```bash
# Show model info
hektor model info all-MiniLM-L6-v2

# Benchmark model
hektor model benchmark all-MiniLM-L6-v2 \
  --batch-size 32 \
  --iterations 100
```

#### Configuration
```bash
# Set default model
hektor model set-default all-MiniLM-L6-v2

# Delete model
hektor model delete old-model
```

### Distributed Operations

#### Cluster Setup
```bash
# Initialize cluster
hektor cluster init \
  --nodes node1:8000,node2:8000,node3:8000 \
  --replication-factor 2 \
  --shards 4

# Add node to cluster
hektor cluster add-node node4:8000

# Remove node
hektor cluster remove-node node2:8000

# Cluster status
hektor cluster nodes
hektor cluster status           # alias

# Cluster health
hektor cluster health
```

#### Replication
```bash
# Setup replication
hektor cluster replicate ./mydb \
  --mode async \
  --targets node2:8000,node3:8000

# Sync replication
hektor cluster replicate ./mydb \
  --mode sync \
  --factor 2

# Semi-sync replication
hektor cluster replicate ./mydb \
  --mode semi-sync \
  --factor 1
```

#### Sharding
```bash
# Setup sharding
hektor cluster shard ./mydb \
  --strategy hash \
  --shards 8

# Range-based sharding
hektor cluster shard ./mydb \
  --strategy range \
  --shards 4

# Consistent hashing
hektor cluster shard ./mydb \
  --strategy consistent \
  --shards 8 \
  --rebalance
```

### Monitoring

#### Metrics
```bash
# Show metrics
hektor monitor metrics ./mydb
hektor metrics ./mydb           # short form

# Prometheus format
hektor metrics ./mydb --prometheus

# Live monitoring
hektor metrics ./mydb --watch --interval 5
```

#### Logs
```bash
# Show logs
hektor monitor logs ./mydb
hektor logs ./mydb              # short form

# Filter by level
hektor logs ./mydb --level error

# Tail logs
hektor logs ./mydb --tail 100

# Follow logs
hektor logs ./mydb --follow

# Filter logs
hektor logs ./mydb --grep "search" --follow

# Date range
hektor logs ./mydb --since "2025-01-07 10:00:00"
```

#### Tracing
```bash
# Trace operations
hektor monitor trace ./mydb --duration 60

# Trace specific operation
hektor monitor trace ./mydb --operation search
```

#### Performance
```bash
# Performance monitoring
hektor monitor perf ./mydb --interval 1 --duration 60
```

### Reports

#### Generate Reports
```bash
# Summary report
hektor report summary ./mydb
hektor report stats ./mydb      # alias

# Usage report
hektor report usage ./mydb --period week
hektor report usage ./mydb \
  --from 2025-01-01 \
  --to 2025-01-31

# Performance report
hektor report perf ./mydb

# Audit log
hektor report audit ./mydb \
  --from 2025-01-01 \
  --user admin \
  --action "delete"

# Data quality report
hektor report quality ./mydb

# Coverage analysis
hektor report coverage ./mydb
```

### Security

#### User Management
```bash
# Create user (password will be prompted interactively)
hektor auth create-user john --role user

# Create user with password from stdin (for automation)
echo "$SECURE_PASSWORD" | hektor auth create-user john --password-stdin --role user

# List users
hektor auth list-users
hektor auth users               # alias

# Change password
hektor auth change-password john

# Grant permissions
hektor auth grant john read ./mydb

# Revoke permissions
hektor auth revoke john write ./mydb
```

#### Encryption
```bash
# Encrypt database
hektor auth encrypt ./mydb \
  --key-file encryption.key \
  --algorithm aes256

# Decrypt database
hektor auth decrypt ./mydb --key-file encryption.key
```

### Configuration

#### Config Management
```bash
# Show config
hektor config show
hektor config show --global
hektor config show --local

# Set value
hektor config set default_metric cosine
hektor config set log_level debug --global

# Get value
hektor config get default_metric

# Unset value
hektor config unset log_level

# Edit config file
hektor config edit

# Validate config
hektor config validate
```

#### Profiles
```bash
# Create profile
hektor config profile create production

# Use profile
hektor config profile use production

# List profiles
hektor config profile list

# Run command with specific profile
hektor --profile production search ./mydb "query"
```

### RAG Operations

#### Initialize RAG
```bash
# Setup RAG system
hektor rag init ./mydb \
  --chunking sentence \
  --chunk-size 512 \
  --chunk-overlap 50

# With framework
hektor rag init ./mydb \
  --framework langchain \
  --chunking semantic
```

#### Add Documents
```bash
# Add documents to RAG
hektor rag add ./mydb ./documents --recursive
```

#### Query RAG
```bash
# Query RAG system
hektor rag query ./mydb "What is the gold price forecast?"

# With LLM parameters
hektor rag query ./mydb "Explain the market trends" \
  -k 5 \
  --llm-model gpt-3.5-turbo \
  --temperature 0.7 \
  --max-tokens 500
```

### Quantization

#### Quantize Database
```bash
# Scalar quantization
hektor quantize ./mydb --method scalar --bits 8

# Product quantization
hektor quantize ./mydb \
  --method product \
  --codebook-size 256

# Binary quantization
hektor quantize ./mydb --method binary
```

#### Quantization Info
```bash
# Show quantization info
hektor quantize info ./mydb

# Benchmark quantization
hektor quantize benchmark ./mydb
```

### Utilities

#### Validation & Repair
```bash
# Validate database
hektor validate ./mydb
hektor util validate ./mydb

# Repair database
hektor repair ./mydb
hektor util repair ./mydb
```

#### Benchmarking
```bash
# Benchmark database
hektor benchmark ./mydb
hektor util benchmark ./mydb \
  --queries 10000 \
  --k 10 \
  --workers 4
```

#### System Info
```bash
# Show system information
hektor system-info
hektor util system-info

# Check dependencies
hektor check-deps
```

#### Completion Scripts
```bash
# Generate bash completion
hektor completion bash > /etc/bash_completion.d/hektor

# Generate zsh completion
hektor completion zsh > ~/.zsh/completion/_hektor

# Generate fish completion
hektor completion fish > ~/.config/fish/completions/hektor.fish
```

### Interactive Shell

#### Start Shell
```bash
# Launch interactive shell
hektor shell ./mydb
```

#### Shell Commands
```
hektor> help                     # Show commands
hektor> search "gold prices"     # Search
hektor> add --text "..."         # Add document
hektor> stats                    # Show stats
hektor> history                  # Command history
hektor> clear                    # Clear screen
hektor> exit                     # Exit shell
```

## Output Formatting

### Format Options
```bash
# JSON output
hektor search ./mydb "query" --format json

# Table output (default)
hektor search ./mydb "query" --format table

# CSV output
hektor search ./mydb "query" --format csv

# YAML output
hektor search ./mydb "query" --format yaml

# Compact output
hektor search ./mydb "query" --format compact
```

### Redirect to File
```bash
# Save to file
hektor search ./mydb "query" --output results.json

# Pipe to file
hektor search ./mydb "query" --format json > results.json

# Pipe to other commands
hektor search ./mydb "query" --format json | jq '.results[].id'
```

## Cheat Sheet

### Most Common Commands
```bash
# Create database
hektor init <path>

# Add document
hektor add <db> --text "..."

# Search
hektor search <db> "query"
hektor s <db> "query"           # short

# Hybrid search
hektor hybrid search <db> "query"
hektor hs <db> "query"          # short

# Ingest data
hektor ingest <db> <source>

# Show info
hektor info <db>

# Export data
hektor export <db> output.jsonl

# Interactive shell
hektor shell <db>
```

### Quick Aliases
```
s     = search
hs    = hybrid search
ls    = list
rm    = delete
col   = collection
cfg   = config
mon   = monitor
```

### Global Flags
```
-v, --verbose       Verbose output
-q, --quiet         Quiet mode
-d, --debug         Debug mode
-f, --format        Output format
-o, --output        Output file
-h, --help          Help
--version           Version
```

## Environment Variables

```bash
# Set environment variables
export HEKTOR_CONFIG=~/.hektor/config.yaml
export HEKTOR_DB_PATH=./mydb
export HEKTOR_LOG_LEVEL=info
export HEKTOR_FORMAT=json
export HEKTOR_NO_COLOR=1
export HEKTOR_PROFILE=production
export HEKTOR_MODELS_DIR=~/.cache/hektor/models

# Use in commands
hektor search $HEKTOR_DB_PATH "query"
```

## Common Workflows

### Daily Journal Ingestion
```bash
#!/bin/bash
# Ingest daily journals
hektor ingest gs ./vectors ../gold_standard/output \
  --journals-only \
  --incremental

# Search recent journals
hektor search ./vectors "market analysis" \
  --type journal \
  --date-from $(date -d '7 days ago' +%Y-%m-%d) \
  -k 20
```

### Backup and Restore
```bash
# Daily backup
hektor backup ./mydb ./backups/mydb-$(date +%Y%m%d).tar.gz --compress

# Restore if needed
hektor restore ./backups/mydb-20250107.tar.gz ./mydb-restored
```

### Batch Processing
```bash
# Ingest multiple directories
for dir in docs reports papers; do
  hektor ingest ./mydb ./$dir --recursive --incremental
done

# Export for different purposes
hektor export ./mydb data.jsonl
hektor export pairs ./mydb training_pairs.jsonl
hektor export embeddings ./mydb embeddings.npy
```

## Tips & Best Practices

1. **Use Aliases**: Learn short aliases for frequently used commands
2. **Profile Management**: Use profiles for different environments
3. **Incremental Ingestion**: Use `--incremental` to skip existing data
4. **Batch Operations**: Use `--workers` for parallel processing
5. **Output Formats**: Use `--format json` for scripting
6. **Progress Feedback**: Use `--verbose` for long operations
7. **Interactive Mode**: Use `hektor shell` for exploration
8. **Help System**: Use `--help` on any command
9. **Tab Completion**: Install completion scripts
10. **Configuration**: Set defaults in config file

## Getting Help

```bash
# General help
hektor help
hektor --help

# Command-specific help
hektor search --help
hektor db --help
hektor hybrid --help

# Interactive help
hektor shell ./mydb
hektor> help

# Documentation
hektor docs
```

## Exit Codes

- `0` - Success
- `1` - General error
- `2` - Invalid arguments
- `3` - Database error
- `4` - I/O error
- `5` - Network error
- `6` - Permission denied
- `7` - Not found
- `8` - Already exists
- `9` - Timeout
- `10` - Interrupted

## More Information

- Full Documentation: `docs/CLI_DESIGN.md`
- Command Maps: `docs/CLI_COMMAND_MAP.md`
- Implementation: `docs/CLI_IMPLEMENTATION.md`
- API Reference: `docs/20_API_REFERENCE.md`
