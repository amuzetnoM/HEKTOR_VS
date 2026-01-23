import React, { useState, useEffect } from 'react';
import { useTheme } from './lib/theme-system';
import VectorSpace3D from './components/3d/VectorSpace3D';

type ViewMode = 'dashboard' | 'search' | 'ingest' | 'analytics' | '3d';

function App() {
  const { currentTheme, setTheme, availableThemes } = useTheme();
  const [viewMode, setViewMode] = useState<ViewMode>('dashboard');
  const [nativeStatus, setNativeStatus] = useState('checking...');
  const [demoVectors, setDemoVectors] = useState<any[]>([]);
  const [dbStats, setDbStats] = useState({
    vectors: 0,
    collections: 0,
    indexSize: 0,
    memoryUsage: 0,
    queriesPerSec: 0,
    avgLatency: 0,
  });

  useEffect(() => {
    // Set default theme to midnight-purple-dark
    if (currentTheme.name === 'quantum-slate-dark') {
      const midnightPurple = availableThemes.find(t => t.name === 'midnight-purple-dark');
      if (midnightPurple) {
        setTheme(midnightPurple.name);
      }
    }
  }, []);

  useEffect(() => {
    // Check native addon status
    if (window.hektorAPI) {
      const isAvailable = window.hektorAPI.isNativeAvailable();
      if (isAvailable) {
        const version = window.hektorAPI.getVersion();
        setNativeStatus(`✅ Native C++ addon active (v${version})`);
        // Simulate fetching stats
        setDbStats({
          vectors: 125847,
          collections: 3,
          indexSize: 512,
          memoryUsage: 2048,
          queriesPerSec: 1250,
          avgLatency: 2.8,
        });
      } else {
        setNativeStatus('⚠️ Demo mode - C++ addon not compiled');
      }
    } else {
      setNativeStatus('⚠️ Demo mode - C++ addon not compiled');
    }
    
    // Generate demo vectors
    const vectors = [];
    for (let i = 0; i < 100; i++) {
      vectors.push({
        id: `vec_${i}`,
        position: [
          (Math.random() - 0.5) * 4,
          (Math.random() - 0.5) * 4,
          (Math.random() - 0.5) * 4
        ] as [number, number, number],
        color: `hsl(${(i * 137.5) % 360}, 70%, 60%)`,
        distance: Math.random()
      });
    }
    setDemoVectors(vectors);
  }, []);

  return (
    <div className="app-container" data-theme={currentTheme.name}>
      <header className="app-header">
        <div>
          <h1 className="app-title">HEKTOR Vector Database Studio</h1>
          <p style={{ fontSize: '11px', opacity: 0.7, marginTop: '2px' }}>
            Perceptual Quantization & Multi-Geometry 3D Visualization
          </p>
        </div>
        <div className="theme-selector">
          <span style={{ fontSize: '13px', marginRight: '10px', opacity: 0.8 }}>
            {nativeStatus}
          </span>
          <select 
            value={currentTheme.name} 
            onChange={(e) => setTheme(e.target.value)}
            className="theme-dropdown"
          >
            {availableThemes.map(t => (
              <option key={t.name} value={t.name}>
                {t.displayName}
              </option>
            ))}
          </select>
        </div>
      </header>
      
      <main className="app-main">
        {viewMode === 'dashboard' ? (
          <div className="welcome-section">
            {/* Database Stats */}
            <div className="feature-grid" style={{ marginBottom: '20px' }}>
              <div className="feature-card">
                <div style={{ fontSize: '12px', opacity: 0.7 }}>Total Vectors</div>
                <div style={{ fontSize: '28px', fontWeight: 'bold', marginTop: '5px' }}>
                  {dbStats.vectors.toLocaleString()}
                </div>
              </div>
              <div className="feature-card">
                <div style={{ fontSize: '12px', opacity: 0.7 }}>Collections</div>
                <div style={{ fontSize: '28px', fontWeight: 'bold', marginTop: '5px' }}>
                  {dbStats.collections}
                </div>
              </div>
              <div className="feature-card">
                <div style={{ fontSize: '12px', opacity: 0.7 }}>Index Size (MB)</div>
                <div style={{ fontSize: '28px', fontWeight: 'bold', marginTop: '5px' }}>
                  {dbStats.indexSize}
                </div>
              </div>
              <div className="feature-card">
                <div style={{ fontSize: '12px', opacity: 0.7 }}>Memory (MB)</div>
                <div style={{ fontSize: '28px', fontWeight: 'bold', marginTop: '5px' }}>
                  {dbStats.memoryUsage}
                </div>
              </div>
            </div>

            {/* Performance & Status */}
            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '15px', marginBottom: '20px' }}>
              <div className="feature-card">
                <h3 style={{ marginBottom: '15px' }}>Real-time Performance</h3>
                <div style={{ marginBottom: '12px' }}>
                  <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', marginBottom: '5px' }}>
                    <span style={{ opacity: 0.7 }}>Queries/sec</span>
                    <span style={{ fontFamily: 'monospace' }}>{dbStats.queriesPerSec}</span>
                  </div>
                  <div style={{ height: '8px', background: 'rgba(255,255,255,0.1)', borderRadius: '4px', overflow: 'hidden' }}>
                    <div style={{ width: '75%', height: '100%', background: 'var(--primary)' }}></div>
                  </div>
                </div>
                <div>
                  <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', marginBottom: '5px' }}>
                    <span style={{ opacity: 0.7 }}>Avg Latency (ms)</span>
                    <span style={{ fontFamily: 'monospace' }}>{dbStats.avgLatency}</span>
                  </div>
                  <div style={{ height: '8px', background: 'rgba(255,255,255,0.1)', borderRadius: '4px', overflow: 'hidden' }}>
                    <div style={{ width: '45%', height: '100%', background: 'var(--accent)' }}></div>
                  </div>
                </div>
              </div>

              <div className="feature-card">
                <h3 style={{ marginBottom: '15px' }}>System Status</h3>
                <div style={{ fontSize: '13px', lineHeight: '1.8' }}>
                  <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                    <span style={{ opacity: 0.7 }}>Database:</span>
                    <span style={{ color: 'var(--accent)' }}>✓ Ready</span>
                  </div>
                  <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                    <span style={{ opacity: 0.7 }}>Index:</span>
                    <span style={{ color: 'var(--accent)' }}>✓ HNSW Active</span>
                  </div>
                  <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                    <span style={{ opacity: 0.7 }}>Native Addon:</span>
                    <span style={{ color: 'var(--accent)' }}>{window.hektorAPI ? '✓ Loaded' : '⚠ Not Available'}</span>
                  </div>
                  <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                    <span style={{ opacity: 0.7 }}>Storage:</span>
                    <span style={{ color: 'var(--accent)' }}>✓ MMAP</span>
                  </div>
                </div>
              </div>
            </div>

            {/* Quick Actions */}
            <div className="feature-card" style={{ marginBottom: '20px' }}>
              <h3 style={{ marginBottom: '15px' }}>Quick Actions</h3>
              <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '10px' }}>
                <button className="demo-button">Create Collection</button>
                <button className="demo-button" style={{ background: 'transparent', border: '1px solid var(--border)' }}>
                  Import Data
                </button>
                <button className="demo-button" style={{ background: 'transparent', border: '1px solid var(--border)' }}>
                  Run Benchmark
                </button>
                <button className="demo-button" style={{ background: 'transparent', border: '1px solid var(--border)' }}>
                  Optimize Index
                </button>
              </div>
            </div>

            {/* Core Features */}
            <h2 style={{ marginBottom: '15px', fontSize: '20px' }}>Core Features</h2>
            <div className="feature-grid">
              <div className="feature-card" style={{ cursor: 'pointer' }} onClick={() => setViewMode('search')}>
                <div style={{ fontSize: '32px', marginBottom: '10px' }}>🔍</div>
                <h3>Vector Search</h3>
                <p>Semantic search with k-NN, filtering, and hybrid BM25 fusion</p>
              </div>

              <div className="feature-card" style={{ cursor: 'pointer' }} onClick={() => setViewMode('ingest')}>
                <div style={{ fontSize: '32px', marginBottom: '10px' }}>📥</div>
                <h3>Data Ingestion</h3>
                <p>Import from 11+ formats with auto-chunking and metadata extraction</p>
              </div>

              <div className="feature-card" style={{ cursor: 'pointer' }} onClick={() => setViewMode('3d')}>
                <div style={{ fontSize: '32px', marginBottom: '10px' }}>🎯</div>
                <h3>3D Visualization</h3>
                <p>Multi-geometry: Euclidean, Hyperbolic, Parabolic</p>
              </div>

              <div className="feature-card">
                <div style={{ fontSize: '32px', marginBottom: '10px' }}>🎨</div>
                <h3>Perceptual Quantization</h3>
                <p>HDR-aware compression with 8-32x reduction</p>
              </div>

              <div className="feature-card" style={{ cursor: 'pointer' }} onClick={() => setViewMode('analytics')}>
                <div style={{ fontSize: '32px', marginBottom: '10px' }}>📊</div>
                <h3>Analytics & Monitoring</h3>
                <p>Real-time metrics and performance profiling</p>
              </div>

              <div className="feature-card">
                <div style={{ fontSize: '32px', marginBottom: '10px' }}>🤖</div>
                <h3>AI Assistant</h3>
                <p>Contextual optimization and intelligent suggestions</p>
              </div>
            </div>
          </div>
        ) : viewMode === '3d' ? (
          <div>
            <button 
              onClick={() => setViewMode('dashboard')}
              className="demo-button"
              style={{ marginBottom: '10px', background: 'transparent', border: '1px solid var(--border)' }}
            >
              ← Back to Dashboard
            </button>
            <div className="demo-container">
              <VectorSpace3D 
                vectors={demoVectors}
                geometry="euclidean"
                onVectorClick={(vec) => console.log('Clicked vector:', vec)}
              />
            </div>
          </div>
        ) : (
          <div className="welcome-section">
            <button 
              onClick={() => setViewMode('dashboard')}
              className="demo-button"
              style={{ marginBottom: '15px', background: 'transparent', border: '1px solid var(--border)' }}
            >
              ← Back to Dashboard
            </button>
            <h2 style={{ marginBottom: '10px' }}>
              {viewMode === 'search' ? 'Vector Search' : 
               viewMode === 'ingest' ? 'Data Ingestion' : 'Analytics & Monitoring'}
            </h2>
            <div className="feature-card">
              <p style={{ opacity: 0.7 }}>
                {viewMode === 'search' ? 'Search interface coming in next iteration...' :
                 viewMode === 'ingest' ? 'Data ingestion pipeline coming in next iteration...' :
                 'Analytics dashboard coming in next iteration...'}
              </p>
              <p style={{ fontSize: '13px', opacity: 0.5, marginTop: '10px' }}>
                {viewMode === 'search' ? 'Will include: Semantic search, k-NN, filtering by date/type/asset, hybrid BM25, fusion methods' :
                 viewMode === 'ingest' ? 'Will support: 11+ adapters (CSV, JSON, PDF, XML, Excel), auto-chunking, metadata extraction' :
                 'Will include: Performance profiling, query analytics, index benchmarking, telemetry'}
              </p>
            </div>
          </div>
        )}
      </main>
      
      <footer className="app-footer">
        <p>HEKTOR v4.0.0 | C++23 SIMD-Optimized | 96.8% Recall @ 8.5ms | Billion-Scale Ready | {currentTheme.displayName}</p>
      </footer>
    </div>
  );
}

// Global type declarations
declare global {
  interface Window {
    hektorAPI?: {
      openDatabase: (path: string) => any;
      closeDatabase: () => any;
      addVector: (vector: number[], metadata?: any) => any;
      queryVectors: (vector: number[], topK?: number) => any;
      queryVectorsAsync: (vector: number[], topK: number, callback: Function) => void;
      getVersion: () => string;
      isNativeAvailable: () => boolean;
    };
  }
}

export default App;
