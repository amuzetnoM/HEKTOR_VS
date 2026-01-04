import { Injectable, signal, computed } from '@angular/core';

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
  cluster?: number; // For visualization coloring
}

export interface Collection {
  name: string;
  dimension: number;
  metric: string;
  documents: VectorDoc[];
  schema: SchemaField[];
}

@Injectable({
  providedIn: 'root'
})
export class VectorDbService {
  collections = signal<Collection[]>([
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

  stats = computed(() => {
    const cols = this.collections();
    const totalDocs = cols.reduce((acc, c) => acc + c.documents.length, 0);
    return {
      latency: '24ms',
      memoryUsage: `${(totalDocs * 1.5).toFixed(1)}MB`,
      totalDocs
    };
  });

  async createCollection(name: string, dimension = 1536, metric = 'cosine') {
    this.collections.update(cols => [...cols, {
      name,
      dimension,
      metric,
      documents: [],
      schema: []
    }]);
    return `Collection ${name} created.`;
  }

  async deleteCollection(name: string) {
    this.collections.update(cols => cols.filter(c => c.name !== name));
    return `Collection ${name} deleted.`;
  }

  async addDocuments(collectionName: string, docs: { content: string, metadata?: any }[]) {
    // Simulate embedding delay
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

  async query(collectionName: string, queryText: string, topK = 5) {
    await new Promise(r => setTimeout(r, 300));
    const col = this.collections().find(c => c.name === collectionName);
    if (!col) throw new Error('Collection not found');
    
    // Mock semantic search: just return random docs
    return col.documents
      .map(d => ({ ...d, score: Math.random() }))
      .sort((a, b) => b.score - a.score)
      .slice(0, topK);
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

  // --- Mock Data Generators ---

  private createSingleDoc(content: string, index: number, metadata: any = {}): VectorDoc {
    // Create random position
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
      
      // Add randomness around cluster center
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
        vector: [], // Stubbed
        projection: p2d,
        projection3d: p3d,
        cluster: clusterIdx
      };
    });
  }
}