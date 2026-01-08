# OpenTelemetry Installation and Setup Guide

## Quick Start

### Option 1: Build without OpenTelemetry (Fallback Mode)

```bash
cd /home/runner/work/hektor/hektor
mkdir build && cd build
cmake .. -DVDB_USE_OPENTELEMETRY=OFF
cmake --build . --config Release
```

**Features in Fallback Mode**:
- ✓ Logging-based tracing
- ✓ Same API (no code changes)
- ✓ Console output for spans
- ✓ Useful for development

### Option 2: Build with OpenTelemetry (Full Features)

#### Step 1: Install OpenTelemetry C++ SDK

**Via vcpkg (Recommended)**:
```bash
# Install vcpkg if not already installed
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Install OpenTelemetry
./vcpkg install opentelemetry-cpp
```

**Via apt (Ubuntu 22.04+)**:
```bash
sudo apt-get update
sudo apt-get install -y \
    libopentelemetry-cpp-dev \
    libopentelemetry-exporter-otlp-dev \
    libopentelemetry-exporter-prometheus-dev
```

**Build from Source**:
```bash
git clone --recursive https://github.com/open-telemetry/opentelemetry-cpp.git
cd opentelemetry-cpp
mkdir build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_SHARED_LIBS=ON \
    -DWITH_OTLP_GRPC=ON \
    -DWITH_OTLP_HTTP=ON \
    -DWITH_PROMETHEUS=ON \
    -DWITH_JAEGER=ON \
    -DWITH_ZIPKIN=ON \
    -DWITH_EXAMPLES=OFF \
    -DWITH_TESTS=OFF

make -j$(nproc)
sudo make install
```

#### Step 2: Build Vector Studio with OpenTelemetry

```bash
cd /home/runner/work/hektor/hektor
mkdir build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DVDB_USE_OPENTELEMETRY=ON \
    -DCMAKE_PREFIX_PATH=/usr/local

cmake --build . --config Release --parallel
```

#### Step 3: Verify Installation

```bash
# Check if telemetry library is linked
ldd ./build/hektor | grep opentelemetry

# Run with telemetry enabled
./build/hektor --help
```

## Observability Stack Setup

### Docker Compose Deployment

Create `docker-compose-observability.yml`:

```yaml
version: '3.8'

services:
  # Grafana Tempo (Trace Backend)
  tempo:
    image: grafana/tempo:latest
    command: [ "-config.file=/etc/tempo.yaml" ]
    volumes:
      - ./tempo.yaml:/etc/tempo.yaml
      - tempo-data:/tmp/tempo
    ports:
      - "3200:3200"   # Tempo UI
      - "4317:4317"   # OTLP gRPC
      - "4318:4318"   # OTLP HTTP
    networks:
      - observability

  # Prometheus (Metrics)
  prometheus:
    image: prom/prometheus:latest
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
      - prometheus-data:/prometheus
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
    ports:
      - "9090:9090"
    networks:
      - observability

  # Grafana (Visualization)
  grafana:
    image: grafana/grafana:latest
    environment:
      - GF_AUTH_ANONYMOUS_ENABLED=true
      - GF_AUTH_ANONYMOUS_ORG_ROLE=Admin
      - GF_SECURITY_ADMIN_PASSWORD=admin
    volumes:
      - ./grafana-datasources.yml:/etc/grafana/provisioning/datasources/datasources.yml
      - grafana-data:/var/lib/grafana
    ports:
      - "3000:3000"
    networks:
      - observability
    depends_on:
      - tempo
      - prometheus

  # OpenTelemetry Collector (Optional but recommended)
  otel-collector:
    image: otel/opentelemetry-collector-contrib:latest
    command: ["--config=/etc/otel-collector-config.yaml"]
    volumes:
      - ./otel-collector-config.yaml:/etc/otel-collector-config.yaml
    ports:
      - "4317:4317"   # OTLP gRPC
      - "4318:4318"   # OTLP HTTP
      - "8888:8888"   # Collector metrics
      - "13133:13133" # Health check
    networks:
      - observability

  # Jaeger (Alternative to Tempo)
  jaeger:
    image: jaegertracing/all-in-one:latest
    environment:
      - COLLECTOR_OTLP_ENABLED=true
    ports:
      - "16686:16686"  # Jaeger UI
      - "14268:14268"  # Jaeger collector
      - "4317:4317"    # OTLP gRPC
    networks:
      - observability

networks:
  observability:
    driver: bridge

volumes:
  tempo-data:
  prometheus-data:
  grafana-data:
```

### Configuration Files

**tempo.yaml**:
```yaml
server:
  http_listen_port: 3200

distributor:
  receivers:
    otlp:
      protocols:
        grpc:
          endpoint: 0.0.0.0:4317
        http:
          endpoint: 0.0.0.0:4318

storage:
  trace:
    backend: local
    local:
      path: /tmp/tempo/blocks

query_frontend:
  search:
    enabled: true
```

