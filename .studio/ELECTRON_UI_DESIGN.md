# HEKTOR Perceptual Quantization Studio
## Native Electron UI - Design Specification

**Version**: 1.0.0  
**Date**: 2026-01-23  
**Status**: Design Phase  
**Target Platform**: Electron + Native C++ Integration

---

## Executive Summary

HEKTOR Perceptual Quantization Studio is an **industry-first native desktop application** for 3D vector database visualization and manipulation. Unlike traditional web-based interfaces, this application leverages native C++ performance with an Electron shell, providing real-time visualization of billion-scale vector spaces with perceptual quantization capabilities that are unmatched in the industry.

**Key Differentiator**: First vector database UI with native 3D visualization of Euclidean, non-Euclidean, hyperbolic, and parabolic vector spaces with HDR-aware perceptual quantization display.

---

## Design Philosophy

### Core Principles

1. **Native Performance First**
   - Direct C++ integration via Node.js Native Addons (N-API)
   - Zero-copy data transfer between C++ core and rendering engine
   - Sub-frame latency for interactions (<16ms @ 60fps, <8ms @ 120fps)

2. **Visual Excellence**
   - Slate modern theme with adaptive brightness
   - Crisp resolution support (4K, 5K, 8K displays)
   - HDR-10/Dolby Vision support for perceptual quantization visualization
   - Smooth 60fps minimum, 120fps target for interactions

3. **Intuitive Intelligence**
   - Embedded AI agent (non-chat, pure functionality)
   - Contextual suggestions based on user actions
   - Predictive query optimization
   - Visual anomaly detection with auto-highlighting

4. **Professional Grade**
   - Lightweight, modern typography (Inter/SF Pro Display)
   - Responsive design with fluid transitions
   - Keyboard-first navigation with comprehensive shortcuts
   - Multi-monitor support with window state persistence

---

## Technology Stack

### Core Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Electron Shell                        │
│  ┌───────────────────────────────────────────────────┐  │
│  │              Frontend Layer (Renderer)            │  │
│  │  • React 18 + TypeScript 5.3                      │  │
│  │  • Three.js R160+ (3D Rendering)                  │  │
│  │  • WebGL 2.0 / WebGPU (Hardware Acceleration)    │  │
│  │  • D3.js v7 (2D Data Visualization)              │  │
│  │  • Tailwind CSS 4.0 (Styling)                    │  │
│  │  • Framer Motion (Animations)                    │  │
│  └───────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────┐  │
│  │               Main Process (Node.js)              │  │
│  │  • TypeScript 5.3                                 │  │
│  │  • Native Addon (N-API)                           │  │
│  │  • IPC Manager (Efficient Communication)         │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                           │
                           ↓ Native Bindings (Zero-Copy)
┌─────────────────────────────────────────────────────────┐
│              HEKTOR C++23 Core Engine                    │
│  • VectorDatabase (SIMD-optimized)                      │
│  • HNSW Index (Billion-scale)                           │
│  • Perceptual Quantization Engine                       │
│  • Embeddings (ONNX Runtime)                            │
│  • Telemetry & Monitoring                               │
└─────────────────────────────────────────────────────────┘
```

### Why This Stack?

**Electron**:
- Native desktop experience with web technologies
- Access to Node.js native modules
- Cross-platform (Windows, macOS, Linux)
- Hardware acceleration via GPU
- Auto-updates and crash reporting

**React 18**:
- Concurrent rendering for smooth interactions
- Suspense for lazy loading heavy visualizations
- Server Components (optional) for precomputed layouts
- Strong TypeScript support

**Three.js**:
- Industry-standard 3D library
- WebGL/WebGPU hardware acceleration
- Extensive shader support for custom rendering
- VR/AR ready for future expansion

**Native C++ Addon (N-API)**:
- Direct memory access to vector data
- Zero-copy buffer sharing
- SIMD operations execute natively
- Thread-safe async operations

---

## Visual Architecture

### Color Palette - "Quantum Slate"

```css
/* Primary - Slate Blues */
--slate-50:   #f8fafc;  /* Background - Light Mode */
--slate-100:  #f1f5f9;  /* Surface - Light Mode */
--slate-800:  #1e293b;  /* Surface - Dark Mode */
--slate-900:  #0f172a;  /* Background - Dark Mode */
--slate-950:  #020617;  /* Deep Background - Dark Mode */

