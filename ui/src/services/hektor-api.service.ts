import { Injectable, signal } from '@angular/core';
import { 
  Collection, 
  VectorDoc, 
  SearchParams, 
  HybridSearchParams,
  HybridResult,
  IndexConfig,
  IndexStats,
  BenchmarkResult,
  ExportConfig,
  TripletExportConfig,
  DatabaseInfo,
  HealthStatus,
  ClusterInfo
} from '../models/core';
import { environment } from '../environments/environment';

/**
 * HEKTOR API Service
 * 
 * Provides HTTP client interface to the HEKTOR Vector Database CLI backend.
 * Maps all CLI commands to REST API endpoints.
 */
@Injectable({
  providedIn: 'root'
})
export class HektorApiService {
  private baseUrl = signal<string>(environment.apiUrl);
  private dbPath = signal<string>('./vectors'); // Default database path

  constructor() {
    // Load settings from localStorage
    const savedUrl = localStorage.getItem('hektor_api_url');
    if (savedUrl) this.baseUrl.set(savedUrl);
    
    const savedPath = localStorage.getItem('hektor_db_path');
    if (savedPath) this.dbPath.set(savedPath);
  }

  setApiUrl(url: string): void {
    this.baseUrl.set(url);
    localStorage.setItem('hektor_api_url', url);
  }

  setDatabasePath(path: string): void {
    this.dbPath.set(path);
    localStorage.setItem('hektor_db_path', path);
  }

