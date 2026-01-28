# HEKTOR SYSTEM MAP 
> Machine-Readable Exhaustive System Documentation <br>
> v4.0.0
---
```
SYSTEM_ID: HEKTOR
VERSION: 4.0.0 (CMake) | 2.0.0 (version.hpp)
TYPE: HIGH_PERFORMANCE_VECTOR_DATABASE
LANGUAGE: C++23
BUILD_SYSTEM: CMake 3.20+
LICENSE: MIT
API_LEVEL: 2
BUILD_DATE: 2026-01-04
```
---

# §1 ARCHITECTURE_TOPOLOGY

```yaml
HEKTOR_ARCHITECTURE:
  CORE_ENGINE:
    language: C++23
    simd: [SSE4, AVX2, AVX512]
    memory_model: memory_mapped_files
    threading: thread_pool_based
    
  LAYERS:
    L0_CORE:
      - types.hpp
      - distance.hpp
      - vector_ops.hpp
    L1_STORAGE:
      - mmap_store
      - sqlite_store
      - pgvector_store
      - metadata_store
    L2_INDEX:
      - hnsw_index
      - flat_index
      - metadata_index
    L3_EMBEDDINGS:
      - text_encoder
      - image_encoder
      - onnx_runtime
    L4_SEARCH:
      - vector_search
      - bm25_engine
      - hybrid_search
    L5_API:
      - cli_interface
      - python_bindings
      - rest_api
```

---

# §2 NAMESPACE_MAP

```
NAMESPACE: vdb
├── NAMESPACE: vdb::embeddings
├── NAMESPACE: vdb::hybrid
├── NAMESPACE: vdb::quantization
├── NAMESPACE: vdb::index
├── NAMESPACE: vdb::storage
├── NAMESPACE: vdb::adapters
├── NAMESPACE: vdb::cli
├── NAMESPACE: vdb::telemetry
├── NAMESPACE: vdb::logging
├── NAMESPACE: vdb::llm
├── NAMESPACE: vdb::framework
```

---

# §3 TYPE_DEFINITIONS

## §3.1 PRIMITIVE_TYPES

```cpp
TYPE: VectorId = uint64_t
  DESCRIPTION: Unique_identifier_for_vectors
  RANGE: [0, 18446744073709551615]

TYPE: Dim = uint32_t
  DESCRIPTION: Vector_dimension_count
  RANGE: [1, 4294967295]

TYPE: Distance = float
  DESCRIPTION: Distance_or_similarity_score
  PRECISION: 32-bit_IEEE754

TYPE: Scalar = float
  DESCRIPTION: Vector_element_data_type
  PRECISION: 32-bit_IEEE754

TYPE: Timestamp = int64_t
  DESCRIPTION: Milliseconds_since_epoch
```

## §3.2 CONSTANTS

```cpp
CONSTANT: TEXT_EMBEDDING_DIM = 384
  MODEL: all-MiniLM-L6-v2

CONSTANT: IMAGE_EMBEDDING_DIM = 512
  MODEL: CLIP_ViT-B/32

CONSTANT: UNIFIED_DIM = 512
  PURPOSE: Unified_embedding_space

CONSTANT: HNSW_M = 16
  PURPOSE: Max_connections_per_layer

CONSTANT: HNSW_EF_CONSTRUCTION = 200
  PURPOSE: Construction_search_width

CONSTANT: HNSW_EF_SEARCH = 50
  PURPOSE: Query_search_width

CONSTANT: HNSW_MAX_ELEMENTS = 1000000
  PURPOSE: Default_index_capacity

CONSTANT: PAGE_SIZE = 4096
  PURPOSE: Storage_page_alignment

CONSTANT: MMAP_CHUNK_SIZE = 67108864
  PURPOSE: Memory_map_chunk_64MB
```

## §3.3 ENUMERATIONS

```cpp
ENUM: DistanceMetric
  VALUES: [Cosine, L2, DotProduct, L2Squared]
  DEFAULT: Cosine

ENUM: DocumentType
  VALUES: [Journal, Chart, CatalystWatchlist, InstitutionalMatrix, 
           EconomicCalendar, WeeklyRundown, ThreeMonthReport, 
           OneYearReport, MonthlyReport, YearlyReport, PreMarket, Unknown]

ENUM: ErrorCode
  VALUES: [Success, InvalidDimension, InvalidVectorId, InvalidState,
           NotTrained, InvalidData, VectorNotFound, IndexFull,
           IndexCorrupted, StorageError, IoError, ModelLoadError,
           InferenceError, InvalidInput, OutOfMemory, NotImplemented,
           NetworkError, ParseError, SystemError, Unknown]

ENUM: SimdLevel
  VALUES: [None=0, SSE4=1, AVX2=2, AVX512=3]

ENUM: Device
  VALUES: [CPU, CUDA, DirectML]

ENUM: LogLevel
  VALUES: [DEBUG=0, INFO=1, WARN=2, ERROR=3, CRITICAL=4, ANOMALY=5]

ENUM: AnomalyType
  VALUES: [UNKNOWN_FORMAT, PARSE_ERROR, DATA_CORRUPTION, UNEXPECTED_SIZE,
           INVALID_ENCODING, SQL_INJECTION_ATTEMPT, MEMORY_ANOMALY,
           PERFORMANCE_DEGRADATION, MISSING_DATA, DUPLICATE_DATA,
           SCHEMA_MISMATCH, CONNECTION_FAILURE, SECURITY_VIOLATION,
           RESOURCE_EXHAUSTION, CUSTOM]

ENUM: ChunkStrategy
  VALUES: [Fixed, Sentence, Paragraph, Semantic, Sliding, Document]

ENUM: FusionMethod
  VALUES: [WeightedSum, RRF, CombSUM, CombMNZ, Borda]

ENUM: ReplicationMode
  VALUES: [None, Async, Sync, SemiSync]

ENUM: ShardingStrategy
  VALUES: [None, Hash, Range, Consistent]

ENUM: DataFormat
  VALUES: [PlainText, Markdown, JSON, CSV, XML, PDF, DOCX, TXT,
           SQL, Excel, Parquet, HTML, API_JSON, RSS, Yahoo_Finance,
           Alpha_Vantage, PNG, JPEG, Unknown]

ENUM: PerceptualCurve
  VALUES: [Linear, Gamma22, Gamma24, PQ_ST2084, HLG_Rec2100]

ENUM: DisplayType
  VALUES: [SDR_BT709, HDR10, HDR10_PLUS, HDR1000, HDR4000, DolbyVision]

ENUM: ColorGamut
  VALUES: [sRGB, DCI_P3, Rec2020]

ENUM: HttpMethod
  VALUES: [GET, POST, PUT, DELETE, PATCH, HEAD]

ENUM: Role
  VALUES: [System, User, Assistant]
```

---

# §4 STRUCT_DEFINITIONS

## §4.1 CORE_STRUCTURES

```cpp
STRUCT: Error
  FIELDS:
    - code: ErrorCode
    - message: string
  METHODS:
    - ok() -> bool
    - operator bool() -> bool

STRUCT: Metadata
  FIELDS:
    - id: VectorId
    - type: DocumentType
    - date: string [YYYY-MM-DD]
    - source_file: string
    - asset: string
    - bias: string [BULLISH|BEARISH|NEUTRAL]
    - gold_price: optional<float>
    - silver_price: optional<float>
    - gsr: optional<float>
    - dxy: optional<float>
    - vix: optional<float>
    - yield_10y: optional<float>
    - content_hash: string [SHA-256]
    - created_at: Timestamp
    - updated_at: Timestamp
    - extra_json: string

STRUCT: SearchResult
  FIELDS:
    - id: VectorId
    - distance: Distance
    - score: float
  OPERATORS:
    - operator<(other) -> bool
    - operator>(other) -> bool

STRUCT: IndexStats
  FIELDS:
    - total_vectors: size_t
    - dimension: size_t
    - memory_usage_bytes: size_t
    - index_size_bytes: size_t
    - metric: DistanceMetric
    - index_type: string
```

## §4.2 CONFIGURATION_STRUCTURES

```cpp
STRUCT: DataChunk
  FIELDS:
    - content: string
    - metadata: unordered_map<string, string>
    - title: optional<string>
    - date: optional<string>
    - source: optional<string>
    - numerical_features: vector<float>
    - chunk_index: size_t [DEFAULT: 0]
    - total_chunks: size_t [DEFAULT: 1]

STRUCT: NormalizedData
  FIELDS:
    - chunks: vector<DataChunk>
    - format: DataFormat
    - source_path: string
    - global_metadata: unordered_map<string, string>
    - confidence: float [DEFAULT: 1.0]
    - sanitized: bool [DEFAULT: false]
    - warnings: vector<string>

STRUCT: DatabaseConfig
  FIELDS:
    - path: filesystem::path
    - dimension: Dim [DEFAULT: 512]
    - metric: DistanceMetric [DEFAULT: Cosine]
    - hnsw_m: size_t [DEFAULT: 16]
    - hnsw_ef_construction: size_t [DEFAULT: 200]
    - hnsw_ef_search: size_t [DEFAULT: 50]
    - max_elements: size_t [DEFAULT: 1000000]
    - text_model_path: string
    - image_model_path: string
    - vocab_path: string
    - auto_download_models: bool [DEFAULT: true]
    - num_threads: int [DEFAULT: 0=auto]
    - memory_only: bool [DEFAULT: false]
    - auto_sync: bool [DEFAULT: true]
    - sync_interval_ms: size_t [DEFAULT: 5000]

STRUCT: HnswConfig
  FIELDS:
    - dimension: Dim [DEFAULT: 512]
    - max_elements: size_t [DEFAULT: 1000000]
    - M: size_t [DEFAULT: 16]
    - ef_construction: size_t [DEFAULT: 200]
    - ef_search: size_t [DEFAULT: 50]
    - metric: DistanceMetric [DEFAULT: Cosine]
    - seed: uint64_t [DEFAULT: 42]
    - allow_replace: bool [DEFAULT: false]
    - num_threads: size_t [DEFAULT: 0=auto]

STRUCT: QueryOptions
  FIELDS:
    - k: size_t [DEFAULT: 10]
    - ef_search: size_t [DEFAULT: 0=use_default]
    - type_filter: optional<DocumentType>
    - date_filter: optional<string>
    - date_from: optional<string>
    - date_to: optional<string>
    - asset_filter: optional<string>
    - bias_filter: optional<string>
    - include_metadata: bool [DEFAULT: true]
    - deduplicate_by_date: bool [DEFAULT: false]

STRUCT: IngestOptions
  FIELDS:
    - overwrite: bool [DEFAULT: false]
    - extract_metadata: bool [DEFAULT: true]
    - date: optional<string>
    - type: optional<DocumentType>
    - bias: optional<string>

STRUCT: IngestConfig
  FIELDS:
    - gold_standard_output: filesystem::path
    - include_journals: bool [DEFAULT: true]
    - include_charts: bool [DEFAULT: true]
    - include_reports: bool [DEFAULT: true]
    - overwrite_existing: bool [DEFAULT: false]
    - incremental: bool [DEFAULT: true]
    - since_date: optional<string>

STRUCT: ChunkConfig
  FIELDS:
    - strategy: ChunkStrategy [DEFAULT: Paragraph]
    - max_chunk_size: size_t [DEFAULT: 1000]
    - overlap: size_t [DEFAULT: 100]
    - preserve_structure: bool [DEFAULT: true]

STRUCT: BM25Config
  FIELDS:
    - k1: float [DEFAULT: 1.2]
    - b: float [DEFAULT: 0.75]
    - min_term_length: size_t [DEFAULT: 2]
    - use_stemming: bool [DEFAULT: true]
    - case_sensitive: bool [DEFAULT: false]

STRUCT: HybridSearchConfig
  FIELDS:
    - vector_weight: float [DEFAULT: 0.7]
    - lexical_weight: float [DEFAULT: 0.3]
    - fusion: FusionMethod [DEFAULT: RRF]
    - rrf_k: size_t [DEFAULT: 60]
    - rerank: bool [DEFAULT: true]

STRUCT: ProductQuantizerConfig
  FIELDS:
    - dimension: Dim [DEFAULT: 512]
    - num_subquantizers: uint32_t [DEFAULT: 8]
    - num_centroids: uint32_t [DEFAULT: 256]
    - num_iterations: uint32_t [DEFAULT: 25]
    - num_threads: uint32_t [DEFAULT: 0=auto]
    - metric: DistanceMetric [DEFAULT: L2]
    - seed: uint64_t [DEFAULT: 42]

STRUCT: TextEncoderConfig
  FIELDS:
    - model_path: filesystem::path
    - vocab_path: filesystem::path
    - output_dim: Dim [DEFAULT: 384]
    - max_seq_length: size_t [DEFAULT: 256]
    - normalize_embeddings: bool [DEFAULT: true]
    - device: Device [DEFAULT: CPU]

STRUCT: ImageEncoderConfig
  FIELDS:
    - model_path: filesystem::path
    - output_dim: Dim [DEFAULT: 512]
    - input_size: size_t [DEFAULT: 224]
    - normalize_embeddings: bool [DEFAULT: true]
    - device: Device [DEFAULT: CPU]

STRUCT: LLMConfig
  FIELDS:
    - model_path: filesystem::path
    - n_ctx: int [DEFAULT: 4096]
    - n_batch: int [DEFAULT: 512]
    - n_threads: int [DEFAULT: 0=auto]
    - n_gpu_layers: int [DEFAULT: 0]
    - use_mmap: bool [DEFAULT: true]
    - use_mlock: bool [DEFAULT: false]
    - rope_freq_base: float [DEFAULT: 0.0=model_default]
    - rope_freq_scale: float [DEFAULT: 0.0=model_default]

STRUCT: GenerationParams
  FIELDS:
    - max_tokens: int [DEFAULT: 1024]
    - temperature: float [DEFAULT: 0.7]
    - top_p: float [DEFAULT: 0.9]
    - top_k: int [DEFAULT: 40]
    - repeat_penalty: float [DEFAULT: 1.1]
    - repeat_last_n: int [DEFAULT: 64]
    - stop_sequences: vector<string>
    - on_token: function<bool(string_view)>

STRUCT: RAGConfig
  FIELDS:
    - top_k: size_t [DEFAULT: 5]
    - max_context_length: size_t [DEFAULT: 2048]
    - relevance_threshold: float [DEFAULT: 0.7]
    - rerank: bool [DEFAULT: true]
    - chunking_strategy: string [DEFAULT: "sentence"]
    - chunk_size: size_t [DEFAULT: 512]
    - chunk_overlap: size_t [DEFAULT: 50]

STRUCT: TelemetryConfig
  FIELDS:
    - enable_tracing: bool [DEFAULT: true]
    - service_name: string [DEFAULT: "vector-studio"]
    - service_version: string [DEFAULT: "3.0.0"]
    - deployment_environment: string [DEFAULT: "production"]
    - sampling_ratio: double [DEFAULT: 1.0]
    - use_tail_based_sampling: bool [DEFAULT: false]
    - trace_exporter: string [DEFAULT: "otlp"]
    - metrics_exporter: string [DEFAULT: "prometheus"]
    - otlp_endpoint: string [DEFAULT: "http://localhost:4317"]
    - prometheus_host: string [DEFAULT: "0.0.0.0"]
    - prometheus_port: int [DEFAULT: 8888]

STRUCT: ReplicationConfig
  FIELDS:
    - mode: ReplicationMode [DEFAULT: None]
    - nodes: vector<NodeConfig>
    - min_replicas: size_t [DEFAULT: 1]
    - sync_timeout_ms: uint32_t [DEFAULT: 5000]
    - heartbeat_interval_ms: uint32_t [DEFAULT: 1000]
    - election_timeout_ms: uint32_t [DEFAULT: 5000]
    - conflict_resolution: enum [DEFAULT: LastWriteWins]

STRUCT: ShardingConfig
  FIELDS:
    - strategy: ShardingStrategy [DEFAULT: None]
    - num_shards: size_t [DEFAULT: 1]
    - shards: vector<ShardConfig>
    - enable_auto_resharding: bool [DEFAULT: false]
    - reshard_threshold_items: size_t [DEFAULT: 1000000]
    - reshard_threshold_imbalance: float [DEFAULT: 0.2]

STRUCT: LoggerConfig
  FIELDS:
    - min_level: LogLevel [DEFAULT: INFO]
    - log_to_console: bool [DEFAULT: true]
    - log_to_file: bool [DEFAULT: true]
    - log_file_path: string [DEFAULT: "vectordb.log"]
    - anomaly_log_path: string [DEFAULT: "anomalies.log"]
    - max_file_size: size_t [DEFAULT: 10485760]
    - max_backup_files: size_t [DEFAULT: 5]
    - include_timestamp: bool [DEFAULT: true]
    - include_thread_id: bool [DEFAULT: true]
    - include_source_location: bool [DEFAULT: true]

STRUCT: HttpClientConfig
  FIELDS:
    - user_agent: string [DEFAULT: "VectorDB/1.0"]
    - rate_limit: RateLimitConfig
    - retry: RetryConfig
    - cache: CacheConfig
    - enable_compression: bool [DEFAULT: true]
    - enable_keepalive: bool [DEFAULT: true]
    - keepalive_timeout: seconds [DEFAULT: 60]
    - max_concurrent_requests: size_t [DEFAULT: 10]
```