/* Accent - Quantum Cyan */
--cyan-400:   #22d3ee;  /* Interactive Elements */
--cyan-500:   #06b6d4;  /* Primary Actions */
--cyan-600:   #0891b2;  /* Hover States */

/* Semantic Colors */
--emerald-500: #10b981; /* Success / High Similarity */
--amber-500:   #f59e0b; /* Warning / Medium Similarity */
--rose-500:    #f43f5e; /* Error / Low Similarity */
--violet-500:  #8b5cf6; /* Special / Perceptual Quant */

/* HDR Gamut (for PQ visualization) */
--hdr-white:   oklch(100% 0 0);    /* 10,000 nits */
--hdr-peak:    oklch(95% 0.25 150); /* Peak highlights */
```

### Typography

**Primary Font**: Inter Variable (Sans-serif)
- Weight: 300-700 (Light to Bold)
- OpenType features: Tabular numbers, case-sensitive forms
- Size scale: 10px, 12px, 14px, 16px, 18px, 24px, 32px, 48px

**Monospace Font**: JetBrains Mono (Code/Data)
- Weight: 400-600 (Regular to SemiBold)
- Ligatures enabled for code
- Size scale: 11px, 12px, 13px, 14px

**Display Font**: SF Pro Display / Segoe UI Variable (System)
- Used for large headings and hero text
- Weight: 600-700 (SemiBold to Bold)

### Layout System

**Grid**: 8px base unit
- Spacing: 4px, 8px, 16px, 24px, 32px, 48px, 64px
- Padding: 8px, 16px, 24px
- Margins: 16px, 24px, 32px

**Breakpoints**: Adaptive for window resize
- Compact: < 1280px width
- Standard: 1280-1920px width
- Wide: 1920-2560px width
- Ultra: > 2560px width (4K+)

**Component Hierarchy**:
```
App Shell (100vh)
├── Title Bar (32px) - Custom, draggable
├── Main Navigation (48px) - Top bar
├── Content Area (flexible)
│   ├── Sidebar (280px, collapsible)
│   ├── Primary View (flexible)
│   └── Inspector Panel (320px, toggleable)
└── Status Bar (24px) - Bottom
```

---

## Logical Blueprint

### Application Modules

#### 1. Vector Space Visualizer (Core Module)

**Purpose**: 3D real-time visualization of vector embeddings

**Features**:
- **Multi-Geometry Support**:
  - Euclidean (standard 3D space)
  - Hyperbolic (Poincaré disk, Klein model)
  - Spherical (unit sphere projection)
  - Parabolic (custom manifolds)
  
- **Rendering Modes**:
  - Point Cloud (billions of points with LOD)
  - Cluster View (hierarchical aggregation)
  - HNSW Graph (edges as connections)
  - Heatmap (density visualization)
  - Flow Field (similarity gradients)

- **Interaction**:
  - Orbit controls (mouse/trackpad)
  - Fly-through navigation (WASD + mouse)
  - Point selection (click, lasso, box select)
  - Distance measurement tool
  - Bookmark camera positions

- **Performance**:
  - Octree spatial indexing for culling
  - GPU instancing for millions of points
  - Level-of-Detail (LOD) based on camera distance
  - Frustum culling for off-screen geometry
  - Target: 60fps @ 1M visible points

**AI Features**:
- Auto-detect interesting clusters
- Suggest optimal viewing angles
- Predict outliers and anomalies
- Recommend projection dimensions

#### 2. Perceptual Quantization Studio

**Purpose**: Visualize and configure HDR-aware quantization

**Features**:
- **Curve Editor**:
  - Visual curve adjustment (PQ ST.2084, HLG)
  - Real-time preview of quantization effect
  - Side-by-side comparison (original vs quantized)
  - Histogram with tone mapping preview

- **Gamut Visualization**:
  - 3D color space rendering (sRGB, P3, Rec.2020)
  - Out-of-gamut indicators
  - Clipping zones highlighted
  - Dynamic range meter (nits scale)

- **Quality Metrics**:
  - PSNR, SSIM displayed in real-time
  - Recall@K comparison charts
  - Compression ratio visualization
  - Quality vs. Performance trade-off graph

- **HDR Display Support**:
  - Auto-detect HDR displays
  - Switch between SDR/HDR preview
  - Peak brightness calibration
  - Tone mapping presets

**AI Features**:
- Suggest optimal curve for dataset
- Auto-tune quantization parameters
- Predict quality impact before applying
- Recommend compression level based on use case

#### 3. Query Workbench

**Purpose**: Interactive query building and execution

**Features**:
- **Visual Query Builder**:
  - Drag-and-drop filter conditions
  - Vector input (text, image, or raw)
  - Hybrid search composer (vector + BM25)
  - Fusion method selector with preview

- **Results Explorer**:
  - Sortable, filterable table
  - Similarity score visualization (bar chart)
  - Metadata inspector (JSON viewer)
  - Related items graph (network diagram)

- **Performance Profiler**:
  - Query execution timeline
  - Bottleneck identification
  - Index usage statistics
  - Cache hit/miss rates

- **History & Favorites**:
  - Query history with re-run
  - Save favorite queries
  - Export results (CSV, JSON, Excel)

**AI Features**:
- Suggest query optimizations
- Auto-complete metadata filters
- Predict result quality
- Recommend similar queries

#### 4. Data Ingestion Pipeline

**Purpose**: Visual data import and transformation

**Features**:
- **Source Configuration**:
  - File picker (CSV, JSON, PDF, Excel, etc.)
  - Database connectors (SQLite, PostgreSQL)
  - HTTP endpoints (APIs, RSS)
  - Drag-and-drop zone for files

- **Transform Editor**:
  - Column mapping interface
  - Text preprocessing options
  - Chunking strategy selector (5 strategies)
  - Metadata enrichment rules

- **Progress Monitor**:
  - Real-time progress bar
  - Throughput graph (docs/sec)
  - Error log with line numbers
  - Success/failure counts

- **Validation**:
  - Schema validation preview
  - Sample data preview (first 10 rows)
  - Dry-run mode (no actual import)

**AI Features**:
- Auto-detect file format
- Suggest chunking strategy
- Predict embedding time
- Recommend metadata fields

#### 5. System Monitor & Analytics

**Purpose**: Real-time health monitoring and performance analytics

**Features**:
- **Dashboard**:
  - Vector count (live counter)
  - Storage usage (pie chart)
  - Query throughput (line graph)
  - Index health score (gauge)

- **Metrics Explorer**:
  - Prometheus metrics viewer
  - Custom metric dashboards
  - Alert configuration
  - Historical trends (time series)

- **Log Viewer**:
  - Real-time log streaming
  - Severity filtering (DEBUG → CRITICAL)
  - Anomaly highlighting
  - Full-text search in logs

- **Distributed Topology** (for clusters):
  - Node health status
  - Replication lag visualization
  - Shard distribution map
  - Load balancing stats

**AI Features**:
- Predict performance degradation
- Auto-suggest index optimizations
- Anomaly detection alerts
- Capacity planning recommendations

#### 6. Settings & Configuration

**Purpose**: Application and database configuration

**Features**:
- **Database Settings**:
  - HNSW parameters (M, ef_construction, ef_search)
  - Distance metric selector
  - Quantization configuration
  - Storage path management

- **UI Preferences**:
  - Theme selector (Light/Dark/Auto)
  - Typography settings
  - Keybinding customization
  - Performance presets

- **Advanced**:
  - SIMD level selection (SSE4/AVX2/AVX-512)
  - Thread count configuration
  - Memory limits
  - GPU acceleration toggle

- **Export/Import**:
  - Configuration backup
  - Profile management
  - Settings sync across machines

---

## 3D Visualization System Architecture

### Rendering Pipeline

```
┌──────────────────────────────────────────────────────┐
│  1. Data Acquisition (C++ → JS)                      │
│     • Query HNSW index for visible vectors          │
│     • Apply spatial culling (octree)                │
│     • Transfer via SharedArrayBuffer (zero-copy)    │
└──────────────────────────────────────────────────────┘
                      ↓
