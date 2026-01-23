import React, { useState, useEffect } from 'react';
import { useTheme } from './lib/theme-system';
import VectorSpace3D from './components/3d/VectorSpace3D';

function App() {
  const { theme, setTheme, availableThemes } = useTheme();
  const [showDemo, setShowDemo] = useState(false);
  const [nativeStatus, setNativeStatus] = useState('checking...');
  const [demoVectors, setDemoVectors] = useState<any[]>([]);

  useEffect(() => {
    // Check native addon status
    if (window.hektorAPI) {
      const isAvailable = window.hektorAPI.isNativeAvailable();
      if (isAvailable) {
        const version = window.hektorAPI.getVersion();
        setNativeStatus(`Native addon loaded (v${version})`);
      } else {
        setNativeStatus('Native addon not available (using mock)');
      }
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
    <div className="app-container" data-theme={theme.id}>
      <header className="app-header">
        <h1 className="app-title">HEKTOR Quantization Studio</h1>
        <div className="theme-selector">
          <select 
            value={theme.id} 
            onChange={(e) => setTheme(e.target.value)}
            className="theme-dropdown"
          >
            {availableThemes.map(t => (
              <option key={t.id} value={t.id}>
                {t.name}
              </option>
            ))}
          </select>
          <button 
            onClick={() => setShowDemo(!showDemo)}
            className="demo-button"
          >
            {showDemo ? 'Hide' : 'Show'} 3D Demo
          </button>
        </div>
      </header>
      
      <main className="app-main">
        {showDemo ? (
          <div className="demo-container">
            <VectorSpace3D 
              vectors={demoVectors}
              geometry="euclidean"
              onVectorClick={(vec) => console.log('Clicked vector:', vec)}
            />
          </div>
        ) : (
          <div className="welcome-section">
            <h2>Welcome to HEKTOR</h2>
            <p className="subtitle">
              Industry-first Perceptual Quantization Studio for 3D Vector Visualization
            </p>
            <div className="status-badge">
              {nativeStatus}
            </div>
            <div className="feature-grid">
              <div className="feature-card">
                <h3>3D Visualization</h3>
                <p>Multi-geometry support: Euclidean, Hyperbolic, Parabolic</p>
              </div>
              <div className="feature-card">
                <h3>Perceptual Quantization</h3>
                <p>HDR-aware quantization with real-time preview</p>
              </div>
              <div className="feature-card">
                <h3>Native Performance</h3>
                <p>Direct C++ integration via N-API for sub-ms latency</p>
              </div>
              <div className="feature-card">
                <h3>AI Assistant</h3>
                <p>Contextual suggestions and query optimization</p>
              </div>
            </div>
          </div>
        )}
      </main>
      
      <footer className="app-footer">
        <p>HEKTOR v4.0.0 | {theme.name} Active | {nativeStatus}</p>
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