  private async request<T>(endpoint: string, options?: RequestInit): Promise<T> {
    const url = `${this.baseUrl()}${endpoint}`;
    
    try {
      const response = await fetch(url, {
        ...options,
        headers: {
          'Content-Type': 'application/json',
          ...options?.headers,
        },
      });

      if (!response.ok) {
        const error = await response.json().catch(() => ({ message: response.statusText }));
        throw new Error(error.message || `HTTP ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error('API request failed:', error);
      throw error;
    }
  }

  // ==================== DATABASE OPERATIONS ====================

  /**
   * Initialize a new database
   * CLI: hektor init <path> [--dimension N] [--metric TYPE] [--preset gold-standard]
   */
  async initDatabase(config: {
    path?: string;
    dimension?: number;
    metric?: 'cosine' | 'euclidean' | 'dot';
    preset?: 'gold-standard';
  }): Promise<{ success: boolean; message: string }> {
    return this.request('/api/db/init', {
      method: 'POST',
      body: JSON.stringify({
        path: config.path || this.dbPath(),
        dimension: config.dimension || 512,
        metric: config.metric || 'cosine',
        preset: config.preset
      })
    });
  }

  /**
   * Get database information
   * CLI: hektor info <path>
   */
  async getDatabaseInfo(path?: string): Promise<DatabaseInfo> {
    const dbPath = path || this.dbPath();
    return this.request(`/api/db/info?path=${encodeURIComponent(dbPath)}`);
  }

  /**
   * Get database statistics
   * CLI: hektor stats <path>
   */
  async getDatabaseStats(path?: string): Promise<{
    totalVectors: number;
    totalCollections: number;
    memoryUsage: number;
    diskUsage: number;
    indexType: string;
  }> {
    const dbPath = path || this.dbPath();
    return this.request(`/api/db/stats?path=${encodeURIComponent(dbPath)}`);
  }

  /**
   * Backup database
   * CLI: hektor backup <path> <backup-path>
   */
  async backupDatabase(backupPath: string, sourcePath?: string): Promise<{ success: boolean; message: string }> {
    return this.request('/api/db/backup', {
      method: 'POST',
      body: JSON.stringify({
        source: sourcePath || this.dbPath(),
        destination: backupPath
      })
    });
  }

  /**
   * Restore database from backup
   * CLI: hektor restore <backup-path> <target-path>
   */
  async restoreDatabase(backupPath: string, targetPath?: string): Promise<{ success: boolean; message: string }> {
    return this.request('/api/db/restore', {
      method: 'POST',
      body: JSON.stringify({
        source: backupPath,
        destination: targetPath || this.dbPath()
      })
    });
  }

  /**
   * Optimize database
   * CLI: hektor optimize <path>
   */
  async optimizeDatabase(path?: string): Promise<{ success: boolean; message: string }> {
    return this.request('/api/db/optimize', {
      method: 'POST',
      body: JSON.stringify({
        path: path || this.dbPath()
      })
    });
  }

  // ==================== COLLECTION OPERATIONS ====================

  /**
   * Create a new collection
   * CLI: hektor collection create <path> <name> [--description DESC]
   */
  async createCollection(config: {
    name: string;
    description?: string;
    path?: string;
  }): Promise<{ success: boolean; message: string }> {
    return this.request('/api/collections/create', {
      method: 'POST',
      body: JSON.stringify({
        path: config.path || this.dbPath(),
        name: config.name,
        description: config.description
      })
    });
  }

  /**
   * List all collections
   * CLI: hektor collection list <path>
   */
  async listCollections(path?: string): Promise<Collection[]> {
    const dbPath = path || this.dbPath();
    return this.request(`/api/collections/list?path=${encodeURIComponent(dbPath)}`);
  }

  /**
   * Delete a collection
   * CLI: hektor collection delete <path> <name>
   */
  async deleteCollection(name: string, path?: string): Promise<{ success: boolean; message: string }> {
    return this.request('/api/collections/delete', {
      method: 'DELETE',
      body: JSON.stringify({
        path: path || this.dbPath(),
        name
      })
    });
  }

  // ==================== DATA OPERATIONS ====================

  /**
   * Add data to database
   * CLI: hektor add <path> --text TEXT | --file FILE [--metadata JSON]
   */
  async addData(config: {
    text?: string;
    file?: File;
    metadata?: Record<string, any>;
    path?: string;
  }): Promise<{ success: boolean; id: string }> {
    const formData = new FormData();
    formData.append('path', config.path || this.dbPath());
    
    if (config.text) {
      formData.append('text', config.text);
    } else if (config.file) {
      formData.append('file', config.file);
    }
    
    if (config.metadata) {
      formData.append('metadata', JSON.stringify(config.metadata));
    }

    return fetch(`${this.baseUrl()}/api/data/add`, {
      method: 'POST',
      body: formData
    }).then(r => r.json());
  }

  /**
   * Query/Search vectors
   * CLI: hektor query <path> <query> [-k N] [--min-score SCORE]
   */
  async queryVectors(params: SearchParams & { path?: string }): Promise<VectorDoc[]> {
    return this.request('/api/data/query', {
      method: 'POST',
      body: JSON.stringify({
        path: params.path || this.dbPath(),
        query: params.query,
        topK: params.topK || 10,
        filter: params.filter,
        minScore: params.minScore
      })
    });
  }

  /**
   * Delete vectors by ID
   * CLI: hektor delete <path> <id>
   */
  async deleteData(id: string, path?: string): Promise<{ success: boolean; message: string }> {
    return this.request('/api/data/delete', {
      method: 'DELETE',
      body: JSON.stringify({
        path: path || this.dbPath(),
        id
      })
    });
  }

  // ==================== SEARCH OPERATIONS ====================

  /**
   * Vector search
   * CLI: hektor search <path> <query> [-k N] [--min-score SCORE]
   */
  async search(params: SearchParams & { path?: string }): Promise<VectorDoc[]> {
    return this.request('/api/search/vector', {
      method: 'POST',
      body: JSON.stringify({
        path: params.path || this.dbPath(),
        query: params.query,
        topK: params.topK || 10,
        filter: params.filter,
        minScore: params.minScore
      })
    });
  }

  /**
   * Hybrid search (Vector + BM25)
   * CLI: hektor hybrid <path> <query> [-k N] [--fusion METHOD] [--vector-weight W]
   */
  async hybridSearch(params: HybridSearchParams & { path?: string }): Promise<HybridResult[]> {
    return this.request('/api/search/hybrid', {
      method: 'POST',
      body: JSON.stringify({
        path: params.path || this.dbPath(),
        query: params.query,
        topK: params.topK || 10,
        vectorWeight: params.vectorWeight || 0.7,
        lexicalWeight: params.lexicalWeight || 0.3,
        fusionMethod: params.fusionMethod || 'rrf',
        filter: params.filter,
        minScore: params.minScore
      })
    });
  }

  // ==================== INDEX OPERATIONS ====================

  /**
   * Build index
   * CLI: hektor index build <path> [--type hnsw] [--hnsw-m M] [--hnsw-ef EF]
   */
  async buildIndex(config: IndexConfig & { path?: string }): Promise<{ success: boolean; message: string }> {
    return this.request('/api/index/build', {
      method: 'POST',
      body: JSON.stringify({
        path: config.path || this.dbPath(),
        indexType: config.indexType || 'hnsw',
        hnswM: config.hnswM || 16,
        hnswEfConstruction: config.hnswEfConstruction || 200
      })
    });
  }

  /**
   * Get index statistics
   * CLI: hektor index stats <path>
   */
  async getIndexStats(path?: string): Promise<IndexStats> {
    const dbPath = path || this.dbPath();
    return this.request(`/api/index/stats?path=${encodeURIComponent(dbPath)}`);
  }

  /**
   * Optimize index
   * CLI: hektor index optimize <path>
   */
  async optimizeIndex(path?: string): Promise<{ success: boolean; message: string }> {
    return this.request('/api/index/optimize', {
      method: 'POST',
      body: JSON.stringify({
        path: path || this.dbPath()
      })
    });
  }

  /**
   * Benchmark index
   * CLI: hektor index benchmark <path> [--queries N]
   */
  async benchmarkIndex(config: { 
    path?: string; 
    numQueries?: number 
  }): Promise<BenchmarkResult> {
    return this.request('/api/index/benchmark', {
      method: 'POST',
      body: JSON.stringify({
        path: config.path || this.dbPath(),
        numQueries: config.numQueries || 100
      })
    });
  }

  // ==================== EXPORT OPERATIONS ====================

  /**
   * Export data
   * CLI: hektor export <path> <output> [--format FORMAT] [--include-vectors]
   */
  async exportData(config: ExportConfig & { 
    path?: string;
    outputFile: string;
  }): Promise<{ success: boolean; filePath: string; fileSize: number }> {
    return this.request('/api/export/data', {
      method: 'POST',
      body: JSON.stringify({
        path: config.path || this.dbPath(),
        outputFile: config.outputFile,
        format: config.format || 'jsonl',
        includeVectors: config.includeVectors || false,
        includeMetadata: config.includeMetadata !== false,
        dateFrom: config.dateFrom,
        dateTo: config.dateTo,
        filter: config.filter
      })
    });
  }

  /**
   * Export pairs for training
   * CLI: hektor export pairs <path> <output> [--num-pairs N]
   */
  async exportPairs(config: {
    path?: string;
    outputFile: string;
    numPairs?: number;
    positiveThreshold?: number;
  }): Promise<{ success: boolean; filePath: string; numPairs: number }> {
    return this.request('/api/export/pairs', {
      method: 'POST',
      body: JSON.stringify({
        path: config.path || this.dbPath(),
        outputFile: config.outputFile,
        numPairs: config.numPairs || 1000,
        positiveThreshold: config.positiveThreshold || 0.7
      })
    });
  }

  /**
   * Export triplets for training
   * CLI: hektor export triplets <path> <output> [--strategy STRAT]
   */
  async exportTriplets(config: TripletExportConfig & {
    path?: string;
    outputFile: string;
  }): Promise<{ success: boolean; filePath: string; numTriplets: number }> {
    return this.request('/api/export/triplets', {
      method: 'POST',
      body: JSON.stringify({
        path: config.path || this.dbPath(),
        outputFile: config.outputFile,
        strategy: config.strategy || 'random',
        numTriplets: config.numTriplets || 1000,
        positiveThreshold: config.positiveThreshold || 0.7,
        negativeThreshold: config.negativeThreshold || 0.3
      })
    });
  }

  // ==================== INGESTION OPERATIONS ====================

  /**
   * Ingest data from various sources
   * CLI: hektor ingest <path> <source> [--format FORMAT] [--chunk-strategy STRAT]
   */
  async ingestData(config: {
    path?: string;
    source: string | File;
    format?: 'auto' | 'json' | 'jsonl' | 'csv' | 'xml' | 'pdf' | 'parquet';
    chunkStrategy?: 'fixed' | 'recursive' | 'semantic' | 'sentence' | 'paragraph';
    chunkSize?: number;
    chunkOverlap?: number;
    workers?: number;
    recursive?: boolean;
  }): Promise<{ 
    success: boolean; 
    documentsIngested: number; 
    chunksCreated: number;
    timeMs: number;
  }> {
    const formData = new FormData();
    formData.append('path', config.path || this.dbPath());
    
    if (typeof config.source === 'string') {
      formData.append('sourcePath', config.source);
    } else {
      formData.append('file', config.source);
    }
    
    formData.append('format', config.format || 'auto');
    formData.append('chunkStrategy', config.chunkStrategy || 'fixed');
    formData.append('chunkSize', String(config.chunkSize || 512));
    formData.append('chunkOverlap', String(config.chunkOverlap || 50));
    formData.append('workers', String(config.workers || 4));
    formData.append('recursive', String(config.recursive || false));

    return fetch(`${this.baseUrl()}/api/ingest`, {
      method: 'POST',
      body: formData
    }).then(r => r.json());
  }

  // ==================== HEALTH & MONITORING ====================

  /**
   * Get health status
   */
  async getHealthStatus(): Promise<HealthStatus> {
    return this.request('/api/health');
  }

  /**
   * Get cluster information
   */
  async getClusterInfo(): Promise<ClusterInfo> {
    return this.request('/api/cluster/info');
  }

  /**
   * Get real-time metrics
   */
  async getMetrics(): Promise<{
    qps: number;
    avgLatencyMs: number;
    p95LatencyMs: number;
    memoryUsageMb: number;
    cpuUsage: number;
  }> {
    return this.request('/api/metrics');
  }
}