┌──────────────────────────────────────────────────────┐
│  2. Geometry Generation (GPU-side)                   │
│     • Instanced rendering for point clouds          │
│     • Dynamic LOD (4 levels)                         │
│     • Edge rendering for HNSW graph                 │
└──────────────────────────────────────────────────────┘
                      ↓
┌──────────────────────────────────────────────────────┐
│  3. Shader Processing                                │
│     • Vertex shader: Position, color, size          │
│     • Fragment shader: Anti-aliasing, glow          │
│     • Post-processing: Bloom, depth-of-field        │
└──────────────────────────────────────────────────────┘
                      ↓
┌──────────────────────────────────────────────────────┐
│  4. Compositing & Display                            │
│     • UI overlay rendering (React Canvas)           │
│     • Selection highlighting                         │
│     • HUD elements (axes, grid, labels)             │
└──────────────────────────────────────────────────────┘
```

### Geometry Types

#### Point Cloud Rendering

**Technique**: GPU Instancing with custom shaders

```glsl
// Vertex Shader
attribute vec3 position;
attribute vec3 color;
attribute float similarity; // 0.0 - 1.0

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform float pointSize;

varying vec3 vColor;
varying float vSimilarity;

void main() {
    vColor = color;
    vSimilarity = similarity;
    
    vec4 mvPosition = viewMatrix * vec4(position, 1.0);
    gl_Position = projectionMatrix * mvPosition;
    
    // Size attenuation based on distance
    gl_PointSize = pointSize * (300.0 / -mvPosition.z);
}