---

# §5 CLASS_DEFINITIONS

## §5.1 CORE_CLASSES

```cpp
CLASS: VectorView
  DESCRIPTION: Non-owning_view_into_vector_data
  CONSTRUCTORS:
    - VectorView()
    - VectorView(const Scalar* data, Dim dim)
    - VectorView(const array<Scalar, N>&)
    - VectorView(const vector<Scalar>&)
  METHODS:
    - data() -> const Scalar*
    - dim() -> Dim
    - size() -> size_t
    - empty() -> bool
    - operator[](size_t) -> Scalar
    - begin() -> const Scalar*
    - end() -> const Scalar*
    - dot(VectorView) -> float
    - cosine_similarity(VectorView) -> float
    - euclidean_distance(VectorView) -> float

CLASS: Vector
  DESCRIPTION: Owning_SIMD-aligned_vector
  ALIGNMENT: VECTOR_ALIGNMENT (SIMD_WIDTH * sizeof(float))
  CONSTRUCTORS:
    - Vector()
    - Vector(Dim dim)
    - Vector(initializer_list<Scalar>)
    - Vector(vector<Scalar>)
  METHODS:
    - data() -> Scalar* | const Scalar*
    - dim() -> Dim
    - size() -> size_t
    - empty() -> bool
    - operator[](size_t) -> Scalar& | Scalar
    - begin() -> Scalar*
    - end() -> Scalar*
    - view() -> VectorView
    - resize(Dim)
    - reserve(Dim)
  CONVERSION:
    - operator VectorView()

CLASS: VectorDatabase
  DESCRIPTION: Main_database_interface
  CONSTRUCTORS:
    - VectorDatabase(const DatabaseConfig&)
  METHODS:
    INITIALIZATION:
      - init() -> Result<void>
      - is_ready() -> bool
      - config() -> const DatabaseConfig&
    TEXT_OPERATIONS:
      - add_text(string_view text, Metadata, IngestOptions) -> Result<VectorId>
      - add_text(string_view text, DocumentType, string_view date, IngestOptions) -> Result<VectorId>
      - query_text(string_view query, QueryOptions) -> Result<QueryResults>
    IMAGE_OPERATIONS:
      - add_image(path, Metadata, IngestOptions) -> Result<VectorId>
      - add_image(path, IngestOptions) -> Result<VectorId>
      - query_image(path, QueryOptions) -> Result<QueryResults>
    VECTOR_OPERATIONS:
      - add_vector(VectorView, Metadata) -> Result<VectorId>
      - query_vector(VectorView, QueryOptions) -> Result<QueryResults>
      - get_vector(VectorId) -> optional<Vector>
    METADATA_OPERATIONS:
      - get_metadata(VectorId) -> optional<Metadata>
      - update_metadata(VectorId, Metadata) -> Result<void>
      - find_by_date(string_view) -> vector<Metadata>
      - find_by_type(DocumentType) -> vector<Metadata>
      - find_by_asset(string_view) -> vector<Metadata>
    DELETION:
      - remove(VectorId) -> Result<void>
      - remove_by_date(string_view) -> Result<size_t>
    BULK_OPERATIONS:
      - add_texts(span<string>, span<Metadata>) -> Result<vector<VectorId>>
      - add_images(span<path>, span<Metadata>) -> Result<vector<VectorId>>
    STATISTICS:
      - size() -> size_t
      - count_by_type(DocumentType) -> size_t
      - all_dates() -> vector<string>
      - stats() -> IndexStats
    MANAGEMENT:
      - optimize()
      - sync() -> Result<void>
      - compact() -> Result<void>
    EXPORT:
      - export_training_data(path) -> Result<void>

CLASS: HnswIndex
  DESCRIPTION: Hierarchical_Navigable_Small_World_graph_index
  CONSTRUCTORS:
    - HnswIndex(const HnswConfig&)
  METHODS:
    CORE:
      - add(VectorId, VectorView) -> Result<void>
      - add_batch(span<VectorId>, span<Vector>) -> Result<void>
      - search(VectorView, size_t k) -> SearchResults
      - search_filtered(VectorView, size_t k, function<bool(VectorId)>) -> SearchResults
      - remove(VectorId) -> Result<void>
      - contains(VectorId) -> bool
      - get_vector(VectorId) -> optional<Vector>
    MANAGEMENT:
      - size() -> size_t
      - capacity() -> size_t
      - empty() -> bool
      - dimension() -> Dim
      - config() -> const HnswConfig&
      - stats() -> IndexStats
      - set_ef_search(size_t)
      - resize(size_t) -> Result<void>
      - optimize()
    PERSISTENCE:
      - save(string_view path) -> Result<void>
      - load(string_view path) -> Result<HnswIndex> [STATIC]
      - serialize() -> vector<uint8_t>
      - deserialize(span<uint8_t>) -> Result<HnswIndex> [STATIC]

CLASS: FlatIndex
  DESCRIPTION: Brute-force_index_for_small_datasets
  CONSTRUCTORS:
    - FlatIndex(Dim dimension, DistanceMetric)
  METHODS:
    - add(VectorId, VectorView) -> Result<void>
    - search(VectorView, size_t k) -> SearchResults
    - contains(VectorId) -> bool
    - get_vector(VectorId) -> optional<Vector>
    - size() -> size_t
    - dimension() -> Dim
    - save(string_view) -> Result<void>
    - load(string_view) -> Result<FlatIndex> [STATIC]
```

## §5.2 STORAGE_CLASSES

```cpp
CLASS: MemoryMappedFile
  DESCRIPTION: Cross-platform_memory_mapped_file_abstraction
  METHODS:
    - open_read(path) -> Result<void>
    - open_write(path, size_t initial_size) -> Result<void>
    - close()
    - is_open() -> bool
    - data() -> uint8_t* | const uint8_t*
    - size() -> size_t
    - resize(size_t) -> Result<void>
    - sync() -> Result<void>

CLASS: VectorStore
  DESCRIPTION: Persistent_vector_storage
  CONSTRUCTORS:
    - VectorStore(const VectorStoreConfig&)
  METHODS:
    - init() -> Result<void>
    - add(VectorId, VectorView) -> Result<void>
    - get(VectorId) -> optional<VectorView>
    - contains(VectorId) -> bool
    - remove(VectorId) -> Result<void>
    - all_ids() -> vector<VectorId>
    - size() -> size_t
    - capacity() -> size_t
    - sync() -> Result<void>
    - compact() -> Result<void>
    - memory_usage() -> size_t

CLASS: MetadataStore
  DESCRIPTION: JSON_Lines_metadata_storage
  CONSTRUCTORS:
    - MetadataStore(const path&)
  METHODS:
    - init() -> Result<void>
    - add(Metadata) -> Result<void>
    - update(Metadata) -> Result<void>
    - get(VectorId) -> optional<Metadata>
    - all() -> vector<Metadata>
    - find_by_date(string_view) -> vector<Metadata>
    - find_by_type(DocumentType) -> vector<Metadata>
    - find_by_asset(string_view) -> vector<Metadata>
    - remove(VectorId) -> Result<void>
    - size() -> size_t
    - sync() -> Result<void>

CLASS: SqliteStore
  DESCRIPTION: SQLite_metadata_and_cache_storage
  CONSTRUCTORS:
    - SqliteStore(const SqliteConfig&)
  METHODS:
    - init() -> Result<void>
    - close()
    METADATA:
      - store_metadata(DocumentMetadata) -> Result<void>
      - get_metadata(string id) -> Result<DocumentMetadata>
      - update_metadata(DocumentMetadata) -> Result<void>
      - delete_metadata(string id) -> Result<void>
      - list_metadata(limit, offset) -> Result<vector<DocumentMetadata>>
    CACHE:
      - cache_put(key, value) -> Result<void>
      - cache_get(key) -> Result<string>
      - cache_delete(key) -> Result<void>
      - cache_clear() -> Result<void>
      - cache_size() -> Result<size_t>
    CONFIG:
      - config_set(key, value) -> Result<void>
      - config_get(key) -> Result<string>
      - config_delete(key) -> Result<void>
      - config_list() -> Result<map<string,string>>
    MAINTENANCE:
      - vacuum() -> Result<void>
      - evict_expired_cache() -> Result<size_t>
      - get_stats() -> Result<map<string,size_t>>

CLASS: PgVectorStore
  DESCRIPTION: PostgreSQL_pgvector_storage_backend
  CONSTRUCTORS:
    - PgVectorStore(const PgVectorConfig&)
  METHODS:
    - init(Dim, DistanceMetric) -> Result<void>
    - close() -> Result<void>
    - add(VectorView, Metadata) -> Result<VectorId>
    - add_batch(vector<pair<VectorView,Metadata>>) -> Result<void>
    - remove(VectorId) -> Result<bool>
    - remove_batch(vector<VectorId>) -> Result<void>
    - get(VectorId) -> Result<optional<Vector>>
    - get_metadata(VectorId) -> Result<optional<Metadata>>
    - update_metadata(VectorId, Metadata) -> Result<void>
    - search(VectorView, k, filter) -> Result<vector<pair<VectorId,Distance>>>
    - count() -> Result<size_t>
    - size_bytes() -> Result<size_t>
    - sync() -> Result<void>
    - vacuum() -> Result<void>
    - create_index() -> Result<void>
    - drop_index() -> Result<void>
    - reindex() -> Result<void>
    - is_connected() -> Result<bool>
    - reconnect() -> Result<void>
```

