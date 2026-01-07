# HEKTOR: System Integration & Architectural Blueprint

## 1. Executive Summary
**HEKTOR** is designed as an Agentic Vector Operations Console. Currently implemented as a "Client-Side Simulator" using in-memory mock data and browser-based AI calls, this document outlines the strict architectural path to transform it into a production-grade, distributed application.

The core transformation involves migrating the "Brain" (AI Agent) and the "Memory" (Vector State) from the browser to a secure Backend-for-Frontend (BFF) layer.

---

## 2. High-Level Architecture

The target architecture follows a **Hexagonal (Ports & Adapters)** pattern where the Angular Frontend acts strictly as an adapter for User Interaction, while the business logic resides in the API Layer.

```mermaid
graph TD
    User[User / Browser] <-->|HTTPS / WSS| CDN[CDN / Edge Layer]
    CDN <-->|Static Assets| FE[Angular Client (HEKTOR)]
    
    subgraph "Secure Zone (Backend)"
        FE <-->|REST / Socket.IO| API[API Gateway / BFF]
        
        API <-->|Orchestration| Agent[Agent Controller (LangChain/Native)]
        API <-->|State| VDB_SVC[Vector Service]
        
        Agent <-->|Inference| LLM[Google Gemini API]
        VDB_SVC <-->|Embed/Query| VDB[(Vector Database: Pinecone/Milvus)]
        VDB_SVC <-->|Compute| Worker[Python Math Worker (UMAP/t-SNE)]
        
        API <-->|Cache & PubSub| Redis[(Redis)]
    end
```

---

## 3. Service Layer Refactoring Strategy

The integration hinges on transforming the two primary Angular services from **State Managers** into **State Proxies**.

### A. `VectorDbService` (The Data Proxy)
Currently, this service generates mock data. In production, it must become a facade for the API.

| Feature | Current Implementation (Mock) | Production Implementation (Integration) |
| :--- | :--- | :--- |
| **Data Source** | `signal<Collection[]>` with in-memory arrays. | Reactive `resource` or `signal` derived from HTTP GET streams. |
| **Ingestion** | `setTimeout` simulation. | **Async Job Queue**. The FE uploads files/text, receiving a Job ID. It polls `GET /jobs/:id` or listens via WebSocket for embedding completion. |
| **Search** | Random sorting logic. | `POST /query` sending the raw text. The Backend handles embedding generation and vector similarity search. |
| **Telemetry** | Calculated locally via `computed()`. | WebSocket subscription (`wss://api.hektor.com/telemetry`) streaming real-time latency and throughput stats from the embedding workers. |

### B. `AgentService` (The Orchestrator)
Currently, the API Key and Logic reside in the browser. This is a security vulnerability.

**Refactoring Protocol:**
1.  **Remove `@google/genai` from Client:** The frontend should never import the LLM SDK directly.
2.  **Establish WebSocket Chat:**
    *   Client emits: `event: 'user_message', payload: { text: "Create a collection..." }`
    *   Server processes: Maintains conversation history, calls Gemini, executes Tools (DB operations).
    *   Server emits: `event: 'agent_chunk', payload: { text: "..." }` (Streaming).
    *   Server emits: `event: 'tool_call', payload: { name: 'create_collection', args: {...} }` (For UI feedback only).

---

## 4. Component Hierarchy & Responsibility

The integration requires strict separation of concerns to maintain performance.

*   **`AppComponent` (Shell & Layout)**
    *   *Responsibility:* Auth Guard checks, Global Error Toast handling, WebSocket connection lifecycle.