// Fragment Shader
varying vec3 vColor;
varying float vSimilarity;

void main() {
    // Circular points with smooth edges
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    
    if (dist > 0.5) discard;
    
    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);
    
    // Glow effect for high similarity
    float glow = pow(vSimilarity, 2.0) * 0.5;
    vec3 finalColor = vColor + vec3(glow);
    
    gl_FragColor = vec4(finalColor, alpha);
}
```

#### Hyperbolic Space (Poincaré Disk)

**Mathematical Model**: Map vectors to hyperbolic space

```typescript
// Hyperbolic distance in Poincaré disk
function hyperbolicDistance(p1: Vector3, p2: Vector3): number {
    const p1Norm = p1.lengthSq();
    const p2Norm = p2.lengthSq();
    const diff = p1.clone().sub(p2).lengthSq();
    
    const numerator = 2 * diff;
    const denominator = (1 - p1Norm) * (1 - p2Norm);
    
    return Math.acosh(1 + numerator / denominator);
}

// Project Euclidean vectors to Poincaré disk
function projectToHyperbolic(euclidean: Vector3): Vector3 {
    const scale = Math.tanh(euclidean.length() / 2);
    return euclidean.normalize().multiplyScalar(scale);
}
```

#### HNSW Graph Visualization

**Graph Rendering**: Dynamic edge bundling

```typescript
// Generate edges from HNSW structure
function generateHNSWEdges(index: HNSWIndex, layer: number): Edge[] {
    const edges: Edge[] = [];
    
    for (const node of index.getNodesAtLayer(layer)) {
        for (const neighbor of node.neighbors) {
            edges.push({
                source: node.position,
                target: neighbor.position,
                strength: node.similarity(neighbor)
            });
        }
    }
    
    return edges;
}

