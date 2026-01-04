# HEKTOR System Map

This system map breaks down **HEKTOR** into its logical layers. Since the app is currently a "Client-Side Simulator" (using mock data), this map highlights exactly where to inject real backend logic.

---

## 1. High-Level Architecture

The application follows a **Zoneless Angular (Signals)** architecture. It relies on a central Service layer to manage state, which is then projected into "Smart" components.

```
[User Input] --> [Components (UI)]
      |               |
      v               v
[AgentService] <-> [VectorDbService] <--> [Data Visualization]
(AI Logic)         (State & Data)         (Three.js / D3)
      |               |
      v               v
[Google Gemini]    [Mock Data / Future Backend]
```

---

## 2. Service Layer (The Integration Core)

This is where **90% of your integration work** will happen.

### A. VectorDbService (The Data Proxy)

Currently, this acts as an in-memory database.

- **Role**: Manages Collections, Documents, Vectors, and Schema.
- **Current State**: Uses `signal<Collection[]>` and generates mock data (`generateMockDocs`).
- **Integration Point**: Replace the internal methods with HTTP calls to your actual Vector DB (Pinecone, Qdrant, Milvus, Weaviate).
  - `query()`: Replace random sort with `http.post('/search', { vector })`.
  - `addDocuments()`: Replace with `http.post('/upsert', { text })`.
  - `createCollection()`: Call your control plane API.

### B. AgentService (The AI Controller)

This is the "Agentic" layer that drives the UI via natural language.

- **Role**: Manages GoogleGenAI chat session and defines Tools.
- **Key Mechanism**: Tool Use (Function Calling). The AI outputs JSON to call `create_collection`, `query_vector`, etc.
- **Integration Point**:
  - **Add Tools**: If you want the AI to "Scrape a Website" or "Summarize PDF," add a new function declaration in `initChat()` → tools configuration.
  - **Context**: Inject real context about your system into `systemInstruction`.

---

## 3. Component Hierarchy (The UI Layer)

The UI is reactive; it simply renders whatever is in the `VectorDbService` signals.

### AppComponent (Shell)

- **Sidebar**: Collection List & Navigation (Drag-and-drop enabled).
- **Main Stage**:
  - **HealthMonitorComponent**: System stats (D3 charts).
  - **ProjectionViewComponent**:
    - **2D**: D3.js (SVG).
    - **3D**: Three.js (WebGL Point Cloud).
    - **Note**: Requires projection `[x,y]` coordinates. If your backend doesn't provide UMAP/t-SNE coordinates, you must calculate them here or in a backend worker.
  - **SchemaBuilderComponent**: Form to define metadata types.
  - **ChatWidgetComponent**: Floating interface for the `AgentService`.

---

## 4. Data Flow Map

### Scenario A: User Clicks UI

1. User clicks "Create Collection".
2. Component calls `VectorDbService.createCollection()`.
3. Signal `collections` updates.
4. Effects in `AppComponent` auto-refresh the sidebar.

### Scenario B: User Asks AI ("Find documents about finance")

1. User types in `ChatWidget`.
2. `AgentService` sends prompt to Gemini.
3. Gemini decides to call tool `query_vector(text="finance")`.
4. `AgentService` intercepts tool call → executes `VectorDbService.query()`.
5. `VectorDbService` returns results.
6. `AgentService` feeds results back to Gemini.
7. Gemini summarizes results in natural language.
8. UI displays the text response and the raw tool output.

---

## 5. Integration Roadmap (Step-by-Step)

To turn this into a real production console:

### Step 1: Swap the Engine (`vector-db.service.ts`)

- Delete `generateMockDocs`.
- Inject `HttpClient`.
- Refactor `collections` signal to be read-only, populated by a `loadCollections()` API call.

### Step 2: Standardize the Data Model

Ensure your backend API returns objects matching the `VectorDoc` interface, specifically:

- `content` (The text chunk).
- `metadata` (JSON object).
- `projection` (Optional: needed for the graph view. If missing, disable `ProjectionViewComponent`).

### Step 3: Expand the Agent (`agent.service.ts`)

The tool definitions in `initChat` map 1:1 to `VectorDbService` methods. If you add a backend endpoint (e.g., "Export to PDF"), add a corresponding Tool Definition there so the AI knows how to use it.