*   **`ChatWidgetComponent` (Smart)**
    *   *Integration:* Connects to `AgentService`. Handles streaming text reconstruction.
    *   *Refactor:* Must handle "Optimistic Updates" (show the user's message immediately) but wait for Server acknowledgment.
*   **`ProjectionViewComponent` (Dumb/Presentational)**
    *   *Integration:* Takes `VectorDoc[]` as input.
    *   *Critical Performance Note:* The backend **MUST** pre-calculate the 2D/3D projection coordinates (using UMAP/t-SNE). Doing this calculation in the browser (JS) for >10,000 points will freeze the main thread. The Frontend should simply render the `[x,y,z]` coordinates provided by the API.
*   **`HealthMonitorComponent` (Smart)**
    *   *Integration:* Subscribes to `VectorDbService.stats$`.
    *   *Refactor:* Replace internal `setInterval` mock data generation with real D3 data binding from the WebSocket stream.

---

## 5. Data Flow Maps

### Scenario A: "Semantic Search"
1.  **User** types "Finance reports" in `Dashboard`.
2.  **Component** calls `VectorDbService.query('finance reports')`.
3.  **Service** POSTs to `/api/v1/collections/{id}/search`.
4.  **Backend**:
    *   Generates embedding for "Finance reports" using Gemini Embedding Model.
    *   Queries Vector DB (e.g., Pinecone) with vector.
    *   Returns JSON list of matches.
5.  **Service** updates the `filteredDocuments` signal.
6.  **UI** renders the table rows.

### Scenario B: "Agentic Command"
1.  **User** types "Delete the user-profiles collection" in Chat.
2.  **AgentService** sends message via WebSocket.
3.  **Backend (Agent)**:
    *   Receives message.
    *   Gemini determines tool call: `delete_collection(name='user-profiles')`.
    *   **Backend** executes SQL/Vector DB delete command.
    *   **Backend** emits `tool_success` event to Client.
4.  **Frontend**:
    *   `VectorDbService` listens for `collection_deleted` event.
    *   Updates local signal `collections` (removes item).
    *   UI Sidebar updates automatically via Signal tracking.

---

## 6. Integration Contract (API Spec)

The Frontend expects the following JSON structures from the Backend:

### Shared Types
```typescript
// The core entity shared between FE and BE
interface VectorDoc {
  id: string;
  content: string;
  metadata: Record<string, any>;
  // Embedding is usually too large to send to FE, so it is omitted or optional
  vector?: number[]; 
  // CRITICAL: These must be calculated server-side via UMAP
  projection: [number, number]; 
  projection3d: [number, number, number];
  cluster_label?: string;
}
```

### Endpoints
*   `GET /api/v1/collections` - List all collections.
*   `POST /api/v1/collections` - Create new.
*   `GET /api/v1/collections/:id/projection` - **Heavy Payload**. Returns the point cloud data. Should be paginated or use an octree format for large datasets (>100k points).
*   `POST /api/v1/agent/chat` - (Or WebSocket) Chat interface.

---

## 7. Production Readiness Mapping

### Error Handling & Resilience
*   **Global Interceptor:** Implement an `HttpInterceptor` in Angular.
    *   Catch `401 Unauthorized` $\to$ Redirect to OAuth Login.
    *   Catch `504 Gateway Timeout` $\to$ Show "Backend is processing... please wait" (essential for long embedding jobs).
*   **Retry Policy:** Use RxJS `retry({ count: 3, delay: 1000 })` on idempotent GET requests.
*   **Graceful Degradation:** If the `Projection` endpoint fails (calculation error), the Table View should still work.

### Performance & Scalability
*   **Virtual Scrolling:** The `app-projection-view` and the Data Table must implement Virtual Scrolling (`@angular/cdk/scrolling`) if displaying more than 1,000 DOM elements.
*   **LOD (Level of Detail):** For the Three.js view:
    1.  Load first 1,000 points (Centroids).
    2.  Load remaining points in background chunks.
    3.  Only fetch Metadata (`content`) when the user hovers/clicks a node.
*   **Binary Formats:** For massive datasets, switch from JSON to **Protocol Buffers** or **Apache Arrow** for transferring vector data to the browser.

### Security Model
*   **Authentication:** OIDC (OpenID Connect) with a provider like Auth0 or Google Identity.
    *   Frontend holds `access_token` in memory (not localStorage).
    *   Interceptor attaches `Authorization: Bearer <token>` to all requests.
*   **CORS:** Backend must whitelist the production domain.
*   **Input Sanitization:** Although Angular sanitizes HTML by default, the Agent output must be treated carefully if it returns Markdown/HTML.

---

## 8. Immediate Next Steps for Developers

1.  **Type Extraction:** Move all interfaces (`VectorDoc`, `Collection`) from `vector-db.service.ts` to a dedicated `src/types/core.models.ts` file to share with the backend team.
2.  **Environment Configuration:**
    *   Create `environment.prod.ts`.
    *   Add `apiUrl` and `wsUrl` properties.
3.  **Mock Toggle:** Add a feature flag `useMockService` in the `VectorDbService`.
    *   `if (environment.useMock) { return this.mockData; } else { return this.http.get(...) }`
    *   This allows UI development to continue while the API is being built.
4.  **Backend Stub:** Spin up a simple Node.js/Express server that serves `static/mock-data.json` on the `/collections` endpoint to test the HTTP Client integration.
