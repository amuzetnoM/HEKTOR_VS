---
title: "Infrastructure Requirements"
version: "4.0.0"
date: "2026-01-22"
status: "Production"
classification: "Internal"
tags: ["infrastructure", "requirements", "sizing"]
audience: "IT Operations, Infrastructure Teams, Architects"
---

# Infrastructure Requirements

## Overview

This document specifies the infrastructure requirements for deploying Hektor Vector Database across various scales and deployment models.

## Compute Requirements

### CPU Specifications

**Minimum Requirements:**
- **Architecture:** x86-64 with SSE4.2 or ARM64 with Neon
- **Cores:** 4 physical cores
- **Clock Speed:** 2.5 GHz+
- **Cache:** 8 MB L3 cache minimum

**Recommended for Production:**
- **Architecture:** x86-64 with AVX2 or AVX-512
- **Cores:** 16-32 physical cores
- **Clock Speed:** 3.0 GHz+ (boost)
- **Cache:** 32 MB+ L3 cache

**Optimal Performance:**
- **Intel:** Xeon Platinum/Gold with AVX-512 (Ice Lake, Sapphire Rapids)
- **AMD:** EPYC Milan/Genoa with AVX2
- **ARM:** AWS Graviton3, Ampere Altra with Neon

**SIMD Instruction Sets:**
- **AVX-512:** 3.1x faster distance calculations (best)
- **AVX2:** 2.4x faster (good)
- **SSE4.2:** 1.8x faster (minimum)
- **ARM Neon:** 2.2x faster (ARM systems)

### Memory Specifications

**Sizing Guidelines (per million vectors, 512-dim):**

| Configuration | Memory per 1M Vectors | Total System RAM |
|---------------|----------------------|------------------|
| **Full Precision** | 2.4 GB | 4 GB+ |
| **Standard Quantization** | 0.8 GB | 2 GB+ |
| **Perceptual Quantization** | 0.5 GB | 1 GB+ |
| **+ HNSW Index** | +40% overhead | |
| **+ OS/Buffers** | +20% overhead | |

**Production Recommendations:**

**Small (1M vectors):**
- **RAM:** 8-16 GB
- **Type:** DDR4-3200 or faster
- **ECC:** Recommended

**Medium (10M vectors):**
- **RAM:** 32-64 GB
- **Type:** DDR4-3200 or faster
- **ECC:** Required

**Large (100M vectors):**
- **RAM:** 128-256 GB
- **Type:** DDR4-3200 or faster
- **ECC:** Required

**Massive (1B vectors):**
- **RAM:** 128 GB per node (10 nodes)
- **Type:** DDR4-3200 or faster
- **ECC:** Required

## Storage Requirements

### Storage Types

**Primary Index Storage (Required):**
- **Type:** NVMe SSD (preferred) or SATA SSD (acceptable)
- **IOPS:** 50,000+ read IOPS (NVMe)
- **Throughput:** 3,000+ MB/s sequential read
- **Latency:** <100μs (NVMe)

**Backup Storage (Optional):**
- **Type:** HDD or S3-compatible object storage
- **Throughput:** 200+ MB/s (for efficient backups)

### Capacity Planning

**Per Million Vectors (512-dim):**

| Component | Full Precision | With PQ | Notes |
|-----------|----------------|---------|-------|
| **Vector Data** | 2.0 GB | 0.4 GB | Main storage |
| **HNSW Index** | 0.8 GB | 0.3 GB | Graph structure |
| **Metadata** | 0.2 GB | 0.2 GB | IDs, tags, etc. |
| **Working Space** | 0.5 GB | 0.5 GB | Temp, buffers |
| **Total** | **3.5 GB** | **1.4 GB** | Per 1M vectors |

**Deployment Sizing:**

| Scale | Vectors | Storage (PQ) | Storage (Full) | RAID Recommended |
|-------|---------|--------------|----------------|------------------|
| Small | 1M | 5 GB | 10 GB | Optional |
| Medium | 10M | 20 GB | 50 GB | RAID 1/10 |
| Large | 100M | 200 GB | 500 GB | RAID 10 |
| Massive | 1B | 2 TB | 5 TB | RAID 10 + Distributed |

### File System Recommendations

**Linux:**
- **ext4:** Good general-purpose, mature
- **XFS:** Better for large files and high concurrency
- **Btrfs:** Snapshot support, compression (optional)

**Mount Options (ext4/XFS):**
```
noatime,nodiratime,discard (for SSD)
```

## Network Requirements

### Bandwidth