## §5.3 EMBEDDING_CLASSES

```cpp
CLASS: TextEncoder
  DESCRIPTION: Semantic_text_embedding_using_MiniLM
  MODEL: all-MiniLM-L6-v2
  OUTPUT_DIM: 384
  METHODS:
    - init(TextEncoderConfig) -> Result<void>
    - is_ready() -> bool
    - encode(string_view) -> Result<vector<float>>
    - encode_batch(vector<string>) -> Result<vector<vector<float>>>
    - dimension() -> Dim
    - device() -> Device

CLASS: ImageEncoder
  DESCRIPTION: Image_embedding_using_CLIP
  MODEL: CLIP_ViT-B/32
  OUTPUT_DIM: 512
  INPUT_SIZE: 224x224
  METHODS:
    - init(ImageEncoderConfig) -> Result<void>
    - is_ready() -> bool
    - encode(path) -> Result<vector<float>>
    - encode(ImageData) -> Result<vector<float>>
    - encode_batch(vector<path>) -> Result<vector<vector<float>>>
    - dimension() -> Dim
    - device() -> Device

CLASS: OnnxSession
  DESCRIPTION: ONNX_Runtime_inference_wrapper
  CONSTRUCTORS:
    - OnnxSession(path model_path, Device)
  METHODS:
    - run(vector<Ort::Value>&) -> vector<Ort::Value>
    - device() -> Device
    - input_names() -> vector<string>
    - output_names() -> vector<string>
    - input_shape(size_t idx) -> vector<int64_t>
    - output_shape(size_t idx) -> vector<int64_t>
    - memory_info() -> Ort::MemoryInfo&

CLASS: Tokenizer
  DESCRIPTION: BERT_WordPiece_tokenizer
  CONSTRUCTORS:
    - Tokenizer(path vocab_path)
  METHODS:
    - encode(string_view, max_length, add_special_tokens) -> vector<int64_t>
    - decode(span<int64_t>) -> string
    - vocab_size() -> size_t
    - has_token(string_view) -> bool

CLASS: ImagePreprocessor
  DESCRIPTION: CLIP-compatible_image_preprocessing
  CONSTRUCTORS:
    - ImagePreprocessor(Size target, array<float,3> mean, array<float,3> std)
  METHODS:
    - process(uint8_t* rgb, width, height) -> vector<float>
    - process_file(path) -> vector<float>
    - center_crop_and_process(uint8_t* rgb, width, height) -> vector<float>
    - target_size() -> Size
    - output_size() -> size_t
```

## §5.4 HYBRID_SEARCH_CLASSES

```cpp
CLASS: BM25Engine
  DESCRIPTION: BM25_full-text_search_engine
  CONSTRUCTORS:
    - BM25Engine(const BM25Config&)
  METHODS:
    - add_document(VectorId, string content) -> Result<void>
    - remove_document(VectorId) -> Result<void>
    - update_document(VectorId, string content) -> Result<void>
    - search(string query, k, min_score) -> Result<vector<BM25Result>>
    - document_count() -> size_t
    - term_count() -> size_t
    - average_document_length() -> float
    - save(string path) -> Result<void>
    - load(string path) -> Result<BM25Engine> [STATIC]

CLASS: KeywordExtractor
  DESCRIPTION: TF-IDF_keyword_extraction
  CONSTRUCTORS:
    - KeywordExtractor(const KeywordConfig&)
  METHODS:
    - extract(string text) -> Result<vector<Keyword>>
    - train(vector<string> documents) -> Result<void>
    - save(string path) -> Result<void>
    - load(string path) -> Result<KeywordExtractor> [STATIC]

CLASS: HybridSearchEngine
  DESCRIPTION: Vector_plus_lexical_search_combiner
  CONSTRUCTORS:
    - HybridSearchEngine(const HybridSearchConfig&)
  METHODS:
    - combine(vector<QueryResult>, vector<BM25Result>, k) -> Result<vector<HybridResult>>
  STATIC_METHODS:
    - weighted_sum(vec_score, lex_score, vec_weight) -> float
    - reciprocal_rank_fusion(vec_rank, lex_rank, k) -> float
    - comb_sum(vec_score, lex_score) -> float
    - comb_mnz(vec_score, lex_score, num_systems) -> float

CLASS: QueryRewriter
  DESCRIPTION: Query_expansion_and_correction
  CONSTRUCTORS:
    - QueryRewriter(const RewriteConfig&)
  METHODS:
    - rewrite(string query) -> Result<string>
    - add_synonym(string term, vector<string> synonyms) -> Result<void>
    - load_synonyms(string path) -> Result<void>
```

## §5.5 QUANTIZATION_CLASSES

```cpp
CLASS: ProductQuantizer
  DESCRIPTION: Product_quantization_for_8-32x_compression
  CONSTRUCTORS:
    - ProductQuantizer(const ProductQuantizerConfig&)
  METHODS:
    - train(span<Vector>) -> Result<void>
    - is_trained() -> bool
    - encode(VectorView) -> Result<vector<uint8_t>>
    - encode_batch(span<Vector>) -> Result<vector<vector<uint8_t>>>
    - decode(span<uint8_t>) -> Result<Vector>
    - compute_distance(VectorView query, span<uint8_t> codes) -> Distance
    - precompute_distance_table(VectorView) -> vector<Distance>
    - compute_distance_precomputed(span<uint8_t>, span<Distance>) -> Distance
    - config() -> const ProductQuantizerConfig&
    - dimension() -> Dim
    - code_size() -> size_t
    - compression_ratio() -> float
    - save(string_view) -> Result<void>
    - load(string_view) -> Result<ProductQuantizer> [STATIC]

CLASS: ScalarQuantizer
  DESCRIPTION: Scalar_quantization_to_8-bit
  CONSTRUCTORS:
    - ScalarQuantizer(const ScalarQuantizerConfig&)
  METHODS:
    - train(span<Vector>) -> Result<void>
    - is_trained() -> bool
    - encode(VectorView) -> Result<vector<uint8_t>>
    - decode(span<uint8_t>) -> Result<Vector>
    - compute_distance(VectorView, span<uint8_t>) -> Distance
    - code_size() -> size_t
    - compression_ratio() -> float
    - save(string_view) -> Result<void>
    - load(string_view) -> Result<ScalarQuantizer> [STATIC]

CLASS: DisplayAwareQuantizer
  DESCRIPTION: Display-optimized_quantization
  CONSTRUCTORS:
    - DisplayAwareQuantizer(const DisplayProfile&)
  METHODS:
    - train(span<Vector>) -> Result<void>
    - is_trained() -> bool
    - encode(VectorView) -> Result<vector<uint8_t>>
    - decode(span<uint8_t>) -> Result<Vector>
    - requantize_for_display(span<uint8_t>, DisplayProfile) -> Result<vector<uint8_t>>
    - set_display_profile(DisplayProfile)
    - display_profile() -> const DisplayProfile&
    - code_size() -> size_t
    - compression_ratio() -> float

CLASS: EnvironmentAwareQuantizer
  DESCRIPTION: Viewing_environment_aware_quantization
  CONSTRUCTORS:
    - EnvironmentAwareQuantizer(DisplayProfile, EnvironmentProfile)
  METHODS:
    - train(span<Vector>) -> Result<void>
    - is_trained() -> bool
    - encode(VectorView) -> Result<vector<uint8_t>>
    - decode(span<uint8_t>) -> Result<Vector>
    - adapt_to_environment(EnvironmentProfile)
    - set_display_profile(DisplayProfile)
    - set_environment_profile(EnvironmentProfile)
    - display_profile() -> const DisplayProfile&
    - environment_profile() -> const EnvironmentProfile&
```

## §5.6 PERCEPTUAL_CURVES_CLASSES

```cpp
CLASS: PQCurve [STATIC]
  DESCRIPTION: SMPTE_ST_2084_Perceptual_Quantizer_for_HDR
  CONSTANTS:
    - PEAK_LUMINANCE: 10000.0f nits
    - M1, M2, C1, C2, C3: PQ_curve_constants
  METHODS:
    - encode(float linear_luminance) -> float [STATIC]
    - decode(float pq_value) -> float [STATIC]
    - encode_batch(span<float>) -> vector<float> [STATIC]
    - decode_batch(span<float>) -> vector<float> [STATIC]

CLASS: HLGCurve [STATIC]
  DESCRIPTION: Hybrid_Log-Gamma_for_BBC/NHK_HDR
  CONSTANTS:
    - A: 0.17883277f
    - B: 0.28466892f
    - C: 0.55991073f
  METHODS:
    - encode(float linear) -> float [STATIC]
    - decode(float hlg) -> float [STATIC]
    - encode_batch(span<float>) -> vector<float> [STATIC]
    - decode_batch(span<float>) -> vector<float> [STATIC]

CLASS: GammaCurve
  DESCRIPTION: sRGB/Rec.709/DCI-P3_gamma_curves
  CONSTRUCTORS:
    - GammaCurve(float gamma = 2.2f)
  METHODS:
    - encode(float linear) -> float
    - decode(float gamma) -> float
    - encode_batch(span<float>) -> vector<float>
    - decode_batch(span<float>) -> vector<float>
```

## §5.7 LLM_ENGINE_CLASSES

```cpp
CLASS: LLMEngine [ABSTRACT]
  DESCRIPTION: Local_LLM_inference_via_llama.cpp
  METHODS:
    - load(LLMConfig) -> bool [VIRTUAL]
    - is_loaded() -> bool [VIRTUAL]
    - unload() [VIRTUAL]
    - model_name() -> string [VIRTUAL]
    - context_size() -> int [VIRTUAL]
    - vocab_size() -> int [VIRTUAL]
    - generate(string_view prompt, GenerationParams) -> string [VIRTUAL]
    - chat(vector<Message>, GenerationParams) -> ChatCompletionResult [VIRTUAL]
    - count_tokens(string_view) -> int [VIRTUAL]
    - embed(string_view) -> optional<vector<float>> [VIRTUAL]

FACTORY_FUNCTION: create_llm_engine() -> unique_ptr<LLMEngine>

UTILITY_FUNCTIONS:
  - find_gguf_models(path directory) -> vector<path>
  - read_gguf_metadata(path model_path) -> optional<GGUFMetadata>
  - apply_chat_template(vector<Message>, string_view template) -> string
```

## §5.8 FRAMEWORK_INTEGRATION_CLASSES

```cpp
CLASS: TensorFlowEmbedder
  DESCRIPTION: TensorFlow_SavedModel_integration
  CONSTRUCTORS:
    - TensorFlowEmbedder(const TensorFlowConfig&)
  METHODS:
    - embed(string text) -> Result<Vector>
    - embed_batch(vector<string>) -> Result<vector<Vector>>
    - dimension() -> Dim
    - is_loaded() -> bool
  STATIC_METHODS:
    - export_for_training(vectors, labels, output_path) -> Result<void>

CLASS: PyTorchEmbedder
  DESCRIPTION: PyTorch_TorchScript_integration
  CONSTRUCTORS:
    - PyTorchEmbedder(const PyTorchConfig&)
  METHODS:
    - embed(string text) -> Result<Vector>
    - embed_batch(vector<string>) -> Result<vector<Vector>>
    - dimension() -> Dim
    - is_loaded() -> bool
    - device() -> string
  STATIC_METHODS:
    - export_for_training(vectors, labels, output_path) -> Result<void>
    - from_trained(model_path, device) -> Result<PyTorchEmbedder>

CLASS: RAGEngine
  DESCRIPTION: Retrieval_Augmented_Generation_toolkit
  CONSTRUCTORS:
    - RAGEngine(const RAGConfig&)
  METHODS:
    - chunk_document(string document) -> Result<vector<string>>
    - build_context(string query, vector<QueryResult>) -> Result<RAGContext>
    - format_prompt(string query, RAGContext, string system_prompt) -> string
    - rerank(string query, vector<QueryResult>) -> Result<vector<QueryResult>>

CLASS: LangChainAdapter
  DESCRIPTION: LangChain_vectorstore_compatibility
  CONSTRUCTORS:
    - LangChainAdapter(const LangChainConfig&)
  METHODS:
    - similarity_search(string query, k) -> Result<vector<QueryResult>>
    - similarity_search_with_score(string query, k) -> Result<vector<QueryResult>>
    - max_marginal_relevance_search(query, k, fetch_k, lambda) -> Result<vector<QueryResult>>
    - as_retriever(string query, k) -> Result<vector<string>>

CLASS: LlamaIndexAdapter
  DESCRIPTION: LlamaIndex_index_compatibility
  CONSTRUCTORS:
    - LlamaIndexAdapter(const LlamaIndexConfig&)
  METHODS:
    - build_index(vector<string>, vector<Metadata>) -> Result<void>
    - query(string query, k) -> Result<RAGContext>
    - insert(string document, Metadata) -> Result<void>
    - delete_document(VectorId) -> Result<void>
    - save(string path) -> Result<void>
    - load(string path) -> Result<LlamaIndexAdapter> [STATIC]

CLASS: DocumentChunker
  DESCRIPTION: Text_chunking_strategies
  CONSTRUCTORS:
    - DocumentChunker(const ChunkingConfig&)
  METHODS:
    - chunk(string document) -> Result<vector<string>>
    - chunk_with_metadata(string document, Metadata) -> Result<vector<ChunkWithMetadata>>
```

