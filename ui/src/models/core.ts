// Core Type Definitions for HEKTOR Vector Studio

// Collection Types
export interface Collection {
  name: string;
  dimension: number;
  metric: 'cosine' | 'euclidean' | 'dot';
  documents: VectorDoc[];
  schema?: SchemaField[];
  embeddingModel?: string;
  isSystem?: boolean;
  createdAt?: string;
  updatedAt?: string;
}

export interface VectorDoc {
  id: string;
  content: string;
  metadata: Record<string, any>;
  vector: number[];
  projection: [number, number];
  projection3d: [number, number, number];
  cluster: number;
  score?: number;
}

// Schema Types
export interface SchemaField {
  name: string;
  type: 'string' | 'number' | 'boolean' | 'date' | 'list';
  required: boolean;
  description?: string;
}

// Ingestion Types
export interface IngestionConfig {
  collectionName: string;
  chunkSize: number;
  chunkOverlap: number;
  chunkingStrategy?: 'recursive' | 'fixed' | 'semantic';
  embeddingModel?: string;
  files?: File[];
  strategy?: 'recursive' | 'fixed' | 'semantic';
}

// Search Types
export interface SearchParams {
  query: string;
  topK: number;
  filter?: Record<string, any>;
  filters?: Record<string, any>;
  includeMetadata?: boolean;
  includeVectors?: boolean;
  minScore?: number;
}

export interface HybridSearchParams {
  query: string;
  topK: number;
  vectorWeight: number;
  lexicalWeight: number;
  fusionMethod: FusionMethod;
  filter?: Record<string, any>;
  minScore?: number;
}

export type FusionMethod = 'rrf' | 'weighted' | 'combsum' | 'combmnz' | 'borda';

export interface HybridResult {
  id: string;
  content: string;
  metadata: Record<string, any>;
  score: number;
  vectorScore?: number;
  lexicalScore?: number;
  fusedScore: number;
  combinedScore?: number;
  matchedKeywords?: string[];
}

// Index Types
export type IndexType = 'flat' | 'hnsw' | 'ivf' | 'pq';

export interface IndexStats {
  indexType: IndexType;
  totalVectors: number;
  dimension: number;
  memoryUsage: number;
  buildTime?: number;
  params?: Record<string, any>;
  type?: string;
  indexSize?: number;
  hnswM?: number;
  hnswEfConstruction?: number;
  buildTimestamp?: string;
  optimized?: boolean;
}

export interface IndexConfig {
  indexType?: IndexType;
  type?: IndexType;
  hnswM?: number;
  hnswEfConstruction?: number;
  hnswEfSearch?: number;
  params?: {
    efConstruction?: number;
    m?: number;
    nlist?: number;
    nprobe?: number;
  };
}

export interface BenchmarkResult {
  qps?: number;
  avgLatencyMs: number;
  p50LatencyMs: number;
  p95LatencyMs: number;
  p99LatencyMs: number;
  recallAt10?: number;
  queriesRun?: number;
  throughputQps?: number;
  recall?: number;
}

// Export Types
export type ExportFormat = 'jsonl' | 'json' | 'csv' | 'parquet';

export interface ExportConfig {
  format: ExportFormat;
  includeVectors: boolean;
  includeMetadata?: boolean;
  dateFrom?: string;
  dateTo?: string;
  filter?: Record<string, any>;
}

export type TripletStrategy = 'random' | 'hard_negative' | 'semi_hard';

export interface TripletExportConfig {
  strategy: TripletStrategy;
  numTriplets?: number;
  anchorField?: string;
  positiveThreshold?: number;
  negativeThreshold?: number;
  negativeSamples?: number;
}

// Database Info Types
export interface DatabaseInfo {
  path: string;
  totalVectors: number;
  totalCollections: number;
  indexType: string;
  memoryUsageBytes: number;
  diskUsageBytes: number;
  version: string;
  createdAt?: string;
  modifiedAt?: string;
}

export interface HealthStatus {
  status: 'healthy' | 'degraded' | 'unhealthy';
  checks: HealthCheck[];
  uptime?: number;
  uptimeSeconds?: number;
  lastCheckAt?: string;
}

export interface HealthCheck {
  name: string;
  status: 'pass' | 'fail' | 'warn';
  message?: string;
  latencyMs?: number;
}

// Cluster Types
export interface ClusterInfo {
  totalNodes: number;
  activeNodes: number;
  mode: 'standalone' | 'cluster';
  nodes?: NodeInfo[];
  replicationFactor?: number;
  shardCount?: number;
}

export interface NodeInfo {
  id: string;
  host: string;
  port: number;
  status: 'active' | 'inactive' | 'syncing';
  role: 'master' | 'replica';
  vectorCount: number;
  memoryUsage: number;
  cpuUsage: number;
  lastHeartbeat: string;
}

// Settings Types
export interface AppSettings {
  apiUrl: string;
  apiKey: string;
  wsUrl?: string;
  enableMockAuth?: boolean;
  useBackend?: boolean;
  defaultEmbeddingModel: string;
  defaultChunkSize?: number;
  defaultTopK?: number;
  defaultDimension?: number;
  defaultMetric?: 'cosine' | 'euclidean' | 'dot';
  theme?: 'dark' | 'light';
  logLevel: LogLevel;
  autoSave?: boolean;
  showTutorial?: boolean;
}

export type LogLevel = 'debug' | 'info' | 'warn' | 'error';

export const EMBEDDING_MODELS = [
  { id: 'text-embedding-004', name: 'Gemini Text Embedding 004', dimension: 768 },
  { id: 'text-embedding-005', name: 'Gemini Text Embedding 005', dimension: 768 },
] as const;
