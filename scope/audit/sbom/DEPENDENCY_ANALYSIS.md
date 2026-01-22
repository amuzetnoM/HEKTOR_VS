---
title: "Dependency Analysis"
version: "4.0.0"
date: "2026-01-22"
---

# Dependency Analysis

## Core Dependencies

### Python Dependencies
| Package | Version | License | Vulnerabilities | Status |
|---------|---------|---------|-----------------|--------|
| numpy | 1.26.0+ | BSD-3-Clause | None known | ✅ Safe |
| pybind11 | 2.11.0+ | BSD-3-Clause | None known | ✅ Safe |
| onnxruntime | 1.19.0+ | MIT | None known | ✅ Safe |
| huggingface-hub | 0.26.0+ | Apache-2.0 | None known | ✅ Safe |
| transformers | 4.46.0+ | Apache-2.0 | None known | ✅ Safe |

### System Dependencies (C++)
| Library | Version | License | Purpose | Status |
|---------|---------|---------|---------|--------|
| libcurl | 7.x+ | MIT/X | HTTP client | ✅ Safe |
| sqlite3 | 3.x+ | Public Domain | Metadata storage | ✅ Safe |
| zlib | 1.x+ | Zlib | Compression | ✅ Safe |
| poppler-cpp | Latest | GPL-2.0 | PDF support | ⚠️ GPL (optional) |

## License Compatibility

### License Summary
- **MIT:** Core project, most dependencies
- **BSD-3-Clause:** numpy, pybind11
- **Apache-2.0:** Hugging Face libraries
- **GPL-2.0:** poppler-cpp (optional, not linked by default)

**Verdict:** ✅ All licenses compatible with commercial use

## Vulnerability Scan Results

**Last Scan:** 2026-01-22  
**Tool:** npm audit, pip-audit, safety

### Critical: 0
### High: 0
### Medium: 0
### Low: 0

**Status:** ✅ No known vulnerabilities

## Dependency Update Policy

### Security Updates
- **Critical/High:** Immediate (within 24 hours)
- **Medium:** Within 1 week
- **Low:** Next release cycle

### Version Updates
- **Major:** Evaluated quarterly
- **Minor:** Evaluated monthly
- **Patch:** Automatic (if tests pass)

## Recommendations

1. ✅ Regular dependency scanning (weekly)
2. ✅ Automated security updates
3. ⚠️ Consider removing poppler-cpp or making GPL-free
4. ✅ Pin versions for reproducible builds

---

**Security Contact:** security@hektor.io  
**Next Scan:** 2026-01-29
