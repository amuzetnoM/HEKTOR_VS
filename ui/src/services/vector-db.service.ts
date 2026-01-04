import { Injectable, signal, computed, inject } from '@angular/core';
import { HttpClient, HttpHeaders, HttpErrorResponse } from '@angular/common/http';
import { firstValueFrom, catchError, throwError } from 'rxjs';

// ============================================================================
// Configuration
// ============================================================================

const API_BASE_URL = 'http://localhost:8080';
const USE_MOCK_MODE = false; // Set to true for demo mode without backend

// ============================================================================
// Interfaces
// ============================================================================

export interface SchemaField {
  name: string;
  type: 'string' | 'number' | 'boolean' | 'date' | 'list';
  required: boolean;
}

export interface VectorDoc {
  id: string;
  content: string;
  metadata: Record<string, any>;
  vector: number[]; 
  projection: [number, number]; 
  projection3d: [number, number, number];
  cluster?: number;
  score?: number; // For search results
}

export interface Collection {
  name: string;
  dimension: number;
  metric: string;
  documents: VectorDoc[];
  schema: SchemaField[];
  document_count?: number;
  created_at?: string;
}

export interface SearchResult {
  id: string;
  score: number;
  content?: string;
  metadata: Record<string, any>;
}

export interface ApiStats {
  total_vectors: number;
  memory_usage_bytes: number;
  index_size: number;
  collections: number;
}

// ============================================================================
// Service
// ============================================================================

@Injectable({
  providedIn: 'root'
})
export class VectorDbService {
  private http = inject(HttpClient);
  private authToken = signal<string | null>(null);
  
  // State
  collections = signal<Collection[]>([]);
  isLoading = signal<boolean>(false);
  error = signal<string | null>(null);
  
  // Computed stats
  stats = computed(() => {
    const cols = this.collections();
    const totalDocs = cols.reduce((acc, c) => acc + c.documents.length, 0);
    return {
      latency: '24ms',
      memoryUsage: `${(totalDocs * 1.5).toFixed(1)}MB`,
      totalDocs
    };
  });

  constructor() {
    // Load auth token from localStorage
    const savedToken = localStorage.getItem('vdb_auth_token');
    if (savedToken) {
      this.authToken.set(savedToken);
    }
    
    // Initialize with mock data if in demo mode
    if (USE_MOCK_MODE) {
      this.initializeMockData();
    } else {
      // Load collections from backend
      this.loadCollections();
    }
  }

  // ============================================================================
  // Authentication
  // ============================================================================

  async login(username: string, password: string): Promise<void> {
    try {
      const response = await firstValueFrom(
        this.http.post<{ access_token: string; token_type: string }>(
          `${API_BASE_URL}/auth/login`,
          { username, password }
        ).pipe(catchError(this.handleError))
      );
      
      this.authToken.set(response.access_token);
      localStorage.setItem('vdb_auth_token', response.access_token);
      this.error.set(null);
    } catch (err) {
      this.error.set('Login failed: ' + (err as Error).message);
      throw err;
    }
  }

  logout(): void {
    this.authToken.set(null);
    localStorage.removeItem('vdb_auth_token');
  }

  // ============================================================================
  // HTTP Helpers
  // ============================================================================

  private getHeaders(): HttpHeaders {
    let headers = new HttpHeaders({
      'Content-Type': 'application/json'
    });
    
    const token = this.authToken();
    if (token) {
      headers = headers.set('Authorization', `Bearer ${token}`);
    }
    
    return headers;
  }

  private handleError(error: HttpErrorResponse) {
    let errorMessage = 'An error occurred';
    
    if (error.error instanceof ErrorEvent) {
      // Client-side error
      errorMessage = `Error: ${error.error.message}`;
    } else {
      // Server-side error
      errorMessage = `Error ${error.status}: ${error.error?.detail || error.message}`;
    }
    
    console.error(errorMessage);
    return throwError(() => new Error(errorMessage));
  }

  // ============================================================================
  // Collection Operations
  // ============================================================================

  async loadCollections(): Promise<void> {
    if (USE_MOCK_MODE) {
      this.initializeMockData();
      return;
    }

    this.isLoading.set(true);
    this.error.set(null);

    try {
      const response = await firstValueFrom(
        this.http.get<Collection[]>(
          `${API_BASE_URL}/collections`,
          { headers: this.getHeaders() }
        ).pipe(catchError(this.handleError))
      );
      
      // Enrich collections with empty documents array if not present
      const enrichedCollections = response.map(col => ({
        ...col,
        documents: col.documents || [],
        schema: col.schema || []
      }));
      
      this.collections.set(enrichedCollections);
    } catch (err) {
      this.error.set('Failed to load collections: ' + (err as Error).message);
      // Fallback to mock data on error
      this.initializeMockData();
    } finally {
      this.isLoading.set(false);
    }
  }

