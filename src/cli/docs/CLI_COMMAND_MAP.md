---
title: "CLI Command Map & Flow Diagrams"
description: "Visual maps and flow diagrams for all CLI commands"
version: "2.3.0"
last_updated: "2026-01-07"
status: "Design"
---

# Hektor CLI - Command Map & Flow Diagrams

## Complete Command Taxonomy

### Command Tree Structure

```
hektor
├── db (database)
│   ├── init (create, new)
│   ├── open
│   ├── close
│   ├── drop (delete, remove)
│   ├── info (stats, status)
│   ├── health (check)
│   ├── optimize (compact)
│   ├── sync (flush)
│   ├── backup
│   ├── restore
│   └── list (ls)
│
├── add (insert, put)
│   ├── --text
│   ├── --file
│   ├── --image
│   ├── --batch
│   └── --stream
│
├── update (modify, edit)
│   ├── --text
│   ├── --metadata
│   └── --tags
│
├── delete (del, rm, remove)
│   └── --force
│
├── get (show, view)
│   └── --format
│
├── search (s, query, find)
│   ├── --top
│   ├── --threshold
│   ├── --filter
│   ├── --type
│   ├── --date-from
│   ├── --date-to
│   └── --explain
│
├── query (q)
│   ├── --query
│   ├── --vector
│   ├── --filter
│   ├── --sort-by
│   └── --limit
│
├── similar (sim)
│   ├── --top
│   └── --exclude-self
│
├── hybrid (hs)
│   ├── search
│   │   ├── --fusion
│   │   ├── --vector-weight
│   │   └── --lexical-weight
│   ├── bm25
│   └── rerank
│
├── ingest (import, load)
│   ├── [source]
│   │   ├── --format
│   │   ├── --adapter
│   │   ├── --recursive
│   │   ├── --pattern
│   │   ├── --chunk-strategy
│   │   └── --incremental
│   ├── gold-standard (gs)
│   │   ├── --journals
│   │   ├── --charts
│   │   └── --reports
│   └── scan
│       └── --show-preview
│
├── index (idx)
│   ├── build
│   │   ├── --type
│   │   ├── --hnsw-m
│   │   └── --hnsw-ef
│   ├── rebuild
│   ├── optimize (compact)
│   ├── stats (info)
│   ├── validate (check)
│   └── benchmark
│       ├── --queries
│       └── --k
│
├── collection (col, coll)
│   ├── list (ls)
│   ├── create (new)
│   │   ├── --dimension
│   │   ├── --metric
│   │   └── --description
│   ├── rename (mv)
│   ├── delete (rm, drop)
│   ├── clone (copy)
│   └── info (stats)
│
├── meta (metadata)
│   ├── tag
│   ├── untag
│   ├── set
│   ├── get
│   ├── delete (rm)
│   ├── tags
│   └── filter
│
├── export (dump, save)
│   ├── [output]
│   │   ├── --format
│   │   ├── --filter
│   │   └── --compress
│   ├── training
│   │   ├── --type
│   │   └── --negative-samples
│   ├── pairs
│   ├── triplets
│   └── embeddings
│       └── --format
│
├── import (restore, load)
│   ├── --format
│   ├── --merge
│   └── --overwrite
│
├── model (models)
│   ├── list (ls)
│   │   ├── --local
│   │   └── --remote
│   ├── download (get, pull)
│   │   ├── --variant
│   │   ├── --quantization
│   │   └── --cache-dir
│   ├── info (show)
│   ├── benchmark (bench)
│   │   ├── --batch-size
│   │   └── --iterations
│   ├── set-default
│   └── delete (rm)
│
├── cluster (dist, distributed)
│   ├── init
│   │   ├── --nodes
│   │   ├── --replication-factor
│   │   └── --shards
│   ├── add-node (join)
│   ├── remove-node (leave)
│   ├── nodes (status)
│   ├── replicate
│   │   ├── --mode
│   │   ├── --targets
│   │   └── --factor
│   ├── shard
│   │   ├── --strategy
│   │   ├── --shards
│   │   └── --rebalance
│   └── health
│
├── monitor (mon)
│   ├── metrics
│   │   ├── --prometheus
│   │   ├── --interval
│   │   └── --watch
│   ├── logs
│   │   ├── --level
│   │   ├── --tail
│   │   ├── --follow
│   │   └── --grep
│   ├── trace
│   │   ├── --operation
│   │   └── --duration
│   ├── alerts
│   └── perf
│       ├── --interval
│       └── --duration
│
├── report (reports)
│   ├── summary (stats)
│   ├── usage
│   │   ├── --period
│   │   ├── --from
│   │   └── --to
│   ├── perf (performance)
│   ├── audit
│   │   ├── --from
│   │   ├── --to
│   │   ├── --user
│   │   └── --action
│   ├── quality
│   └── coverage
│
├── auth (security, sec)
│   ├── create-user
│   │   ├── --password
│   │   └── --role
│   ├── list-users (users)
│   ├── change-password
│   ├── grant
│   ├── revoke
│   ├── encrypt
│   │   ├── --key-file
│   │   └── --algorithm
│   └── decrypt
│
├── config (cfg, conf)
│   ├── show
│   │   ├── --global
│   │   └── --local
│   ├── set
│   │   └── --global
│   ├── get
│   ├── unset
│   ├── edit
│   ├── validate
│   └── profile
│       ├── create
│       ├── use
│       └── list
│
├── rag
│   ├── init
│   │   ├── --chunking
│   │   ├── --chunk-size
│   │   ├── --chunk-overlap
│   │   └── --framework
│   ├── add
│   ├── query
│   │   ├── --top
│   │   ├── --llm-model
│   │   ├── --temperature
│   │   └── --max-tokens
│   └── config
│
├── quantize (quant)
│   ├── [db]
│   │   ├── --method
│   │   ├── --bits
│   │   └── --codebook-size
│   ├── info
│   └── benchmark
│
├── util (utils)
│   ├── validate (check)
│   ├── repair (fix)
│   ├── benchmark (bench)
│   └── system-info (sysinfo)
│
├── shell (repl, interactive)
├── version (--version, -V)
├── help (--help, -h, ?)
├── completion
│   ├── bash
│   ├── zsh
│   └── fish
└── check-deps
```