**Single-Node Deployment:**
- **Minimum:** 1 Gbps
- **Recommended:** 10 Gbps
- **Use Case:** Client-server communication, backups

**Distributed Deployment:**
- **Minimum:** 10 Gbps
- **Recommended:** 25-100 Gbps
- **Use Case:** Node-to-node replication, data distribution

### Latency

**Client-Server:**
- **Same datacenter:** <1ms preferred
- **Cross-region:** <10ms acceptable
- **Internet:** <50ms (may impact performance)

**Node-to-Node (Distributed):**
- **Same datacenter:** <0.5ms required
- **Same availability zone:** <0.1ms optimal

### Firewall Requirements

**Inbound:**
- **Port 50051:** gRPC API (default)
- **Port 8080:** REST API (optional)
- **Port 9090:** Metrics/Prometheus (optional)
- **Port 22:** SSH (management)

**Outbound:**
- **Port 443:** HTTPS (Hugging Face model downloads)
- **Port 50051:** gRPC (for distributed setups)

## Infrastructure by Scale

### Small Deployment (1M Vectors)

**Single Node:**
- **Instance Type (AWS):** c7i.2xlarge or equivalent
- **vCPU:** 8 cores
- **RAM:** 16 GB
- **Storage:** 50 GB NVMe SSD
- **Network:** 10 Gbps
- **Monthly Cost:** $200-250

**Use Cases:**
- Development/staging
- Small applications (<100 QPS)
- Proof of concept

### Medium Deployment (10M Vectors)

**2-Node Cluster:**
- **Instance Type:** c7i.4xlarge or equivalent
- **vCPU:** 16 cores per node
- **RAM:** 32 GB per node
- **Storage:** 200 GB NVMe SSD per node
- **Network:** 25 Gbps
- **Monthly Cost:** $800-1,000

**Use Cases:**
- Production applications (100-500 QPS)
- High availability required
- Multi-tenant SaaS

### Large Deployment (100M Vectors)

**5-Node Cluster:**
- **Instance Type:** c7i.8xlarge or equivalent
- **vCPU:** 32 cores per node
- **Storage:** 500 GB NVMe SSD per node
- **RAM:** 64 GB per node
- **Network:** 25 Gbps
- **Monthly Cost:** $2,500-3,500

**Use Cases:**
- Large-scale production (500-2000 QPS)
- Enterprise deployments
- Multi-region

### Massive Deployment (1B Vectors)

**10-Node Cluster:**
- **Instance Type:** c7i.16xlarge or equivalent
- **vCPU:** 64 cores per node
- **RAM:** 128 GB per node
- **Storage:** 1 TB NVMe SSD per node
- **Network:** 100 Gbps
- **Monthly Cost:** $12,000-15,000

**Use Cases:**
- Hyperscale applications (>5000 QPS)
- Global deployments
- Multi-billion vector support

## Cloud Provider Specifications

### AWS

**Recommended Instance Families:**
- **c7i:** Latest generation, AVX-512 support
- **c6i:** Previous generation, good price/performance
- **c7g:** Graviton3 (ARM), cost-effective

**Storage:**
- **gp3:** General purpose SSD (good)
- **io2:** Provisioned IOPS SSD (better)
- **Instance Store NVMe:** Highest performance (ephemeral)

**Example Configuration (10M vectors):**
```
Instance: c7i.4xlarge (16 vCPU, 32 GB RAM)
Storage: 200 GB gp3 (3000 IOPS, 125 MB/s)
Network: Enhanced networking enabled
Monthly: ~$500/instance
```

### Azure

**Recommended VM Families:**
- **Fsv2-series:** Compute optimized, AVX-512
- **Ddsv5-series:** General purpose with local SSD
- **Dasv5-series:** AMD EPYC, cost-effective

**Storage:**
- **Premium SSD:** Production workloads
- **Premium SSD v2:** Highest performance
- **Local NVMe:** Ephemeral, best performance

**Example Configuration (10M vectors):**
```
Instance: Standard_F16s_v2 (16 vCPU, 32 GB RAM)
Storage: 200 GB Premium SSD
Network: Accelerated networking
Monthly: ~$600/instance
```

### Google Cloud Platform (GCP)

**Recommended Machine Families:**
- **c3-series:** Compute optimized, latest gen
- **c2-series:** High-frequency, AVX-512
- **n2-series:** General purpose, balanced

**Storage:**
- **pd-ssd:** Persistent SSD (good)
- **pd-extreme:** Highest IOPS (better)
- **Local SSD:** Best performance (ephemeral)