## §5.9 DATA_ADAPTER_CLASSES

```cpp
INTERFACE: IDataAdapter
  METHODS:
    - can_handle(path) -> bool [VIRTUAL]
    - can_handle(string_view content) -> bool [VIRTUAL]
    - parse(path, ChunkConfig) -> Result<NormalizedData> [VIRTUAL]
    - parse_content(string_view, ChunkConfig, source_hint) -> Result<NormalizedData> [VIRTUAL]
    - write(NormalizedData, path) -> Result<void> [VIRTUAL]
    - sanitize(NormalizedData&) -> Result<void> [VIRTUAL]
    - name() -> string [VIRTUAL]
    - supported_formats() -> vector<DataFormat> [VIRTUAL]
    - supports_write() -> bool [VIRTUAL]

CLASS: CSVAdapter : IDataAdapter
  FORMATS: [CSV]
  FEATURES: delimiter_config, quote_handling, header_detection

CLASS: JSONAdapter : IDataAdapter
  FORMATS: [JSON, API_JSON]
  FEATURES: object/array/lines/nested_detection, flattening

CLASS: XMLAdapter : IDataAdapter
  FORMATS: [XML]
  FEATURES: hierarchy_preservation, namespace_handling, attribute_extraction

CLASS: PDFAdapter : IDataAdapter
  FORMATS: [PDF]
  FEATURES: table_extraction, layout_preservation, TOC_extraction
  DEPENDENCY: Poppler

CLASS: ExcelAdapter : IDataAdapter
  FORMATS: [Excel]
  FEATURES: multi-sheet, formula_evaluation, chart_metadata
  DEPENDENCY: xlsxwriter

CLASS: ParquetAdapter : IDataAdapter
  FORMATS: [Parquet]
  FEATURES: row/column_chunking, schema_extraction, batch_reading

CLASS: TextAdapter : IDataAdapter
  FORMATS: [PlainText, TXT, Markdown]
  FEATURES: encoding_detection, markdown_detection, code_detection
  DEPENDENCY: ICU (optional)

CLASS: SQLiteAdapter : IDataAdapter
  FORMATS: [SQL]
  FEATURES: multi-table, custom_queries, schema_extraction

CLASS: PgvectorAdapter : IDataAdapter
  FORMATS: [pgvector]
  FEATURES: bidirectional_sync, vector_column, similarity_queries

CLASS: HTTPAdapter : IDataAdapter
  FORMATS: [HTTP_JSON, HTTP_XML, HTTP_CSV]
  FEATURES: content-type_detection, auth_headers, caching

CLASS: DataAdapterManager
  DESCRIPTION: Universal_adapter_registry_and_auto-detection
  METHODS:
    - register_adapter(unique_ptr<IDataAdapter>)
    - auto_parse(path, ChunkConfig) -> Result<NormalizedData>
    - auto_parse_content(string_view, ChunkConfig, hint) -> Result<NormalizedData>
    - detect_format(path) -> DataFormat
    - detect_format_content(string_view) -> DataFormat
    - supported_formats() -> vector<DataFormat>
    - parse_batch(vector<path>, ChunkConfig, max_parallel) -> Result<vector<NormalizedData>>
```

## §5.10 HTTP_CLIENT_CLASS

```cpp
CLASS: HttpClient
  DESCRIPTION: Production_HTTP_client_with_retry_and_caching
  CONSTRUCTORS:
    - HttpClient(HttpClientConfig)
  METHODS:
    REQUESTS:
      - get(url, headers) -> Result<HttpResponse>
      - post(url, body, headers) -> Result<HttpResponse>
      - put(url, body, headers) -> Result<HttpResponse>
      - del(url, headers) -> Result<HttpResponse>
      - request(HttpRequest) -> Result<HttpResponse>
      - download(url, output_path, ProgressCallback) -> Result<void>
    CONFIGURATION:
      - set_default_header(key, value)
      - remove_default_header(key)
      - set_rate_limit(endpoint, RateLimitConfig)
      - clear_cache() -> Result<void>
      - clear_cache_for_url(url) -> Result<void>
    STATISTICS:
      - get_statistics() -> Statistics
      - reset_statistics()

CLASS: UrlBuilder
  METHODS:
    - set_scheme(string) -> UrlBuilder&
    - set_host(string) -> UrlBuilder&
    - set_port(int) -> UrlBuilder&
    - set_path(string) -> UrlBuilder&
    - add_query(key, value) -> UrlBuilder&
    - build() -> string
```

## §5.11 INGEST_CLASSES

```cpp
CLASS: GoldStandardIngest
  DESCRIPTION: Gold_Standard_format_ingestion_pipeline
  CONSTRUCTORS:
    - GoldStandardIngest(VectorDatabase&)
  METHODS:
    - ingest(IngestConfig) -> Result<IngestStats>
    - ingest_journal(path) -> Result<VectorId>
    - ingest_chart(path) -> Result<VectorId>
    - ingest_report(path) -> Result<VectorId>
    - watch(IngestConfig, function<bool()> should_stop) -> Result<void>
    - last_stats() -> const IngestStats&
```

## §5.12 REPLICATION_CLASSES

```cpp
CLASS: ReplicationManager
  DESCRIPTION: Multi-node_replication_management
  CONSTRUCTORS:
    - ReplicationManager(const ReplicationConfig&)
  METHODS:
    LIFECYCLE:
      - start() -> Result<void>
      - stop() -> Result<void>
    REPLICATION:
      - replicate_add(VectorId, VectorView, Metadata) -> Result<void>
      - replicate_remove(VectorId) -> Result<void>
      - replicate_update(VectorId, Metadata) -> Result<void>
    NODE_MANAGEMENT:
      - add_replica(NodeConfig) -> Result<void>
      - remove_replica(string node_id) -> Result<void>
      - get_replicas() -> Result<vector<NodeConfig>>
    HEALTH:
      - is_healthy() -> Result<bool>
      - get_primary_node() -> Result<string>
      - promote_to_primary() -> Result<void>
      - demote_from_primary() -> Result<void>
    FAILOVER:
      - trigger_failover() -> Result<void>
      - set_failover_callback(function<void(string)>)

CLASS: ShardingManager
  DESCRIPTION: Horizontal_partitioning_management
  CONSTRUCTORS:
    - ShardingManager(const ShardingConfig&)
  METHODS:
    LIFECYCLE:
      - start() -> Result<void>
      - stop() -> Result<void>
    ROUTING:
      - get_shard_for_id(VectorId) -> Result<string>
      - get_shard_for_key(string key) -> Result<string>
      - get_all_shards() -> Result<vector<string>>
    MANAGEMENT:
      - add_shard(ShardConfig) -> Result<void>
      - remove_shard(string shard_id) -> Result<void>
      - rebalance_shards() -> Result<void>
    STATISTICS:
      - get_shard_size(string shard_id) -> Result<size_t>
      - get_shard_imbalance() -> Result<float>
      - needs_resharding() -> Result<bool>
    RESHARDING:
      - trigger_resharding() -> Result<void>
      - set_resharding_callback(function<void(size_t,size_t)>)

CLASS: DistributedVectorDatabase
  DESCRIPTION: Distributed_database_with_replication_and_sharding
  CONSTRUCTORS:
    - DistributedVectorDatabase(ReplicationConfig, ShardingConfig)
  METHODS:
    INITIALIZATION:
      - init(Dim, DistanceMetric) -> Result<void>
      - close() -> Result<void>
    OPERATIONS:
      - add(VectorView, Metadata) -> Result<VectorId>
      - remove(VectorId) -> Result<bool>
      - get(VectorId) -> Result<optional<Vector>>
      - update_metadata(VectorId, Metadata) -> Result<void>
      - search(VectorView, k, filter) -> Result<vector<QueryResult>>
    CLUSTER:
      - add_node(NodeConfig) -> Result<void>
      - remove_node(string node_id) -> Result<void>
      - get_all_nodes() -> Result<vector<NodeConfig>>
      - is_cluster_healthy() -> Result<bool>
```

## §5.13 TELEMETRY_CLASSES

```cpp
CLASS: TelemetrySpan
  DESCRIPTION: RAII_distributed_tracing_span
  CONSTRUCTORS:
    - TelemetrySpan(string operation_name, optional<TraceContext> parent)
  METHODS:
    - set_attribute(key, string|int64_t|double|bool value)
    - set_attributes(SpanAttributes)
    - add_event(name, attributes)
    - record_error(string message)
    - set_status(bool success, string description)
    - get_context() -> TraceContext

CLASS: TelemetryMetrics [SINGLETON]
  METHODS:
    - instance() -> TelemetryMetrics&
    - increment_counter(name, value, labels)
    - record_histogram(name, value, labels)
    - set_gauge(name, value, labels)
    - record_search_latency(latency_ms, result_count)
    - record_insert_operation(vector_count, latency_ms)
    - record_index_build_time(duration_seconds)
    - record_memory_usage(bytes)

CLASS: TelemetryManager [SINGLETON]
  METHODS:
    - instance() -> TelemetryManager&
    - initialize(TelemetryConfig)
    - shutdown()
    - is_enabled() -> bool
```

## §5.14 LOGGING_CLASSES

```cpp
CLASS: Logger [SINGLETON]
  DESCRIPTION: Thread-safe_logging_with_anomaly_detection
  METHODS:
    - instance() -> Logger&
    - initialize(LoggerConfig)
    - log(LogLevel, string_view message, string_view file, int line)
    - log_anomaly(AnomalyType, string_view message, string_view file, int line)
    - debug(string_view, file, line)
    - info(string_view, file, line)
    - warn(string_view, file, line)
    - error(string_view, file, line)
    - critical(string_view, file, line)
```

## §5.15 CLI_CLASSES

```cpp
CLASS: CLI
  DESCRIPTION: Main_CLI_entry_point
  CONSTRUCTORS:
    - CLI(int argc, char** argv)
  METHODS:
    - run() -> int
    - get_commands() -> const map<string, shared_ptr<CommandBase>>&

INTERFACE: CommandBase
  METHODS:
    - name() -> string [VIRTUAL]
    - description() -> string [VIRTUAL]
    - aliases() -> vector<string> [VIRTUAL]
    - usage() -> string [VIRTUAL]
    - help() -> string [VIRTUAL]
    - execute(args, options) -> int [VIRTUAL]

CLASS: InteractiveShell
  DESCRIPTION: REPL_interactive_shell
  CONSTRUCTORS:
    - InteractiveShell(CLI*, string db_path)
  METHODS:
    - run() -> int
    - set_database(string path)

CLASS: OutputFormatter
  DESCRIPTION: Multi-format_output_rendering
  FORMATS: [Table, JSON, CSV]
  METHODS:
    - format_table(headers, rows) -> string
    - format_keyvalue(data) -> string
    - format_success(message) -> string
    - format_error(message) -> string
    - set_format(Format)
```

---

# §6 FUNCTION_MAP

## §6.1 DISTANCE_FUNCTIONS

```cpp
FUNCTION: dot_product(const Scalar* a, const Scalar* b, Dim n) -> float
  SIMD: AVX2/AVX512_optimized
  DESCRIPTION: Compute_dot_product_of_two_vectors

FUNCTION: euclidean_distance(const Scalar* a, const Scalar* b, Dim n) -> float
  SIMD: AVX2/AVX512_optimized
  DESCRIPTION: Compute_L2_distance

FUNCTION: squared_euclidean(const Scalar* a, const Scalar* b, Dim n) -> float
  SIMD: AVX2/AVX512_optimized
  DESCRIPTION: Compute_squared_L2_without_sqrt

FUNCTION: cosine_similarity(const Scalar* a, const Scalar* b, Dim n) -> float
  SIMD: AVX2/AVX512_optimized
  DESCRIPTION: Compute_cosine_similarity

FUNCTION: cosine_distance(const Scalar* a, const Scalar* b, Dim n) -> float
  DESCRIPTION: Compute_1_minus_cosine_similarity

FUNCTION: compute_distance(VectorView a, VectorView b, DistanceMetric) -> Distance
  DESCRIPTION: Generic_distance_computation_by_metric

FUNCTION: l2_distance(VectorView a, VectorView b) -> Distance
FUNCTION: l2_squared_distance(VectorView a, VectorView b) -> Distance
FUNCTION: cosine_distance(VectorView a, VectorView b) -> Distance
FUNCTION: dot_product(VectorView a, VectorView b) -> Distance
FUNCTION: cosine_similarity(VectorView a, VectorView b) -> Distance
```

## §6.2 VECTOR_OPERATIONS