## Command Flow Diagrams

### 1. Database Lifecycle Flow

```
┌─────────────┐
│   START     │
└──────┬──────┘
       │
       ▼
┌─────────────────────────┐
│  hektor db init <path>  │  ◄─── Initialize new database
└──────┬──────────────────┘
       │
       ▼
┌─────────────────────────┐
│  hektor add <db> ...    │  ◄─── Add documents
└──────┬──────────────────┘
       │
       ▼
┌─────────────────────────┐
│ hektor index build <db> │  ◄─── Build/optimize index
└──────┬──────────────────┘
       │
       ▼
┌─────────────────────────┐
│  hektor search <db> ... │  ◄─── Query database
└──────┬──────────────────┘
       │
       ├────► hektor db backup <db>  ◄─── Backup
       │
       ├────► hektor db optimize <db>  ◄─── Optimize
       │
       ├────► hektor export <db> ...  ◄─── Export data
       │
       └────► hektor db close <db>  ◄─── Close/cleanup
```

### 2. Data Ingestion Flow

```
                           ┌──────────────────┐
                           │   Data Source    │
                           └────────┬─────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    │               │               │
                    ▼               ▼               ▼
            ┌───────────┐   ┌───────────┐   ┌───────────┐
            │   Files   │   │    API    │   │ Database  │
            │(CSV,PDF,..)│   │  (HTTP)   │   │(SQL,pgvec)│
            └─────┬─────┘   └─────┬─────┘   └─────┬─────┘
                  │               │               │
                  └───────────────┼───────────────┘
                                  │
                                  ▼
                    ┌──────────────────────────┐
                    │  hektor ingest <db>      │
                    │  --format <fmt>          │
                    │  --chunk-strategy <strat>│
                    └──────────┬───────────────┘
                               │
                    ┌──────────┼──────────┐
                    │          │          │
                    ▼          ▼          ▼
           ┌──────────┐ ┌──────────┐ ┌──────────┐
           │  Parse   │ │  Chunk   │ │ Extract  │
           │          │ │          │ │ Metadata │
           └────┬─────┘ └────┬─────┘ └────┬─────┘
                │            │            │
                └────────────┼────────────┘
                             │
                             ▼
                    ┌──────────────┐
                    │   Embed      │
                    │  (ONNX)      │
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────┐
                    │   Store in   │
                    │  VectorDB    │
                    └──────────────┘
```

### 3. Search Flow (Vector + Hybrid)