**prometheus.yml**:
```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s
  external_labels:
    cluster: 'vector-studio'
    environment: 'development'

scrape_configs:
  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9090']

  - job_name: 'vector-studio'
    static_configs:
      - targets: ['host.docker.internal:9090']
    metrics_path: '/metrics'

  - job_name: 'otel-collector'
    static_configs:
      - targets: ['otel-collector:8888']
```

**grafana-datasources.yml**:
```yaml
apiVersion: 1

datasources:
  - name: Prometheus
    type: prometheus
    access: proxy
    url: http://prometheus:9090
    isDefault: false
    editable: true

  - name: Tempo
    type: tempo
    access: proxy
    url: http://tempo:3200
    isDefault: true
    editable: true
    jsonData:
      tracesToLogs:
        datasourceUid: 'loki'
      tracesToMetrics:
        datasourceUid: 'prometheus'
      serviceMap:
        datasourceUid: 'prometheus'
```

**otel-collector-config.yaml**:
```yaml
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317
      http:
        endpoint: 0.0.0.0:4318

processors:
  batch:
    timeout: 1s
    send_batch_size: 1024
  
  memory_limiter:
    check_interval: 1s
    limit_mib: 512
  
  # Add resource attributes
  resource:
    attributes:
      - key: service.namespace
        value: vector-studio
        action: upsert

exporters:
  # Export to Tempo
  otlp/tempo:
    endpoint: tempo:4317
    tls:
      insecure: true
  
  # Export to Prometheus
  prometheus:
    endpoint: "0.0.0.0:8889"
  
  # Export to Jaeger
  jaeger:
    endpoint: jaeger:14250
    tls:
      insecure: true
  
  # Debug: print to console
  logging:
    loglevel: info

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [memory_limiter, batch, resource]
      exporters: [otlp/tempo, jaeger, logging]
    
    metrics:
      receivers: [otlp]
      processors: [memory_limiter, batch]
      exporters: [prometheus, logging]
```

### Start the Stack

```bash
# Start all services
docker-compose -f docker-compose-observability.yml up -d

# Check status
docker-compose -f docker-compose-observability.yml ps

# View logs
docker-compose -f docker-compose-observability.yml logs -f tempo

# Access UIs
# Grafana: http://localhost:3000 (admin/admin)
# Prometheus: http://localhost:9090
# Tempo: http://localhost:3200
# Jaeger: http://localhost:16686
```

## Running Vector Studio with Telemetry

### Environment Variables

```bash
export OTEL_SERVICE_NAME="vector-studio"
export OTEL_SERVICE_VERSION="3.0.0"
export OTEL_DEPLOYMENT_ENVIRONMENT="development"
export OTEL_RESOURCE_ATTRIBUTES="service.namespace=vectordb,host.name=$(hostname)"

# OTLP Exporter
export OTEL_EXPORTER_OTLP_ENDPOINT="http://localhost:4317"
export OTEL_EXPORTER_OTLP_PROTOCOL="grpc"

# Sampling
export OTEL_TRACES_SAMPLER="parentbased_traceidratio"
export OTEL_TRACES_SAMPLER_ARG="1.0"  # 100% for development

# Prometheus Metrics
export OTEL_METRICS_EXPORTER="prometheus"
export OTEL_EXPORTER_PROMETHEUS_HOST="0.0.0.0"
export OTEL_EXPORTER_PROMETHEUS_PORT="9090"
```

### Configuration File

Create `telemetry-config.json`:

```json
{
  "telemetry": {
    "enabled": true,
    "service_name": "vector-studio",
    "service_version": "3.0.0",
    "deployment_environment": "development",
    "sampling": {
      "ratio": 1.0,
      "tail_based": false
    },
    "exporters": {
      "traces": {
        "type": "otlp",
        "endpoint": "http://localhost:4317",
        "headers": {}
      },
      "metrics": {
        "type": "prometheus",
        "host": "0.0.0.0",
        "port": 9090
      }
    },
    "resource_attributes": {
      "service.namespace": "vectordb",
      "deployment.environment": "development"
    }
  }
}
```

### Start Vector Studio

```bash
# With environment variables
./build/hektor --config config.yaml

# Or programmatically in code
```

## Kubernetes Deployment

### Helm Values

Create `values-telemetry.yaml`:

