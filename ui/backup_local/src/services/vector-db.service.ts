import { Injectable, signal, computed } from '@angular/core';
import { GoogleGenAI } from '@google/genai';
import { Collection, VectorDoc, SchemaField, IngestionConfig, SearchParams, EMBEDDING_MODELS } from '../models/core';
import { Subject } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class VectorDbService {
  // Real State - Starts Empty
  collections = signal<Collection[]>([]);
  
  // Telemetry Stream for Health Monitor
  telemetry$ = new Subject<{ type: 'latency' | 'throughput', value: number }>();

  private ai: GoogleGenAI;

  stats = computed(() => {
    const cols = this.collections();
    const totalDocs = cols.reduce((acc, c) => acc + c.documents.length, 0);
    // Rough estimate: 8 bytes per float, 768 dims approx 6KB per vector + content overhead
    const size = (totalDocs * 0.01).toFixed(2); 
    return {
      latency: '0ms', // This is now dynamic in the components via telemetry$
      memoryUsage: `${size}MB`,
      totalDocs
    };
  });

  constructor() {
    const apiKey = process.env['API_KEY'] || '';
    this.ai = new GoogleGenAI({ apiKey });
  }

  async createCollection(name: string, dimension = 768, metric: 'cosine' | 'euclidean' | 'dot' = 'cosine') {
    if (this.collections().some(c => c.name === name)) {
      throw new Error(`Collection ${name} already exists.`);
    }

    // Default to Gemini Embedding 004 if 768, else try to match or default
    let model = 'text-embedding-004';
    if (dimension === 1536) model = 'text-embedding-3-small';

    this.collections.update(cols => [...cols, {
      name,
      dimension,
      metric,
      embeddingModel: model,
      documents: [],
      schema: [],
      created_at: Date.now()
    }]);
    return `Collection ${name} created.`;
  }

  async deleteCollection(name: string) {
    this.collections.update(cols => cols.filter(c => c.name !== name));
    return `Collection ${name} deleted.`;
  }

  /**
   * Real Data Ingestion
   * 1. Reads files
   * 2. Chunks text
   * 3. Generates Embeddings via API
   * 4. Stores in memory
   */
  async ingestData(config: IngestionConfig): Promise<string> {
    const col = this.collections().find(c => c.name === config.collectionName);
    if (!col) throw new Error(`Collection ${config.collectionName} not found.`);

    let totalChunks = 0;
    const startTime = Date.now();

    for (const file of config.files) {
      const text = await this.readFileContent(file);
      const chunks = this.chunkText(text, config.chunkSize, config.chunkOverlap);
      totalChunks += chunks.length;

      // Process in batches to respect rate limits and show progress
      const batchSize = 5;
      for (let i = 0; i < chunks.length; i += batchSize) {
        const batch = chunks.slice(i, i + batchSize);
        const batchStart = Date.now();
        
        const docs: VectorDoc[] = await Promise.all(batch.map(async (chunk, idx) => {
          const vector = await this.generateEmbedding(chunk, col.embeddingModel);
          return {
            id: `doc_${Date.now()}_${i + idx}`,
            content: chunk,
            metadata: { source: file.name, ingested_at: new Date().toISOString() },
            vector: vector,
            // Simple projection: Map first 3 dims to 3D space (Real vectors are high-dim, this is a visualization heuristic)
            projection: [vector[0] * 100, vector[1] * 100], 
            projection3d: [vector[0] * 50, vector[1] * 50, vector[2] * 50],
            cluster: 0 // In a real app, we'd run K-Means here
          };
        }));

        this.addDocsToCollection(config.collectionName, docs);
        
        // Emit Telemetry
        const latency = Date.now() - batchStart;
        this.telemetry$.next({ type: 'latency', value: latency / batch.length });
        this.telemetry$.next({ type: 'throughput', value: batch.length });
      }
    }

    const duration = (Date.now() - startTime) / 1000;
    return `Ingested ${totalChunks} chunks in ${duration.toFixed(1)}s.`;
  }

  async addDocuments(collectionName: string, docs: { content: string, metadata?: any }[]) {
    const col = this.collections().find(c => c.name === collectionName);
    if (!col) throw new Error('Collection not found');

    const vectorDocs: VectorDoc[] = [];
    const start = Date.now();

    for (const d of docs) {
      const vector = await this.generateEmbedding(d.content, col.embeddingModel);
      vectorDocs.push({
        id: `doc_${Date.now()}_${Math.random().toString(36).substr(2, 5)}`,
        content: d.content,
        metadata: d.metadata || {},
        vector: vector,
        projection: [vector[0] * 100, vector[1] * 100],
        projection3d: [vector[0] * 50, vector[1] * 50, vector[2] * 50],
        cluster: 0
      });
    }

    this.addDocsToCollection(collectionName, vectorDocs);
    
    this.telemetry$.next({ type: 'latency', value: (Date.now() - start) / docs.length });
    this.telemetry$.next({ type: 'throughput', value: docs.length });

    return `Added ${docs.length} documents.`;
  }

  /**
   * Real Vector Search (Cosine Similarity)
   */
  async query(collectionName: string, params: SearchParams) {
    const col = this.collections().find(c => c.name === collectionName);
    if (!col) throw new Error('Collection not found');
    if (col.documents.length === 0) return [];

    const start = Date.now();

    // 1. Embed Query
    const queryVector = await this.generateEmbedding(params.query, col.embeddingModel);

    // 2. Filter Candidates (Metadata)
    let candidates = col.documents;
    if (params.filters && Object.keys(params.filters).length > 0) {
      candidates = candidates.filter(doc => {
        return Object.entries(params.filters!).every(([key, val]) => {
           return doc.metadata[key] == val || JSON.stringify(doc.metadata[key]).includes(String(val));
        });
      });
    }

    // 3. Vector Similarity
    const results = candidates.map(doc => {
      if (!doc.vector) return { ...doc, score: 0 };
      const score = this.cosineSimilarity(queryVector, doc.vector);
      return { ...doc, score };
    });

    // 4. Sort and TopK
    const final = results
      .filter(d => d.score >= params.minScore)
      .sort((a, b) => b.score - a.score)
      .slice(0, params.topK);

    this.telemetry$.next({ type: 'latency', value: Date.now() - start });

    return final;
  }

  updateSchema(collectionName: string, schema: SchemaField[]) {
    this.collections.update(cols => cols.map(c => {
      if (c.name === collectionName) return { ...c, schema };
      return c;
    }));
  }

  reorderCollections(fromIndex: number, toIndex: number) {
    this.collections.update(cols => {
      const newCols = [...cols];
      const [moved] = newCols.splice(fromIndex, 1);
      newCols.splice(toIndex, 0, moved);
      return newCols;
    });
  }

  // --- Private Helpers ---

  private addDocsToCollection(name: string, newDocs: VectorDoc[]) {
    this.collections.update(cols => cols.map(c => {
      if (c.name === name) {
        return { ...c, documents: [...c.documents, ...newDocs] };
      }
      return c;
    }));
  }

  private async generateEmbedding(text: string, model: string): Promise<number[]> {
    try {
      // Use standard Embedding 004 for better performance
      const response = await this.ai.models.embedContent({
        model: 'text-embedding-004',
        content: { parts: [{ text }] }
      });
      
      if (response.embedding && response.embedding.values) {
        return response.embedding.values;
      }
      // Fallback if model fails (rare) or returns empty
      console.warn('Embedding failed, returning zero vector');
      return new Array(768).fill(0);
    } catch (e) {
      console.error('Embedding API Error:', e);
      throw new Error('Failed to generate embedding. Please check API Key.');
    }
  }

  private cosineSimilarity(vecA: number[], vecB: number[]): number {
    const dotProduct = vecA.reduce((sum, a, i) => sum + a * vecB[i], 0);
    const magA = Math.sqrt(vecA.reduce((sum, a) => sum + a * a, 0));
    const magB = Math.sqrt(vecB.reduce((sum, b) => sum + b * b, 0));
    if (magA === 0 || magB === 0) return 0;
    return dotProduct / (magA * magB);
  }

  private chunkText(text: string, size: number, overlap: number): string[] {
    const chunks: string[] = [];
    // Naive split by words/characters. In prod, use a tokenizer.
    // Approximating 1 token ~= 4 chars
    const charSize = size * 4;
    const charOverlap = overlap * 4;
    
    let start = 0;
    while (start < text.length) {
      const end = Math.min(start + charSize, text.length);
      chunks.push(text.slice(start, end));
      start += charSize - charOverlap;
    }
    return chunks;
  }

  private readFileContent(file: File): Promise<string> {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = (e) => resolve(e.target?.result as string);
      reader.onerror = (e) => reject(e);
      reader.readAsText(file);
    });
  }
}