  async createCollection(name: string, dimension = 1536, metric = 'cosine'): Promise<string> {
    if (USE_MOCK_MODE) {
      return this.createCollectionMock(name, dimension, metric);
    }

    this.isLoading.set(true);
    this.error.set(null);

    try {
      const response = await firstValueFrom(
        this.http.post<Collection>(
          `${API_BASE_URL}/collections`,
          { name, dimension, metric },
          { headers: this.getHeaders() }
        ).pipe(catchError(this.handleError))
      );
      
      // Add to local state
      this.collections.update(cols => [...cols, {
        ...response,
        documents: [],
        schema: []
      }]);
      
      return `Collection ${name} created successfully.`;
    } catch (err) {
      this.error.set('Failed to create collection: ' + (err as Error).message);
      throw err;
    } finally {
      this.isLoading.set(false);
    }
  }

  async deleteCollection(name: string): Promise<string> {
    if (USE_MOCK_MODE) {
      return this.deleteCollectionMock(name);
    }

    this.isLoading.set(true);
    this.error.set(null);

    try {
      await firstValueFrom(
        this.http.delete(
          `${API_BASE_URL}/collections/${name}`,
          { headers: this.getHeaders() }
        ).pipe(catchError(this.handleError))
      );
      
      // Remove from local state
      this.collections.update(cols => cols.filter(c => c.name !== name));
      
      return `Collection ${name} deleted successfully.`;
    } catch (err) {
      this.error.set('Failed to delete collection: ' + (err as Error).message);
      throw err;
    } finally {
      this.isLoading.set(false);
    }
  }

  // ============================================================================
  // Document Operations
  // ============================================================================

  async addDocuments(
    collectionName: string,
    docs: { content: string; metadata?: any }[]
  ): Promise<string> {
    if (USE_MOCK_MODE) {
      return this.addDocumentsMock(collectionName, docs);
    }

    this.isLoading.set(true);
    this.error.set(null);

    try {
      const response = await firstValueFrom(
        this.http.post<{ ids: string[]; count: number; message: string }>(
          `${API_BASE_URL}/collections/${collectionName}/documents/batch`,
          { documents: docs },
          { headers: this.getHeaders() }
        ).pipe(catchError(this.handleError))
      );
      
      // Reload collection to get updated documents
      await this.loadCollections();
      
      return response.message;
    } catch (err) {
      this.error.set('Failed to add documents: ' + (err as Error).message);
      throw err;
    } finally {
      this.isLoading.set(false);
    }
  }

  // ============================================================================
  // Search Operations
  // ============================================================================

  async query(collectionName: string, queryText: string, topK = 5): Promise<VectorDoc[]> {
    if (USE_MOCK_MODE) {
      return this.queryMock(collectionName, queryText, topK);
    }

    this.isLoading.set(true);
    this.error.set(null);

    try {
      const response = await firstValueFrom(
        this.http.post<SearchResult[]>(
          `${API_BASE_URL}/collections/${collectionName}/search`,
          { query: queryText, k: topK },
          { headers: this.getHeaders() }
        ).pipe(catchError(this.handleError))
      );
      
      // Convert search results to VectorDoc format
      return response.map(r => ({
        id: r.id,
        content: r.content || '',
        metadata: r.metadata,
        vector: [],
        projection: [Math.random() * 800 - 400, Math.random() * 600 - 300] as [number, number],
        projection3d: [
          (Math.random() - 0.5) * 100,
          (Math.random() - 0.5) * 100,
          (Math.random() - 0.5) * 100
        ] as [number, number, number],
        score: r.score
      }));
    } catch (err) {
      this.error.set('Search failed: ' + (err as Error).message);
      throw err;
    } finally {
      this.isLoading.set(false);
    }
  }

  // ============================================================================
  // Schema Operations
  // ============================================================================

  updateSchema(collectionName: string, schema: SchemaField[]): void {
    this.collections.update(cols => cols.map(c => {
      if (c.name === collectionName) return { ...c, schema };
      return c;
    }));
  }

  reorderCollections(fromIndex: number, toIndex: number): void {
    this.collections.update(cols => {
      const newCols = [...cols];
      const [moved] = newCols.splice(fromIndex, 1);
      newCols.splice(toIndex, 0, moved);
      return newCols;
    });
  }

  // ============================================================================
  // Stats
  // ============================================================================

  async getApiStats(): Promise<ApiStats> {
    if (USE_MOCK_MODE) {
      return {
        total_vectors: this.stats().totalDocs,
        memory_usage_bytes: parseFloat(this.stats().memoryUsage) * 1024 * 1024,
        index_size: 0,
        collections: this.collections().length
      };
    }

    try {
      return await firstValueFrom(
        this.http.get<ApiStats>(
          `${API_BASE_URL}/stats`,
          { headers: this.getHeaders() }
        ).pipe(catchError(this.handleError))
      );
    } catch (err) {
      this.error.set('Failed to get stats: ' + (err as Error).message);
      throw err;
    }
  }