**Example Configuration (10M vectors):**
```
Instance: c3-standard-16 (16 vCPU, 64 GB RAM)
Storage: 200 GB pd-ssd
Network: tier_1 (premium)
Monthly: ~$550/instance
```

## On-Premises Specifications

### Server Specifications

**Small Server (1-10M vectors):**
- **CPU:** Intel Xeon Silver/Gold or AMD EPYC
- **Cores:** 16-32 physical cores
- **RAM:** 64-128 GB ECC DDR4-3200
- **Storage:** 2x 1TB NVMe SSD (RAID 1)
- **Network:** 10 Gbps NIC
- **Cost:** $5,000-10,000

**Large Server (10-100M vectors):**
- **CPU:** Intel Xeon Platinum or AMD EPYC
- **Cores:** 32-64 physical cores
- **RAM:** 256-512 GB ECC DDR4-3200
- **Storage:** 4x 2TB NVMe SSD (RAID 10)
- **Network:** 25 Gbps NIC
- **Cost:** $15,000-30,000

### Rack Infrastructure

**Per Rack (10 servers):**
- **Power:** 2x 20A circuits minimum
- **Cooling:** 15-20 kW heat load
- **Networking:** 2x 100 Gbps switches (redundant)
- **Storage:** Optional NAS/SAN for backups

## Operating System Requirements

### Supported Operating Systems

**Linux (Recommended):**
- **Ubuntu 22.04 LTS** (primary development platform)
- Ubuntu 20.04 LTS (supported)
- CentOS Stream 9 / Rocky Linux 9
- Debian 11/12
- Amazon Linux 2023

**macOS:**
- macOS 12 (Monterey) or later
- Apple Silicon (M1/M2) supported

**Windows:**
- Windows Server 2019/2022
- Windows 10/11 (development only)

### System Dependencies

**Required Packages (Ubuntu/Debian):**
```bash
sudo apt-get install -y \
  build-essential cmake ninja-build \
  libcurl4-openssl-dev libsqlite3-dev \
  zlib1g-dev libpoppler-cpp-dev \
  libicu-dev libpq-dev
```

**Required Packages (RHEL/CentOS):**
```bash
sudo dnf install -y \
  gcc-c++ cmake ninja-build \
  libcurl-devel sqlite-devel \
  zlib-devel poppler-cpp-devel \
  libicu-devel postgresql-devel
```

## Container Requirements

### Docker

**Minimum Docker Version:** 20.10+  
**Recommended:** 24.0+

**Resource Allocation:**
```bash
docker run -d \
  --name hektor \
  --cpus="16" \
  --memory="32g" \
  --shm-size="8g" \
  -v /data:/var/lib/hektor \
  hektor/hektor:4.0.0
```

### Kubernetes

**Minimum Kubernetes Version:** 1.24+  
**Recommended:** 1.28+

**Node Requirements:**
- **Labels:** `hektor.io/instance-type=compute`
- **Taints:** Optional for dedicated nodes
- **Storage Class:** Block storage with high IOPS

**Resource Requests/Limits:**
```yaml
resources:
  requests:
    cpu: "8"
    memory: "16Gi"
  limits:
    cpu: "16"
    memory: "32Gi"
```

## Monitoring Requirements

### Metrics Collection

**Prometheus:**
- **Scrape Interval:** 15s
- **Retention:** 30+ days
- **Storage:** ~10 GB per node

**Grafana:**
- **Dashboards:** Pre-built dashboards available
- **Alerts:** Recommended alert rules included

### Log Storage

**Centralized Logging:**
- **Solution:** ELK Stack, Loki, or cloud-native
- **Retention:** 7-30 days (compliance dependent)
- **Volume:** ~100 MB per node per day

## Backup Requirements

### Backup Storage

**Capacity:**
- **Full Backup:** 100% of index size
- **Incremental:** 10-20% daily
- **Retention:** 7-30 days

**Performance:**
- **Throughput:** 200+ MB/s (for efficient backups)
- **Type:** Object storage (S3, Azure Blob, GCS)

### Backup Schedule

**Recommended:**
- **Full Backup:** Weekly
- **Incremental:** Daily
- **Snapshot:** Hourly (if supported by storage)

---

**Related Documents:**
- [Deployment Architecture](./03_DEPLOYMENT_ARCHITECTURE.md)
- [Operational Model](./04_OPERATIONAL_MODEL.md)

**Document Classification:** Internal  
**Review Cycle:** Quarterly  
**Owner:** Infrastructure & Platform Engineering