```
                           ┌──────────────┐
                           │ User Query   │
                           └──────┬───────┘
                                  │
                    ┌─────────────┴─────────────┐
                    │                           │
                    ▼                           ▼
          ┌─────────────────┐         ┌─────────────────┐
          │ hektor search    │         │ hektor hybrid   │
          │ (Vector Only)    │         │ (Vector + BM25) │
          └────────┬─────────┘         └────────┬────────┘
                   │                            │
                   ▼                            ▼
          ┌─────────────────┐         ┌─────────────────┐
          │  1. Embed Query │         │  1. Embed Query │
          │     (ONNX)      │         │     (ONNX)      │
          └────────┬────────┘         └────────┬────────┘
                   │                            │
                   ▼                   ┌────────┴────────┐
          ┌─────────────────┐         │                 │
          │  2. HNSW Search │         ▼                 ▼
          │                 │  ┌──────────┐      ┌──────────┐
          └────────┬────────┘  │ 2a. HNSW │      │ 2b. BM25 │
                   │            │  Search  │      │  Search  │
                   │            └────┬─────┘      └────┬─────┘
                   │                 │                 │
                   │                 └────────┬────────┘
                   │                          │
                   │                          ▼
                   │                ┌──────────────────┐
                   │                │  3. Fusion       │
                   │                │  (RRF/Weighted)  │
                   │                └────────┬─────────┘
                   │                         │
                   └─────────────┬───────────┘
                                 │
                                 ▼
                        ┌─────────────────┐
                        │  4. Apply       │
                        │     Filters     │
                        └────────┬────────┘
                                 │
                                 ▼
                        ┌─────────────────┐
                        │  5. Rank &      │
                        │     Return      │
                        └────────┬────────┘
                                 │
                                 ▼
                        ┌─────────────────┐
                        │   Results       │
                        └─────────────────┘
```

### 4. RAG Pipeline Flow

```
                           ┌──────────────┐
                           │  Documents   │
                           └──────┬───────┘
                                  │
                                  ▼
                    ┌──────────────────────┐
                    │  hektor rag init     │
                    │  --chunking sentence │
                    └──────────┬───────────┘
                               │
                    ┌──────────┼──────────┐
                    │          │          │
                    ▼          ▼          ▼
           ┌──────────┐ ┌──────────┐ ┌──────────┐
           │  Chunk   │ │  Embed   │ │  Index   │
           │          │ │          │ │          │
           └────┬─────┘ └────┬─────┘ └────┬─────┘
                │            │            │
                └────────────┼────────────┘
                             │
                             ▼
                    ┌──────────────┐
                    │  VectorDB    │
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────────────┐
                    │  hektor rag query    │
                    │  "What is X?"        │
                    └──────────┬───────────┘
                               │
                    ┌──────────┼──────────┐
                    │          │          │
                    ▼          ▼          ▼
           ┌──────────┐ ┌──────────┐ ┌──────────┐
           │ Retrieve │ │  Rerank  │ │ Generate │
           │ Top-K    │ │ Context  │ │   LLM    │
           └────┬─────┘ └────┬─────┘ └────┬─────┘
                │            │            │
                └────────────┼────────────┘
                             │
                             ▼
                    ┌──────────────┐
                    │   Answer     │
                    └──────────────┘
```

### 5. Distributed System Flow

```
                    ┌──────────────────────┐
                    │ hektor cluster init  │
                    │ --nodes 3            │
                    │ --replication-factor 2│
                    └──────────┬───────────┘
                               │
                ┌──────────────┼──────────────┐
                │              │              │
                ▼              ▼              ▼
        ┌───────────┐  ┌───────────┐  ┌───────────┐
        │  Node 1   │  │  Node 2   │  │  Node 3   │
        │ (Primary) │  │ (Replica) │  │ (Replica) │
        └─────┬─────┘  └─────┬─────┘  └─────┬─────┘
              │              │              │
              └──────────────┼──────────────┘
                             │
                             ▼
                    ┌──────────────────┐
                    │  Write Request   │
                    └──────────┬───────┘
                               │
                    ┌──────────┼──────────┐
                    │          │          │
                    ▼          ▼          ▼
           ┌──────────┐ ┌──────────┐ ┌──────────┐
           │ Primary  │ │ Replica  │ │ Replica  │
           │  Write   │ │  Sync    │ │  Sync    │
           └────┬─────┘ └────┬─────┘ └────┬─────┘
                │            │            │
                └────────────┼────────────┘
                             │
                             ▼
                    ┌──────────────┐
                    │   ACK to     │
                    │   Client     │
                    └──────────────┘
```

