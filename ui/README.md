# HEKTOR Vector Studio

![Version](https://img.shields.io/badge/version-3.0.0-blue?style=flat-square)
![Angular](https://img.shields.io/badge/Angular-21.0-dd0031?style=flat-square&logo=angular)
![Status](https://img.shields.io/badge/status-development-orange?style=flat-square)

**HEKTOR** is an Agentic Vector Operations Console - a premium Angular-based UI for managing vector databases with AI-powered assistance.

![HEKTOR Dashboard](docs/assets/dashboard-preview.png)

## ✨ Features

### 🗂️ Collection Management
- Create, delete, and reorder vector collections
- Support for multiple distance metrics (cosine, euclidean, dot)
- Dynamic schema builder for metadata fields

### 🔍 Vector Visualization
- **2D/3D Projections** - Interactive Three.js visualization
- **Cluster Analysis** - Color-coded semantic groupings
- **Pan/Zoom/Rotate** - Full orbital controls

### 🤖 AI Agent (Dynamic Tool Registry)
- Natural language interface for database operations
- **RAG for Tools** - Tools stored as vectors for infinite scalability
- Real-time tool retrieval based on user intent

### 📊 Health Monitoring
- Live latency and throughput charts (D3.js)
- Memory usage tracking
- System-wide telemetry support

### 🔐 Authentication
- JWT-based authentication
- Auto token refresh
- Role-based access control (prepared)

### 📥 Data Ingestion
- Drag-and-drop file upload
- Configurable chunking (recursive, fixed, semantic)
- Real-time embedding generation

---

## 🚀 Quick Start

### Prerequisites
- Node.js 20+
- npm or pnpm

### Installation

```bash
# Clone the repository
git clone https://github.com/amuzetnoM/vectorstudio-zgui.git
cd vectorstudio-zgui

# Install dependencies
npm install

# Set your API key (for Gemini embeddings)
export API_KEY="your-google-api-key"

# Start development server
npm run dev
```

### Access
Open [http://localhost:4200](http://localhost:4200) in your browser.

**Dev Mode Login**: Any email (with @) and password (4+ chars) works.

---

## 📁 Project Structure

```
ui/
├── src/
│   ├── app.component.ts          # Main application component
│   ├── app.component.html        # Main template with auth gate
│   ├── components/
│   │   ├── login.component.ts    # Authentication UI
│   │   ├── chat-sidebar.component.ts  # AI Agent chat
│   │   ├── projection-view.component.ts  # 3D visualization
│   │   ├── health-monitor.component.ts   # Telemetry charts
│   │   ├── ingestion-wizard.component.ts # File upload
│   │   ├── playground.component.ts       # Query testing
│   │   └── schema-builder.component.ts   # Metadata schema
│   ├── services/
│   │   ├── auth.service.ts       # JWT authentication
│   │   ├── agent.service.ts      # AI agent with dynamic tools
│   │   └── vector-db.service.ts  # Vector database operations
│   ├── guards/
│   │   └── auth.guard.ts         # Route protection
│   ├── interceptors/
│   │   └── auth.interceptor.ts   # HTTP Bearer token injection
│   ├── utils/
│   │   └── jwt.utils.ts          # JWT decoding utilities
│   ├── environments/
│   │   └── environment.ts        # Configuration
│   └── models/
│       └── core.ts               # TypeScript interfaces
├── docs/                         # Component documentation
├── integration.md                # Enterprise integration blueprint
├── package.json
└── angular.json
```

---

## 🔧 Configuration

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `API_KEY` | Google Gemini API key | Required |
| `API_URL` | Backend API URL | `http://localhost:3000/api` |
| `WS_URL` | WebSocket URL | `ws://localhost:3000/ws` |

### Feature Flags

Edit `src/environments/environment.ts`:

```typescript
export const environment = {
  production: false,
  apiUrl: 'http://localhost:3000/api',
  enableMockAuth: true,  // Set false for real backend
  useBackend: false,     // Set true when BFF is ready
};
```

---

## 🎨 Technology Stack

| Category | Technology |
|----------|------------|
| Framework | Angular 21 |
| Styling | TailwindCSS |
| State | Angular Signals |
| Visualization | Three.js, D3.js |
| AI | Google Gemini SDK |
| Auth | JWT |

---

## 📖 Documentation

| Document | Description |
|----------|-------------|
| [Integration Blueprint](integration.md) | Enterprise architecture & migration guide |
| [Authentication](docs/AUTHENTICATION.md) | JWT auth implementation details |
| [Components](docs/COMPONENTS.md) | Component API reference |
| [Services](docs/SERVICES.md) | Service layer documentation |

---

## 🏗️ Architecture

### Current State (Thick Client)
```
Browser → Angular → Gemini SDK → In-Memory DB
```

### Target State (Thin Client)
```
Browser → Angular → BFF API → Vector DB (Pinecone/Weaviate)
                  ↘ WebSocket → Agent Controller → Gemini
```

See [integration.md](integration.md) for the complete migration blueprint.

---

## 🔐 Authentication

### Development Mode
Mock authentication is enabled by default. Use any email/password:
- Email: Must contain `@`
- Password: Minimum 4 characters
- Include `admin` in email for admin role

### Production Mode
Set `enableMockAuth: false` in environment.ts and implement:

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/auth/login` | POST | Authenticate user |
| `/auth/logout` | POST | Invalidate token |
| `/auth/refresh` | POST | Refresh token |

---

## 🤖 AI Agent

The agent uses a **Dynamic Tool Registry** pattern:

1. User sends natural language prompt
2. System embeds the prompt
3. Searches `system_tools` collection for relevant tools
4. Injects only matching tools into Gemini context
5. Executes tool calls and returns results

### Available Tools
- `create_collection` - Create new vector collection
- `delete_collection` - Delete existing collection
- `add_documents` - Ingest text documents
- `query_vector` - Semantic search

---

## 📊 Visualization

### 2D Mode
- D3.js scatter plot with zoom/pan
- Color-coded clusters

### 3D Mode
- Three.js point cloud
- Orbital controls (rotate, zoom, pan)
- Auto-rotation with pause on interaction
- ~50,000 points @ 60 FPS

---

## 🛠️ Development

### Commands

```bash
# Development server
npm run dev

# Production build
npm run build

# Preview production
npm run preview
```

### Adding New Tools

Add tools by inserting into the `system_tools` collection:

```typescript
await db.addDocuments('system_tools', [{
  content: 'export_csv: Export collection data to CSV file',
  metadata: {
    type: 'tool_definition',
    tool_name: 'export_csv',
    schema: { /* Gemini function schema */ }
  }
}]);
```

---

## 📝 License

MIT License - See [LICENSE](LICENSE) for details.

---

## 🤝 Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing`)
5. Open Pull Request

---

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/amuzetnoM/vectorstudio-zgui/issues)
- **Documentation**: [docs/](docs/)

---

**Built with ❤️ for the AI-native future**
