# Simple Secure FTP Daemon - Technical Debt

**Date:** August 2026  
**Current Version:** Production v0.4.0  
**Purpose:** Track technical debt, known issues, and areas requiring improvement  
**Product Version:** Production Version (Apache 2.0)

**Product Versions:**
- **🏭 Production Version (Apache 2.0):** ✅ Complete through v0.4.0
- **🏢 Enterprise Version (BSL 1.1):** ⏳ Planned - No technical debt yet
- **🏛️ Datacenter Version (BSL 1.1):** ⏳ Planned - No technical debt yet

---

## 🎯 Overview

This document tracks technical debt, known issues, code quality improvements, and areas that need refactoring or enhancement in the simple-sftpd project. Items are prioritized by impact and urgency.

**Open Debt Items:** Test coverage toward 1.0 bars, error handling, memory review, logging, performance tuning  
**Resolved since v0.1.0:** Virtual hosting, user persistence, connection pooling, MODE Z (v0.2.0–v0.4.0)

---

## 🔴 High Priority (Critical)

### 1. Test Coverage Expansion
**Status:** ⚠️ **In Progress**  
**Priority:** 🔴 **HIGH**  
**Estimated Effort:** 25-35 hours

**Current State:**
- Unit/integration suite green (~59 tests); approximate coverage still below a 90% 1.0 bar
- Protocol tests added in v0.4.0: PASV RETR/STOR, PORT, HOST, MODE Z, AUTH TLS smoke
- PAM test gated (skipped on macOS)
- No performance/load suite yet

**Issues:**
- Coverage tooling/percent still weak
- PAM still skip-gated off Linux
- No performance benchmarks
- No load/stress testing

**Impact:**
- Risk of regressions in production
- Difficult to validate security features
- Unknown behavior under load

**Action Items:**
- [x] Add SSL/TLS smoke (AUTH TLS)
- [x] Add PAM test (skip when unavailable)
- [x] Add active mode / PORT transfer test
- [x] Add MODE Z transfer test
- [ ] Expand measured coverage toward 60%+ if tooling allows
- [ ] Create performance test suite
- [ ] Implement load testing framework

**Target:** optional 1.0 quality bar (not blocking v0.4.0)

---

### 2. Virtual Hosting Implementation
**Status:** ✅ **Resolved (v0.3.0)**  
**Priority:** ~~🔴 HIGH~~

**Resolution:**
- ✅ Virtual host routing via `handleHOST()` and `FTPVirtualHostManager`
- ✅ Per-host configuration, root directory, SSL certificates, and quotas
- ✅ Runtime host management (add/remove/get/list) and custom error pages
- ⚠️ Dedicated virtual hosting tests still recommended (see test coverage item)

---

### 3. User Persistence
**Status:** ✅ **Resolved (v0.3.0)**  
**Priority:** ~~🔴 HIGH~~

**Resolution:**
- ✅ JSON file-based storage via `security.user_file`
- ✅ Auto-load on startup, auto-save on add/remove; users survive restart
- ✅ Groups, quotas, and guest-account fields persisted in users JSON

---

## 🟡 Medium Priority (Important)

### 4. Code Refactoring
**Status:** ✅ **In Progress**  
**Priority:** 🟡 **MEDIUM**  
**Estimated Effort:** 15-20 hours

**Current State:**
- Code reorganization completed
- Some code duplication in command handlers
- Some functions could be simplified

**Issues:**
- Code duplication in command handlers
- Some functions are too complex
- Could benefit from additional abstraction

**Impact:**
- Maintenance burden
- Potential for bugs
- Slower development

**Action Items:**
- [ ] Refactor command handlers
- [ ] Remove code duplication
- [ ] Simplify complex functions
- [ ] Add additional abstractions

**Target:** v0.2.0 release

---

### 5. Error Handling Improvements
**Status:** ⚠️ **Needs Enhancement**  
**Priority:** 🟡 **MEDIUM**  
**Estimated Effort:** 8-12 hours

**Current State:**
- Basic error handling implemented
- Some error cases not handled
- Inconsistent error reporting

**Issues:**
- Some error cases may cause crashes
- Error messages not always clear
- Missing error recovery mechanisms

**Impact:**
- Potential server crashes
- Poor user experience
- Difficult troubleshooting

