# Hektor CLI - Complete Usage Guide

**Version**: 3.0.0  
**Last Updated**: 2026-01-08

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Quick Start](#quick-start)
4. [Interactive Mode](#interactive-mode)
5. [Database Management](#database-management)
6. [Data Operations](#data-operations)
7. [Search Operations](#search-operations)
8. [Hybrid Search](#hybrid-search)
9. [Data Ingestion](#data-ingestion)
10. [NLP Operations](#nlp-operations)
11. [Model Management](#model-management)
12. [Index Management](#index-management)
13. [Collections](#collections)
14. [Metadata & Tagging](#metadata--tagging)
15. [Monitoring & Alerts](#monitoring--alerts)
16. [Reports](#reports)
17. [Export Operations](#export-operations)
18. [Advanced Workflows](#advanced-workflows)

---

## Introduction

The Hektor CLI is a comprehensive command-line interface for the Hektor Vector Database with native NLP capabilities. It provides 78+ commands covering all aspects of vector database operations, from basic CRUD to advanced AI-powered search and monitoring.

### Key Features

- ✅ **78+ Commands** - Complete database lifecycle management
- ✅ **Interactive REPL** - Full-featured shell with history and tab completion
- ✅ **Native NLP Engine** - 12 NLP operations (bare-metal C++ implementation)
- ✅ **Hybrid Search** - Vector + BM25 with 5 fusion methods
- ✅ **Multi-Source Ingestion** - 10+ data sources (CSV, JSON, PDF, Excel, XML, etc.)
- ✅ **Model Management** - Download, quantize, benchmark models
- ✅ **Real-time Monitoring** - Prometheus metrics, logs, traces
- ✅ **Beautiful Output** - Color-coded with progress indicators
- ✅ **Multiple Formats** - JSON, Table, CSV output

---

## Installation

### Building from Source

```bash
# Clone the repository
git clone https://github.com/amuzetnoM/hektor.git
cd hektor

# Build with CMake
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# The hektor executable will be in build/
./hektor --help
```

### Add to PATH

```bash
# Linux/Mac
export PATH=$PATH:/path/to/hektor/build

# Windows (PowerShell)
$env:PATH += ";C:\path\to\hektor\build"
```

---

## Quick Start

### Initialize a Database

```bash
# Basic initialization
hektor init ./mydb

# With custom dimension
hektor init ./mydb --dimension 384

# With preset configuration
hektor init ./mydb --preset gold-standard
```

### Add Documents

```bash
# Add from text
hektor add ./mydb --text "Gold prices are rising"

# Add from file
hektor add ./mydb --file document.txt --type journal

# Add with metadata
hektor add ./mydb --text "Market analysis" --metadata '{"author":"John","date":"2026-01-08"}'
```

### Search

```bash
# Basic search
hektor search ./mydb "gold outlook" -k 20

# Short alias
hektor s ./mydb "gold" -k 10

# With filters
hektor search ./mydb "forecast" \
  --type journal \
  --date-from 2026-01-01 \
  --explain
```

---

## Interactive Mode

The interactive REPL provides a shell-like experience with all CLI commands available.

### Start Interactive Shell

```bash
# Start with database
hektor shell ./mydb

# Start without database (can set later)
hektor repl

# Alias
hektor interactive
```

### Interactive Session Example

```
╔══════════════════════════════════════════════════════╗
║    Hektor Vector Database - Interactive Shell    ║
║                  Version 3.0.0                   ║
╚══════════════════════════════════════════════════════╝

Type help for available commands, exit to quit

hektor > use ./mydb
✓ Database set to: ./mydb

hektor:./mydb > search "gold outlook" -k 10
[████████████████████] 100% (0.12s)

┌──────┬────────────────────────┬───────┐
│ ID   │ Text                   │ Score │
├──────┼────────────────────────┼───────┤
│ 1234 │ Gold market outlook... │ 0.953 │
│ 5678 │ Gold price forecast... │ 0.921 │
└──────┴────────────────────────┴───────┘

✓ Found 10 results in 0.12s

hektor:./mydb > history
  1: use ./mydb
  2: search "gold outlook" -k 10

hektor:./mydb > exit
Goodbye!
```

### Shell Features

- **Command History**: UP/DOWN arrows to navigate
- **Tab Completion**: Press TAB to autocomplete commands
- **Persistent History**: Saved in `~/.hektor_history`
- **Built-in Commands**: `exit`, `quit`, `help`, `history`, `clear`, `use`
- **Color Output**: Success (green), errors (red), warnings (yellow)
- **Progress Indicators**: Progress bars for long operations

---

## Database Management

### Initialize Database

```bash
# Basic initialization
hektor init ./mydb

# With custom settings
hektor init ./mydb \
  --dimension 512 \
  --metric cosine \
  --index-type hnsw

# With presets
hektor init ./mydb --preset gold-standard
hektor init ./mydb --preset nlp-optimized
hektor init ./mydb --preset high-performance
```

### Database Info

```bash
# Show database information
hektor info ./mydb

# JSON output
hektor info ./mydb --format json
```

### Optimize Database

```bash
# Optimize for better performance
hektor optimize ./mydb

# With specific options
hektor optimize ./mydb --reindex --compact
```

### Backup & Restore

```bash
# Backup database
hektor backup ./mydb ./backup.tar.gz --compress

# Restore from backup
hektor restore ./backup.tar.gz ./mydb-restored

# Incremental backup
hektor backup ./mydb ./backup-inc.tar.gz --incremental
```

### Health Check

```bash
# Basic health check
hektor health ./mydb

# Detailed diagnostics
hektor health ./mydb --detailed

# With repair suggestions
hektor health ./mydb --suggest-fixes
```

### List Databases

```bash
# List all databases in directory
hektor db:list --path /data

# With details
hektor db:list --path /data --detailed
```

---

## Data Operations

### Add Documents

```bash
# From text
hektor add ./mydb --text "Gold prices rising"

# From file
hektor add ./mydb --file document.txt

# With metadata
hektor add ./mydb \
  --text "Market analysis" \
  --type journal \
  --metadata '{"author":"John Doe","date":"2026-01-08"}'
```

### Get Document

```bash
# Get by ID
hektor get ./mydb 12345

# JSON output
hektor get ./mydb 12345 --format json

# Include vectors
hektor get ./mydb 12345 --include-vectors
```

### Update Document

```bash
# Update text
hektor update ./mydb 12345 --text "Updated content"

# Update metadata
hektor update ./mydb 12345 --metadata '{"status":"reviewed"}'
```

### Delete Document

```bash
# Delete with confirmation
hektor delete ./mydb 12345

# Delete without confirmation
hektor delete ./mydb 12345 --force

# Short alias
hektor rm ./mydb 12345 --force
```

### Batch Operations

```bash
# Batch insert from JSONL file
hektor batch ./mydb documents.jsonl

# With parallel workers
hektor batch ./mydb documents.jsonl --workers 8 --chunk-size 100

# With progress bar
hektor batch ./mydb large-dataset.jsonl --show-progress
```

### List Documents

```bash
# List all documents
hektor list ./mydb

# Short alias
hektor ls ./mydb

# With pagination
hektor list ./mydb --limit 50 --offset 100

# With filters
hektor list ./mydb --type journal --date-from 2026-01-01
```

---

## Search Operations

### Semantic Search

```bash
# Basic search
hektor search ./mydb "gold outlook" -k 20

# Short alias
hektor s ./mydb "gold"

# With similarity threshold
hektor search ./mydb "forecast" -k 20 --min-score 0.8

# With filters
hektor search ./mydb "analysis" \
  --type journal \
  --tags gold,bullish \
  --date-from 2026-01-01 \
  --date-to 2026-01-31

# With explanation
hektor search ./mydb "forecast" -k 10 --explain

# JSON output
hektor search ./mydb "gold" -k 10 --format json
```

---

## Hybrid Search

Hybrid search combines vector similarity with BM25 full-text search for improved results.

### Hybrid Vector + BM25

```bash
# Basic hybrid search with RRF fusion
hektor hybrid:search ./mydb "gold outlook" --fusion rrf -k 20

# Short alias
hektor hs ./mydb "analysis" --fusion rrf

# Weighted fusion
hektor hybrid:search ./mydb "forecast" \
  --fusion weighted \
  --vector-weight 0.8 \
  --lexical-weight 0.2 \
  -k 20

# CombSUM fusion
hektor hs ./mydb "market analysis" --fusion combsum

# All fusion methods: rrf, weighted, combsum, combmnz, borda
```

### BM25 Only

```bash
# Full-text search only
hektor hybrid:bm25 ./mydb "gold prices" -k 10

# With filters
hektor hybrid:bm25 ./mydb "analysis" \
  --type journal \
  --date-from 2026-01-01 \
  -k 20
```

---

## Data Ingestion

Ingest data from multiple sources with intelligent chunking strategies.

### Supported Formats

- CSV, JSON, JSONL
- PDF documents
- Excel spreadsheets (.xlsx, .xls)
- XML documents
- Parquet files
- SQLite databases
- PostgreSQL with pgvector
- HTTP/REST APIs

### Basic Ingestion

```bash
# Ingest CSV file
hektor ingest ./mydb data.csv

# Ingest JSON
hektor ingest ./mydb documents.json

# Ingest PDF files
hektor ingest ./mydb document.pdf
```

### Advanced Ingestion

```bash
# Ingest directory recursively
hektor ingest ./mydb ./documents --format pdf --recursive

# With chunking strategy
hektor ingest ./mydb ./docs \
  --format pdf \
  --chunk-strategy semantic \
  --chunk-size 512 \
  --overlap 100

# With parallel processing
hektor ingest ./mydb ./data \
  --format csv \
  --workers 8 \
  --batch-size 1000

# From PostgreSQL
hektor ingest ./mydb \
  postgres://host/db \
  --adapter pgvector \
  --table embeddings

# From HTTP API
hektor ingest ./mydb \
  http://api.example.com/data \
  --format json \
  --headers "Authorization: Bearer TOKEN"
```

### Chunking Strategies

```bash
# Fixed-size chunks
hektor ingest ./mydb docs.pdf --chunk-strategy fixed --chunk-size 512

# Sentence-based chunks
hektor ingest ./mydb docs.pdf --chunk-strategy sentence

# Paragraph-based chunks
hektor ingest ./mydb docs.pdf --chunk-strategy paragraph

# Semantic chunks (NLP-powered)
hektor ingest ./mydb docs.pdf --chunk-strategy semantic

# Recursive chunks
hektor ingest ./mydb docs.pdf --chunk-strategy recursive
```

### Scan Preview

```bash
# Scan without importing
hektor ingest:scan ./documents --recursive

# With statistics
hektor ingest:scan ./data --format csv --show-stats
```

---

## NLP Operations

Native NLP engine for advanced text processing (bare-metal C++ implementation).

### Text Preprocessing

```bash
# Basic preprocessing
hektor nlp:preprocess --text "The Gold Prices Are Rising!"

# With options
hektor nlp:preprocess \
  --text "Gold prices rising!" \
  --lowercase \
  --remove-punctuation \
  --stem
```

### Tokenization

```bash
# BPE tokenization
hektor nlp:tokenize --text "gold prices" --model bpe

# WordPiece tokenization
hektor nlp:tokenize --text "gold prices" --model wordpiece

# SentencePiece tokenization
hektor nlp:tokenize --text "gold prices" --model sentencepiece
```

### Embedding Generation

```bash
# Generate embeddings
hektor nlp:embed --text "gold prices rising" --model all-MiniLM-L6-v2

# With multiple texts
hektor nlp:embed \
  --texts "gold prices" "silver forecast" "market analysis" \
  --model all-MiniLM-L6-v2

# From file
hektor nlp:embed --file texts.txt --model bert-base
```

### Semantic Similarity

```bash
# Calculate similarity
hektor nlp:similarity \
  --text1 "gold prices rising" \
  --text2 "gold market increasing" \
  --metric cosine

# Available metrics: cosine, euclidean, dot
```

### Named Entity Recognition

```bash
# Extract entities
hektor nlp:ner --text "Gold prices in London reached $2000 on Monday."

# JSON output
hektor nlp:ner --text "..." --format json
```

### Query Analysis & Expansion

```bash
# Analyze query
hektor nlp:query --text "gold forecast" --analyze

# Expand query
hektor nlp:query --text "gold forecast" --expand

# Both
hektor nlp:query --text "gold forecast" --analyze --expand
```

### Sentiment Analysis

```bash
# Analyze sentiment
hektor nlp:sentiment --text "Gold prices are soaring! Great news!"

# With score
hektor nlp:sentiment --text "..." --show-score
```

### Text Classification

```bash
# Classify text
hektor nlp:classify --text "..." --topics finance,sports,tech

# With confidence scores
hektor nlp:classify --text "..." --topics finance,sports --show-confidence
```

### Text Completion

```bash
# Complete text
hektor nlp:complete --text "Gold prices are" --max-tokens 50

# With custom model
hektor nlp:complete --text "..." --model gpt2 --max-tokens 100
```

### End-to-End Pipeline

```bash
# Full NLP pipeline
hektor nlp:pipeline \
  --text "..." \
  --stages preprocess,tokenize,embed,ner \
  --model bert-base

# Save results
hektor nlp:pipeline \
  --text "..." \
  --stages preprocess,tokenize,embed \
  --output results.json
```

### Train Custom Models

```bash
# Train model
hektor nlp:train \
  --base-model bert-base \
  --train-data training.jsonl \
  --epochs 10 \
  --output custom-bert

# With validation
hektor nlp:train \
  --base-model bert-base \
  --train-data train.jsonl \
  --val-data val.jsonl \
  --output custom-bert
```

### Model Evaluation

```bash
# Evaluate model
hektor nlp:evaluate \
  --model custom-bert \
  --test-data test.jsonl

# With metrics
hektor nlp:evaluate \
  --model custom-bert \
  --test-data test.jsonl \
  --metrics accuracy,f1,precision,recall
```

---

## Model Management

### List Models

```bash
# List all models
hektor model:list

# Filter by category
hektor model:list --category nlp
hektor model:list --category embedding
hektor model:list --category llm
```

### Download Models

```bash
# Download model
hektor model:download all-MiniLM-L6-v2

# With progress
hektor model:download bert-base --show-progress

# From specific source
hektor model:download all-mpnet-base-v2 --source huggingface
```

### Model Information

```bash
# Show model info
hektor model:info all-MiniLM-L6-v2

# With detailed stats
hektor model:info bert-base --detailed
```

### Benchmark Models

```bash
# Benchmark single model
hektor model:benchmark all-MiniLM-L6-v2 --texts 1000

# Compare models
hektor model:benchmark \
  --models all-MiniLM-L6-v2,all-mpnet-base-v2 \
  --texts 1000 \
  --compare
```

### Quantize Models

```bash
# INT8 quantization
hektor model:quantize bert-base \
  --type int8 \
  --output bert-base-int8

# INT4 quantization
hektor model:quantize bert-large \
  --type int4 \
  --output bert-large-int4

# With calibration data
hektor model:quantize bert-base \
  --type int8 \
  --calibration-data calib.jsonl \
  --output bert-base-int8
```

### Set Default Model

```bash
# Set default
hektor model:set-default all-MiniLM-L6-v2

# For specific task
hektor model:set-default bert-base --task classification
```

### Remove Models

```bash
# Remove model
hektor model:remove bert-large

# Force remove
hektor model:remove bert-large --force
```

### Clear Cache

```bash
# Clear all model cache
hektor model:cache-clear

# Clear specific model
hektor model:cache-clear --model bert-base
```

---

## Index Management

### Build Index

```bash
# Build HNSW index
hektor index:build ./mydb --type hnsw

# With custom parameters
hektor index:build ./mydb \
  --type hnsw \
  --hnsw-m 32 \
  --hnsw-ef 200 \
  --hnsw-ef-construction 400

# Flat index
hektor index:build ./mydb --type flat
```

### Optimize Index

```bash
# Optimize index
hektor index:optimize ./mydb

# With specific optimizations
hektor index:optimize ./mydb --reindex --compact
```

### Index Statistics

```bash
# Show index stats
hektor index:stats ./mydb

# Detailed statistics
hektor index:stats ./mydb --detailed

# JSON output
hektor index:stats ./mydb --format json
```

### Benchmark Index

```bash
# Benchmark search performance
hektor index:benchmark ./mydb --queries 1000 --k 10

# With custom query file
hektor index:benchmark ./mydb \
  --query-file queries.txt \
  --k 20 \
  --detailed
```

---

## Collections

### Create Collection

```bash
# Create collection
hektor collection:create ./mydb journals

# With description
hektor collection:create ./mydb journals \
  --description "Market journals and analysis"

# With metadata
hektor collection:create ./mydb journals \
  --metadata '{"category":"finance","public":true}'
```

### List Collections

```bash
# List all collections
hektor collection:list ./mydb

# Short alias
hektor col:ls ./mydb

# With details
hektor collection:list ./mydb --detailed
```

### Collection Info

```bash
# Show collection info
hektor collection:info ./mydb journals

# With statistics
hektor collection:info ./mydb journals --stats
```

### Delete Collection

```bash
# Delete with confirmation
hektor collection:delete ./mydb old-data

# Force delete
hektor collection:delete ./mydb old-data --force
```

---

## Metadata & Tagging

### Add Tags

```bash
# Add tags to document
hektor meta:tag ./mydb 12345 --tags gold,bullish,analysis

# Add multiple tags
hektor meta:tag ./mydb 12345 --tags "gold,silver,forecast"
```

### Remove Tags

```bash
# Remove specific tags
hektor meta:untag ./mydb 12345 --tags old-tag

# Remove all tags
hektor meta:untag ./mydb 12345 --all
```

### Set Metadata

```bash
# Set single attribute
hektor meta:set ./mydb 12345 --key author --value "John Doe"

# Set multiple attributes
hektor meta:set ./mydb 12345 \
  --metadata '{"author":"John Doe","date":"2026-01-08","status":"reviewed"}'
```

### Get Metadata

```bash
# Get all metadata
hektor meta:get ./mydb 12345

# Get specific key
hektor meta:get ./mydb 12345 --key author

# JSON output
hektor meta:get ./mydb 12345 --format json
```

### Delete Metadata

```bash
# Delete specific key
hektor meta:delete ./mydb 12345 --key temp-field

# Delete all metadata
hektor meta:delete ./mydb 12345 --all
```

### Filter by Metadata

```bash
# Filter documents
hektor meta:filter ./mydb --tags gold --type journal

# Complex filter
hektor meta:filter ./mydb \
  --tags gold,bullish \
  --type journal \
  --date-from 2026-01-01 \
  --author "John Doe"
```

### List All Tags

```bash
# List tags
hektor meta:list-tags ./mydb

# With counts
hektor meta:list-tags ./mydb --with-counts
```

### Search by Metadata

```bash
# Search by metadata
hektor meta:search ./mydb --author "John Doe"

# Combined with vector search
hektor meta:search ./mydb \
  --tags gold,bullish \
  --combine-with-vector "market outlook"
```

---

## Monitoring & Alerts

### Prometheus Metrics

```bash
# Export Prometheus metrics
hektor monitor:metrics ./mydb

# Specific format
hektor monitor:metrics ./mydb --format prometheus

# With custom port
hektor monitor:metrics ./mydb --port 9090
```

### View Logs

```bash
# View recent logs
hektor monitor:logs ./mydb

# Filter by level
hektor monitor:logs ./mydb --level error

# Tail logs
hektor monitor:logs ./mydb --tail 100 --follow

# Time range
hektor monitor:logs ./mydb \
  --from "2026-01-08 00:00" \
  --to "2026-01-08 23:59"
```

### Distributed Tracing

```bash
# View trace
hektor monitor:trace ./mydb --trace-id abc123

# All traces in time range
hektor monitor:trace ./mydb --from "2026-01-08 10:00"
```

### Alert Management

```bash
# List active alerts
hektor monitor:alerts ./mydb --active

# All alerts
hektor monitor:alerts ./mydb

# Alert history
hektor monitor:alerts ./mydb --history --from "2026-01-01"
```

### Performance Statistics

```bash
# Show performance stats
hektor monitor:performance ./mydb

# Time window
hektor monitor:performance ./mydb --window 1h
hektor monitor:performance ./mydb --window 24h

# Detailed breakdown
hektor monitor:performance ./mydb --detailed
```

### System Health

```bash
# Health check
hektor monitor:health ./mydb

# Detailed diagnostics
hektor monitor:health ./mydb --detailed

# JSON output
hektor monitor:health ./mydb --format json
```

### Monitoring Dashboard

```bash
# Launch dashboard
hektor monitor:dashboard ./mydb

# Custom port
hektor monitor:dashboard ./mydb --port 9090

# With authentication
hektor monitor:dashboard ./mydb --auth --user admin
```

---

## Reports

### Database Summary

```bash
# Generate summary report
hektor report:summary ./mydb

# With visualizations
hektor report:summary ./mydb --charts

# Export to file
hektor report:summary ./mydb --output summary.pdf
```

### Usage Statistics

```bash
# Usage report
hektor report:usage ./mydb

# Time period
hektor report:usage ./mydb --period 7d
hektor report:usage ./mydb --period 30d

# Detailed breakdown
hektor report:usage ./mydb --period 7d --detailed
```

### Audit Trail

```bash
# Audit report
hektor report:audit ./mydb

# Time range
hektor report:audit ./mydb \
  --from 2026-01-01 \
  --to 2026-01-31

# Filter by user
hektor report:audit ./mydb --user "john.doe"

# Filter by action
hektor report:audit ./mydb --action delete
```

### Data Quality

```bash
# Quality report
hektor report:quality ./mydb

# With specific checks
hektor report:quality ./mydb \
  --check-duplicates \
  --check-empty \
  --check-anomalies

# Export results
hektor report:quality ./mydb --output quality-report.json
```

### Index Coverage

```bash
# Coverage analysis
hektor report:coverage ./mydb

# With recommendations
hektor report:coverage ./mydb --suggest-improvements
```

### Export Reports

```bash
# Export all reports
hektor report:export ./mydb ./reports/

# Specific format
hektor report:export ./mydb ./reports/ --format pdf

# Specific reports
hektor report:export ./mydb ./reports/ \
  --reports summary,usage,audit \
  --format html
```

---

## Export Operations

### Export Data

```bash
# Export to JSONL
hektor export:data ./mydb output.jsonl

# Include vectors
hektor export:data ./mydb output.jsonl --include-vectors

# Include metadata
hektor export:data ./mydb output.jsonl --include-metadata

# Filter export
hektor export:data ./mydb output.jsonl \
  --type journal \
  --tags gold \
  --date-from 2026-01-01
```

### Export Training Pairs

```bash
# Export positive pairs
hektor export:pairs ./mydb pairs.jsonl

# With similarity threshold
hektor export:pairs ./mydb pairs.jsonl --min-score 0.8

# Limit number
hektor export:pairs ./mydb pairs.jsonl --limit 10000
```

### Export Training Triplets

```bash
# Export triplets for contrastive learning
hektor export:triplets ./mydb triplets.jsonl

# With hard negatives
hektor export:triplets ./mydb triplets.jsonl \
  --negative-samples 10 \
  --strategy hard

# With specific similarity ranges
hektor export:triplets ./mydb triplets.jsonl \
  --positive-min 0.8 \
  --negative-max 0.3
```

---

## Advanced Workflows

### NLP-Powered Search Workflow

```bash
# 1. Initialize with NLP optimization
hektor init ./mydb --preset nlp-optimized

# 2. Ingest with full NLP pipeline
hektor ingest ./mydb documents/ \
  --format pdf \
  --nlp-pipeline preprocess,tokenize,embed,ner \
  --model all-MiniLM-L6-v2 \
  --extract-entities \
  --recursive

# 3. Build optimized index
hektor index:build ./mydb --type hnsw --hnsw-m 32

# 4. Search with NLP features
hektor search ./mydb "gold market forecast" \
  --nlp-analyze \
  --expand-query \
  --extract-entities \
  -k 20

# 5. Hybrid search with fusion
hektor hybrid:search ./mydb "market analysis" \
  --fusion rrf \
  --nlp-preprocess \
  -k 20
```

### Production Deployment Workflow

```bash
# 1. Initialize production database
hektor init ./prod-db --preset high-performance

# 2. Ingest data with parallel processing
hektor ingest ./prod-db /data/sources \
  --format csv \
  --workers 16 \
  --batch-size 5000 \
  --recursive

# 3. Build and optimize index
hektor index:build ./prod-db --type hnsw
hektor index:optimize ./prod-db --reindex

# 4. Setup monitoring
hektor monitor:dashboard ./prod-db --port 9090 &

# 5. Generate baseline reports
hektor report:summary ./prod-db --output baseline.pdf
hektor report:quality ./prod-db --check-all

# 6. Setup backup schedule (use cron)
hektor backup ./prod-db ./backups/db-$(date +%Y%m%d).tar.gz --compress
```

### Model Fine-tuning Workflow

```bash
# 1. Export training data
hektor export:triplets ./mydb training.jsonl \
  --negative-samples 10 \
  --strategy hard \
  --limit 100000

# 2. Train custom model
hektor nlp:train \
  --base-model all-MiniLM-L6-v2 \
  --train-data training.jsonl \
  --val-data validation.jsonl \
  --epochs 10 \
  --output custom-model

# 3. Evaluate performance
hektor nlp:evaluate \
  --model custom-model \
  --test-data test.jsonl \
  --metrics accuracy,f1

# 4. Benchmark vs base model
hektor model:benchmark \
  --models all-MiniLM-L6-v2,custom-model \
  --texts 1000 \
  --compare

# 5. Quantize for production
hektor model:quantize custom-model \
  --type int8 \
  --output custom-model-int8

# 6. Set as default
hektor model:set-default custom-model-int8
```

### Batch Processing Workflow

```bash
# 1. Scan source data
hektor ingest:scan /data/incoming --recursive --show-stats

# 2. Batch ingest with progress
hektor batch ./mydb /data/incoming/*.jsonl \
  --workers 8 \
  --chunk-size 1000 \
  --show-progress

# 3. Verify data quality
hektor report:quality ./mydb --check-all

# 4. Update metadata in bulk
for id in $(hektor list ./mydb --format json | jq -r '.[] | .id'); do
  hektor meta:tag ./mydb $id --tags batch-processed-2026-01-08
done

# 5. Export processed results
hektor export:data ./mydb processed-output.jsonl \
  --tags batch-processed-2026-01-08 \
  --include-metadata
```

---

## Global Options

All commands support these global options:

```bash
--help, -h           Show help for command
--version, -v        Show version information
--quiet, -q          Suppress non-essential output
--verbose, -V        Enable verbose logging
--debug, -d          Enable debug mode
--format, -f         Output format: table, json, csv
--output, -o         Output file path
--no-color           Disable color output
```

### Examples

```bash
# JSON output
hektor search ./mydb "gold" -k 10 --format json

# Save to file
hektor search ./mydb "gold" -k 10 --output results.json

# Verbose mode
hektor ingest ./mydb data.csv --verbose

# Debug mode
hektor index:build ./mydb --debug
```

---

## Configuration

### Environment Variables

```bash
# Default database path
export HEKTOR_DB_PATH=./mydb

# Default output format
export HEKTOR_OUTPUT_FORMAT=json

# Default model
export HEKTOR_DEFAULT_MODEL=all-MiniLM-L6-v2

# Disable colors
export HEKTOR_NO_COLOR=1

# Log level
export HEKTOR_LOG_LEVEL=INFO
```

### Configuration File

Create `~/.hektorrc` or `.hektorrc` in project directory:

```yaml
database:
  default_path: ./mydb
  default_dimension: 384
  default_metric: cosine

output:
  default_format: table
  colors: true
  progress_bars: true

nlp:
  default_model: all-MiniLM-L6-v2
  cache_dir: ~/.hektor/models

search:
  default_k: 20
  default_min_score: 0.0

ingestion:
  default_workers: 4
  default_batch_size: 1000
  default_chunk_strategy: semantic
```

---

## Troubleshooting

### Common Issues

**Database not found:**
```bash
# Check if database exists
hektor info ./mydb

# Initialize if needed
hektor init ./mydb
```

**Out of memory:**
```bash
# Use smaller batch size
hektor batch ./mydb data.jsonl --chunk-size 100

# Reduce workers
hektor ingest ./mydb data.csv --workers 2
```

**Slow search:**
```bash
# Optimize index
hektor index:optimize ./mydb

# Check index statistics
hektor index:stats ./mydb --detailed

# Rebuild index
hektor index:build ./mydb --type hnsw
```

**Model download fails:**
```bash
# Check network connection
ping huggingface.co

# Use alternative source
hektor model:download all-MiniLM-L6-v2 --source github

# Download with retry
hektor model:download all-MiniLM-L6-v2 --retry 3
```

### Getting Help

```bash
# General help
hektor --help

# Command-specific help
hektor search --help
hektor nlp:embed --help

# List all commands
hektor help

# Check version
hektor --version
```

---

## Performance Tips

1. **Use Batch Operations**: Process multiple documents at once
2. **Optimize Index**: Run `hektor index:optimize` regularly
3. **Quantize Models**: Use INT8 quantization for faster inference
4. **Parallel Processing**: Increase workers for ingestion
5. **Monitor Performance**: Use `hektor monitor:performance` to identify bottlenecks

---

## Next Steps

- Read [CLI Design](./CLI_DESIGN.md) for complete specification
- Check [CLI Implementation](./CLI_IMPLEMENTATION.md) for technical details
- Browse [CLI Quick Reference](./CLI_QUICK_REFERENCE.md) for examples
- Review [NLP Guide](./NLP_GUIDE.md) for NLP engine documentation

---

**For more information, visit**: https://github.com/amuzetnoM/hektor
