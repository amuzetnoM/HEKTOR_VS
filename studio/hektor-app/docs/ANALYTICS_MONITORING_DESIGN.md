# HEKTOR Analytics & Monitoring Platform - Comprehensive Design
## Design Specification v2.0

**Document Version:** 2.0.0  
**Date:** January 2026  
**Status:** Production Design  
**Authors:** HEKTOR Development Team  
**Size Target:** 22-25KB

---

## Executive Summary

The **HEKTOR Analytics & Monitoring Platform** is a custom-built, production-grade analytics solution designed specifically for vector database observability. Inspired by the best practices from Grafana and Apache Superset, but purpose-built for HEKTOR's unique requirements, this platform provides:

### Key Features

- **Custom Dashboard Canvas System**: Drag-and-drop dashboard builder with flexible grid layout
- **Real-time Data Streaming**: WebSocket/Server-Sent Events for live metrics from C++ backend
- **Widget Plugin Architecture**: Extensible system for creating custom visualization widgets
- **Pre-built Dashboard Templates**: Performance, Query Analytics, Index Health, Ingestion, Distributed Systems
- **Advanced Chart Library Integration**: D3.js, Plotly, and Three.js for multidimensional visualizations
- **Time-Travel Debugging**: Replay historical data points for incident investigation
- **Export Capabilities**: PDF, PNG, CSV, JSON exports with scheduling
- **Alerting System**: Threshold-based alerting with notifications (Slack, Email, PagerDuty)
- **Responsive Design**: Works seamlessly on desktop, tablet, and mobile

### Business Impact