### 6. Model Management Flow

```
                    ┌──────────────────────┐
                    │ hektor model list    │
                    │ --remote             │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  List Available      │
                    │  Models from Hub     │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │ hektor model download│
                    │ all-MiniLM-L6-v2     │
                    └──────────┬───────────┘
                               │
                    ┌──────────┼──────────┐
                    │          │          │
                    ▼          ▼          ▼
           ┌──────────┐ ┌──────────┐ ┌──────────┐
           │ Download │ │ Validate │ │  Cache   │
           │  Model   │ │ Checksum │ │  Local   │
           └────┬─────┘ └────┬─────┘ └────┬─────┘
                │            │            │
                └────────────┼────────────┘
                             │
                             ▼
                    ┌──────────────────────┐
                    │ hektor model         │
                    │ set-default          │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  Model Ready for Use │
                    └──────────────────────┘
```

### 7. Export/Training Data Flow

```
                    ┌──────────────────────┐
                    │   VectorDB with      │
                    │   Documents          │
                    └──────────┬───────────┘
                               │
                ┌──────────────┼──────────────┐
                │              │              │
                ▼              ▼              ▼
    ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
    │ hektor export│  │ hektor export│  │ hektor export│
    │ (data dump)  │  │   pairs      │  │  triplets    │
    └──────┬───────┘  └──────┬───────┘  └──────┬───────┘
           │                 │                 │
           │                 ▼                 ▼
           │      ┌──────────────┐  ┌──────────────┐
           │      │Find Similar  │  │Generate Hard │
           │      │   Pairs      │  │  Negatives   │
           │      └──────┬───────┘  └──────┬───────┘
           │             │                 │
           │             └────────┬────────┘
           │                      │
           └──────────────────────┼──────────────────┐
                                  │                  │
                                  ▼                  ▼
                        ┌──────────────┐  ┌──────────────┐
                        │Export Format │  │  Optional    │
                        │(JSONL/CSV)   │  │ Compression  │
                        └──────┬───────┘  └──────┬───────┘
                               │                 │
                               └────────┬────────┘
                                        │
                                        ▼
                              ┌──────────────┐
                              │Training File │
                              │   Ready      │
                              └──────────────┘
```

## Command Aliases Reference

### Most Used Commands - Short Aliases

| Full Command | Short Alias | Description |
|-------------|-------------|-------------|
| `hektor search` | `hektor s` | Search database |
| `hektor add` | `hektor a` | Add document |
| `hektor get` | `hektor g` | Get by ID |
| `hektor delete` | `hektor rm` | Delete document |
| `hektor list` | `hektor ls` | List items |
| `hektor info` | `hektor i` | Show info |
| `hektor query` | `hektor q` | Advanced query |
| `hektor hybrid search` | `hektor hs` | Hybrid search |
| `hektor collection` | `hektor col` | Collection ops |
| `hektor config` | `hektor cfg` | Config management |
| `hektor model` | `hektor m` | Model operations |
| `hektor cluster` | `hektor cl` | Cluster ops |
| `hektor monitor` | `hektor mon` | Monitoring |
| `hektor export` | `hektor ex` | Export data |
| `hektor import` | `hektor imp` | Import data |

### Action Aliases

| Action | Aliases | Context |
|--------|---------|---------|
| create | new, init, make | Creating resources |
| delete | del, rm, remove, drop | Removing resources |
| list | ls, show | Listing resources |
| info | stats, status, show | Getting information |
| update | modify, edit, set | Updating resources |
| add | insert, put | Adding data |
| get | show, view, fetch | Retrieving data |
| search | find, query, lookup | Searching |
| optimize | compact, tune | Optimization |
| validate | check, verify | Validation |
| backup | save, snapshot | Backup operations |
| restore | load, recover | Restore operations |

## Command Priority Matrix

### Priority 1 (Essential - Implement First)
```
✓ hektor db init
✓ hektor db info
✓ hektor add
✓ hektor search
✓ hektor get
✓ hektor delete
✓ hektor ingest
✓ hektor export
✓ hektor help
✓ hektor version
```

