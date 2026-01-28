# Vector Studio REST API

Robust FastAPI wrapper around the Vector Studio C++ backend.

## Features

- **Full CRUD Operations** - Collections and documents
- **Semantic Search** - Real ONNX embeddings with HNSW index
- **Authentication** - JWT-based auth with role-based access
- **Rate Limiting** - Configurable per-endpoint limits
- **CORS Support** - Cross-origin requests enabled
- **Prometheus Metrics** - Built-in observability
- **Health Checks** - Kubernetes-ready endpoints
- **OpenAPI Docs** - Auto-generated API documentation
- **Type Safety** - Full Pydantic validation

## Quick Start

### Installation

```bash
# Install dependencies
pip install -r requirements.txt

# Copy environment template
cp .env.example .env

# Edit configuration
nano .env
```

### Running Locally

```bash
# Development mode (auto-reload)
uvicorn main:app --reload --host 0.0.0.0 --port 8080

# Production mode
uvicorn main:app --host 0.0.0.0 --port 8080 --workers 4
```

### Using Docker

```bash
# Build image
docker build -t vector-studio-api ..

# Run container
docker run -p 8080:8080 \
  -v $(pwd)/data:/data \
  -e SECRET_KEY=your-secret-key \
  vector-studio-api
```

## API Documentation

Once running, visit:

- **Interactive Docs**: http://localhost:8080/docs
- **ReDoc**: http://localhost:8080/redoc
- **OpenAPI JSON**: http://localhost:8080/openapi.json

## Authentication

### Login

```bash
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```

Response:
```json
{
  "access_token": "eyJ0eXAiOiJKV1QiLCJhbGc...",
  "token_type": "bearer"
}
```

### Using the Token

```bash
export TOKEN="your-access-token"

curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:8080/collections
```

## API Examples

### Create Collection

```bash
curl -X POST http://localhost:8080/collections \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "my-docs",
    "dimension": 384,
    "metric": "cosine"
  }'
```

### Add Documents

```bash
curl -X POST http://localhost:8080/collections/my-docs/documents/batch \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "documents": [
      {
        "content": "Machine learning is a subset of AI",
        "metadata": {"category": "ml"}
      },
      {
        "content": "Deep learning uses neural networks",
        "metadata": {"category": "dl"}
      }
    ]
  }'
```

### Search

```bash
curl -X POST http://localhost:8080/collections/my-docs/search \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "artificial intelligence",
    "k": 5
  }'
```

Response:
```json
[
  {
    "id": "vec_abc123",
    "score": 0.95,
    "content": "Machine learning is a subset of AI",
    "metadata": {"category": "ml"}
  }
]
```

### Get Statistics

```bash
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:8080/stats
```

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `VDB_PATH` | `./data/vectors` | Database storage path |
| `SECRET_KEY` | **Required** | JWT secret key |
| `ACCESS_TOKEN_EXPIRE_MINUTES` | `60` | Token expiration time |
| `HOST` | `0.0.0.0` | Server host |
| `PORT` | `8080` | Server port |
| `WORKERS` | `4` | Number of workers |
| `LOG_LEVEL` | `INFO` | Logging level |
| `DEBUG` | `false` | Debug mode |
| `CORS_ORIGINS` | `*` | Allowed origins |
| `RATE_LIMIT_ENABLED` | `true` | Enable rate limiting |
| `RATE_LIMIT_DEFAULT` | `100/minute` | Default rate limit |

### Example .env File

```bash
# Database
VDB_PATH=/data/vectors

# Security
SECRET_KEY=your-very-secure-secret-key-min-32-chars
ACCESS_TOKEN_EXPIRE_MINUTES=60

# Server
HOST=0.0.0.0
PORT=8080
WORKERS=4
DEBUG=false
LOG_LEVEL=INFO

# CORS
CORS_ORIGINS=http://localhost:4200,https://app.example.com

# Rate Limiting
RATE_LIMIT_ENABLED=true
RATE_LIMIT_DEFAULT=100/minute
```

## Monitoring

### Health Check

```bash
curl http://localhost:8080/health
```

