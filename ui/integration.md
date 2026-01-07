# HEKTOR: Enterprise Integration & Architectural Blueprint v2.1

## 1. System Overview & Status
**HEKTOR** is an Agentic Vector Operations Console.
*   **Current State:** "Thick Client" / Serverless. The Angular application currently handles business logic, embedding generation (via direct Google GenAI SDK calls), and in-memory vector search. It is fully functional for small-scale datasets (<5,000 vectors) using the user's API Key.
*   **Target State:** "Thin Client" / Enterprise Platform. The Angular application becomes a UI Adapter for a scalable backend that handles long-running ingestion jobs, massive vector storage (Milvus/Pinecone), and heavy dimensionality reduction (UMAP).

---

## 2. Target Architecture (The Hexagonal Pattern)

We are moving from a 2-Tier (Browser <-> LLM) to a 3-Tier architecture.

```mermaid
graph TD
    User[User Device] <-->|HTTPS / WSS| CDN[Edge / CDN]
    CDN <-->|Load| FE[Angular Client (HEKTOR)]
    
    subgraph "Infrastructure Layer"
        FE <-->|REST (Commands)| API[BFF API Gateway (NestJS/Go)]
        FE <-->|WebSocket (Streams)| API
        
        API <-->|State| REDIS[(Redis Pub/Sub)]
        API <-->|Vector Ops| VDB[(Vector DB: Pinecone/Weaviate)]
        
        subgraph "Async Compute Workers"
            INGEST[Ingestion Worker]
            MATH[Dimensionality Worker (UMAP/t-SNE)]
        end
        
        API -->|Job Queue| INGEST
        API -->|Job Queue| MATH
        
        INGEST <-->|Embed| LLM[Google Gemini API]
        AGENT[Agent Controller] <-->|Inference| LLM
    end
```

---

## 3. Core Service Migration Strategy

### A. `VectorDbService` (The Data Layer)
Currently, this service manages `signal<Collection[]>` in memory. In production, this state must be remote.

| Feature | Current Implementation (Client-Side) | Production Protocol (Server-Side) |
| :--- | :--- | :--- |
| **Storage** | In-Memory Arrays. | **Database**: Persisted in Vector DB (indexes) + PostgreSQL (metadata/schema). |
| **Ingestion** | `ingestData()`: Reads File -> Chunks -> Calls Gemini Embed -> Push to Array. | **Multipart Upload**: FE sends files + Config to `POST /api/ingest`. Backend generates Job ID. FE polls/listens for completion. |
| **Search** | `cosineSimilarity()` in JS. | **Offload**: `POST /api/search`. Backend performs ANN (Approximate Nearest Neighbor) search. |
| **Projection** | Heuristic (Scaling vectors). | **Compute**: Backend runs UMAP on the heavy vectors and returns just the `[x,y,z]` coords to FE. |

### B. `AgentService` (The Orchestrator)
Currently, the Browser holds the `ChatSession` state and executes tools. This exposes the API Key.

**Refactoring Requirements:**
1.  **Remove SDK:** Uninstall `@google/genai` from the Frontend.
2.  **WebSocket Protocol:**
    *   **Upstream:** `{ type: 'prompt', text: "Create a collection", context: { currentCollection: 'v1' } }`
    *   **Downstream (Token):** `{ type: 'chunk', content: "Sure, I..." }`
    *   **Downstream (Action):** `{ type: 'tool_start', name: 'create_collection' }`
    *   **Downstream (Result):** `{ type: 'tool_end', result: "Success" }`

---

## 4. The Dynamic Tool Registry ("RAG for Tools")
**Requirement:** The user asked if models can "learn" tools from the database.
**Solution:** Store tool definitions in the Vector DB itself.

### The `system_tools` Collection
Create a reserved collection named `system_tools` (Dimension: 768).

**Schema:**
```json
{
  "name": "create_invoice",
  "description": "Generates a PDF invoice for a client...",
  "parameters": { ...JSON Schema... },
  "permission_level": "admin"
}
```

### Execution Flow
1.  **User Prompt:** "Please generate an invoice for Acme Corp."
2.  **Agent Logic (Backend):**
    *   Generate embedding for the prompt.
    *   **Search `system_tools`:** Query the vector DB for tools semantically related to "generate invoice".
    *   **Retrieve:** Get top 3 tools (e.g., `create_invoice`, `send_email`, `calculate_tax`).
    *   **Inference:** Call Gemini API, injecting *only* these 3 tools into the `tools` configuration.