### Priority 2 (High Value - Implement Second)
```
◐ hektor hybrid search
◐ hektor index build
◐ hektor collection (list, create, delete)
◐ hektor model (list, download, info)
◐ hektor query
◐ hektor similar
◐ hektor meta (tag, filter)
◐ hektor db optimize
◐ hektor config
◐ hektor logs
```

### Priority 3 (Advanced - Implement Third)
```
○ hektor cluster
○ hektor rag
○ hektor quantize
○ hektor auth
○ hektor monitor
○ hektor report
○ hektor shell (interactive)
○ hektor completion
```

## Command Complexity Levels

### Level 1: Beginner Commands
Simple, safe operations with minimal options
```
hektor db init <path>
hektor add <db> --text "..."
hektor search <db> "query"
hektor get <db> <id>
hektor info <db>
hektor help
```

### Level 2: Intermediate Commands
More options, requires understanding of concepts
```
hektor ingest <db> <source> --format csv --chunk-strategy sentence
hektor hybrid search <db> "query" --fusion rrf -k 20
hektor index build <db> --type hnsw --hnsw-m 32
hektor collection create <db> <name> --dimension 512
hektor export <db> output.jsonl --include-vectors
```

### Level 3: Advanced Commands
Complex operations, requires expertise
```
hektor cluster init --nodes host1,host2,host3 --shards 8
hektor rag init <db> --framework langchain --chunking semantic
hektor quantize <db> --method product --codebook-size 256
hektor auth encrypt <db> --algorithm aes256 --key-file key.pem
hektor export training <db> triplets.jsonl --negative-samples 5
```

## Error Handling Flows

### 1. Invalid Database Path
```
hektor search /invalid/path "query"
    │
    ▼
┌─────────────────────────┐
│ Check path exists       │
└────┬────────────────────┘
     │ NO
     ▼
┌─────────────────────────┐
│ ERROR: Database not     │
│ found at /invalid/path  │
│                         │
│ Did you mean:           │
│  - ./mydb               │
│  - /data/prod_db        │
│                         │
│ Create new: hektor db   │
│ init /invalid/path      │
└─────────────────────────┘
```

### 2. Model Not Available
```
hektor add <db> --text "..."
    │
    ▼
┌─────────────────────────┐
│ Check model exists      │
└────┬────────────────────┘
     │ NO
     ▼
┌─────────────────────────┐
│ ERROR: Model not found  │
│                         │
│ Download model:         │
│ hektor model download   │
│   all-MiniLM-L6-v2      │
│                         │
│ Or use different model: │
│ hektor config set       │
│   default_model <name>  │
└─────────────────────────┘
```

### 3. Permission Denied
```
hektor db delete <db>
    │
    ▼
┌─────────────────────────┐
│ Check permissions       │
└────┬────────────────────┘
     │ NO
     ▼
┌─────────────────────────┐
│ ERROR: Permission denied│
│                         │
│ Solutions:              │
│ 1. Use sudo/admin       │
│ 2. Check file ownership │
│ 3. Verify write access  │
│                         │
│ For destructive ops,    │
│ use --force flag        │
└─────────────────────────┘
```

## Interactive Mode Flow

### Shell Session
```
$ hektor shell ./mydb

Hektor v2.3.0 - Interactive Shell
Database: ./mydb (512 dims, 10,234 vectors)
Type 'help' for commands, 'exit' to quit

hektor> help
Available commands:
  search <query>        - Search database
  add --text "..."      - Add document
  get <id>              - Get document by ID
  stats                 - Show statistics
  config                - Show configuration
  history               - Show command history
  clear                 - Clear screen
  exit                  - Exit shell

hektor> search "gold prices"
Searching for: "gold prices"

Results (10 found):
1. [0.945] Gold prices surge on inflation fears
2. [0.892] Weekly gold price analysis
3. [0.875] Central banks increase gold reserves
...

hektor> stats
Database Statistics:
  Total vectors: 10,234
  Dimension: 512
  Index type: HNSW
  Memory: 24.5 MB

hektor> exit
Goodbye!
```

## Next Implementation Steps

1. **Command Parser** - Parse command line arguments
2. **Command Router** - Route to appropriate handlers
3. **Handler Implementation** - Implement each command category
4. **Output Formatter** - Format output (table, JSON, etc.)
5. **Help System** - Generate dynamic help
6. **Tab Completion** - Auto-completion for shell
7. **Interactive Mode** - REPL with history
8. **Config System** - Profile management
9. **Error Handling** - User-friendly error messages
10. **Testing** - Integration tests for all commands
