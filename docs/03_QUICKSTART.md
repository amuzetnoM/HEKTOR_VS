---
title: "Quick Start"
description: "Quick deployment with Docker and Kubernetes"
version: "3.0.0"
last_updated: "2026-01-06"
sidebar_position: 3
category: "Tutorial"
---

Vector Studio can be deployed in **under 5 minutes** using Docker or Kubernetes!

> **Note:** For detailed installation from source, see **[02_INSTALLATION.md](02_INSTALLATION.md)**.

---

## Option 1: Docker Compose (Recommended)

### Prerequisites
- Docker 24.0+
- Docker Compose 2.0+

### Steps

1. **Clone the repository**:
   ```bash
   git clone https://github.com/amuzetnoM/hektor.git
   cd hektor
   ```

2. **Set environment variables**:
   ```bash
   # Create .env file
   cat > .env << EOF
   SECRET_KEY=$(openssl rand -hex 32)
   POSTGRES_PASSWORD=$(openssl rand -hex 16)
   GRAFANA_PASSWORD=admin123
   LOG_LEVEL=INFO
   EOF
   ```

3. **Start the stack**:
   ```bash
   docker-compose up -d
   ```

4. **Wait for services to be ready** (~30 seconds):
   ```bash
   docker-compose logs -f api
   # Wait for "Application startup complete"
   ```

5. **Verify deployment**:
   ```bash
   curl http://localhost:8080/health
   ```

### Access Services

- **API Documentation**: http://localhost:8080/docs
- **Grafana**: http://localhost:3000 (admin/admin123)
- **Prometheus**: http://localhost:9090
- **PostgreSQL**: localhost:5432

### First API Call

```bash
# Login
TOKEN=$(curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  | jq -r '.access_token')

# Create collection
curl -X POST http://localhost:8080/collections \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"name":"test","dimension":384,"metric":"cosine"}'

# Add documents
curl -X POST http://localhost:8080/collections/test/documents/batch \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "documents": [
      {"content": "Machine learning is amazing"},
      {"content": "Deep learning uses neural networks"},
      {"content": "AI is transforming the world"}
    ]
  }'

# Search
curl -X POST http://localhost:8080/collections/test/search \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"query":"artificial intelligence","k":2}'
```

---

## Option 2: Kubernetes

### Prerequisites
- Kubernetes cluster
- kubectl configured
- 8GB+ available RAM

### Steps

1. **Clone repository**:
   ```bash
   git clone https://github.com/amuzetnoM/hektor.git
   cd hektor
   ```

2. **Update secrets**:
   ```bash
   # Edit k8s/configmap.yaml
   # Change SECRET_KEY to a secure value
   nano k8s/configmap.yaml
   ```

3. **Deploy**:
   ```bash
   kubectl apply -f k8s/namespace.yaml
   kubectl apply -f k8s/configmap.yaml
   kubectl apply -f k8s/pvc.yaml
   kubectl apply -f k8s/deployment.yaml
   kubectl apply -f k8s/service.yaml
   kubectl apply -f k8s/hpa.yaml
   ```

4. **Wait for pods**:
   ```bash
   kubectl get pods -n vector-studio -w
   # Wait for all pods to be Running
   ```

5. **Access API**:
   ```bash
   kubectl port-forward -n vector-studio svc/vector-studio-api 8080:80
   curl http://localhost:8080/health
   ```

---

## Option 3: Local Development

**For complete local development setup instructions, see [02_INSTALLATION.md](02_INSTALLATION.md).**

This section provides a quick overview for local development. For detailed steps, build options, and troubleshooting, refer to the full installation guide.

### Prerequisites
- Python 3.11+
- CMake 3.20+
- C++20 compiler
- 4GB+ RAM

### Quick Steps

```bash
# Clone repository
git clone https://github.com/amuzetnoM/vector_studio.git
cd vector_studio

# Install dependencies (see 02_INSTALLATION.md for OS-specific commands)
# Build, download models, and run (see 02_INSTALLATION.md for details)
```

**See [02_INSTALLATION.md](02_INSTALLATION.md) for:**
- Detailed dependency installation for each OS
- Build configuration options
- Model download instructions
- Python environment setup
- Troubleshooting common issues

---

## Next Steps

### 1. Explore the API

Visit http://localhost:8080/docs for interactive API documentation.

### 2. Monitor Performance

- **Grafana**: http://localhost:3000
  - Import dashboard from `k8s/monitoring/grafana-dashboard.json`
  - View real-time metrics

- **Prometheus**: http://localhost:9090
  - Query metrics directly
  - Set up alerts

### 3. Integrate with Your App

**Python Example**:
```python
import requests

# Login
response = requests.post('http://localhost:8080/auth/login',
    json={'username': 'admin', 'password': 'admin123'})
token = response.json()['access_token']

headers = {'Authorization': f'Bearer {token}'}

# Create collection
requests.post('http://localhost:8080/collections',
    headers=headers,
    json={'name': 'my-docs', 'dimension': 384, 'metric': 'cosine'})

# Add documents
requests.post('http://localhost:8080/collections/my-docs/documents/batch',
    headers=headers,
    json={'documents': [
        {'content': 'Your document here'},
        {'content': 'Another document'}
    ]})

# Search
results = requests.post('http://localhost:8080/collections/my-docs/search',
    headers=headers,
    json={'query': 'search query', 'k': 5})

print(results.json())
```

**JavaScript Example**:
```javascript
// Login
const loginResponse = await fetch('http://localhost:8080/auth/login', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({username: 'admin', password: 'admin123'})
});
const {access_token} = await loginResponse.json();

const headers = {
  'Authorization': `Bearer ${access_token}`,
  'Content-Type': 'application/json'
};

// Create collection
await fetch('http://localhost:8080/collections', {
  method: 'POST',
  headers,
  body: JSON.stringify({name: 'my-docs', dimension: 384, metric: 'cosine'})
});

// Search
const searchResponse = await fetch('http://localhost:8080/collections/my-docs/search', {
  method: 'POST',
  headers,
  body: JSON.stringify({query: 'search query', k: 5})
});
const results = await searchResponse.json();
console.log(results);
```

### 4. Configure for Production

See **[16_DEPLOYMENT.md](16_DEPLOYMENT.md)** for:
- Security hardening
- Performance tuning
- High availability setup
- Backup procedures

---

## Troubleshooting

### Port already in use
```bash
# Change port in docker-compose.yml or .env
PORT=8081 docker-compose up -d
```

### Out of memory
```bash
# Reduce workers
WORKERS=2 docker-compose up -d
```

### Can't connect to API
```bash
# Check logs
docker-compose logs api

# Restart services
docker-compose restart
```

### Database errors
```bash
# Reset database
docker-compose down -v
docker-compose up -d
```

---

## Resources

- **Full Documentation**: [00_INDEX.md](00_INDEX.md)
- **Installation Guide**: [02_INSTALLATION.md](02_INSTALLATION.md)
- **API Reference**: http://localhost:8080/docs
- **Deployment Guide**: [16_DEPLOYMENT.md](16_DEPLOYMENT.md)
- **Issues**: https://github.com/amuzetnoM/hektor/issues

---

## Support

Need help?

- 📖 Check the [documentation](docs/)
- 🐛 Report [issues](https://github.com/amuzetnoM/hektor/issues)
- 💬 Join our [Discord](https://discord.gg/vector-studio)

---

**Happy Vector Searching! 🚀**