3.  **Result:** The model calls the correct tool without ever having it hardcoded in the prompt context. This allows for **Infinite Tool Scalability**.

---

## 5. Ingestion Pipeline Specification
The `IngestionWizardComponent` gathers complex configuration that must be serialized correctly.

**Endpoint:** `POST /api/v1/collections/{id}/ingest`
**Content-Type:** `multipart/form-data`

**Payload Structure:**
1.  **`files`**: Binary file streams (PDF, TXT, JSON).
2.  **`config`**: JSON String
    ```json
    {
      "chunkSize": 512,
      "chunkOverlap": 50,
      "strategy": "recursive" | "fixed" | "semantic",
      "embeddingModel": "text-embedding-004"
    }
    ```

**Backend Behavior:**
1.  Receive files.
2.  Acknowledge with `202 Accepted` and `jobId`.
3.  **Worker:** Extract text -> Chunk (LangChain) -> Embed (Gemini) -> Upsert (Pinecone).
4.  **Worker:** Calculate new UMAP projection (incrementally if possible).
5.  **Notify:** Push update via WebSocket to Client to refresh `ProjectionView`.

---

## 6. Telemetry & Monitoring Integration
The `HealthMonitorComponent` currently uses `db.telemetry$` (RxJS Subject) to visualize local API latency.

**Production Stream:**
The backend must emit system-wide telemetry, not just the current user's latency.

**WebSocket Topic:** `system:metrics`
```json
{
  "timestamp": 1715431200000,
  "global_ingest_rate": 4500, // docs/sec
  "p99_latency_ms": 120,
  "active_collections": 58,
  "cluster_health": "green"
}
```
The Frontend maps this JSON directly into the D3 Charts in `HealthMonitorComponent`.

---

## 7. Visualization & Compute (The "Math" Problem)
**Critical Constraint:** The `ProjectionViewComponent` expects `[x,y]` and `[x,y,z]` coordinates. Real vector embeddings are 768 or 1536 dimensions.

**The Problem:** Running UMAP/t-SNE on 10,000 vectors (1536d) in the browser via WebAssembly is possible but slow (5-10s freeze).

**The Solution (Server-Side Calculation):**
1.  **Trigger:** When ingestion completes.
2.  **Compute:** Python worker (using `umap-learn`) fetches vectors, reduces to 3D.
3.  **Storage:** Save `projection_x`, `projection_y`, `projection_z` in SQL/Metadata DB.
4.  **Fetch:** `GET /api/v1/collections/{id}/points` returns **only**:
    ```json
    [
      { "id": "1", "x": 0.12, "y": -0.4, "z": 0.5, "c": 1 },
      ...
    ]
    ```
    *Payload Size:* ~500KB for 10k points (manageable).

**Client-Side Rendering (New Capabilities):**
The Frontend uses **Three.js** with **OrbitControls** for interaction.
*   **Zoom/Pan:** Supported via mouse wheel and right-click drag.
*   **Rotate:** Left-click drag.
*   **Auto-Rotation:** Enabled by default, pauses on user interaction.
*   **Performance:** Can handle ~50,000 points at 60FPS using `THREE.Points` and BufferGeometry. For >100k points, we must switch to custom shaders or level-of-detail loading from the backend.

---

## 8. API Specification (BFF Contract)

### Collections
*   `GET /collections` - Summary list (name, count, metric).
*   `POST /collections` - Create (name, dim, model).
*   `DELETE /collections/:name` - Drop.

### Documents
*   `GET /collections/:name/query?q=...&topK=10` - Semantic Search.
*   `GET /collections/:name/points` - **Binary/Protobuf** preferred for point cloud data.
*   `POST /collections/:name/ingest` - File upload.

### Agent
*   `WS /agent/chat` - Stateful bi-directional stream.

---

## 9. Security & Deployment

1.  **Environment Variables**:
    *   **Frontend**: `API_URL`, `WS_URL` (No API Keys!).
    *   **Backend**: `GOOGLE_API_KEY`, `PINECONE_KEY`, `REDIS_URL`.
2.  **Authentication**:
    *   **JWT** Strategy.
    *   `AppComponent` checks `AuthService.isAuthenticated()`.
    *   All HTTP calls include `Authorization: Bearer <token>`.
3.  **Dockerization**:
    *   **Frontend**: Multi-stage build (Node build -> Nginx Alpine).
    *   **Backend**: Node.js 20+ (Distroless).
    *   **Worker**: Python 3.11 (for efficient UMAP/Pandas support).
