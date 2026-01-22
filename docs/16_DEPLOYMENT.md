---
title: "Deployment Guide"
description: "Docker, Kubernetes, and production deployment"
version: "4.0.0"
last_updated: "2026-01-22"
sidebar_position: 16
category: "Operations"
---

![Version](https://img.shields.io/badge/version-4.0.0-blue?style=flat-square)
![Last Updated](https://img.shields.io/badge/updated-2026--01--22-green?style=flat-square)
![Type](https://img.shields.io/badge/type-operations-yellow?style=flat-square)

## Overview

This guide covers deploying Vector Studio to production using Docker, Kubernetes, or bare metal.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Docker Deployment](#docker-deployment)
3. [Kubernetes Deployment](#kubernetes-deployment)
4. [Bare Metal Deployment](#bare-metal-deployment)
5. [Configuration](#configuration)
6. [Monitoring](#monitoring)
7. [Security](#security)
8. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### System Requirements

- **CPU**: 4+ cores (8+ recommended)
- **RAM**: 8GB minimum (16GB+ recommended)
- **Storage**: 50GB+ SSD
- **OS**: Linux (Ubuntu 22.04 LTS recommended), macOS, or Windows

### Software Requirements

- Docker 24.0+ and Docker Compose 2.0+
- OR Kubernetes 1.25+
- OR Python 3.11+ and C++20 compiler

---

## Docker Deployment

### Quick Start

1. **Clone the repository**:
   ```bash
   git clone https://github.com/amuzetnoM/hektor.git
   cd hektor
   ```

2. **Set environment variables**:
   ```bash
   cp .env.example .env
   # Edit .env with your configuration
   nano .env
   ```

3. **Start the stack**:
   ```bash
   docker-compose up -d
   ```

4. **Verify deployment**:
   ```bash
   curl http://localhost:8080/health
   ```

### Production Configuration

Create a `.env` file:

```bash
# Security
SECRET_KEY=your-very-secure-secret-key-min-32-chars
POSTGRES_PASSWORD=your-secure-postgres-password
GRAFANA_PASSWORD=your-grafana-password

# Logging
LOG_LEVEL=INFO

# Version
VERSION=latest
```

### Services

After deployment, the following services will be available:

- **API Server**: http://localhost:8080
  - Health: http://localhost:8080/health
  - Docs: http://localhost:8080/docs
  - Metrics: http://localhost:8080/metrics

- **Prometheus**: http://localhost:9090
- **Grafana**: http://localhost:3000 (admin/admin)
- **PostgreSQL**: localhost:5432

### Scaling

To scale the API service:

```bash
docker-compose up -d --scale api=3
```

---

## Kubernetes Deployment

### Prerequisites

- Kubernetes cluster (EKS, GKE, AKS, or self-hosted)
- kubectl configured
- Helm 3+ (optional but recommended)

### Deploy with kubectl

1. **Create namespace**:
   ```bash
   kubectl apply -f k8s/namespace.yaml
   ```

2. **Configure secrets**:
   ```bash
   # Edit k8s/configmap.yaml with your values
   kubectl apply -f k8s/configmap.yaml
   ```

3. **Create persistent storage**:
   ```bash
   kubectl apply -f k8s/pvc.yaml
   ```

4. **Deploy the application**:
   ```bash
   kubectl apply -f k8s/deployment.yaml
   kubectl apply -f k8s/service.yaml
   kubectl apply -f k8s/ingress.yaml
   kubectl apply -f k8s/hpa.yaml
   ```

5. **Deploy monitoring**:
   ```bash
   kubectl apply -f k8s/monitoring/servicemonitor.yaml
   ```

### Verify Deployment

```bash
# Check pods
kubectl get pods -n vector-studio

# Check services
kubectl get svc -n vector-studio

# View logs
kubectl logs -n vector-studio -l app=vector-studio -f

# Check health
kubectl port-forward -n vector-studio svc/vector-studio-api 8080:80
curl http://localhost:8080/health
```

### Update Deployment

```bash
# Update image
kubectl set image deployment/vector-studio-api \
  api=ghcr.io/amuzetnom/vector_studio:v4.0.0 \
  -n vector-studio

# Rollback if needed
kubectl rollout undo deployment/vector-studio-api -n vector-studio
```

---

## Bare Metal Deployment

### Build from Source

1. **Install dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get update
   sudo apt-get install -y build-essential cmake ninja-build \
     libsqlite3-dev libpq-dev libcurl4-openssl-dev \
     python3.11 python3.11-dev python3-pip

   # macOS
   brew install cmake ninja sqlite postgresql curl python@3.11
   ```

2. **Build C++ backend**:
   ```bash
   cmake -B build -G Ninja \
     -DCMAKE_BUILD_TYPE=Release \
     -DVDB_BUILD_PYTHON=ON \
     -DVDB_USE_AVX2=ON
   cmake --build build -j$(nproc)
   sudo cmake --install build
   ```

3. **Install Python dependencies**:
   ```bash
   pip3 install -r requirements.txt
   pip3 install -r api/requirements.txt
   cd build && pip3 install .
   ```

4. **Download models**:
   ```bash
   python3 scripts/download_models.py --all
   ```

5. **Run API server**:
   ```bash
   cd api
   uvicorn main:app --host 0.0.0.0 --port 8080 --workers 4
   ```

### Systemd Service

Create `/etc/systemd/system/vector-studio.service`:

```ini
[Unit]
Description=Vector Studio API Server
After=network.target

[Service]
Type=simple
User=vectordb
WorkingDirectory=/opt/vector-studio/api
Environment="PATH=/usr/local/bin:/usr/bin"
Environment="VDB_PATH=/var/lib/vector-studio/data"
ExecStart=/usr/local/bin/uvicorn main:app --host 0.0.0.0 --port 8080 --workers 4
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable vector-studio
sudo systemctl start vector-studio
sudo systemctl status vector-studio
```

---

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `VDB_PATH` | `./data/vectors` | Database storage path |
| `SECRET_KEY` | - | JWT secret key (required) |
| `HOST` | `0.0.0.0` | API server host |
| `PORT` | `8080` | API server port |
| `WORKERS` | `4` | Number of worker processes |
| `LOG_LEVEL` | `INFO` | Logging level |
| `CORS_ORIGINS` | `*` | Allowed CORS origins |
| `RATE_LIMIT_ENABLED` | `true` | Enable rate limiting |
| `RATE_LIMIT_DEFAULT` | `100/minute` | Default rate limit |

### Performance Tuning

**For high throughput**:
```bash
WORKERS=8
RATE_LIMIT_DEFAULT=1000/minute
```

**For low latency**:
```bash
WORKERS=2
VDB_USE_AVX2=ON
VDB_USE_AVX512=ON  # If CPU supports it
```

---

## Monitoring

### Prometheus Metrics

Available metrics:

- `vdb_api_requests_total` - Total API requests
- `vdb_api_request_duration_seconds` - Request latency
- `vdb_api_active_connections` - Active connections
- `vdb_operations_total` - Database operations
- `vdb_vectors_total` - Total vectors stored

### Grafana Dashboards

1. Access Grafana: http://localhost:3000
2. Login: admin / admin (change password!)
3. Import dashboard from `k8s/monitoring/grafana-dashboard.json`

### Alerts

Configure alerts in Prometheus for:

- High error rate (>5%)
- High latency (P95 >500ms)
- Low disk space (<10%)
- High memory usage (>90%)

---

## Security

### Best Practices

1. **Change default credentials**:
   ```bash
   # Generate secure secret key
   openssl rand -hex 32
   ```

2. **Enable HTTPS**:
   - Use reverse proxy (nginx, Caddy)
   - Configure TLS certificates
   - Redirect HTTP to HTTPS

3. **Firewall rules**:
   ```bash
   # Allow only necessary ports
   sudo ufw allow 8080/tcp
   sudo ufw enable
   ```

4. **Regular updates**:
   ```bash
   docker-compose pull
   docker-compose up -d
   ```

5. **Backup data**:
   ```bash
   # Backup vector database
   tar -czf backup-$(date +%Y%m%d).tar.gz data/

   # Backup PostgreSQL
   docker exec vector-studio-postgres pg_dump -U vectordb vectordb > backup.sql
   ```

### Authentication

The API uses JWT tokens. To get a token:

```bash
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```

Use the token in subsequent requests:

```bash
curl -H "Authorization: Bearer YOUR_TOKEN" \
  http://localhost:8080/collections
```

---

## Troubleshooting

### Common Issues

**1. API won't start**

```bash
# Check logs
docker-compose logs api

# Common causes:
# - Port 8080 already in use
# - Missing environment variables
# - Database connection failed
```

**2. High memory usage**

```bash
# Reduce workers
WORKERS=2 docker-compose up -d

# Check vector count
curl http://localhost:8080/stats
```

**3. Slow search**

```bash
# Enable AVX2 optimization
# Rebuild with: -DVDB_USE_AVX2=ON

# Check index size
curl http://localhost:8080/stats
```

**4. Database errors**

```bash
# Check PostgreSQL
docker-compose logs postgres

# Reset database
docker-compose down -v
docker-compose up -d
```

### Debug Mode

Enable debug logging:

```bash
LOG_LEVEL=DEBUG docker-compose up -d
docker-compose logs -f api
```

### Performance Profiling

```bash
# Install profiling tools
pip install py-spy

# Profile API server
py-spy top --pid $(pgrep -f uvicorn)
```

---

## Support

- **Documentation**: https://github.com/amuzetnoM/hektor/tree/main/docs
- **Issues**: https://github.com/amuzetnoM/hektor/issues
- **Discord**: https://discord.gg/vector-studio

---

## License

MIT License - see LICENSE file for details