```cpp
FUNCTION: normalize(Vector& v) -> void
  DESCRIPTION: In-place_L2_normalization

FUNCTION: normalized(VectorView v) -> Vector
  DESCRIPTION: Return_L2_normalized_copy

FUNCTION: l2_norm(VectorView v) -> Distance
  DESCRIPTION: Compute_vector_magnitude

FUNCTION: add(VectorView a, VectorView b) -> Vector
  DESCRIPTION: Element-wise_addition

FUNCTION: subtract(VectorView a, VectorView b) -> Vector
  DESCRIPTION: Element-wise_subtraction

FUNCTION: scale(VectorView v, Scalar s) -> Vector
  DESCRIPTION: Scalar_multiplication

FUNCTION: add_scaled(VectorView a, VectorView b, Scalar scale) -> Vector
  DESCRIPTION: a_plus_b_times_scale

FUNCTION: multiply(VectorView a, VectorView b) -> Vector
  DESCRIPTION: Element-wise_multiplication

FUNCTION: mean(span<const Vector> vectors) -> Vector
  DESCRIPTION: Compute_centroid_of_vectors

FUNCTION: batch_distance(VectorView query, span<const Vector> targets, DistanceMetric) -> vector<Distance>
  DESCRIPTION: Compute_distances_to_all_targets

FUNCTION: brute_force_knn(VectorView query, span<const Vector> vectors, size_t k, DistanceMetric) -> SearchResults
  DESCRIPTION: Exact_k-nearest_neighbor_search
```

## §6.3 INGEST_FUNCTIONS

```cpp
FUNCTION: parse_markdown(string_view content, string_view filename) -> Result<ParsedDocument>
  DESCRIPTION: Parse_Gold_Standard_markdown_document

FUNCTION: detect_document_type(string_view filename, string_view content) -> DocumentType
  DESCRIPTION: Auto-detect_document_type

FUNCTION: extract_date_from_filename(string_view filename) -> optional<string>
  DESCRIPTION: Extract_YYYY-MM-DD_from_filename

FUNCTION: extract_market_data(string_view text) -> MarketData
  DESCRIPTION: Extract_gold/silver/dxy/vix/yield_from_text

FUNCTION: chunk_document(ParsedDocument, ChunkConfig) -> vector<string>
  DESCRIPTION: Split_document_into_chunks

FUNCTION: chunk_text(string_view text, ChunkConfig) -> vector<string>
  DESCRIPTION: Split_text_into_chunks

FUNCTION: content_hash(string_view content) -> string
  DESCRIPTION: Compute_SHA-256_hash

FUNCTION: file_hash(path) -> Result<string>
  DESCRIPTION: Compute_SHA-256_of_file
```

## §6.4 IMAGE_FUNCTIONS

```cpp
FUNCTION: load_image(path) -> Result<ImageData>
  DESCRIPTION: Load_PNG/JPEG_via_stb_image

FUNCTION: load_image_memory(span<uint8_t>) -> Result<ImageData>
  DESCRIPTION: Load_image_from_memory_buffer

FUNCTION: save_image(path, ImageData) -> Result<void>
  DESCRIPTION: Save_as_PNG

FUNCTION: parse_chart_path(path) -> ChartInfo
  DESCRIPTION: Extract_asset/date/timeframe_from_chart_filename

FUNCTION: preprocess_chart(ImageData) -> ImageData
  DESCRIPTION: Optimize_chart_image_for_embedding
```

## §6.5 UTILITY_FUNCTIONS

```cpp
FUNCTION: now_timestamp() -> Timestamp
  DESCRIPTION: Current_time_in_milliseconds

FUNCTION: timestamp_to_iso(Timestamp) -> string
  DESCRIPTION: Convert_to_ISO_8601_string

FUNCTION: document_type_name(DocumentType) -> string_view
  DESCRIPTION: Get_document_type_string_name

FUNCTION: detect_best_device() -> Device
  DESCRIPTION: Detect_best_available_compute_device

FUNCTION: device_name(Device) -> string
  DESCRIPTION: Get_device_display_name

FUNCTION: format_to_string(DataFormat) -> string
  DESCRIPTION: Convert_DataFormat_to_string
```

## §6.6 FACTORY_FUNCTIONS

```cpp
FUNCTION: create_gold_standard_db(path) -> Result<VectorDatabase>
  DESCRIPTION: Create_database_with_Gold_Standard_presets

FUNCTION: open_database(path) -> Result<VectorDatabase>
  DESCRIPTION: Open_existing_database

FUNCTION: create_llm_engine() -> unique_ptr<LLMEngine>
  DESCRIPTION: Create_llama.cpp_LLM_engine
```

---

# §7 CLI_COMMANDS

## §7.1 DATABASE_COMMANDS

```
COMMAND: hektor init <path> [OPTIONS]
  DESCRIPTION: Initialize_new_hektor_database
  OPTIONS:
    --dimension DIM      Vector_dimension (default: 512)
    --metric METRIC      cosine|euclidean|dot (default: cosine)
    --preset PRESET      gold-standard|default
  ALIASES: []

COMMAND: hektor db:info <path>
  DESCRIPTION: Show_database_information
  ALIASES: [info]

COMMAND: hektor db:optimize <path>
  DESCRIPTION: Optimize_database_performance
  ALIASES: [optimize]

COMMAND: hektor db:backup <path> <dest> [--compress]
  DESCRIPTION: Backup_database_to_archive
  ALIASES: [backup]

COMMAND: hektor db:restore <src> <path>
  DESCRIPTION: Restore_database_from_backup
  ALIASES: [restore]

COMMAND: hektor db:health <path>
  DESCRIPTION: Check_database_health
  ALIASES: [health]

COMMAND: hektor db:list [--path DIR]
  DESCRIPTION: List_all_databases
  ALIASES: [db:ls]
```

## §7.2 COLLECTION_COMMANDS

```
COMMAND: hektor collection:create <db> <name> [--description TEXT]
  DESCRIPTION: Create_new_collection
  ALIASES: [col create]

COMMAND: hektor collection:list <db>
  DESCRIPTION: List_all_collections
  ALIASES: [collection:ls, col ls]

COMMAND: hektor collection:delete <db> <name> [--force]
  DESCRIPTION: Delete_collection
  ALIASES: [col delete]

COMMAND: hektor collection:info <db> <name>
  DESCRIPTION: Show_collection_information
  ALIASES: []
```

## §7.3 DATA_COMMANDS

```
COMMAND: hektor add <db> [OPTIONS]
  DESCRIPTION: Add_document_to_database
  OPTIONS:
    --text TEXT          Document_text_content
    --file FILE          Read_from_file
    --type TYPE          Document_type
    --date DATE          Date_YYYY-MM-DD
    --asset ASSET        Asset_name
  ALIASES: []

COMMAND: hektor get <db> <id>
  DESCRIPTION: Get_document_by_ID
  ALIASES: []

COMMAND: hektor delete <db> <id> [--force]
  DESCRIPTION: Delete_document
  ALIASES: [rm]

COMMAND: hektor update <db> <id> [OPTIONS]
  DESCRIPTION: Update_existing_document
  OPTIONS:
    --text TEXT          New_text_content
    --metadata JSON      Update_metadata
    --tags TAG1,TAG2     Update_tags
  ALIASES: []

COMMAND: hektor batch <db> <file> [OPTIONS]
  DESCRIPTION: Batch_insert_from_file
  OPTIONS:
    --format FORMAT      json|jsonl|csv (default: jsonl)
    --workers N          Parallel_workers (default: 4)
    --chunk-size N       Batch_size (default: 100)
    --skip-errors        Continue_on_errors
  ALIASES: []

COMMAND: hektor list <db> [OPTIONS]
  DESCRIPTION: List_documents
  OPTIONS:
    --limit N            Max_documents (default: 100)
    --offset N           Skip_N_documents
    --type TYPE          Filter_by_type
  ALIASES: [ls]
```

## §7.4 SEARCH_COMMANDS

```
COMMAND: hektor search <db> <query> [OPTIONS]
  DESCRIPTION: Semantic_search
  OPTIONS:
    -k NUM               Number_of_results (default: 10)
    --type TYPE          Filter_by_document_type
    --date-from DATE     Filter_by_date_start
    --date-to DATE       Filter_by_date_end
    --asset ASSET        Filter_by_asset
  ALIASES: [s]

COMMAND: hektor hybrid:search <db> <query> [OPTIONS]
  DESCRIPTION: Hybrid_vector_plus_BM25_search
  OPTIONS:
    -k NUM               Number_of_results (default: 10)
    --fusion METHOD      rrf|weighted|combsum|combmnz|borda (default: rrf)
    --vector-weight W    Vector_weight_0.0-1.0 (default: 0.7)
    --lexical-weight W   Lexical_weight_0.0-1.0 (default: 0.3)
    --rrf-k K            RRF_k_parameter (default: 60)
  ALIASES: [hs]

COMMAND: hektor hybrid:bm25 <db> <query> [-k NUM]
  DESCRIPTION: BM25_full-text_search_only
  ALIASES: []
```

## §7.5 INGEST_COMMANDS

```
COMMAND: hektor ingest <db> <source> [OPTIONS]
  DESCRIPTION: Import_data_from_various_sources
  OPTIONS:
    --format FORMAT      csv|json|pdf|excel|xml|parquet|sqlite|postgres|http
    --chunk-strategy S   fixed|sentence|paragraph|semantic|recursive
    --chunk-size N       Chunk_size (default: 512)
    --overlap N          Chunk_overlap (default: 50)
    --recursive          Process_directories_recursively
    --pattern PATTERN    File_pattern (e.g., "*.pdf")
    --workers N          Parallel_workers (default: 4)
  ALIASES: []

COMMAND: hektor ingest:scan <source> [OPTIONS]
  DESCRIPTION: Preview_what_would_be_ingested
  OPTIONS:
    --format FORMAT      Source_format
    --recursive          Scan_recursively
  ALIASES: []
```

## §7.6 INDEX_COMMANDS

```
COMMAND: hektor index:build <db> [OPTIONS]
  DESCRIPTION: Build_or_rebuild_index
  OPTIONS:
    --type TYPE          hnsw|flat (default: hnsw)
    --hnsw-m M           HNSW_M_parameter (default: 16)
    --hnsw-ef EF         HNSW_ef_construction (default: 200)
    --force              Rebuild_if_exists
  ALIASES: []

COMMAND: hektor index:optimize <db>
  DESCRIPTION: Optimize_index_performance
  ALIASES: []

COMMAND: hektor index:stats <db>
  DESCRIPTION: Show_index_statistics
  ALIASES: []

COMMAND: hektor index:benchmark <db> [OPTIONS]
  DESCRIPTION: Benchmark_index_search_performance
  OPTIONS:
    --queries N          Test_queries (default: 100)
    --k K                Top-k_results (default: 10)
  ALIASES: []
```

## §7.7 EXPORT_COMMANDS

```
COMMAND: hektor export:data <db> <output> [OPTIONS]
  DESCRIPTION: Export_database_data
  OPTIONS:
    --format FORMAT      json|jsonl|csv|parquet (default: jsonl)
    --include-vectors    Include_vector_embeddings
  ALIASES: []

COMMAND: hektor export:pairs <db> <output> [OPTIONS]
  DESCRIPTION: Export_training_pairs
  OPTIONS:
    --min-score SCORE    Minimum_similarity (default: 0.7)
    --limit N            Maximum_pairs
  ALIASES: []

COMMAND: hektor export:triplets <db> <output> [OPTIONS]
  DESCRIPTION: Export_training_triplets
  OPTIONS:
    --negative-samples N  Negatives_per_anchor (default: 5)
    --strategy STRATEGY   hard|random|semi-hard (default: hard)
  ALIASES: []
```

---

# §8 PYTHON_API

## §8.1 MODULE: pyvdb

```python
MODULE: pyvdb
  VERSION: {VERSION_MAJOR}.{VERSION_MINOR}.{VERSION_PATCH}

ENUMS:
  - DistanceMetric: [Cosine, L2, DotProduct]
  - DocumentType: [Journal, Chart, CatalystWatchlist, InstitutionalMatrix,
                   EconomicCalendar, WeeklyRundown, ThreeMonthReport,
                   OneYearReport, MonthlyReport, PreMarket, Unknown]
  - Device: [CPU, CUDA, DirectML]  # if VDB_USE_ONNX_RUNTIME
  - Role: [System, User, Assistant]  # if VDB_USE_LLAMA_CPP

CLASSES:
  - Metadata
  - QueryOptions
  - QueryResult
  - DatabaseConfig
  - IndexStats
  - VectorDatabase
  - IngestStats
  - IngestConfig
  - GoldStandardIngest
  - LLMConfig  # if VDB_USE_LLAMA_CPP
  - GenerationParams  # if VDB_USE_LLAMA_CPP
  - ChatCompletionResult  # if VDB_USE_LLAMA_CPP
  - Message  # if VDB_USE_LLAMA_CPP
  - GGUFMetadata  # if VDB_USE_LLAMA_CPP
  - LLMEngine  # if VDB_USE_LLAMA_CPP

FUNCTIONS:
  - create_gold_standard_db(path) -> VectorDatabase
  - open_database(path) -> VectorDatabase
  - detect_best_device() -> Device  # if VDB_USE_ONNX_RUNTIME
```

## §8.2 CLASS: pyvdb.VectorDatabase