// Edge bundling for clarity
function bundleEdges(edges: Edge[]): BundledEdge[] {
    // Use force-directed layout for edge bundling
    // Reduces visual clutter for dense graphs
    return forceDirectedBundle(edges, {
        iterations: 10,
        bundleStrength: 0.8
    });
}
```

### Performance Optimizations

1. **Spatial Indexing**: Octree for O(log n) spatial queries
2. **LOD System**: 4 levels based on camera distance
3. **Frustum Culling**: Only render visible geometry
4. **GPU Instancing**: Single draw call for millions of points
5. **WebWorkers**: Offload data processing from main thread
6. **SharedArrayBuffer**: Zero-copy data transfer
7. **Object Pooling**: Reuse Three.js objects
8. **Request Animation Frame**: Sync with display refresh

**Target Performance**:
- 1M points @ 60fps (1080p)
- 10M points @ 30fps (4K)
- <16ms frame time (99th percentile)

---

## AI Agent Architecture

### Embedded Intelligence (Non-Chat)

**Purpose**: Contextual functionality enhancement, not conversation

**Implementation**: Local ML model (ONNX Runtime)

```typescript
interface AIAgent {
    // Analyze current view and suggest actions
    analyzeSituation(context: ViewContext): Suggestion[];
    
    // Predict next likely action
    predictIntent(userActions: Action[]): Intent;
    
    // Optimize query for better results
    optimizeQuery(query: Query): OptimizedQuery;
    
    // Detect visual anomalies in data
    detectAnomalies(vectors: Vector[]): Anomaly[];
    
    // Suggest visualization parameters
    suggestVisualization(data: Dataset): VisualizationConfig;
}
```

### Contextual Suggestions

**Trigger**: User hovers over element, selects data, or pauses

**Examples**:

1. **Vector Selection**:
   ```
   💡 "These 347 vectors form a tight cluster.
       Suggest: Create a new label 'Cluster_A'?"
   ```

2. **Query Optimization**:
   ```
   💡 "Your query could be 3x faster with ef_search=100.
       Current: 245ms → Predicted: 82ms"
   ```

3. **Anomaly Detection**:
   ```
   ⚠️ "Detected 12 outlier vectors (>3σ from centroid).
       Suggest: Review for data quality issues?"
   ```

4. **Performance Tuning**:
   ```
   💡 "HNSW index is 78% full. Consider increasing max_elements
       to avoid performance degradation."
   ```

### Visual Indicators

**Suggestion Badge**: Small pulsing icon in top-right of relevant component
**Hover Tooltip**: Expands to show full suggestion
**Inline Actions**: One-click to apply suggestion
**Dismiss**: Close suggestion without applying

---

## Features & Functions

### Core Features Matrix

| Feature | Module | Priority | Complexity |
|---------|--------|----------|------------|
| 3D Point Cloud Visualization | Visualizer | P0 | High |
| Hyperbolic Space Projection | Visualizer | P1 | High |
| HNSW Graph Rendering | Visualizer | P1 | Medium |
| Perceptual Quantization Editor | PQ Studio | P0 | High |
| HDR Display Support | PQ Studio | P1 | Medium |
| Visual Query Builder | Query Workbench | P0 | Medium |
| Hybrid Search Composer | Query Workbench | P1 | Medium |
| Data Import Pipeline | Ingestion | P0 | Low |
| Real-time Metrics Dashboard | Monitor | P1 | Low |
| AI Contextual Suggestions | AI Agent | P2 | High |
| Anomaly Detection | AI Agent | P2 | Medium |
| Export/Import Configuration | Settings | P1 | Low |

**Priority Levels**:
- P0: Must-have for MVP
- P1: Important for full release
- P2: Nice-to-have, can ship later

### User Workflows

#### Workflow 1: Explore Existing Database

1. **Open Database** → File picker
2. **View Overview** → Dashboard shows stats
3. **Visualize Vectors** → 3D view loads with LOD
4. **Inspect Cluster** → Click to zoom, see metadata
5. **Query Similar** → Right-click → "Find Similar"
6. **Export Results** → Save to CSV/JSON

**AI Assists**:
- Auto-suggests interesting clusters
- Recommends optimal viewing angle
- Predicts similar queries

#### Workflow 2: Configure Perceptual Quantization

1. **Open PQ Studio** → Navigate to PQ module
2. **Select Dataset** → Choose vectors to quantize
3. **Adjust Curve** → Drag curve control points
4. **Preview Effect** → Real-time before/after comparison
5. **View Metrics** → Check recall@K impact
6. **Apply Quantization** → Confirm and execute
7. **Monitor Progress** → Progress bar with ETA

**AI Assists**:
- Suggests optimal curve based on data distribution
- Predicts quality impact (±X% recall)
- Recommends compression ratio

#### Workflow 3: Ingest New Data

1. **Open Ingestion** → Navigate to data import
2. **Select Source** → Drag-and-drop files or connect DB
3. **Preview Schema** → Review columns and types
4. **Configure Transform** → Map fields, set chunking
5. **Dry Run** → Test with 100 samples
6. **Start Ingestion** → Begin full import
7. **Monitor Progress** → Real-time throughput graph

**AI Assists**:
- Auto-detects file format
- Suggests chunking strategy (Semantic for long docs)
- Predicts total time and storage needed

---

## UI Component Library

### Atomic Design System

**Atoms** (Basic building blocks):
- Button (Primary, Secondary, Danger, Ghost)
- Input (Text, Number, Select, Checkbox, Radio)
- Icon (Feather Icons set)
- Badge (Status indicators)
- Tooltip (Hover info)

**Molecules** (Simple combinations):
- SearchBar (Input + Icon)
- Slider (Input + Label + Value)
- DropdownMenu (Button + Menu)
- Card (Container + Header + Content)
- ProgressBar (Bar + Label + Percentage)

**Organisms** (Complex components):
- DataTable (Sortable, filterable table)
- ChartPanel (D3.js chart with controls)
- 3DViewport (Three.js canvas with controls)
- QueryBuilder (Visual query composition)
- FileUploader (Drag-drop + progress)

**Templates** (Page layouts):
- DashboardLayout (Sidebar + Content + Inspector)
- FullscreenLayout (Immersive 3D view)
- ModalLayout (Overlay with backdrop)

### Component Examples

#### Button Component

```tsx
interface ButtonProps {
  variant: 'primary' | 'secondary' | 'danger' | 'ghost';
  size: 'sm' | 'md' | 'lg';
  icon?: React.ReactNode;
  loading?: boolean;
  disabled?: boolean;
  onClick?: () => void;
  children: React.ReactNode;
}

