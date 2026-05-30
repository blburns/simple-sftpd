# Production Version Performance Tuning

**Version:** 0.3.0  
**License:** Apache 2.0

---

## Overview

This guide covers performance tuning for the Production Version of Simple Secure FTP Daemon (simple-sftpd).

## Performance Targets

- **Throughput:** Optimized for file transfer workloads
- **Concurrent Connections:** Configurable connection limits
- **Latency:** Responsive command handling
- **Memory:** Efficient connection and buffer management

## Tuning Recommendations

### Connection Limits

Adjust max connections and timeouts for your workload:

```yaml
connection:
  max_connections: 1000
  max_connections_per_ip: 50
  connection_timeout: 600
  data_timeout: 600
  idle_timeout: 1800
  backlog: 500
```

For INI format:

```ini
[connection]
max_connections = 1000
max_connections_per_ip = 50
connection_timeout = 600
backlog = 500
```

### Transfer Settings

- **sendfile / mmap:** On Linux and macOS, RETR can use `use_sendfile` and memory-mapped I/O when enabled in config (see `config/advanced/` examples).
- **Connection pooling:** `FTPConnectionManager` reuses connection resources where configured.
- **File cache:** Metadata cache reduces repeated stat operations on hot paths.
- **Buffer size:** Larger buffers can improve throughput for large files (32KB–64KB is typical).
- **Rate limiting:** Use `max_transfer_rate` and rate_limit settings to avoid one client starving others.
- **Compression:** The `Compression` class is implemented but not yet integrated into on-the-wire RETR/STOR.

```yaml
transfer:
  buffer_size: 32768
  max_transfer_rate: 52428800

rate_limit:
  enabled: true
  max_requests_per_minute: 1000
  max_transfers_per_minute: 50
  max_bytes_per_minute: 524288000
```

### Passive Mode Port Range

Use a dedicated port range for passive data connections and ensure firewall allows it:

```yaml
passive:
  enabled: true
  min_port: 49152
  max_port: 65535
```

### Logging Overhead

Reduce logging in production to lower I/O and CPU:

```yaml
logging:
  log_level: "WARN"
  log_format: "JSON"
  log_to_console: false
  log_to_file: true
```

### SSL/TLS

- Use TLS 1.2+ only in production.
- Prefer cipher suites that support hardware acceleration (e.g. AES-GCM).
- Ensure certificate and key files are on fast local storage.

## Monitoring

- Use the built-in performance monitor (if enabled) for connection counts and transfer statistics.
- Correlate with system metrics (CPU, network, disk I/O) to identify bottlenecks.
- Consider external monitoring (e.g. Prometheus, health checks) for availability and latency.

---

**Last Updated:** May 2026  
**Version:** 0.3.0