Response:
```json
{
  "status": "healthy",
  "version": "3.0.0",
  "database": "healthy",
  "uptime_seconds": 3600.5
}
```

### Prometheus Metrics

```bash
curl http://localhost:8080/metrics
```

Available metrics:
- `vdb_api_requests_total` - Total requests
- `vdb_api_request_duration_seconds` - Request latency
- `vdb_api_active_connections` - Active connections
- `vdb_operations_total` - Database operations
- `vdb_operation_duration_seconds` - Operation latency
- `vdb_vectors_total` - Total vectors

## Error Handling

The API uses standard HTTP status codes:

- `200` - Success
- `201` - Created
- `400` - Bad Request (validation error)
- `401` - Unauthorized (invalid/missing token)
- `403` - Forbidden
- `404` - Not Found
- `422` - Unprocessable Entity
- `429` - Too Many Requests (rate limit)
- `500` - Internal Server Error

Error response format:
```json
{
  "detail": "Error message here"
}
```

## Rate Limiting

Default limits:
- Authentication: 5 requests/minute
- Batch operations: 10 requests/minute
- Other endpoints: 100 requests/minute

Override with `RATE_LIMIT_DEFAULT` environment variable.

## Security

### Best Practices

1. **Always use HTTPS in production**
2. **Change default credentials**
3. **Use strong SECRET_KEY** (min 32 characters)
4. **Enable rate limiting**
5. **Restrict CORS origins**
6. **Regular security updates**

### Generating Secure Keys

```bash
# Generate SECRET_KEY
openssl rand -hex 32

# Or with Python
python -c "import secrets; print(secrets.token_hex(32))"
```

## Performance

### Benchmarks

On a 4-core CPU with 8GB RAM:

- **Search latency**: 2-5ms (100K vectors)
- **Throughput**: 500+ queries/second
- **Indexing**: 100+ documents/second

### Optimization Tips

1. **Increase workers** for high concurrency
2. **Enable AVX2/AVX-512** for faster distance calculations
3. **Use SSD storage** for better I/O
4. **Tune rate limits** based on load
5. **Monitor metrics** to identify bottlenecks

## Testing

### Run Integration Tests

```bash
# Start API server
uvicorn main:app --host 0.0.0.0 --port 8080 &

# Run tests
pytest ../tests/test_api_integration.py -v

# Stop server
kill %1
```

### Manual Testing

```bash
# Test health
curl http://localhost:8080/health

# Test authentication
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'

# Test metrics
curl http://localhost:8080/metrics
```

## Deployment

See [DEPLOYMENT.md](../docs/DEPLOYMENT.md) for production deployment guide.

### Quick Deploy with Docker Compose

```bash
cd ..
docker-compose up -d
```

### Kubernetes

```bash
kubectl apply -f ../k8s/
```

## Troubleshooting

### API won't start

```bash
# Check logs
tail -f api.log

# Common issues:
# - Port already in use
# - Missing SECRET_KEY
# - Database path not writable
```

### High latency

```bash
# Check metrics
curl http://localhost:8080/metrics | grep duration

# Increase workers
WORKERS=8 uvicorn main:app

# Enable debug logging
LOG_LEVEL=DEBUG uvicorn main:app
```

### Database errors

```bash
# Check database path
ls -la $VDB_PATH

# Check permissions
chmod 755 $VDB_PATH

# Reset database
rm -rf $VDB_PATH/*
```

## Development

### Code Structure

```
api/
├── main.py              # FastAPI application
├── requirements.txt     # Python dependencies
├── .env.example        # Environment template
└── README.md           # This file
```

### Adding New Endpoints

1. Define Pydantic models
2. Add route handler
3. Update OpenAPI docs
4. Add tests
5. Update this README

### Running Tests

```bash
pytest ../tests/test_api_integration.py -v --tb=short
```

## License

MIT License - see ../LICENSE for details

## Support

- **Issues**: https://github.com/amuzetnoM/hektor/issues
- **Docs**: https://github.com/amuzetnoM/hektor/tree/main/docs
- **Discord**: https://discord.gg/vector-studio