const Button: React.FC<ButtonProps> = ({
  variant = 'primary',
  size = 'md',
  icon,
  loading,
  disabled,
  onClick,
  children
}) => {
  const baseClasses = 'rounded-lg font-medium transition-all duration-200';
  
  const variantClasses = {
    primary: 'bg-cyan-500 hover:bg-cyan-600 text-white',
    secondary: 'bg-slate-700 hover:bg-slate-600 text-slate-100',
    danger: 'bg-rose-500 hover:bg-rose-600 text-white',
    ghost: 'bg-transparent hover:bg-slate-800 text-slate-300'
  };
  
  const sizeClasses = {
    sm: 'px-3 py-1.5 text-sm',
    md: 'px-4 py-2 text-base',
    lg: 'px-6 py-3 text-lg'
  };
  
  return (
    <button
      className={`${baseClasses} ${variantClasses[variant]} ${sizeClasses[size]}`}
      disabled={disabled || loading}
      onClick={onClick}
    >
      {loading && <Spinner className="mr-2" />}
      {icon && <span className="mr-2">{icon}</span>}
      {children}
    </button>
  );
};
```

#### 3D Viewport Component

```tsx
interface ViewportProps {
  vectors: Vector[];
  selectedIds: Set<number>;
  onSelect: (ids: number[]) => void;
  renderMode: 'points' | 'hnsw' | 'heatmap';
  geometry: 'euclidean' | 'hyperbolic' | 'spherical';
}

