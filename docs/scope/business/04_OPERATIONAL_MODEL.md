---
title: "Operational Model"
version: "4.0.0"
date: "2026-01-22"
status: "Production"
tags: ["operations", "sla", "support"]
---

# Operational Model

## Day-to-Day Operations

### Monitoring
- Prometheus metrics every 15s
- Grafana dashboards
- PagerDuty/Opsgenie alerts
- Key metrics: Latency (p99), QPS, memory, errors

### Health Checks
- /health endpoint every 30s
- Node availability checks
- Index corruption detection
- Automated recovery procedures

## Backup & Recovery

### Backup Schedule
- Full backup: Weekly (Sunday 2 AM)
- Incremental: Daily (2 AM)
- Snapshots: Hourly (if supported)
- Retention: 30 days

### Recovery Procedures
1. Identify failure scope
2. Switch to standby replica (if available)
3. Restore from latest backup
4. Validate data integrity
5. Resume normal operations

### RTO/RPO Targets
- RTO (Recovery Time): <1 hour
- RPO (Recovery Point): <15 minutes

## SLA Definitions

### Availability Tiers

**Standard (99.9%):**
- Max downtime: 43.8 minutes/month
- Support: Business hours
- Response: 24 hours

**Premium (99.95%):**
- Max downtime: 21.9 minutes/month
- Support: 24/7
- Response: 4 hours

**Enterprise (99.99%):**
- Max downtime: 4.38 minutes/month
- Support: 24/7 dedicated
- Response: 1 hour

### Performance SLAs

**Latency:**
- p50: <2ms
- p95: <5ms
- p99: <10ms

**Throughput:**
- Min QPS: 200 (single node)
- Scaling: Linear up to 10 nodes

**Uptime:**
- Measured monthly
- Excludes scheduled maintenance
- Credits for violations

## Maintenance Windows

### Scheduled Maintenance
- Frequency: Monthly
- Window: Sunday 2-6 AM UTC
- Notification: 7 days advance
- Duration: <4 hours

### Emergency Maintenance
- As needed for critical issues
- Notification: 4 hours if possible
- Immediate for security vulnerabilities

## Support Processes

### Ticket Priority

**P0 - Critical:**
- Production down
- Data loss/corruption
- Response: 15 minutes (Enterprise), 1 hour (Premium)

**P1 - High:**
- Severe performance degradation
- Partial outage
- Response: 4 hours (Enterprise/Premium), 24 hours (Standard)

**P2 - Medium:**
- Minor performance issues
- Feature requests
- Response: 1 business day

**P3 - Low:**
- Questions, documentation
- Response: 3 business days

### Escalation Path
1. L1: Support engineer (initial triage)
2. L2: Senior engineer (investigation)
3. L3: Development team (code fixes)
4. Emergency: On-call architect

## Operational Metrics

### Key Performance Indicators (KPIs)

**System Health:**
- Uptime: >99.9%
- Error rate: <0.1%
- p99 latency: <10ms

**Support:**
- Ticket response time (by priority)
- Resolution time
- Customer satisfaction score

**Capacity:**
- CPU utilization: <70% average
- Memory utilization: <80% average
- Disk utilization: <75%

### Capacity Planning

**Quarterly Review:**
- Growth projections
- Resource forecasting
- Budget planning

**Triggers for Scaling:**
- CPU >70% for 7 days
- Memory >80% for 3 days
- p99 latency >15ms for 24 hours

## Runbooks

### Common Procedures

**1. Node Addition**
```bash
# Add new node to cluster
./scripts/add_node.sh <node-ip>
# Rebalance data
./scripts/rebalance.sh
```

**2. Index Rebuild**
```bash
# Rebuild HNSW index
hektor-cli rebuild-index --collection <name>
```

**3. Backup Restoration**
```bash
# Restore from S3 backup
./scripts/restore.sh s3://backups/2026-01-22
```

**4. Performance Debugging**
```bash
# Profile query performance
hektor-cli profile --duration 60s
# Check SIMD usage
hektor-cli info | grep SIMD
```

---

**Document Classification:** Internal  
**Owner:** SRE / Operations Team