```python
CLASS: pyvdb.VectorDatabase
  CONSTRUCTOR: VectorDatabase(config: DatabaseConfig)
  
  METHODS:
    # Initialization
    init() -> None
    is_ready() -> bool
    config() -> DatabaseConfig
    
    # Text operations
    add_text(text: str, metadata: Metadata) -> int
    add_text(text: str, type: DocumentType, date: str) -> int
    query_text(query: str, options: QueryOptions = QueryOptions()) -> List[QueryResult]
    search(query: str, k: int = 10) -> List[QueryResult]
    
    # Image operations
    add_image(path: str, metadata: Metadata) -> int
    add_image(path: str) -> int
    query_image(path: str, options: QueryOptions = QueryOptions()) -> List[QueryResult]
    
    # Vector operations (numpy support)
    add_vector(vector: np.ndarray, metadata: Metadata) -> int
    query_vector(vector: np.ndarray, options: QueryOptions = QueryOptions()) -> List[QueryResult]
    get_vector(id: int) -> Optional[np.ndarray]
    
    # Metadata operations
    get_metadata(id: int) -> Optional[Metadata]
    find_by_date(date: str) -> List[Metadata]
    find_by_type(type: DocumentType) -> List[Metadata]
    find_by_asset(asset: str) -> List[Metadata]
    
    # Deletion
    remove(id: int) -> None
    remove_by_date(date: str) -> int
    
    # Statistics
    __len__() -> int
    size() -> int
    count_by_type(type: DocumentType) -> int
    all_dates() -> List[str]
    stats() -> IndexStats
    
    # Management
    optimize() -> None
    sync() -> None
    compact() -> None
    
    # Export
    export_training_data(output_path: str) -> None
```

---

# §9 REST_API

## §9.1 ENDPOINTS

```yaml
REST_API:
  BASE_URL: http://{host}:{port}
  VERSION: 2.0.0
  
  SYSTEM:
    GET /health:
      RESPONSE: {status, version, database, uptime_seconds}
      AUTH: None
      
    GET /metrics:
      RESPONSE: Prometheus_metrics
      AUTH: None
      
    GET /stats:
      RESPONSE: {total_vectors, memory_usage_bytes, index_size, collections}
      AUTH: Bearer_token
  
  AUTHENTICATION:
    POST /auth/login:
      REQUEST: {username, password}
      RESPONSE: {access_token, token_type}
      RATE_LIMIT: 5/minute
  
  COLLECTIONS:
    POST /collections:
      REQUEST: {name, dimension, metric}
      RESPONSE: CollectionInfo
      AUTH: Bearer_token
      
    GET /collections:
      RESPONSE: List[CollectionInfo]
      AUTH: Bearer_token
      
    DELETE /collections/{name}:
      RESPONSE: {message}
      AUTH: Bearer_token
  
  DOCUMENTS:
    POST /collections/{name}/documents:
      REQUEST: {content, metadata, document_type}
      RESPONSE: {id, message}
      AUTH: Bearer_token
      
    POST /collections/{name}/documents/batch:
      REQUEST: {documents: List[Document]}
      RESPONSE: {ids, count, message}
      AUTH: Bearer_token
      RATE_LIMIT: 10/minute
  
  SEARCH:
    POST /collections/{name}/search:
      REQUEST: {query, k, filters}
      RESPONSE: List[SearchResult]
      AUTH: Bearer_token

METRICS_EXPOSED:
  - vdb_api_requests_total{method, endpoint, status}
  - vdb_api_request_duration_seconds{method, endpoint}
  - vdb_api_active_connections
  - vdb_operations_total{operation, collection}
  - vdb_operation_duration_seconds{operation}
  - vdb_vectors_total{collection}
```

---

# §10 BUILD_SYSTEM

## §10.1 CMAKE_OPTIONS

```cmake
OPTIONS:
  VDB_BUILD_TESTS: ON|OFF [DEFAULT: ON]
  VDB_BUILD_BENCHMARKS: ON|OFF [DEFAULT: OFF]
  VDB_BUILD_PYTHON: ON|OFF [DEFAULT: ON]
  VDB_USE_AVX2: ON|OFF [DEFAULT: ON]
  VDB_USE_AVX512: ON|OFF [DEFAULT: OFF]
  VDB_ENABLE_GPU: ON|OFF [DEFAULT: OFF]
  VDB_USE_LLAMA_CPP: ON|OFF [DEFAULT: ON]
  VDB_USE_ONNX_RUNTIME: ON|OFF [DEFAULT: OFF]
  VDB_USE_TENSORFLOW: ON|OFF [DEFAULT: OFF]
  VDB_USE_PYTORCH: ON|OFF [DEFAULT: OFF]
  VDB_USE_GRPC: ON|OFF [DEFAULT: OFF]
  VDB_BUILD_DISTRIBUTED: ON|OFF [DEFAULT: OFF]
  VDB_USE_PROMETHEUS: ON|OFF [DEFAULT: OFF]
  VDB_USE_OPENTELEMETRY: ON|OFF [DEFAULT: OFF]
```

## §10.2 TARGETS

```cmake
TARGETS:
  vdb_core:
    TYPE: STATIC_LIBRARY
    SOURCES: src/core/*.cpp, src/index/*.cpp, src/storage/*.cpp, etc.
    
  hektor:
    TYPE: EXECUTABLE
    SOURCES: src/cli/*.cpp
    LINKS: vdb_core
    
  pyvdb:
    TYPE: MODULE_LIBRARY (pybind11)
    SOURCES: bindings/python/pyvdb.cpp
    LINKS: vdb_core
    
  vdb_tests:
    TYPE: EXECUTABLE
    SOURCES: tests/*.cpp
    LINKS: vdb_core, GTest
```

## §10.3 DEPENDENCIES

```
REQUIRED_DEPENDENCIES:
  - nlohmann_json: v3.11.3 (FetchContent)
  - fmt: v10.2.1 (FetchContent)
  
OPTIONAL_DEPENDENCIES:
  - CURL: HTTP_client
  - Poppler: PDF_parsing
  - xlsxwriter: Excel_support
  - ICU: Unicode_support
  - SQLite3: Database_storage
  - libpq: PostgreSQL_support
  - ONNX_Runtime: Model_inference
  - llama.cpp: LLM_inference
  - pybind11: Python_bindings
  - GTest: Unit_testing
  - OpenTelemetry: Tracing
  - Prometheus: Metrics
```

---

# §11 FILE_MAP

```
c:\workspace\hektor\
├── include/vdb/
│   ├── core.hpp              # Core_types_and_constants
│   ├── database.hpp          # VectorDatabase_class
│   ├── distance.hpp          # Distance_functions
│   ├── index.hpp             # HnswIndex_FlatIndex
│   ├── storage.hpp           # VectorStore_MetadataStore
│   ├── ingest.hpp            # Ingestion_pipeline
│   ├── batch.hpp             # Batch_operations
│   ├── hybrid_search.hpp     # BM25_HybridSearch
│   ├── logging.hpp           # Logger_AnomalyDetection
│   ├── telemetry.hpp         # OpenTelemetry_integration
│   ├── replication.hpp       # Distributed_replication
│   ├── framework_integration.hpp  # TensorFlow_PyTorch_RAG
│   ├── version.hpp           # Version_info
│   ├── adapters/
│   │   ├── data_adapter.hpp  # IDataAdapter_interface
│   │   ├── csv_adapter.hpp
│   │   ├── json_adapter.hpp
│   │   ├── xml_adapter.hpp
│   │   ├── pdf_adapter.hpp
│   │   ├── excel_adapter.hpp
│   │   ├── parquet_adapter.hpp
│   │   ├── text_adapter.hpp
│   │   ├── sqlite_adapter.hpp
│   │   ├── pgvector_adapter.hpp
│   │   ├── http_adapter.hpp
│   │   └── http_client.hpp
│   ├── cli/
│   │   ├── cli.hpp
│   │   ├── command_base.hpp
│   │   ├── interactive_shell.hpp
│   │   ├── output_formatter.hpp
│   │   ├── colors.hpp
│   │   ├── progress.hpp
│   │   └── commands/
│   │       ├── db_commands.hpp
│   │       ├── collection_commands.hpp
│   │       ├── data_commands.hpp
│   │       ├── search_commands.hpp
│   │       ├── ingest_commands.hpp
│   │       ├── index_commands.hpp
│   │       ├── hybrid_commands.hpp
│   │       └── export_commands.hpp
│   ├── embeddings/
│   │   ├── text.hpp
│   │   ├── image.hpp
│   │   ├── onnx_runtime.hpp
│   │   └── stb_image.h
│   ├── index/
│   │   └── metadata_index.hpp
│   ├── llm/
│   │   └── llm_engine.hpp
│   ├── quantization/
│   │   ├── scalar_quantizer.hpp
│   │   ├── product_quantizer.hpp
│   │   ├── adaptive_quantizer.hpp
│   │   ├── perceptual_curves.hpp
│   │   └── structured_quantizer.hpp
│   └── storage/
│       ├── sqlite_store.hpp
│       ├── pgvector_store.hpp
│       └── unified_store.hpp
├── src/
│   ├── database.cpp
│   ├── adapters/*.cpp
│   ├── cli/*.cpp
│   ├── core/
│   │   ├── distance.cpp
│   │   ├── vector_ops.cpp
│   │   ├── thread_pool.cpp
│   │   └── telemetry.cpp
│   ├── embeddings/
│   │   ├── text_encoder.cpp
│   │   ├── image_encoder.cpp
│   │   └── onnx_runtime.cpp
│   ├── framework/
│   │   ├── adapters.cpp
│   │   ├── pytorch_embedder.cpp
│   │   ├── tensorflow_embedder.cpp
│   │   └── rag_engine.cpp
│   ├── hybrid/
│   │   ├── bm25_engine.cpp
│   │   └── hybrid_search_engine.cpp
│   ├── index/
│   │   ├── flat.cpp
│   │   ├── hnsw.cpp
│   │   └── metadata_index.cpp
│   ├── ingest/
│   │   ├── gold_standard_ingest.cpp
│   │   └── markdown_parser.cpp
│   ├── llm/
│   │   └── llama_engine.cpp
│   ├── quantization/
│   │   ├── scalar_quantizer.cpp
│   │   ├── product_quantizer.cpp
│   │   ├── adaptive_quantizer.cpp
│   │   └── perceptual_curves.cpp
│   └── storage/
│       ├── metadata.cpp
│       ├── mmap_store.cpp
│       └── sqlite_store.cpp
├── bindings/python/
│   └── pyvdb.cpp
├── api/
│   ├── main.py               # FastAPI_REST_server
│   └── requirements.txt
├── tests/
│   ├── test_distance.cpp
│   ├── test_flat_index.cpp
│   ├── test_hnsw.cpp
│   ├── test_hybrid_search.cpp
│   ├── test_bm25.cpp
│   ├── test_embeddings.cpp
│   ├── test_ingest.cpp
│   ├── test_storage.cpp
│   ├── test_vector_ops.cpp
│   ├── test_rag.cpp
│   └── test_perceptual_quantization.cpp
├── examples/
│   ├── hybrid_search_demo.py
│   ├── rag_framework_demo.py
│   ├── distributed_demo.py
│   └── python_hybrid_search.py
└── scripts/
    ├── setup.ps1
    ├── setup.sh
    ├── build.ps1
    ├── download_models.py
    └── verify-installation.py
```

---

# §12 LOGIC_FLOW_MAP

## §12.1 VECTOR_ADD_FLOW

```
FLOW: add_text(text, metadata)
  1. CHECK: is_ready()
  2. ENCODE: text_encoder_.encode(text) -> vector<float>[384]
  3. PROJECT: project_text_embedding(text_emb) -> Vector[512]
  4. NORMALIZE: normalize(projected)
  5. GENERATE_ID: next_id()
  6. STORE_VECTOR: vectors_->add(id, vector)
  7. STORE_METADATA: metadata_->add(metadata)
  8. INDEX_ADD: index_->add(id, vector)
  9. RETURN: id
```

## §12.2 VECTOR_SEARCH_FLOW

```
FLOW: query_text(query, options)
  1. CHECK: is_ready()
  2. ENCODE: text_encoder_.encode(query) -> vector<float>[384]
  3. PROJECT: project_text_embedding(query_emb) -> Vector[512]
  4. NORMALIZE: normalize(projected)
  5. SEARCH: index_->search(query_vec, options.k * 2)
  6. IF options.has_filters:
     6.1. FILTER: apply_filters(results, options)
  7. FOR each result:
     7.1. GET_METADATA: metadata_->get(result.id)
     7.2. CALCULATE_SCORE: 1.0 - distance
  8. SORT: by score descending
  9. TRUNCATE: to options.k
  10. RETURN: QueryResults
```

## §12.3 HYBRID_SEARCH_FLOW

```
FLOW: hybrid_search(query, options)
  1. PARALLEL:
     1a. VECTOR_SEARCH: query_text(query, options)
     1b. LEXICAL_SEARCH: bm25_->search(query, k)
  2. FUSION: combine(vector_results, lexical_results, options.fusion)
     2a. IF RRF:
         score = 1/(rrf_k + vec_rank) + 1/(rrf_k + lex_rank)
     2b. IF WeightedSum:
         score = vec_weight * vec_score + lex_weight * lex_score
  3. SORT: by combined_score descending
  4. IF options.rerank:
     4.1. RERANK: reranker.rerank(query, results)
  5. RETURN: HybridResults
```

## §12.4 HNSW_ADD_FLOW