```yaml
vector-studio:
  replicaCount: 3
  
  env:
    - name: OTEL_SERVICE_NAME
      value: "vector-studio"
    - name: OTEL_EXPORTER_OTLP_ENDPOINT
      value: "http://otel-collector.observability:4317"
    - name: OTEL_TRACES_SAMPLER_ARG
      value: "0.1"  # 10% sampling in production
  
  resources:
    requests:
      memory: "512Mi"
      cpu: "500m"
    limits:
      memory: "2Gi"
      cpu: "2000m"

otel-collector:
  enabled: true
  mode: deployment
  config:
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317
    exporters:
      otlp:
        endpoint: tempo.observability:4317
        tls:
          insecure: true
```

### Deploy

```bash
# Install OpenTelemetry Operator
kubectl apply -f https://github.com/open-telemetry/opentelemetry-operator/releases/latest/download/opentelemetry-operator.yaml

# Deploy Vector Studio with telemetry
helm install vector-studio ./helm/vector-studio -f values-telemetry.yaml

# Deploy observability stack
kubectl apply -f k8s/observability/
```

## Verification

### 1. Check Telemetry Initialization

```bash
# Check logs for telemetry messages
docker logs vector-studio | grep -i "telemetry\|opentelemetry"

# Expected output:
# [INFO] Initializing OpenTelemetry with service: vector-studio
# [INFO] Using OTLP trace exporter: http://localhost:4317
# [INFO] OpenTelemetry initialized successfully
```

### 2. Verify Traces in Grafana

1. Open Grafana: http://localhost:3000
2. Navigate to Explore → Select Tempo datasource
3. Search for service name: `vector-studio`
4. View trace timeline and spans

### 3. Verify Metrics in Prometheus

```bash
# Query Prometheus
curl 'http://localhost:9090/api/v1/query?query=vector_search_latency_ms'

# Or use Prometheus UI
open http://localhost:9090
```

### 4. Test Trace Propagation

```bash
# Start Vector Studio
./build/hektor &

# Make a search request
curl -X POST http://localhost:8080/search \
  -H "Content-Type: application/json" \
  -d '{"query": "test query", "k": 10}'

# Check trace in Grafana
# Should see: api.search → vector.search → index.search
```

## Troubleshooting

### Issue: Traces not appearing

**Solution 1**: Check collector connectivity
```bash
# Test OTLP endpoint
curl -v http://localhost:4317

# Check collector logs
docker logs otel-collector
```

**Solution 2**: Verify sampling configuration
```cpp
// Temporarily set to 100%
config.sampling_ratio = 1.0;
```

**Solution 3**: Enable debug logging
```bash
export OTEL_LOG_LEVEL=debug
./build/hektor
```

### Issue: High memory usage

**Solution**: Reduce batch size
```yaml
# otel-collector-config.yaml
processors:
  batch:
    timeout: 500ms
    send_batch_size: 256  # Reduced from 1024
```

### Issue: Build fails with OpenTelemetry

**Solution 1**: Check OpenTelemetry installation
```bash
pkg-config --list-all | grep opentelemetry
```

**Solution 2**: Build without OpenTelemetry
```bash
cmake .. -DVDB_USE_OPENTELEMETRY=OFF
```

**Solution 3**: Specify OpenTelemetry path
```bash
cmake .. \
  -DVDB_USE_OPENTELEMETRY=ON \
  -DCMAKE_PREFIX_PATH=/usr/local \
  -Dopentelemetry-cpp_DIR=/usr/local/lib/cmake/opentelemetry-cpp
```

## Performance Tuning

### Production Settings

```cpp
TelemetryConfig config;
config.sampling_ratio = 0.1;  // 10% sampling
config.use_tail_based_sampling = true;  // Always sample errors
config.batch_size = 512;
config.export_interval = std::chrono::seconds(5);
config.max_queue_size = 2048;
```

### Environment Variables

```bash
# Production sampling
export OTEL_TRACES_SAMPLER_ARG="0.1"

# Batch configuration
export OTEL_BSP_MAX_QUEUE_SIZE=2048
export OTEL_BSP_SCHEDULE_DELAY=5000
export OTEL_BSP_MAX_EXPORT_BATCH_SIZE=512
```

## Next Steps

1. **Review Documentation**:
   - Read `docs/15_OPENTELEMETRY_TRACING.md` for detailed usage
   - Read `docs/15_EBPF_OBSERVABILITY.md` for advanced profiling

2. **Customize Dashboards**:
   - Import pre-built Grafana dashboards
   - Create custom queries for your use cases

3. **Set Up Alerts**:
   - Configure Prometheus alerting rules
   - Set up notification channels

4. **Enable eBPF Profiling**:
   - Install eBPF tools
   - Deploy continuous profiling agents

## Support

For issues or questions:
- Documentation: `docs/15_OPENTELEMETRY_TRACING.md`
- GitHub Issues: https://github.com/amuzetnoM/hektor/issues
- OpenTelemetry Docs: https://opentelemetry.io/docs/

---

**Version**: 3.0.0  
**Last Updated**: 2026-01-08