const Viewport3D: React.FC<ViewportProps> = ({
  vectors,
  selectedIds,
  onSelect,
  renderMode,
  geometry
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const sceneRef = useRef<THREE.Scene>();
  const rendererRef = useRef<THREE.WebGLRenderer>();
  
  useEffect(() => {
    // Initialize Three.js scene
    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
    const renderer = new THREE.WebGLRenderer({ 
      canvas: canvasRef.current!,
      antialias: true,
      alpha: true
    });
    
    sceneRef.current = scene;
    rendererRef.current = renderer;
    
    // Setup lights, controls, etc.
    setupScene(scene, camera, renderer);
    
    // Animation loop
    const animate = () => {
      requestAnimationFrame(animate);
      renderer.render(scene, camera);
    };
    animate();
    
    return () => {
      renderer.dispose();
    };
  }, []);
  
  useEffect(() => {
    // Update geometry when vectors change
    if (sceneRef.current) {
      updateGeometry(sceneRef.current, vectors, renderMode, geometry);
    }
  }, [vectors, renderMode, geometry]);
  
  return (
    <div className="relative w-full h-full">
      <canvas ref={canvasRef} className="w-full h-full" />
      <ViewportControls onModeChange={/* ... */} />
      <ViewportHUD stats={/* ... */} />
    </div>
  );
};
```

---

## Keyboard Shortcuts

### Global Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl/Cmd + N` | New Database |
| `Ctrl/Cmd + O` | Open Database |
| `Ctrl/Cmd + S` | Save Configuration |
| `Ctrl/Cmd + Q` | Quit Application |
| `Ctrl/Cmd + ,` | Open Settings |
| `Ctrl/Cmd + K` | Command Palette |
| `F11` | Toggle Fullscreen |
| `Ctrl/Cmd + /` | Show Keyboard Shortcuts |

### 3D Viewport Shortcuts

| Shortcut | Action |
|----------|--------|
| `W/A/S/D` | Fly navigation (forward/left/back/right) |
| `Q/E` | Fly up/down |
| `Shift` | Move faster (10x speed) |
| `Space` | Reset camera to home position |
| `F` | Focus on selected vectors |
| `G` | Toggle grid |
| `X` | Toggle axes |
| `1/2/3` | Switch to Front/Side/Top view |
| `[/]` | Decrease/Increase point size |
| `L` | Cycle through render modes |
| `H` | Toggle HNSW graph |

### Selection Shortcuts

| Shortcut | Action |
|----------|--------|
| `Click` | Select single point |
| `Shift + Click` | Add to selection |
| `Ctrl/Cmd + Click` | Toggle selection |
| `Ctrl/Cmd + A` | Select all |
| `Ctrl/Cmd + D` | Deselect all |
| `B` | Box select (drag to select) |
| `C` | Lasso select (draw to select) |

### Query Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl/Cmd + Enter` | Execute query |
| `Ctrl/Cmd + Shift + F` | Advanced filter |
| `Ctrl/Cmd + R` | Repeat last query |
| `Ctrl/Cmd + E` | Export results |

---

## Performance Targets

### Startup Performance

| Metric | Target | Acceptable |
|--------|--------|------------|
| Cold Start | < 2s | < 3s |
| Database Open (1M vectors) | < 500ms | < 1s |
| Database Open (100M vectors) | < 2s | < 4s |
| UI First Paint | < 200ms | < 500ms |
| 3D Scene Initial Render | < 1s | < 2s |

### Runtime Performance

| Operation | Target | Acceptable |
|-----------|--------|------------|
| Frame Rate (1M visible points) | 60fps | 45fps |
| Frame Rate (10M visible points) | 30fps | 24fps |
| Query Execution (<100 results) | < 50ms | < 100ms |
| Query Execution (1K results) | < 200ms | < 500ms |
| Selection Response | < 16ms | < 33ms |
| Camera Movement Latency | < 8ms | < 16ms |

### Memory Usage

| Scenario | Target | Maximum |
|----------|--------|---------|
| Base Application | 100 MB | 150 MB |
| 1M vectors loaded | 250 MB | 400 MB |
| 10M vectors loaded | 1 GB | 2 GB |
| 100M vectors loaded | 5 GB | 10 GB |

---

## Accessibility

### WCAG 2.1 Level AA Compliance

**Visual**:
- Minimum contrast ratio 4.5:1 for normal text
- Minimum contrast ratio 3:1 for large text (18pt+)
- Focus indicators visible on all interactive elements
- Color not used as only means of conveying information

**Keyboard**:
- All functionality accessible via keyboard
- Logical tab order throughout application
- Skip navigation links provided
- Keyboard shortcuts documented

**Screen Readers**:
- ARIA labels on all interactive elements
- ARIA live regions for dynamic content
- Semantic HTML structure
- Alternative text for all visualizations (textual descriptions)

**Other**:
- Adjustable text size (100% - 200%)
- Reduced motion mode (disables animations)
- High contrast mode
- Keyboard shortcut remapping

---

## Security & Privacy

### Data Protection

1. **Local-First**: All data stored locally by default
2. **Encryption at Rest**: AES-256 encryption for sensitive databases
3. **Secure IPC**: Encrypted communication between renderer and main process
4. **No Telemetry**: Zero user tracking or analytics by default
5. **Open Source**: Fully auditable codebase

### Best Practices

- Content Security Policy (CSP) enforced
- No eval() or unsafe JavaScript execution
- Sandboxed renderer process
- Limited Node.js integration in renderer
- Regular security audits and dependency updates

---

## Cross-Platform Considerations

### Windows

- Native Windows 11 styling (Mica, rounded corners)
- DirectX 12 / Vulkan for graphics (via WebGPU)
- Windows Defender SmartScreen signing
- Auto-update via Squirrel.Windows

### macOS

- Native macOS styling (vibrancy, materials)
- Metal for graphics (via WebGPU)
- Apple notarization
- Auto-update via Squirrel.Mac
- Touch Bar support (optional)

### Linux

- GTK+ theme integration
- Wayland and X11 support
- Vulkan for graphics
- AppImage / Flatpak / Snap distribution
- Auto-update via AppImage update mechanism

---

## File Format Specifications

### Project File (.hektor)

**Format**: JSON with binary blob for vector cache

```json
{
  "version": "1.0.0",
  "database": {
    "path": "/path/to/database",
    "autoOpen": true
  },
  "ui": {
    "theme": "dark",
    "layout": "dashboard",
    "windows": [
      {
        "id": "main",
        "bounds": { "x": 0, "y": 0, "width": 1920, "height": 1080 },
        "maximized": false
      }
    ]
  },
  "viewport": {
    "camera": {
      "position": [0, 0, 10],
      "target": [0, 0, 0]
    },
    "renderMode": "points",
    "geometry": "euclidean"
  },
  "queries": [
    {
      "name": "Recent searches",
      "query": "SELECT * FROM vectors WHERE date > '2026-01-01'",
      "timestamp": 1706918400
    }
  ]
}
```

### Export Formats

**Supported Exports**:
- CSV (vectors + metadata)
- JSON (structured)
- Excel (XLSX with charts)
- PNG/JPEG (3D viewport screenshots)
- MP4/WebM (3D viewport recordings)
- Parquet (high-performance columnar)

---

## Testing Strategy

### Unit Tests

- Jest for JavaScript/TypeScript
- Vitest for Vite-powered components
- Coverage target: 80%+

### Integration Tests

- Electron test runner
- End-to-end workflows
- Database operations
- IPC communication

### Visual Regression Tests

- Percy or Chromatic for UI snapshots
- 3D viewport rendering tests
- Theme consistency checks

### Performance Tests

- Lighthouse CI for web performance
- Custom benchmarks for 3D rendering
- Memory profiling with Chrome DevTools
- Load testing with large datasets

### Manual Testing

- Cross-platform testing (Windows, macOS, Linux)
- Different display configurations (1080p, 4K, HDR)
- Accessibility testing with screen readers
- Usability testing with target users

---

## Future Enhancements

### Phase 2 Features

- VR/AR support (Meta Quest, Apple Vision Pro)
- Collaborative editing (multiple users, real-time sync)
- Cloud database integration (optional)
- Plugin system for custom visualizations
- Advanced filtering with visual query language
- Timeline view for temporal data
- Animated transitions between vector states

### Phase 3 Features

- Neural network training integration (export → train → import)
- Custom shader editor for advanced visualizations
- Batch processing with job queue
- Advanced analytics (PCA, t-SNE, UMAP integration)
- Distributed database management UI
- Performance regression testing suite

---