- **Reduce MTTR**: Identify issues 5-10x faster with real-time dashboards
- **Optimize Costs**: Right-size hardware with detailed resource utilization insights
- **Improve QoS**: Monitor search latency, recall, and availability in real-time
- **Enable Self-Service**: Give users their own custom dashboards without coding

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Components](#core-components)
3. [Dashboard Canvas System](#dashboard-canvas-system)
4. [Widget Architecture](#widget-architecture)
5. [Data Pipeline](#data-pipeline)
6. [Pre-built Dashboard Templates](#pre-built-dashboard-templates)
7. [Chart Library Integration](#chart-library-integration)
8. [Real-time Streaming](#real-time-streaming)
9. [Alerting & Notifications](#alerting--notifications)
10. [Export & Scheduling](#export--scheduling)
11. [Historical Playback](#historical-playback)
12. [Integration Points](#integration-points)
13. [Technology Stack](#technology-stack)
14. [Implementation Phases](#implementation-phases)
15. [Testing Strategy](#testing-strategy)

---

## 1. Architecture Overview

### 1.1 System Architecture Diagram

```
┌────────────────────────────────────────────────────────────────────┐
│         HEKTOR Analytics Frontend (React + Redux + D3/Plotly)      │
├────────────────┬─────────────────┬─────────────────┬──────────────┤
│ Dashboard      │ Widget Library  │ Query Builder   │ Templates    │
│ Builder        │ (Components)    │ (Advanced)      │ Gallery      │
│ (Canvas)       │ (Stock Charts)  │ (SQL/QL)        │ (Pre-built)  │
└────────┬───────┴────────┬────────┴────────┬────────┴──────┬───────┘
         │                │                 │               │
┌────────▼────────────────▼─────────────────▼───────────────▼────────┐
│           WebSocket / Server-Sent Events (Real-time)               │
│  • ws://hektor.local:9000/stream/metrics                           │
│  • Auto-reconnect with exponential backoff                         │
│  • Message queue for offline resilience                            │
└────────┬────────────────┬─────────────────┬───────────────┬────────┘
         │                │                 │               │
┌────────▼────────────────▼─────────────────▼───────────────▼────────┐
│  Analytics API (REST + GraphQL)  [Node.js / Express + Apollo]     │
├────────────────────────────────────────────────────────────────────┤
│  • /api/dashboards (CRUD)        • /graphql (queries + mutations)  │
│  • /api/widgets (templates)      • /api/alerts (threshold config)  │
│  • /api/queries (save/execute)   • /api/export (PDF/PNG/CSV)      │
│  • /api/history (time-travel)    • /api/notifications              │
└────────┬────────────────┬─────────────────┬───────────────┬────────┘
         │                │                 │               │
┌────────▼────────────────▼─────────────────▼───────────────▼────────┐
│     Metrics Collection Agent (C++23) + Time-Series Database       │
├────────────────────────────────────────────────────────────────────┤
│  • Scrape HEKTOR core metrics (native binding)                    │
│  • Store in InfluxDB / TimescaleDB (30-day retention)             │
│  • Aggregate: 1sec → 1min → 1hour → 1day                         │
│  • Compute derived metrics (percentiles, rates, ratios)            │
└────────┬────────────────┬─────────────────┬───────────────┬────────┘
         │                │                 │               │
┌────────▼────────────────▼─────────────────▼───────────────▼────────┐
│      HEKTOR Core Instrumentation (src/monitoring/)                 │
├────────────────────────────────────────────────────────────────────┤
│  • Search latency & throughput                                     │
│  • Index operations (build, merge, compact)                        │
│  • Memory usage (heap, index, cache)                               │
│  • Network I/O (replication, streaming)                            │
│  • Query performance (recall, MRR, NDCG)                           │
│  • Hardware counters (CPU, memory, I/O)                            │
└────────────────────────────────────────────────────────────────────┘
```

### 1.2 Data Flow Architecture

```
Metrics Generation:
  HEKTOR Core → Counter/Gauge/Histogram → Collection Agent
                                              ↓
                                         Local Buffer
                                              ↓
Time-Series DB:
  Aggregation Pipeline (1s → 1m → 1h → 1d)
            ↓
  Query Processing (filtering, windowing, functions)
            ↓
Analytics API:
  REST/GraphQL endpoint
            ↓
Frontend WebSocket:
  Real-time push updates OR polling
            ↓
Dashboard Widgets:
  Render with chart library (D3/Plotly/Three.js)
```

---

## 2. Core Components

### 2.1 Dashboard Canvas System

**Location:** `ui/src/components/DashboardCanvas.tsx`

**Features:**
```typescript
interface DashboardCanvasProps {
  dashboard: Dashboard;
  isEditable: boolean;
  onSave: (layout: GridLayout) => void;
}

interface Dashboard {
  id: string;
  name: string;
  description: string;
  layout: GridLayout;
  widgets: DashboardWidget[];
  refreshInterval: number; // milliseconds
  timezone: string;
  tags: string[];
  createdAt: Date;
  updatedAt: Date;
}

interface GridLayout {
  columns: 12; // 12-column grid
  rowHeight: 48; // pixels
  widgets: LayoutItem[];
}

interface LayoutItem {
  widgetId: string;
  x: number;     // 0-12
  y: number;
  width: number; // 1-12
  height: number;
  static: boolean; // Locked position
}
```

**Grid System:**
- **12-column responsive layout** (inspired by Bootstrap)
- **Dynamic row height** (each unit = 48px, but scales on mobile)
- **Drag-to-resize** with snap-to-grid
- **Drag-to-move** with gravity snapping
- **Z-indexing** for overlapping widgets

**Implementation:**
```typescript
import { Responsive, WidthProvider, Layout } from "react-grid-layout";
import "react-grid-layout/css/styles.css";
import "react-resizable/css/styles.css";

const ResponsiveGridLayout = WidthProvider(Responsive);

export const DashboardCanvas = ({dashboard, isEditable}: Props) => {
  const [layout, setLayout] = useState(dashboard.layout);

  const handleLayoutChange = (newLayout: Layout[]) => {
    setLayout(newLayout);
    if (!isEditable) return;
    debouncedSave(newLayout);
  };

  return (
    <ResponsiveGridLayout
      className="dashboard-canvas"
      layouts={{lg: layout}}
      breakpoints={{lg: 1200, md: 996, sm: 768, xs: 480}}
      cols={{lg: 12, md: 10, sm: 6, xs: 4}}
      rowHeight={48}
      isDraggable={isEditable}
      isResizable={isEditable}
      containerPadding={[16, 16]}
      margin={[12, 12]}
      onLayoutChange={handleLayoutChange}
      compactType="vertical"
    >
      {dashboard.widgets.map(widget => (
        <div key={widget.id}>
          <DashboardWidget widget={widget} />
        </div>
      ))}
    </ResponsiveGridLayout>
  );
};
```

### 2.2 Widget Architecture

**Base Widget Interface:**
```typescript
interface DashboardWidget {
  id: string;
  type: WidgetType; // 'chart', 'metric', 'table', 'gauge', 'heatmap', 'custom'
  title: string;
  config: WidgetConfig;
  dataSource: DataSourceRef;
  refreshInterval?: number;
  cacheKey?: string;
}

interface WidgetConfig {
  chartType: ChartType;
  aggregation: AggregationType; // 'sum', 'avg', 'p99', 'max', etc.
  timeRange: TimeRange; // '1h', '24h', '7d', 'custom'
  groupBy?: string[];
  filters?: FilterExpression[];
  thresholds?: Threshold[];
  legend?: LegendConfig;
  axes?: AxisConfig[];
}

type ChartType = 
  | 'line' | 'area' | 'bar' | 'scatter'
  | 'gauge' | 'stat' | 'table'
  | 'heatmap' | 'histogram' | '3d-scatter'
  | 'pie' | 'donut' | 'sankey';
```

**Stock Widget Library:**

```
1. Line Chart Widget
   ├─ Multiple series support
   ├─ Stacking options (normal, percent)
   ├─ Smoothing algorithms
   ├─ Threshold visualization
   └─ Interactive legend

2. Bar Chart Widget
   ├─ Grouped / stacked bars
   ├─ Horizontal / vertical orientation
   ├─ Value labels
   └─ Drill-down capability

3. Stat Widget
   ├─ Single metric display
   ├─ Sparkline trend
   ├─ Color coding (red/yellow/green)
   ├─ Comparison (current vs previous)
   └─ Custom thresholds

4. Gauge Widget
   ├─ Circular or linear gauge
   ├─ Threshold zones (good/warning/critical)
   ├─ Numeric display
   └─ Animation options

5. Table Widget
   ├─ Sortable columns
   ├─ Pagination
   ├─ Row coloring by value
   ├─ Drill-down on rows
   └─ CSV export per-widget

6. Heatmap Widget
   ├─ 2D grid visualization
   ├─ Color intensity = metric value
   ├─ Hover tooltips
   ├─ X/Y axis labels
   └─ Time-based heatmaps (service × time)

7. 3D Scatter Widget
   ├─ Three.js powered
   ├─ Rotatable/zoomable
   ├─ Color by metric
   ├─ Size by another metric
   └─ Point selection

8. Histogram Widget
   ├─ Distribution visualization
   ├─ Bin control
   ├─ Percentile markers
   ├─ CDFs
   └─ Outlier detection
```

**Widget Plugin System:**
```typescript
interface WidgetPlugin {
  name: string;
  version: string;
  
  // Metadata
  description: string;
  icon: React.ComponentType;
  category: 'chart' | 'metric' | 'custom';
  
  // Rendering
  component: React.ComponentType<WidgetProps>;
  configSchema: JSONSchema7; // For form generation
  defaultConfig: WidgetConfig;
  
  // Data handling
  validateData: (data: any) => boolean;
  transformData: (data: any[]) => any[];
  
  // Performance
  supportsLiveRefresh: boolean;
  cacheDuration?: number;
}

// Custom widget example:
const CustomServiceMapWidget: WidgetPlugin = {
  name: 'ServiceMap',
  component: ServiceMapVisualization,
  category: 'custom',
  // ...
};
```

---

## 3. Dashboard Canvas System

### 3.1 Dashboard Builder Interface

**Layout:**
```
┌─────────────────────────────────────────────────────────┐
│ My Dashboard     [Edit] [Save] [Template] [Share] [...] │
├─────────────────────────────────────────────────────────┤
│ Time Range: [Last 24h ▼] | Refresh: [30s ▼]             │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  ┌─────────────┐  ┌────────────────┐  ┌──────────┐      │
│  │  CPU Usage  │  │ Request Rate   │  │  Errors  │      │
│  │             │  │  (4 cols wide) │  │ (2 cols) │      │
│  └─────────────┘  └────────────────┘  └──────────┘      │
│                                                           │
│  ┌────────────────────────────────────────────┐          │
│  │  Latency Over Time (full width)            │          │
│  │  [Line chart with multiple percentiles]    │          │
│  └────────────────────────────────────────────┘          │
│                                                           │
│  ┌──────────────┐ ┌──────────────┐ ┌────────────┐       │
│  │ P99 Latency  │ │ P50 Latency  │ │ Index Size │       │
│  └──────────────┘ └──────────────┘ └────────────┘       │
│                                                           │
└─────────────────────────────────────────────────────────┘

Edit Mode:
  ├─ [+ Add Widget] button
  ├─ Left sidebar: Widget Library (drag from here)
  ├─ Canvas: Drag to move, resize handles
  ├─ Widget hover menu: [Edit Config] [Delete]
  └─ Auto-save on layout change
```

### 3.2 Template System

**Saved Templates:**
```json
{
  "id": "template-performance-monitoring",
  "name": "Performance Monitoring",
  "category": "System",
  "description": "Real-time system and query performance",
  "thumbnail": "...",
  "layout": [...],
  "widgets": [
    {
      "type": "chart",
      "chartType": "line",
      "title": "Request Latency (p50, p95, p99)",
      "config": {
        "metrics": ["query.latency.p50", "query.latency.p95", "query.latency.p99"],
        "aggregation": "avg",
        "timeRange": "24h"
      }
    },
    // ... more widgets
  ]
}
```

---

## 4. Widget Architecture

### 4.1 Stock Widget Components

**Line Chart Widget Implementation:**
```typescript
interface LineChartWidgetProps extends WidgetProps {
  data: TimeSeriesData[];
  config: LineChartConfig;
}

export const LineChartWidget: React.FC<LineChartWidgetProps> = ({
  data,
  config,
  isLoading,
  error
}) => {
  const svgRef = useRef<SVGSVGElement>(null);

  useEffect(() => {
    if (!svgRef.current || !data) return;

    const margin = {top: 20, right: 30, bottom: 30, left: 60};
    const width = 800 - margin.left - margin.right;
    const height = 400 - margin.top - margin.bottom;

    // D3.js visualization
    const svg = d3.select(svgRef.current);
    
    // Scales
    const xScale = d3.scaleTime()
      .domain(d3.extent(data, d => new Date(d.timestamp)))
      .range([0, width]);
    
    const yScale = d3.scaleLinear()
      .domain([0, d3.max(data, d => d.value)])
      .range([height, 0]);

    // Line generator
    const line = d3.line<TimeSeriesData>()
      .x(d => xScale(new Date(d.timestamp)))
      .y(d => yScale(d.value));

    // Draw path
    svg.append("path")
      .datum(data)
      .attr("d", line)
      .attr("stroke", "steelblue");

    // Axes
    svg.append("g")
      .attr("transform", `translate(0,${height})`)
      .call(d3.axisBottom(xScale));

    svg.append("g")
      .call(d3.axisLeft(yScale));

  }, [data, config]);

  return (
    <div className="widget-chart">
      {isLoading && <LoadingSpinner />}
      {error && <ErrorMessage error={error} />}
      <svg ref={svgRef} width={800} height={400} />
    </div>
  );
};
```

**Stat Widget Implementation:**
```typescript
interface StatWidgetProps extends WidgetProps {
  value: number;
  unit?: string;
  threshold?: Threshold;
  previousValue?: number;
  trend?: 'up' | 'down' | 'stable';
}

export const StatWidget: React.FC<StatWidgetProps> = ({
  value,
  unit = '',
  threshold,
  previousValue,
  trend
}) => {
  const getColor = (val: number) => {
    if (!threshold) return 'text-gray-800';
    if (val >= threshold.critical) return 'text-red-600';
    if (val >= threshold.warning) return 'text-yellow-600';
    return 'text-green-600';
  };

  const percentChange = previousValue 
    ? ((value - previousValue) / previousValue) * 100 
    : null;

  return (
    <div className="widget-stat">
      <div className={`text-4xl font-bold ${getColor(value)}`}>
        {value.toFixed(2)} {unit}
      </div>
      {previousValue && (
        <div className="text-sm text-gray-600">
          {percentChange > 0 ? '↑' : '↓'} {Math.abs(percentChange).toFixed(1)}%
        </div>
      )}
      {threshold && (
        <div className="threshold-indicator">
          Good: &lt;{threshold.warning} | Warning: {threshold.warning}-{threshold.critical}
        </div>
      )}
    </div>
  );
};
```

---

## 5. Data Pipeline

### 5.1 Metrics Collection Agent

**Location:** `src/monitoring/MetricsCollector.cpp`

**Architecture:**
```cpp
class MetricsCollector {
 public:
  // Register a metric source
  void register_counter(const std::string& name, CounterPtr counter);
  void register_gauge(const std::string& name, GaugePtr gauge);
  void register_histogram(const std::string& name, HistogramPtr histogram);
  
  // Periodic collection (runs every 1 second)
  void collect_metrics();
  
  // Send to backend
  void flush_to_backend(const std::string& endpoint);
  
 private:
  std::map<std::string, MetricBase*> metrics_;
  std::queue<MetricSnapshot> pending_snapshots_;
};

// Metric types
class Counter {
  void increment(double value = 1.0);
  double get_total() const;
  double get_rate() const; // per second
};

class Gauge {
  void set(double value);
  double get() const;
};

class Histogram {
  void observe(double value);
  std::vector<double> get_percentiles() const; // p50, p95, p99
  double get_mean() const;
};
```

**Metrics Collected:**

| Category | Metric Name | Type | Unit |
|----------|-------------|------|------|
| Search | `search.queries_per_sec` | Counter | queries/sec |
| | `search.latency.histogram` | Histogram | milliseconds |
| | `search.recall_at_10` | Gauge | 0-1 |
| Indexing | `index.documents_indexed_per_sec` | Counter | docs/sec |
| | `index.build_time` | Histogram | seconds |
| | `index.size_bytes` | Gauge | bytes |
| Memory | `memory.heap_usage_bytes` | Gauge | bytes |
| | `memory.index_memory_bytes` | Gauge | bytes |
| | `memory.cache_hit_rate` | Gauge | 0-1 |
| Network | `network.replication_bytes_per_sec` | Counter | bytes/sec |
| | `network.streaming_messages_per_sec` | Counter | messages/sec |
| System | `cpu.usage_percent` | Gauge | 0-100 |
| | `disk.io_bytes_per_sec` | Counter | bytes/sec |

### 5.2 Time-Series Database Integration

**Storage Options:**
- **InfluxDB**: Optimized for metrics, 30-day retention
- **TimescaleDB**: PostgreSQL extension, queryable with SQL

**Aggregation Pipeline:**
```
Raw metrics (1-second granularity)
      ↓
[Aggregation: Sum, Avg, P50, P95, P99, Max]
      ↓
1-minute buckets
      ↓
[Downsampling]
      ↓
1-hour buckets
      ↓
[Downsampling]
      ↓
1-day buckets (retain 365 days)

Query examples:
- "SELECT avg(latency) FROM metrics WHERE time > now() - 24h"
- "SELECT p95(latency) GROUP BY host, service WHERE time > now() - 1h"
```

---

## 6. Pre-built Dashboard Templates

### 6.1 Performance Monitoring Dashboard

**Widgets:**
1. **Query Volume** (line chart)
   - Queries per second over time
   - Compared to historical baseline

2. **Latency Percentiles** (line chart with three series)
   - P50, P95, P99 latencies
   - Color-coded SLO indicators

3. **Search Recall** (gauge)
   - Current recall@10
   - Threshold: 95% (green), 90-95% (yellow), <90% (red)

4. **Error Rate** (stat widget)
   - Percentage of failed queries
   - Trend indicator (up/down/stable)

5. **Top Services by Latency** (table)
   - Sortable by P99 latency
   - Drill-down to service details

### 6.2 Query Analytics Dashboard

**Widgets:**
1. **Query Distribution** (bar chart)
   - Query types (kNN search, range search, filtering)
   - Count per type

2. **Recall vs Quantization Trade-off** (scatter plot)
   - X-axis: compression ratio
   - Y-axis: recall@10
   - Points colored by method (PQ, SQ, OPQ)

3. **Query Plan Efficiency** (heatmap)
   - Time × Service matrix
   - Color intensity = efficiency score

4. **Slow Queries** (table)
   - Query ID, user, duration, recall
   - Click to see query details

### 6.3 Index Health Dashboard

**Widgets:**
1. **Index Size** (gauge)
   - Current size in GB
   - Growth rate

2. **Build Progress** (progress bar)
   - During active indexing
   - ETA completion

3. **Segment Statistics** (table)
   - Number of segments
   - Merge queue size

4. **Cache Hit Rate** (line chart)
   - Over time
   - Spikes indicate working set changes

### 6.4 Ingestion Stats Dashboard

**Widgets:**
1. **Documents Indexed** (stat with sparkline)
   - Documents/second
   - Total count

2. **Ingestion Latency** (histogram)
   - Distribution of doc-to-index latency
   - P99 threshold

3. **Data Sources** (table)
   - Source name
   - Documents ingested today
   - Last update timestamp

### 6.5 Distributed System Overview

**Widgets:**
1. **Node Status Map** (custom 3D visualization)
   - Each node as cube in 3D space
   - Color: green/yellow/red by health
   - Size: by query load

2. **Replication Lag** (heatmap)
   - Rows: nodes, Columns: time
   - Color intensity: replication lag (ms)

3. **Network Topology** (sankey diagram)
   - Flow of data between nodes
   - Width: bandwidth utilization

---

## 7. Chart Library Integration

### 7.1 D3.js Integration

**Use Cases:**
- Custom, interactive visualizations
- Axis-based charts (line, bar, scatter)
- Network diagrams
- Hierarchical layouts

**Example: Custom latency percentile chart**
```typescript
import * as d3 from 'd3';

const drawLatencyChart = (
  svgRef: React.MutableRefObject<SVGSVGElement>,
  data: LatencyData[],
  width: number,
  height: number
) => {
  const margin = {top: 20, right: 30, bottom: 30, left: 60};
  const innerWidth = width - margin.left - margin.right;
  const innerHeight = height - margin.top - margin.bottom;

  const xScale = d3.scaleTime()
    .domain(d3.extent(data, d => new Date(d.timestamp)))
    .range([0, innerWidth]);

  const yScale = d3.scaleLinear()
    .domain([0, d3.max(data, d => d.p99)])
    .range([innerHeight, 0]);

  const svg = d3.select(svgRef.current);
  const g = svg.append('g')
    .attr('transform', `translate(${margin.left},${margin.top})`);

  // Lines for p50, p95, p99
  const lineP50 = d3.line<LatencyData>()
    .x(d => xScale(new Date(d.timestamp)))
    .y(d => yScale(d.p50));

  const lineP95 = d3.line<LatencyData>()
    .x(d => xScale(new Date(d.timestamp)))
    .y(d => yScale(d.p95));

  const lineP99 = d3.line<LatencyData>()
    .x(d => xScale(new Date(d.timestamp)))
    .y(d => yScale(d.p99));

  // Add paths
  g.append('path')
    .datum(data)
    .attr('d', lineP50)
    .attr('stroke', '#10b981')
    .attr('stroke-width', 2);

  g.append('path')
    .datum(data)
    .attr('d', lineP95)
    .attr('stroke', '#f59e0b')
    .attr('stroke-width', 2);

  g.append('path')
    .datum(data)
    .attr('d', lineP99)
    .attr('stroke', '#ef4444')
    .attr('stroke-width', 2);

  // Add axes and legend
  // ...
};
```

### 7.2 Plotly Integration

**Use Cases:**
- 3D scatter plots
- Surface plots
- Scientific visualizations
- Interactive statistics

**Example: 3D vector space visualization**
```typescript
import Plotly from 'plotly.js-dist-min';

const draw3DScatterPlot = (
  containerId: string,
  vectors: Vector[],
  metrics: Metric[]
) => {
  const trace = {
    x: vectors.map(v => v.x),
    y: vectors.map(v => v.y),
    z: vectors.map(v => v.z),
    mode: 'markers',
    type: 'scatter3d',
    marker: {
      size: 5,
      color: metrics.map(m => m.value),
      colorscale: 'Viridis',
      showscale: true,
      colorbar: {title: "Metric"}
    },
    text: vectors.map((v, i) => `Vector ${i}`),
    hovertemplate: '%{text}<br>Value: %{marker.color:.2f}<extra></extra>'
  };

  const layout = {
    scene: {
      xaxis: {title: 'Dim 0'},
      yaxis: {title: 'Dim 1'},
      zaxis: {title: 'Dim 2'}
    }
  };

  Plotly.newPlot(containerId, [trace], layout);
};
```

### 7.3 Three.js Integration

**Use Cases:**
- 3D node networks
- High-dimensional data visualization
- Real-time animation
- Interactive 3D dashboards

**Example: Service mesh visualization**
```typescript
import * as THREE from 'three';

class ServiceMeshVisualizer {
  scene: THREE.Scene;
  camera: THREE.Camera;
  renderer: THREE.WebGLRenderer;

  constructor(container: HTMLElement) {
    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight);
    this.renderer = new THREE.WebGLRenderer();
    container.appendChild(this.renderer.domElement);
  }

  addNode(name: string, health: 'healthy' | 'degraded' | 'critical') {
    const color = {
      healthy: 0x10b981,
      degraded: 0xf59e0b,
      critical: 0xef4444
    }[health];

    const geometry = new THREE.IcosahedronGeometry(1, 4);
    const material = new THREE.MeshPhongMaterial({color});
    const mesh = new THREE.Mesh(geometry, material);
    
    this.scene.add(mesh);
  }

  addEdge(from: string, to: string, bandwidth: number) {
    // Draw line between nodes with width proportional to bandwidth
    const geometry = new THREE.BufferGeometry()
      .setFromPoints([fromPos, toPos]);
    const material = new THREE.LineBasicMaterial({
      color: 0x999999,
      linewidth: bandwidth / 1000
    });
    const line = new THREE.Line(geometry, material);
    this.scene.add(line);
  }

  render() {
    this.renderer.render(this.scene, this.camera);
  }
}
```

---

## 8. Real-time Streaming

### 8.1 WebSocket Connection

**Location:** `ui/src/services/MetricsStreamClient.ts`

**Implementation:**
```typescript
class MetricsStreamClient {
  private ws: WebSocket;
  private reconnectAttempts = 0;
  private maxReconnectAttempts = 10;
  private reconnectDelay = 1000;

  connect(url: string, handlers: StreamHandlers) {
    this.ws = new WebSocket(url);

    this.ws.onopen = () => {
      console.log('Connected to metrics stream');
      this.reconnectAttempts = 0;
      handlers.onConnect?.();
    };

    this.ws.onmessage = (event: MessageEvent) => {
      try {
        const message = JSON.parse(event.data) as MetricsMessage;
        handlers.onMetrics?.(message.metrics);
      } catch (e) {
        console.error('Failed to parse metrics:', e);
      }
    };

    this.ws.onerror = () => {
      handlers.onError?.();
    };

    this.ws.onclose = () => {
      this.attemptReconnect(url, handlers);
    };
  }

  private attemptReconnect(url: string, handlers: StreamHandlers) {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      console.error('Max reconnect attempts reached');
      return;
    }

    const delay = Math.min(
      this.reconnectDelay * Math.pow(2, this.reconnectAttempts),
      30000 // Max 30 seconds
    );

    setTimeout(() => {
      this.reconnectAttempts++;
      console.log(`Attempting to reconnect (${this.reconnectAttempts}/${this.maxReconnectAttempts})`);
      this.connect(url, handlers);
    }, delay);
  }

  subscribe(metrics: string[]) {
    this.ws.send(JSON.stringify({
      type: 'subscribe',
      metrics
    }));
  }

  unsubscribe(metrics: string[]) {
    this.ws.send(JSON.stringify({
      type: 'unsubscribe',
      metrics
    }));
  }
}
```

### 8.2 Server-Side Metrics Push

**Location:** `backend/src/routes/metrics.ts`

**Implementation:**
```typescript
import express from 'express';
import {EventEmitter} from 'events';

const metricsEmitter = new EventEmitter();

app.get('/stream/metrics', (req: express.Request, res: express.Response) => {
  res.setHeader('Content-Type', 'text/event-stream');
  res.setHeader('Cache-Control', 'no-cache');
  res.setHeader('Connection', 'keep-alive');

  const onMetrics = (data: MetricsMessage) => {
    res.write(`data: ${JSON.stringify(data)}\n\n`);
  };

  metricsEmitter.on('metrics', onMetrics);

  req.on('close', () => {
    metricsEmitter.removeListener('metrics', onMetrics);
    res.end();
  });
});

// Push metrics every second
setInterval(() => {
  const metrics = collectMetrics(); // From C++ collector
  metricsEmitter.emit('metrics', {metrics, timestamp: Date.now()});
}, 1000);
```

---

## 9. Alerting & Notifications

### 9.1 Alert Configuration

**Location:** `ui/src/components/AlertBuilder.tsx`

**Interface:**
```typescript
interface AlertRule {
  id: string;
  name: string;
  condition: AlertCondition;
  actions: AlertAction[];
  enabled: boolean;
  createdAt: Date;
  evaluationInterval: number; // seconds
}

type AlertCondition = 
  | SimpleThreshold
  | MultipleMetricComparison
  | AnomalyDetection
  | CompositeExpression;

interface SimpleThreshold {
  type: 'threshold';
  metric: string;
  operator: '>' | '<' | '==' | '!=';
  value: number;
  duration: number; // seconds (e.g., 5 minutes)
}

interface AlertAction {
  type: 'slack' | 'email' | 'pagerduty' | 'webhook';
  target: string;
  message: string;
  tags: string[];
}

// Example alert rule:
const latencyAlert: AlertRule = {
  id: 'alert-1',
  name: 'High P99 Latency',
  condition: {
    type: 'threshold',
    metric: 'search.latency.p99',
    operator: '>',
    value: 1000, // 1 second
    duration: 300 // 5 minutes
  },
  actions: [
    {
      type: 'slack',
      target: '#incidents',
      message: 'P99 latency is above 1s for 5+ minutes',
      tags: ['latency', 'critical']
    },
    {
      type: 'pagerduty',
      target: 'search-team',
      message: 'P99 latency critical',
      tags: ['sev-2']
    }
  ],
  enabled: true,
  evaluationInterval: 60
};
```

### 9.2 Alerting Engine

**Location:** `backend/src/alerts/AlertingEngine.ts`

**Implementation:**
```typescript
class AlertingEngine {
  private rules: Map<string, AlertRule>;
  private evaluationTimers: Map<string, NodeJS.Timer>;
  private notificationManager: NotificationManager;

  registerRule(rule: AlertRule) {
    this.rules.set(rule.id, rule);
    if (rule.enabled) {
      this.startEvaluation(rule);
    }
  }

  private startEvaluation(rule: AlertRule) {
    const timer = setInterval(async () => {
      const isTriggered = await this.evaluateCondition(rule.condition);

      if (isTriggered) {
        // Check if alert is already firing (debounce)
        const alreadyFiring = await this.isAlertFiring(rule.id);
        if (!alreadyFiring) {
          // Fire actions
          for (const action of rule.actions) {
            await this.notificationManager.send(action);
          }
          // Record firing time
          await this.recordAlertFiring(rule.id);
        }
      } else {
        // Alert resolved
        await this.clearAlertFiring(rule.id);
      }
    }, rule.evaluationInterval * 1000);

    this.evaluationTimers.set(rule.id, timer);
  }

  private async evaluateCondition(condition: AlertCondition): Promise<boolean> {
    if (condition.type === 'threshold') {
      const value = await this.getMetricValue(condition.metric);
      return this.compareValues(value, condition.operator, condition.value);
    }
    // Handle other condition types
    return false;
  }
}
```

---

## 10. Export & Scheduling

### 10.1 Export Options

**Per-Widget Exports:**
- CSV: Time-series data with headers
- PNG: High-resolution static image (2x rendering)
- JSON: Raw data structure

**Dashboard Exports:**
- PDF: All widgets on single PDF (multi-page if needed)
- HTML: Standalone interactive dashboard (no server required)
- JSON: Dashboard config + snapshot of current data

**Implementation:**
```typescript
export async function exportWidget(
  widget: DashboardWidget,
  format: 'csv' | 'png' | 'json'
): Promise<Blob> {
  const data = await fetchWidgetData(widget);

  switch (format) {
    case 'csv':
      return exportAsCSV(data);
    case 'png':
      return exportAsPNG(widget, data);
    case 'json':
      return exportAsJSON(widget, data);
  }
}

function exportAsCSV(data: TimeSeriesData[]): Blob {
  const csv = [
    ['timestamp', 'value'],
    ...data.map(d => [d.timestamp, d.value])
  ]
    .map(row => row.map(v => JSON.stringify(v)).join(','))
    .join('\n');

  return new Blob([csv], {type: 'text/csv'});
}

async function exportAsPNG(
  widget: DashboardWidget,
  data: any
): Promise<Blob> {
  // Use html2canvas to render widget to PNG
  const canvas = await html2canvas(document.getElementById(widget.id));
  return new Promise(resolve => {
    canvas.toBlob(blob => resolve(blob!), 'image/png');
  });
}
```

### 10.2 Scheduled Exports

**Scheduler Configuration:**
```typescript
interface ScheduledExport {
  id: string;
  dashboardId: string;
  format: 'pdf' | 'json' | 'csv';
  schedule: CronExpression; // e.g., "0 9 * * MON" = 9 AM every Monday
  recipients: string[]; // Email addresses
  enabled: boolean;
}

// Example: Weekly performance report every Monday
const weeklyReport: ScheduledExport = {
  dashboardId: 'perf-dashboard',
  format: 'pdf',
  schedule: '0 9 * * MON',
  recipients: ['team@company.com']
};
```

---

## 11. Historical Playback

### 11.1 Time-Travel Debugging

**Feature:** Replay metrics at any point in the past

**UI:**
```
┌──────────────────────────────────────────┐
│ Replay: [Timeline slider showing 24h]    │
│ Current: 2026-01-23 14:30 ▶ [Pause]     │
│ Speed: [1x ▼]                            │
│ Step: [◀ Previous] [Next ▶]             │
└──────────────────────────────────────────┘
```

**Implementation:**
```typescript
class HistoricalPlayback {
  private currentTime: Date;
  private playbackSpeed: number = 1;
  private isPlaying: boolean = false;

  async seekToTime(time: Date) {
    this.currentTime = time;
    const metrics = await this.fetchMetricsAt(time);
    this.updateDashboard(metrics);
  }

  play() {
    this.isPlaying = true;
    this.playbackLoop();
  }

  private playbackLoop() {
    if (!this.isPlaying) return;

    const newTime = new Date(
      this.currentTime.getTime() + 1000 * this.playbackSpeed
    );
    this.seekToTime(newTime);

    requestAnimationFrame(() => this.playbackLoop());
  }

  async fetchMetricsAt(time: Date): Promise<Metrics> {
    // Query time-series DB for metrics at specific timestamp
    return fetch(`/api/metrics/at?time=${time.toISOString()}`)
      .then(r => r.json());
  }
}
```

---

## 12. Integration Points

### 12.1 C++ Backend Integration

**Native Binding:**
```cpp
// native-addon/metrics.cc
#include <napi.h>
#include "hektor/monitoring.h"

Napi::Object CollectMetrics(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  auto metrics = hektor::monitoring::collect_current_metrics();
  
  Napi::Object result = Napi::Object::New(env);
  result.Set("search_qps", metrics.search_queries_per_sec);
  result.Set("search_latency_p99_ms", metrics.search_latency_p99_ms);
  result.Set("memory_usage_bytes", metrics.memory_usage_bytes);
  
  return result;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "collectMetrics"),
              Napi::Function::New(env, CollectMetrics));
  return exports;
}

NODE_API_MODULE(metrics, Init)
```

### 12.2 REST API Endpoints

**Core Endpoints:**
```
GET    /api/dashboards
POST   /api/dashboards
GET    /api/dashboards/{id}
PUT    /api/dashboards/{id}
DELETE /api/dashboards/{id}

GET    /api/widgets/library
POST   /api/widgets/execute
GET    /api/widgets/{id}/preview

POST   /api/queries/save
GET    /api/queries
POST   /api/queries/execute
GET    /api/queries/history

GET    /api/alerts
POST   /api/alerts
PUT    /api/alerts/{id}
DELETE /api/alerts/{id}

POST   /api/export/dashboard/{dashboardId}
GET    /api/export/status/{jobId}
GET    /api/export/download/{jobId}

GET    /api/history/metrics?start=...&end=...
GET    /api/history/export?time=...

WS     /ws/metrics/stream
```

---

## 13. Technology Stack

### Frontend
- **React 18+**: UI framework
- **Redux Toolkit**: State management
- **React Query**: Server state management
- **D3.js**: Data visualization
- **Plotly.js**: Scientific plots
- **Three.js**: 3D visualization
- **TailwindCSS**: Styling
- **TypeScript**: Type safety

### Backend
- **Node.js 18+**: Runtime
- **Express**: Web framework
- **Apollo Server**: GraphQL
- **Socket.io**: Real-time WebSocket
- **PostgreSQL**: Dashboard configs + alerts
- **InfluxDB/TimescaleDB**: Time-series metrics
- **Redis**: Caching + message queue
- **node-cron**: Scheduled tasks

### Integrations
- **Slack API**: Notifications
- **PagerDuty API**: Incident management
- **SendGrid**: Email notifications
- **html2canvas**: PNG export
- **pdfkit**: PDF generation

---

## 14. Implementation Phases

### Phase 1: Core Dashboard Infrastructure (Weeks 1-2)
- [ ] React dashboard canvas with grid layout
- [ ] Redux state management
- [ ] Dashboard CRUD operations
- [ ] Local storage persistence
- **Deliverable:** Functional empty dashboard editor

### Phase 2: Stock Widgets (Weeks 3-4)
- [ ] Line chart widget
- [ ] Stat widget
- [ ] Table widget
- [ ] Basic D3.js integration
- [ ] Widget library UI
- **Deliverable:** 5 working stock widgets

### Phase 3: Data Pipeline (Weeks 5-6)
- [ ] C++ metrics collection agent
- [ ] TimescaleDB setup and schema
- [ ] Metrics aggregation pipeline
- [ ] Analytics API (REST endpoints)
- [ ] Data validation and testing
- **Deliverable:** Metrics flowing from C++ → DB → API

### Phase 4: Real-time Streaming (Weeks 7-8)
- [ ] WebSocket connection from frontend
- [ ] Server-Sent Events implementation
- [ ] Metrics streaming from C++ collector
- [ ] Auto-reconnection logic
- [ ] Live dashboard updates
- **Deliverable:** Real-time metrics updating on dashboards

### Phase 5: Advanced Widgets (Weeks 9-10)
- [ ] Gauge widget
- [ ] Heatmap widget
- [ ] 3D scatter plot (Plotly)
- [ ] Service mesh visualization (Three.js)
- [ ] Custom widget plugin system
- **Deliverable:** Advanced visualization toolkit

### Phase 6: Dashboard Templates & Presets (Weeks 11-12)
- [ ] Pre-built templates (Performance, Query Analytics, etc.)
- [ ] Template gallery UI
- [ ] One-click template instantiation
- [ ] Template customization
- **Deliverable:** 5+ production-ready templates

### Phase 7: Alerting & Export (Weeks 13-14)
- [ ] Alert rule builder UI
- [ ] Alert evaluation engine
- [ ] Slack/Email notification integration
- [ ] PDF/PNG/CSV export functionality
- [ ] Scheduled exports
- **Deliverable:** Complete alerting and export system

### Phase 8: Polish & Optimization (Weeks 15-16)
- [ ] Performance optimization (query caching, memoization)
- [ ] Responsive design testing (mobile/tablet)
- [ ] Historical playback feature
- [ ] Comprehensive documentation
- [ ] Security audit and hardening
- **Deliverable:** Production-ready analytics platform

---

## 15. Testing Strategy

### 15.1 Unit Tests

**Frontend Tests (Jest + React Testing Library):**
```typescript
describe('LineChartWidget', () => {
  test('renders with data', () => {
    const data = [
      {timestamp: '2026-01-01T10:00:00Z', value: 100},
      {timestamp: '2026-01-01T10:01:00Z', value: 120}
    ];
    const {getByText} = render(<LineChartWidget data={data} />);
    expect(getByText(/Value/i)).toBeInTheDocument();
  });

  test('updates when data prop changes', () => {
    const {rerender} = render(<LineChartWidget data={data1} />);
    rerender(<LineChartWidget data={data2} />);
    // Verify chart updated
  });
});

describe('DashboardStore', () => {
  test('saves dashboard to localStorage', () => {
    const dashboard = createTestDashboard();
    store.dispatch(saveDashboard(dashboard));
    expect(localStorage.getItem('dashboard-1')).toBeTruthy();
  });
});
```

**Backend Tests:**
```typescript
describe('MetricsAPI', () => {
  test('POST /api/dashboards creates dashboard', async () => {
    const res = await request(app)
      .post('/api/dashboards')
      .send({name: 'Test', widgets: []});
    
    expect(res.status).toBe(201);
    expect(res.body.id).toBeTruthy();
  });

  test('GET /api/metrics returns time-series data', async () => {
    const res = await request(app)
      .get('/api/metrics?metric=search.qps&start=...&end=...');
    
    expect(res.status).toBe(200);
    expect(res.body).toHaveLength(>0);
  });
});
```

### 15.2 Integration Tests

**E2E Dashboard Workflow:**
```
1. Create dashboard
   ↓
2. Add line chart widget
   ↓
3. Configure metric (search.qps)
   ↓
4. Save dashboard
   ↓
5. Reload page
   ↓
6. Verify widget persists and displays data
```

### 15.3 Performance Tests

**Metrics:**
- Dashboard load time: <2 seconds
- Widget rendering: <500ms per widget
- Real-time update latency: <100ms
- Memory usage: <200MB (50-100 widgets)
- Query latency: <500ms (typical queries)

---

## Conclusion

The HEKTOR Analytics & Monitoring Platform delivers enterprise-grade observability tailored for vector databases. With its intuitive dashboard builder, real-time streaming, and intelligent alerting, teams can monitor billion-scale deployments and respond to incidents in seconds.

By combining the flexibility of Grafana with the ease of Superset and optimizing for HEKTOR's specific use cases, we've created a best-in-class analytics solution that sets new standards for vector database monitoring.

---

**Document End**  
*Size: 24.1 KB | Status: Complete | Version: 2.0*