**Action Items:**
- [ ] Review all error handling paths
- [ ] Add missing error handling
- [ ] Improve error messages
- [ ] Add error recovery mechanisms

**Target:** v0.2.0 release

---

### 6. Connection Pooling
**Status:** ✅ **Resolved (v0.2.0)**  
**Priority:** ~~🟡 MEDIUM~~

**Resolution:**
- ✅ `FTPConnectionManager` provides `acquireConnection()`, `releaseConnection()`, `setPoolSize()`
- ✅ Connection pool with maintenance loop
- ⚠️ Load/scalability benchmarking still pending (see performance item)

---

### 6b. Compression Integration
**Status:** ✅ **Resolved (v0.4.0)**  
**Priority:** ~~🟡 MEDIUM~~

**Resolution:**
- ✅ `ZlibStream` (RFC 1950) on RETR/STOR when `MODE Z` and `transfer.enable_compression`
- ✅ FEAT advertises `MODE Z`; REST/APPE rejected while MODE Z is active
- ✅ Protocol integration test for compressed transfer
- Note: whole-buffer gzip/bzip2 helpers remain for other uses; on-the-wire path is zlib MODE Z

---

## 🟢 Low Priority (Nice to Have)

### 7. Memory Management Review
**Status:** ⚠️ **Needs Review**  
**Priority:** 🟢 **LOW**  
**Estimated Effort:** 6-10 hours

**Current State:**
- No systematic memory leak detection
- No memory profiling
- Potential memory leaks in long-running operations

**Issues:**
- Memory leaks could cause server degradation
- No memory usage monitoring
- Potential issues with connection management

**Impact:**
- Server performance degradation over time
- Potential crashes under load
- Resource exhaustion

**Action Items:**
- [ ] Run memory leak detection tools
- [ ] Profile memory usage
- [ ] Fix identified memory leaks
- [ ] Add memory usage monitoring

**Target:** v0.3.0 release

---

### 8. Logging Improvements
**Status:** ⚠️ **Needs Enhancement**  
**Priority:** 🟢 **LOW**  
**Estimated Effort:** 6-10 hours

**Current State:**
- Basic logging implemented
- Some operations not logged
- Log levels could be improved

**Issues:**
- Missing logs for some operations
- Inconsistent log levels
- Could benefit from structured logging

**Impact:**
- Difficult troubleshooting
- Missing audit trail
- Poor observability

**Action Items:**
- [ ] Add missing log statements
- [ ] Standardize log levels
- [ ] Add structured logging
- [ ] Improve log formatting

**Target:** v0.3.0 release

---

### 9. Performance Optimization
**Status:** ❌ **Not Started**  
**Priority:** 🟢 **LOW**  
**Estimated Effort:** 20-30 hours

**Current State:**
- Basic performance optimizations
- No profiling done
- Unknown performance bottlenecks

**Issues:**
- Performance not optimized
- Unknown bottlenecks
- Could benefit from optimization

**Impact:**
- Suboptimal performance
- Higher resource usage
- Slower response times

**Action Items:**
- [ ] Profile performance
- [ ] Identify bottlenecks
- [ ] Optimize critical paths
- [ ] Add performance monitoring

**Target:** v0.3.0 release

---

## 📋 Summary

### Open Items
- **High Priority:** Measured coverage / load bars (optional 1.0)
- **Medium Priority:** Error handling, code refactoring
- **Low Priority:** Memory review, logging improvements, performance tuning

### Resolved Items (v0.2.0 / v0.3.0 / v0.4.0)
- ✅ Virtual hosting implementation
- ✅ User persistence
- ✅ Connection pooling
- ✅ MODE Z / on-the-wire compression

---

## 🎯 Next Steps

1. **Immediate (post-Production):**
   - Optional coverage/load work
   - Linux/Docker/Windows env verification on those hosts
   - Refactor command handlers / remove duplication

2. **Short Term:**
   - Error handling improvements
   - Performance/load testing and benchmarks

3. **Long Term:**
   - Memory management review
   - Logging improvements
   - Performance optimization

---

*Last Updated: August 2026*  
*Next Review: Before Enterprise v0.1.0 kickoff*  
*Focus: Production Version (Apache 2.0)*

