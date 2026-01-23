---
title: "Archived Scope Data - Source Files"
version: "4.0.0"
date: "2026-01-22"
status: "Archived"
---

# Archived Scope Data

> **Note:** This directory contains archived source data files. All documentation has been moved to `/scope/`.

## Migration Notice

As of version 4.0.0 (2026-01-22), all scope documentation has been reorganized and moved to the `/scope/` directory.

### Source Files (Archived)

This directory now contains only the source JSON files:

- **COMPETITOR_ANALYSIS.json** - Raw competitive analysis data (source of truth)
- **HEKTOR_ANALYSIS.json** - Raw Hektor technical analysis data (source of truth)

### New Documentation Location

**All documentation is now available at:** `/scope/`

```
/scope/
├── README.md                          # Master index
├── COMPETITOR_ANALYSIS.json           # Copied from docs/SCOPE
├── HEKTOR_ANALYSIS.json              # Copied from docs/SCOPE
│
├── market-research/                   # Market analysis (7 documents)
├── business/                          # Business documentation (6 documents)
├── stakeholders/                      # Stakeholder papers (5 documents)
├── audit/                             # Audit reports (10 documents)
└── suggestions/                       # Improvements (4 documents)
```

### What Changed

**Removed:**
- Old markdown files (COMPETITOR_ANALYSIS.md, HEKTOR_ANALYSIS.md)
- Outdated documentation

**Added:**
- Comprehensive market research documentation
- Professional business case and infrastructure docs
- Stakeholder-specific papers
- Complete audit reports (quality, security, SBOM, compliance)
- Improvement suggestions and roadmap

### Accessing New Documentation

```bash
# View master index
cat /scope/README.md

# View market research
ls /scope/market-research/

# View business documentation
ls /scope/business/

# View audit reports
ls /scope/audit/
```

---

**For the latest documentation, please refer to `/scope/`**