  // ============================================================================
  // Mock Mode Implementations (Fallback)
  // ============================================================================

  private initializeMockData(): void {
    this.collections.set([
      {
        name: 'knowledge-base-v1',
        dimension: 1536,
        metric: 'cosine',
        schema: [
          { name: 'source', type: 'string', required: true },
          { name: 'created_at', type: 'date', required: true }
        ],
        documents: this.generateMockDocs(150)
      },
      {
        name: 'user-profiles',
        dimension: 768,
        metric: 'euclidean',
        schema: [
          { name: 'user_id', type: 'string', required: true },
          { name: 'interests', type: 'list', required: false }
        ],
        documents: this.generateMockDocs(50)
      }
    ]);
  }

  private async createCollectionMock(name: string, dimension: number, metric: string): Promise<string> {
    this.collections.update(cols => [...cols, {
      name,
      dimension,
      metric,
      documents: [],
      schema: []
    }]);
    return `Collection ${name} created.`;
  }

  private async deleteCollectionMock(name: string): Promise<string> {
    this.collections.update(cols => cols.filter(c => c.name !== name));
    return `Collection ${name} deleted.`;
  }

  private async addDocumentsMock(
    collectionName: string,
    docs: { content: string; metadata?: any }[]
  ): Promise<string> {
    await new Promise(r => setTimeout(r, 800));
    
    this.collections.update(cols => cols.map(c => {
      if (c.name === collectionName) {
        const newDocs = docs.map((d, i) => this.createSingleDoc(d.content, i, d.metadata));
        return { ...c, documents: [...c.documents, ...newDocs] };
      }
      return c;
    }));
    return `Added ${docs.length} documents to ${collectionName}.`;
  }

  private async queryMock(collectionName: string, queryText: string, topK: number): Promise<VectorDoc[]> {
    await new Promise(r => setTimeout(r, 300));
    const col = this.collections().find(c => c.name === collectionName);
    if (!col) throw new Error('Collection not found');
    
    return col.documents
      .map(d => ({ ...d, score: Math.random() }))
      .sort((a, b) => (b.score || 0) - (a.score || 0))
      .slice(0, topK);
  }

  private createSingleDoc(content: string, index: number, metadata: any = {}): VectorDoc {
    return {
      id: `vec_${Math.random().toString(36).substr(2, 9)}`,
      content: content,
      metadata: metadata || { source: 'user_upload', created_at: new Date().toISOString() },
      vector: [],
      projection: [Math.random() * 800 - 400, Math.random() * 600 - 300],
      projection3d: [
        (Math.random() - 0.5) * 100,
        (Math.random() - 0.5) * 100,
        (Math.random() - 0.5) * 100
      ],
      cluster: 0
    };
  }

  private generateMockDocs(count: number): VectorDoc[] {
    const clusters = [
      { center: [200, 150], center3d: [30, 30, 30], label: "Technical Docs" },
      { center: [-200, -100], center3d: [-30, -20, 10], label: "Marketing" },
      { center: [50, -200], center3d: [10, -40, -30], label: "Legal" },
      { center: [-150, 200], center3d: [-40, 40, -10], label: "Support" }
    ];

    const contents = [
      "Neural networks architecture optimization",
      "Q3 Marketing strategy and budget allocation",
      "Terms of service and privacy policy updates",
      "Customer support ticket: Login issues",
      "React vs Angular performance benchmarks",
      "Social media campaign for product launch",
      "GDPR compliance checklist 2024",
      "Password reset workflow troubleshooting",
      "Transformer models in NLP applications",
      "Brand identity guidelines v2.0"
    ];

    return Array.from({ length: count }, (_, i) => {
      const clusterIdx = Math.floor(Math.random() * clusters.length);
      const cluster = clusters[clusterIdx];
      
      const spread2d = 100;
      const spread3d = 25;

      const p2d: [number, number] = [
        cluster.center[0] + (Math.random() - 0.5) * spread2d,
        cluster.center[1] + (Math.random() - 0.5) * spread2d
      ];

      const p3d: [number, number, number] = [
        cluster.center3d[0] + (Math.random() - 0.5) * spread3d,
        cluster.center3d[1] + (Math.random() - 0.5) * spread3d,
        cluster.center3d[2] + (Math.random() - 0.5) * spread3d
      ];

      return {
        id: `vec_${Math.random().toString(36).substr(2, 9)}`,
        content: contents[Math.floor(Math.random() * contents.length)] + ` (Ref: ${i})`,
        metadata: { 
            category: cluster.label,
            timestamp: Date.now() - Math.floor(Math.random() * 10000000)
        },
        vector: [],
        projection: p2d,
        projection3d: p3d,
        cluster: clusterIdx
      };
    });
  }
}