```
FLOW: hnsw.add(id, vector)
  1. LOCK: mutex_
  2. CHECK: size() < max_elements
  3. SELECT_LEVEL: random_level()
  4. CREATE_NODE: {id, level, vector, connections[]}
  5. IF first_node:
     5.1. SET: entry_point_ = id
     5.2. RETURN
  6. FIND_ENTRY: start from entry_point_ at max_level_
  7. FOR layer = max_level_ down to new_node_level+1:
     7.1. SEARCH_LAYER: find closest node
     7.2. UPDATE: entry_point for next layer
  8. FOR layer = min(max_level_, new_node_level) down to 0:
     8.1. SEARCH_LAYER: find ef_construction nearest
     8.2. SELECT_NEIGHBORS: heuristic selection up to M
     8.3. CONNECT: bidirectional edges
     8.4. PRUNE: if connections > M_max
  9. IF new_level > max_level_:
     9.1. UPDATE: max_level_ = new_level
     9.2. UPDATE: entry_point_ = id
  10. INCREMENT: element_count_
```

## §12.5 HNSW_SEARCH_FLOW

```
FLOW: hnsw.search(query, k)
  1. LOCK_SHARED: mutex_
  2. IF empty: RETURN []
  3. SET: ep = entry_point_
  4. FOR layer = max_level_ down to 1:
     4.1. SEARCH_LAYER: find closest, ef=1
     4.2. UPDATE: ep = closest
  5. SEARCH_LAYER_0: find ef_search nearest
  6. SORT: by distance ascending
  7. TRUNCATE: to k results
  8. CONVERT: to SearchResults
  9. RETURN: results
```

## §12.6 BM25_SEARCH_FLOW

```
FLOW: bm25.search(query, k)
  1. TOKENIZE: query -> terms[]
  2. FOR each term:
     2.1. LOOKUP: posting_list[term]
     2.2. FOR each doc in posting_list:
          2.2.1. CALCULATE: tf = term_freq_in_doc
          2.2.2. CALCULATE: idf = log((N - df + 0.5) / (df + 0.5))
          2.2.3. CALCULATE: tf_component = tf * (k1 + 1) / (tf + k1 * (1 - b + b * dl/avgdl))
          2.2.4. ACCUMULATE: doc_score += idf * tf_component
  3. SORT: by score descending
  4. TRUNCATE: to k results
  5. RETURN: BM25Results
```

## §12.7 INGEST_FLOW

```
FLOW: ingest(config)
  1. DISCOVER: find files in config.gold_standard_output
  2. FILTER: by config settings (journals, charts, reports)
  3. FOR each file:
     3.1. CHECK: is_ingested(file)
     3.2. DETECT_TYPE: detect_document_type(filename)
     3.3. PARSE: parse_markdown(content)
     3.4. EXTRACT: extract_market_data(content)
     3.5. CREATE_METADATA: from parsed data
     3.6. IF chart:
          3.6.1. ENCODE_IMAGE: image_encoder.encode(path)
          3.6.2. ADD: db.add_image(path, metadata)
     3.7. ELSE:
          3.7.1. CHUNK: chunk_document(parsed)
          3.7.2. FOR each chunk:
               ADD: db.add_text(chunk, metadata)
     3.8. UPDATE: stats
  4. RETURN: IngestStats
```

---

# §13 HUMAN_READABLE_SECTION

## §13.1 WHAT IS HEKTOR?

```
HEKTOR (also known as VectorDB/Vector Studio) is a high-performance 
vector database written in C++23 with Python bindings and REST API.

CORE CAPABILITIES:
- Semantic vector search using HNSW index
- Multi-modal embeddings (text via MiniLM, images via CLIP)
- Hybrid search (vector + BM25 full-text)
- Product quantization for memory efficiency
- Distributed operation (replication, sharding)
- Local LLM inference via llama.cpp
- RAG (Retrieval Augmented Generation) support
- Multiple data format adapters

DESIGNED FOR:
- Gold Standard financial data processing
- AI/ML training data management
- Semantic document search
- Image similarity search
- Real-time inference applications
```

## §13.2 HOW TO USE EFFICIENTLY

```
QUICK START:
  # Initialize database
  hektor init ./mydb --preset gold-standard
  
  # Add documents
  hektor add ./mydb --file journal.md --type journal
  
  # Search
  hektor search ./mydb "gold price outlook" -k 10
  
  # Hybrid search (better for keyword-heavy queries)
  hektor hs ./mydb "inflation CPI data" --fusion rrf

PYTHON USAGE:
  import pyvdb
  
  db = pyvdb.create_gold_standard_db("./mydb")
  db.init()
  
  # Add text
  id = db.add_text("Gold surging on inflation fears", 
                   pyvdb.DocumentType.Journal, "2026-01-15")
  
  # Search
  results = db.search("precious metals outlook", k=10)
  
  # With numpy vectors
  import numpy as np
  vec = np.random.randn(512).astype(np.float32)
  db.add_vector(vec, metadata)

PERFORMANCE TIPS:
  1. Use batch operations for bulk inserts
  2. Set appropriate HNSW ef_search (higher = more accurate, slower)
  3. Use filters to narrow search space
  4. Enable quantization for large datasets (8-32x compression)
  5. Use hybrid search when queries contain specific terms
  6. Pre-compute embeddings for repeated queries
```

## §13.3 OPTIMAL CONFIGURATION

```
FOR SMALL DATASETS (<100K vectors):
  dimension: 512
  hnsw_m: 16
  hnsw_ef_construction: 200
  hnsw_ef_search: 50
  
FOR MEDIUM DATASETS (100K-1M vectors):
  dimension: 512
  hnsw_m: 32
  hnsw_ef_construction: 400
  hnsw_ef_search: 100
  use_quantization: true (Product Quantizer)
  
FOR LARGE DATASETS (>1M vectors):
  dimension: 512
  hnsw_m: 48
  hnsw_ef_construction: 500
  hnsw_ef_search: 200
  use_quantization: true
  enable_sharding: true
  num_shards: ceil(vectors / 1000000)

FOR HIGH ACCURACY:
  hnsw_ef_search: 500+
  fusion_method: RRF (for hybrid)
  rerank: true
  
FOR LOW LATENCY:
  hnsw_ef_search: 20-50
  use_quantization: true
  memory_only: true (if fits in RAM)
```

## §13.4 UPGRADING HEKTOR

```
UPGRADE PATHS:

1. ADD GPU ACCELERATION:
   cmake -DVDB_ENABLE_GPU=ON -DVDB_USE_ONNX_RUNTIME=ON ..
   
2. ENABLE DISTRIBUTED MODE:
   cmake -DVDB_BUILD_DISTRIBUTED=ON -DVDB_USE_GRPC=ON ..
   
3. ADD TELEMETRY:
   cmake -DVDB_USE_OPENTELEMETRY=ON -DVDB_USE_PROMETHEUS=ON ..
   
4. ENABLE LOCAL LLM:
   cmake -DVDB_USE_LLAMA_CPP=ON ..
   # Download GGUF models to ./models/

5. CUSTOM EMBEDDINGS:
   # Use TensorFlow or PyTorch embedders
   cmake -DVDB_USE_TENSORFLOW=ON ..
   # OR
   cmake -DVDB_USE_PYTORCH=ON ..
```

## §13.5 COMPLEMENTARY TOOLS

```
RECOMMENDED PAIRINGS:

FOR RAG APPLICATIONS:
  - LangChain (use LangChainAdapter)
  - LlamaIndex (use LlamaIndexAdapter)
  - Local LLMs: Llama-3, Mistral, Phi-3 (via llama.cpp)
  
FOR DATA PIPELINES:
  - Apache Airflow (orchestration)
  - dbt (data transformation)
  - Great Expectations (data quality)
  
FOR MONITORING:
  - Prometheus + Grafana (metrics)
  - Jaeger (distributed tracing)
  - ELK Stack (logging)
  
FOR SCALING:
  - Kubernetes (container orchestration)
  - Redis (caching)
  - PostgreSQL + pgvector (backup/sync)
  
FOR ML WORKFLOWS:
  - MLflow (experiment tracking)
  - Weights & Biases (training monitoring)
  - Hugging Face (model hub)
```

## §13.6 DEPLOYMENT SETUP

```
PRODUCTION DEPLOYMENT:

1. CONTAINERIZED:
   docker-compose up -d
   # Uses Dockerfile in root

2. KUBERNETES:
   kubectl apply -f k8s/
   # Includes deployment, service, ingress

3. BARE METAL:
   # Build release
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . --config Release
   
   # Run API server
   uvicorn api.main:app --host 0.0.0.0 --port 8080 --workers 4

ENVIRONMENT VARIABLES:
  VDB_PATH: /data/vectors
  SECRET_KEY: <your-secret>
  CORS_ORIGINS: https://your-domain.com
  RATE_LIMIT_ENABLED: true
  LOG_LEVEL: INFO

SECURITY CHECKLIST:
  [ ] Change default SECRET_KEY
  [ ] Enable HTTPS/TLS
  [ ] Configure CORS origins
  [ ] Enable rate limiting
  [ ] Set up authentication
  [ ] Enable audit logging
  [ ] Regular backups
```

---

# §14 TEST_COVERAGE

```
TEST_FILES:
  C++:
    - test_distance.cpp: Distance_function_tests
    - test_flat_index.cpp: FlatIndex_tests
    - test_hnsw.cpp: HNSW_index_tests
    - test_bm25.cpp: BM25_engine_tests
    - test_bm25_new.cpp: Extended_BM25_tests
    - test_hybrid_search.cpp: Hybrid_search_tests
    - test_hybrid_search_new.cpp: Extended_hybrid_tests
    - test_embeddings.cpp: Embedding_model_tests
    - test_ingest.cpp: Ingestion_pipeline_tests
    - test_storage.cpp: Storage_backend_tests
    - test_vector_ops.cpp: Vector_operation_tests
    - test_rag.cpp: RAG_pipeline_tests
    - test_rag_new.cpp: Extended_RAG_tests
    - test_perceptual_quantization.cpp: PQ/HLG/Gamma_curve_tests
    
  Python:
    - conftest.py: Pytest_configuration
    - test_api_integration.py: REST_API_integration_tests
    - test_documentation.py: Documentation_validation

TEST_FRAMEWORK: GoogleTest (C++), pytest (Python)
BUILD_COMMAND: cmake --build build --target vdb_tests
RUN_COMMAND: ctest --output-on-failure --verbose
```

---

# §15 SECTOR_ARCHITECTURE_DIAGRAM

