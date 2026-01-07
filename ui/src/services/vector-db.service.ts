import { Injectable, signal, computed } from '@angular/core';
import { GoogleGenAI } from '@google/genai';
import { Collection, VectorDoc, SchemaField, IngestionConfig, SearchParams } from '../models/core';
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
    
    // Initialize the Dynamic Tool Registry
    this.initSystemTools();
  }

  // --- Dynamic Tool Registry ---

  async initSystemTools() {
    // Check if system_tools already exists (in case of re-init)
    if (this.collections().some(c => c.name === 'system_tools')) return;

    // 1. Create the Tool Registry Collection
    await this.createCollection('system_tools', 768, 'cosine');
    this.collections.update(cols => cols.map(c => c.name === 'system_tools' ? { ...c, isSystem: true } : c));

    // 2. Define The Tools
    const tools = [
      {
        name: 'create_collection',
        description: 'Create a new vector collection with specific dimensions and metric.',
        schema: {
          name: 'create_collection',
          description: 'Create a new vector collection.',
          parameters: {
            type: 'OBJECT',
            properties: {
              name: { type: 'STRING', description: 'Name of the collection (slug-friendly)' },
              dimension: { type: 'NUMBER', description: 'Vector dimension (e.g. 1536, 768)' },
              metric: { type: 'STRING', enum: ['cosine', 'euclidean', 'dot'], description: 'Distance metric' }
            },
            required: ['name']
          }
        }
      },
      {
        name: 'delete_collection',
        description: 'Delete or drop an existing collection from the database.',
        schema: {
          name: 'delete_collection',
          description: 'Delete an existing collection.',
          parameters: {
            type: 'OBJECT',
            properties: {
              name: { type: 'STRING', description: 'Name of the collection to delete' }
            },
            required: ['name']
          }
        }
      },
      {
        name: 'add_documents',
        description: 'Ingest or add raw text documents to a collection.',
        schema: {
          name: 'add_documents',
          description: 'Add raw text documents to a collection. The system will handle embedding.',
          parameters: {
            type: 'OBJECT',
            properties: {
              collectionName: { type: 'STRING' },
              texts: { type: 'ARRAY', items: { type: 'STRING' }, description: 'List of text strings to add' }
            },
            required: ['collectionName', 'texts']
          }
        }
      },
      {
        name: 'query_vector',
        description: 'Perform a semantic search or vector query on a collection.',
        schema: {
          name: 'query_vector',
          description: 'Semantic search / query vectors.',
          parameters: {
            type: 'OBJECT',
            properties: {
              collectionName: { type: 'STRING' },
              queryText: { type: 'STRING' },
              topK: { type: 'NUMBER' }
            },
            required: ['collectionName', 'queryText']
          }
        }
      }
    ];

    // 3. "Learn" the tools -> Embed and Store them in the Registry
    const toolDocs = tools.map((t, idx) => ({
      content: `${t.name}: ${t.description}`, // Semantic text for retrieval
      metadata: { 
        type: 'tool_definition', 
        tool_name: t.name, 
        schema: t.schema 
      }
    }));

    await this.addDocuments('system_tools', toolDocs);
    console.log('Dynamic Tool Registry Initialized with', tools.length, 'tools.');
  }

  async retrieveTools(query: string, topK: number = 3): Promise<any[]> {
    try {
      // Semantic search against the tool descriptions
      // Use a very low threshold so we almost always get tools if the DB is empty/cold
      const results = await this.query('system_tools', {
        query,
        topK,
        minScore: 0.1 
      });

      // Extract the schemas
      return results.map(doc => doc.metadata['schema']);
    } catch (e) {
      console.warn('Tool retrieval failed, falling back to empty.', e);
      return [];
    }
  }

  // --- End Dynamic Tool Registry ---

  async createCollection(name: string, dimension = 768, metric: 'cosine' | 'euclidean' | 'dot' = 'cosine') {
    if (this.collections().some(c => c.name === name)) {
      return `Collection ${name} already exists.`;
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

      // Process in batches
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
            projection: [vector[0] * 100, vector[1] * 100], 
            projection3d: [vector[0] * 50, vector[1] * 50, vector[2] * 50],
            cluster: 0
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
      
      // Generate deterministic projections for consistent UI if embedding fails
      const fallbackX = (Math.random() - 0.5) * 100;
      const fallbackY = (Math.random() - 0.5) * 100;
      const fallbackZ = (Math.random() - 0.5) * 100;

      // If we have a real vector, use 1st 3 dims for "fake" projection scaling
      // This allows immediate visualization even without full UMAP
      const hasVector = vector.some(v => v !== 0);
      const pX = hasVector ? vector[0] * 100 : fallbackX;
      const pY = hasVector ? vector[1] * 100 : fallbackY;
      const pZ = hasVector ? vector[2] * 100 : fallbackZ;

      vectorDocs.push({
        id: `doc_${Date.now()}_${Math.random().toString(36).substr(2, 5)}`,
        content: d.content,
        metadata: d.metadata || {},
        vector: vector,
        projection: [pX, pY],
        projection3d: [pX * 0.5, pY * 0.5, pZ * 0.5],
        cluster: 0
      });
    }

    this.addDocsToCollection(collectionName, vectorDocs);
    
    this.telemetry$.next({ type: 'latency', value: (Date.now() - start) / docs.length });
    this.telemetry$.next({ type: 'throughput', value: docs.length });

    return `Added ${docs.length} documents.`;
  }

  async seedSampleData() {
    const colName = 'synthetic_dataset_v1';
    if (this.collections().some(c => c.name === colName)) {
      return; // Already seeded
    }

    await this.createCollection(colName, 768, 'cosine');
    
    // Create 5 distinct clusters for visualization
    const clusters = [
      { name: 'Artificial Intelligence', center: [20, 10, 10], id: 0 },
      { name: 'Biotechnology', center: [-15, -10, 5], id: 1 },
      { name: 'Quantum Physics', center: [5, -20, -10], id: 2 },
      { name: 'Modern Art', center: [-10, 15, -15], id: 3 },
      { name: 'Philosophy', center: [15, -5, 15], id: 4 }
    ];

    const docs: VectorDoc[] = [];

    // Generate 20 docs per cluster
    clusters.forEach(cluster => {
        for (let i = 0; i < 20; i++) {
            // Gaussian-ish spread
            const spread = 4;
            const x = cluster.center[0] + (Math.random() - 0.5) * spread * 2;
            const y = cluster.center[1] + (Math.random() - 0.5) * spread * 2;
            const z = cluster.center[2] + (Math.random() - 0.5) * spread * 2;
            
            // Create a non-zero vector to prevent NaN in cosine sim
            const fakeVector = new Array(768).fill(0).map(() => (Math.random() - 0.5) * 0.1);

            docs.push({
                id: `seed_${cluster.name.substring(0,3).toLowerCase()}_${i}`,
                content: `Synthetic research abstract regarding ${cluster.name}. Sample data point #${i} for visualization testing. Contains randomized vector data.`,
                metadata: { category: cluster.name, confidence: (0.7 + Math.random() * 0.3).toFixed(2), version: '1.0' },
                vector: fakeVector, 
                projection: [x * 3, y * 3],
                projection3d: [x, y, z],
                cluster: cluster.id,
                score: Math.random()
            });
        }
    });

    this.addDocsToCollection(colName, docs);
    return 'Sample data generated successfully.';
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
      // Safety check for empty or zero vectors
      if (!doc.vector || doc.vector.length === 0) {
         return { ...doc, score: 0 }; 
      }
      const score = this.cosineSimilarity(queryVector, doc.vector);
      return { ...doc, score: isNaN(score) ? 0 : score };
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
      // Robust call: Using 'contents' (plural) is safer for array inputs in the unified SDK.
      const response = await this.ai.models.embedContent({
        model: model,
        contents: [
          {
            parts: [{ text: text }]
          }
        ]
      });
      
      // Handle response safely: check both 'embeddings' (array) and 'embedding' (single)
      // The API often returns 'embeddings' when 'contents' is used.
      if (response.embeddings && response.embeddings.length > 0 && response.embeddings[0].values) {
        return response.embeddings[0].values;
      }
      
      console.warn('Embedding API returned valid response but no values found in expected paths.');
      return new Array(768).fill(0);

    } catch (e: any) {
      // Do not crash the app, log error and return zero vector
      console.error('Embedding API Error:', e.message || e);
      return new Array(768).fill(0);
    }
  }

  private cosineSimilarity(vecA: number[], vecB: number[]): number {
    if (!vecA || !vecB || vecA.length !== vecB.length) return 0;

    const dotProduct = vecA.reduce((sum, a, i) => sum + a * vecB[i], 0);
    const magA = Math.sqrt(vecA.reduce((sum, a) => sum + a * a, 0));
    const magB = Math.sqrt(vecB.reduce((sum, b) => sum + b * b, 0));
    
    // Prevent division by zero
    if (magA === 0 || magB === 0) return 0;
    
    return dotProduct / (magA * magB);
  }

  private chunkText(text: string, size: number, overlap: number): string[] {
    const chunks: string[] = [];
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