```
┌───────────────────────────────────────────────────────────────────────────┐
│                          HEKTOR SYSTEM ARCHITECTURE                         │
│                        Frontend/Backend Sectorization                       │
└───────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                              FRONTEND SECTOR                                  │
│                         (Angular 21 + TypeScript)                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                               │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐          │
│  │  UI Components   │  │    Services      │  │  Guards/Models   │          │
│  ├──────────────────┤  ├──────────────────┤  ├──────────────────┤          │
│  │ • Login          │  │ • Auth Service   │  │ • Auth Guards    │          │
│  │ • Playground     │  │ • API Service    │  │ • HTTP Intercept │          │
│  │ • Database Ops   │  │ • Vector DB Svc  │  │ • DTO Models     │          │
│  │ • Index Manager  │  │ • Agent Service  │  │ • Interfaces     │          │
│  │ • Health Monitor │  │                  │  │                  │          │
│  │ • Cluster View   │  │                  │  │                  │          │
│  │ • Hybrid Search  │  │                  │  │                  │          │
│  │ • Projections    │  │                  │  │                  │          │
│  │ • Export Manager │  │                  │  │                  │          │
│  │ • Ingestion Wiz  │  │                  │  │                  │          │
│  │ • Schema Builder │  │                  │  │                  │          │
│  │ • Settings       │  │                  │  │                  │          │
│  │ • Chat Sidebar   │  │                  │  │                  │          │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘          │
│                                                                               │
│  Build: Angular CLI + Vite │ Deploy: Vercel │ Package: npm                  │
└───────────────────────────────┬───────────────────────────────────────────────┘
                                │
                        HTTP/REST (JSON)
                        JWT Authentication
                                │
┌───────────────────────────────▼───────────────────────────────────────────────┐
│                            INTEGRATION LAYER                                   │
│                          (REST API + Python Bridge)                            │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                │
│  ┌──────────────────────────────────────────────────────────────────────┐    │
│  │                    FastAPI REST Server (Python)                       │    │
│  │  Location: /api/main.py                                               │    │
│  ├──────────────────────────────────────────────────────────────────────┤    │
│  │  Endpoints:                                                           │    │
│  │  • GET  /health, /metrics, /stats                                    │    │
│  │  • POST /auth/login                                                  │    │
│  │  • POST /collections, GET /collections, DELETE /collections/{name}   │    │
│  │  • POST /collections/{name}/documents, .../documents/batch           │    │
│  │  • POST /collections/{name}/search                                   │    │
│  └──────────────────────────────┬───────────────────────────────────────┘    │
│                                  │                                            │
│                          pybind11 Bridge                                      │
│                       (Near-Native Performance)                               │
│                                  │                                            │
│  ┌──────────────────────────────▼───────────────────────────────────────┐    │
│  │                   Python Bindings (pyvdb module)                      │    │
│  │  • VectorDatabase, QueryOptions, QueryResult                          │    │
│  │  • Metadata, IndexStats, LLMEngine                                    │    │
│  │  • GoldStandardIngest                                                │    │
│  └───────────────────────────────────────────────────────────────────────┘    │
└────────────────────────────────┬───────────────────────────────────────────────┘
                                 │
                         Shared Memory Access
                                 │
┌────────────────────────────────▼────────────────────────────────────────────────┐
│                              BACKEND SECTOR                                      │
│                    (C++23 High-Performance Engine)                               │
├──────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │                         L5: API LAYER                                     │  │
│  │  • CLI Interface        • Python Bindings       • REST API               │  │
│  └──────────────────────────────┬────────────────────────────────────────────┘  │
│                                 │                                               │
│  ┌──────────────────────────────▼────────────────────────────────────────────┐  │
│  │                         L4: SEARCH LAYER                                   │  │
│  │  • vector_search.cpp    • bm25_engine.cpp    • hybrid_search.cpp          │  │
│  │  • rag_pipeline.cpp     • Fusion algorithms (RRF, CombSUM, etc.)          │  │
│  └──────────────────────────────┬────────────────────────────────────────────┘  │
│                                 │                                               │
│  ┌──────────────────────────────▼────────────────────────────────────────────┐  │
│  │                       L3: EMBEDDINGS LAYER                                 │  │
│  │  • text_encoder.cpp (ONNX MiniLM)    • image_encoder.cpp (ONNX CLIP)      │  │
│  │  • onnx_runtime_wrapper.cpp          • Local embeddings (no API calls)    │  │
│  └──────────────────────────────┬────────────────────────────────────────────┘  │
│                                 │                                               │
│  ┌──────────────────────────────▼────────────────────────────────────────────┐  │
│  │                         L2: INDEX LAYER                                    │  │
│  │  • hnsw_index.cpp (M=16, O(log n))  • flat_index.cpp (brute-force)        │  │
│  │  • metadata_index.cpp                                                      │  │
│  └──────────────────────────────┬────────────────────────────────────────────┘  │
│                                 │                                               │
│  ┌──────────────────────────────▼────────────────────────────────────────────┐  │
│  │                        L1: STORAGE LAYER                                   │  │
│  │  • mmap_store.cpp (memory-mapped, zero-copy)                               │  │
│  │  • sqlite_store.cpp (metadata)  • pgvector_store.cpp (PostgreSQL)         │  │
│  │  • metadata_store.cpp                                                      │  │
│  └──────────────────────────────┬────────────────────────────────────────────┘  │
│                                 │                                               │
│  ┌──────────────────────────────▼────────────────────────────────────────────┐  │
│  │                         L0: CORE LAYER                                     │  │
│  │  • types.hpp (VectorId, Dim, Distance, Scalar)                             │  │
│  │  • distance.cpp (Cosine, L2, Dot Product) - SIMD optimized (AVX2/AVX-512) │  │
│  │  • vector_ops.cpp (batch operations, thread-safe)                          │  │
│  └────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                  │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │                      ADVANCED FEATURES (Cross-Layer)                       │  │
│  │  • quantization.cpp (Perceptual PQ, Scalar, Display-aware)                 │  │
│  │  • distributed.cpp (Replication: async/sync/semi-sync, Sharding)           │  │
│  │  • llm_engine.cpp (llama.cpp integration)                                  │  │
│  │  • telemetry.cpp (OpenTelemetry), ebpf_tracer.cpp (eBPF observability)    │  │
│  │  • http_adapter.cpp, http_client.cpp (HTTP integration)                    │  │
│  │  • data_ingestion.cpp (XML, JSON, CSV, Excel, PDF, Parquet, SQLite)       │  │
│  └────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                  │
│  Build: CMake + Ninja │ Compiler: GCC 11+/Clang 14+ │ Standards: C++23          │
│  SIMD: AVX2/AVX-512  │ Optimizations: -O3 -march=native                         │
└──────────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│                          DISTRIBUTED ARCHITECTURE                              │
│                         (Backend Sector - Horizontal Scale)                    │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                │
│     ┌─────────────┐      ┌─────────────┐      ┌─────────────┐               │
│     │   Node 1    │◄─────┤   Node 2    │◄─────┤   Node 3    │               │
│     │  (Primary)  │      │  (Replica)  │      │  (Replica)  │               │
│     └─────────────┘      └─────────────┘      └─────────────┘               │
│           │                     │                     │                       │
│           └─────────────────────┴─────────────────────┘                       │
│                  gRPC/Protobuf Communication                                  │
│                  Replication: async/sync/semi-sync                            │
│                  Sharding: hash/range/consistent                              │
│                                                                                │
└────────────────────────────────────────────────────────────────────────────────┘
```

---

# §16 VERSION_HISTORY

```
VERSION: 4.0.0
  LANGUAGE: C++23
  INDEX: HNSW
  EMBEDDINGS: MiniLM-L6 (text), CLIP ViT-B/32 (image)
  FEATURES:
    - Full hybrid search (vector + BM25 + RRF)
    - Product quantization
    - Distributed mode (replication, sharding)
    - Local LLM via llama.cpp
    - OpenTelemetry tracing
    - Multiple data adapters
    - Framework integrations (TensorFlow, PyTorch, LangChain, LlamaIndex)
```

---

# §16 SYSTEM_SECTORIZATION

## §16.1 FRONTEND_SECTOR

```yaml
FRONTEND:
  TYPE: Angular_Single_Page_Application
  LANGUAGE: TypeScript
  FRAMEWORK: Angular_21.0.5
  LOCATION: /ui/
  
  COMPONENTS:
    User_Interface:
      - login.component.ts: Authentication_UI
      - playground.component.ts: Interactive_query_testing
      - database-ops.component.ts: Database_management_UI
      - index-manager.component.ts: Index_configuration_UI
      - health-monitor.component.ts: System_health_visualization
      - cluster-view.component.ts: Distributed_system_monitoring
      - hybrid-search.component.ts: Hybrid_search_interface
      - projection-view.component.ts: Vector_space_visualization
      - export-manager.component.ts: Data_export_interface
      - ingestion-wizard.component.ts: Data_ingestion_wizard
      - schema-builder.component.ts: Schema_design_tool
      - settings.component.ts: Application_settings
      - chat-sidebar.component.ts: LLM_chat_interface
    
    Services:
      - auth.service.ts: Frontend_authentication_logic
      - hektor-api.service.ts: REST_API_client
      - vector-db.service.ts: Vector_database_operations
      - agent.service.ts: AI_agent_interactions
    
    Guards:
      - Authentication_guards
      - Route_protection
    
    Interceptors:
      - HTTP_interceptors
      - Token_management
    
    Models:
      - TypeScript_interfaces
      - Data_transfer_objects
  
  BUILD_SYSTEM:
    Tool: Angular_CLI + Vite
    Config: angular.json
    Output: dist/
    Package_Manager: npm
    Dependencies: package.json
  
  DEPLOYMENT:
    Platform: Vercel
    Config: vercel.json
    Build_Command: ng build --configuration production
    Static_Files: true
  
  FEATURES:
    - Responsive_design
    - Real-time_updates
    - Vector_visualization_(2D/3D_projections)
    - Interactive_query_interface
    - Authentication_flow
    - Cluster_monitoring
    - Health_dashboards
    - Data_ingestion_wizards
    - Export_management
```

## §16.2 BACKEND_SECTOR

```yaml
BACKEND:
  TYPE: High_Performance_C++_Engine + Python_REST_API
  LANGUAGES: [C++23, Python]
  LOCATION: /src/, /api/
  
  CORE_ENGINE:
    Language: C++23
    Location: /src/
    Components:
      Core_Operations:
        - vector_ops.cpp: SIMD-optimized_vector_operations
        - distance.cpp: Distance_metric_implementations
        - types.hpp: Core_type_definitions
      
      Storage_Layer:
        - mmap_store.cpp: Memory-mapped_storage
        - sqlite_store.cpp: SQLite_metadata_storage
        - pgvector_store.cpp: PostgreSQL_integration
        - metadata_store.cpp: Metadata_management
      
      Index_Layer:
        - hnsw_index.cpp: HNSW_algorithm_implementation
        - flat_index.cpp: Brute-force_index
        - metadata_index.cpp: Metadata_indexing
      
      Embeddings_Layer:
        - text_encoder.cpp: ONNX_text_embeddings
        - image_encoder.cpp: ONNX_image_embeddings
        - onnx_runtime_wrapper.cpp: ONNX_integration
      
      Search_Layer:
        - vector_search.cpp: Vector_similarity_search
        - bm25_engine.cpp: BM25_full-text_search
        - hybrid_search.cpp: Fusion_algorithms
      
      Adapters:
        - http_adapter.cpp: HTTP_request_handling
        - http_client.cpp: HTTP_client_operations
        - data_ingestion.cpp: Multi-format_data_ingestion
      
      Advanced_Features:
        - quantization.cpp: Vector_quantization_(PQ/SQ/Display-aware)
        - distributed.cpp: Replication_and_sharding
        - llm_engine.cpp: Local_LLM_integration
        - rag_pipeline.cpp: RAG_implementation
        - telemetry.cpp: OpenTelemetry_integration
        - ebpf_tracer.cpp: eBPF_observability
    
    Build_System:
      Tool: CMake + Ninja
      Config: CMakeLists.txt
      Compiler: GCC_11+/Clang_14+
      Standards: C++23
      Optimizations: -O3 -march=native
      SIMD: AVX2/AVX-512
  
  REST_API_SERVER:
    Language: Python
    Location: /api/
    Framework: FastAPI
    Main_File: main.py
    
    Endpoints:
      System:
        - GET /health: Health_check
        - GET /metrics: Prometheus_metrics
        - GET /stats: Database_statistics
      
      Authentication:
        - POST /auth/login: User_authentication
      
      Collections:
        - POST /collections: Create_collection
        - GET /collections: List_collections
        - DELETE /collections/{name}: Delete_collection
      
      Documents:
        - POST /collections/{name}/documents: Add_document
        - POST /collections/{name}/documents/batch: Batch_insert
      
      Search:
        - POST /collections/{name}/search: Vector_search
    
    Integration:
      - Python_bindings_(pybind11): pyvdb_module
      - C++_core_wrapper: Direct_integration
      - Authentication: JWT_tokens
      - CORS: Cross-origin_support
      - Rate_limiting: Built-in
    
    Dependencies:
      - FastAPI: REST_framework
      - pydantic: Data_validation
      - python-jose: JWT_handling
      - python-multipart: File_upload
      - uvicorn: ASGI_server
  
  PYTHON_BINDINGS:
    Module: pyvdb
    Technology: pybind11
    Location: /bindings/python/
    
    Exports:
      - VectorDatabase: Main_database_class
      - Metadata: Metadata_handling
      - QueryOptions: Query_configuration
      - QueryResult: Search_results
      - IndexStats: Statistics
      - LLMEngine: LLM_integration
      - GoldStandardIngest: Domain-specific_ingestion
  
  COMMUNICATION:
    Frontend_to_Backend:
      Protocol: HTTP/HTTPS
      Format: JSON
      Authentication: JWT_Bearer_tokens
      API_Version: 2.0.0
    
    Internal_Communication:
      C++_to_Python: pybind11_bindings
      Distributed_Nodes: gRPC/Protobuf
      Service_Discovery: Built-in
      Health_Checks: Heartbeat_system
```

## §16.3 INTEGRATION_LAYER

```yaml
INTEGRATION:
  Frontend_to_REST_API:
    Client: hektor-api.service.ts
    Protocol: HTTP/REST
    Format: JSON
    Authentication: JWT
    Base_URL: Configurable_environment
  
  REST_API_to_Core:
    Bridge: Python_bindings_(pyvdb)
    Technology: pybind11
    Memory: Shared_memory_access
    Performance: Near-native_speed
  
  Data_Flow:
    User_Request:
      1. Angular_UI_(TypeScript)
      2. HTTP_Request_(JSON)
      3. FastAPI_REST_Server_(Python)
      4. pybind11_Bridge
      5. C++_Core_Engine
      6. Response_Path_Reversed
    
    Vector_Search:
      1. UI_sends_query_text
      2. REST_API_receives_request
      3. Python_calls_pyvdb.VectorDatabase.query_text()
      4. C++_performs_ONNX_embedding
      5. C++_executes_HNSW_search
      6. Results_returned_through_chain
      7. UI_displays_results
```

---

# §17 DEPENDENCY_GRAPH

```
HEKTOR_CORE
├── nlohmann_json (required)
├── fmt (required)
├── ONNX Runtime (optional: embeddings)
│   ├── MiniLM-L6-v2 (text model)
│   └── CLIP ViT-B/32 (image model)
├── llama.cpp (optional: LLM)
├── CURL (optional: HTTP)
├── SQLite3 (optional: metadata storage)
├── libpq (optional: PostgreSQL/pgvector)
├── Poppler (optional: PDF parsing)
├── xlsxwriter (optional: Excel support)
├── ICU (optional: Unicode)
├── OpenTelemetry (optional: tracing)
├── Prometheus (optional: metrics)
├── gRPC/Protobuf (optional: distributed)
├── pybind11 (optional: Python bindings)
└── GTest (dev: testing)
```

---

```
DOCUMENT_END
GENERATED: 2026-01-22
HASH: [SHA256_OF_CONTENT]
COMPLETENESS: EXHAUSTIVE
MACHINE_READABLE: TRUE
HUMAN_READABLE_SECTION: §13
SECTORIZATION_ADDED: §15_ARCHITECTURE_DIAGRAM, §16_SYSTEM_SECTORIZATION
SECTORS: [FRONTEND_Angular_TypeScript, BACKEND_C++_Python, INTEGRATION_REST_API]
```
