# TCP Module

## Overview

The TCP module provides a cross-platform, thread-pooled TCP server abstraction for Windows and POSIX systems. It handles connection lifecycle management, worker thread dispatch, backpressure control, rate limiting, and optional TLS encryption. The API uses lambda callbacks for connection handling, enabling flexible server architectures without manual thread management.

## Key Features

- **Cross-Platform**: Works on Windows (Winsock2) and POSIX (BSD sockets)
- **Thread Pool**: Configurable worker pool dispatches connection handlers
- **Lambda Callbacks**: Uses `ak_lambda_t` for connect, handle, and disconnect hooks
- **Backpressure Control**: Per-connection receive buffer limits prevent memory exhaustion
- **Rate Limiting**: Per-IP connection limits and connection rate throttling
- **Graceful Shutdown**: Configurable drain timeout for in-flight handlers
- **TCP Keepalive**: Configurable keepalive probes for dead connection detection
- **Socket Tuning**: Buffer sizes, linger options, and timeout configuration
- **TLS Support**: Optional OpenSSL-based encryption (compile-time flag)
- **Statistics**: Connection counts, bytes transferred, rejection reasons
- **Structured Errors**: Typed error codes with human-readable strings

## Concepts

### Server Lifecycle

A TCP server follows a simple lifecycle:

1. **Configure**: Create a configuration with bind address, port, and callbacks
2. **Create**: Allocate server resources with `ak_tcp_server_new()`
3. **Start**: Begin accepting connections with `ak_tcp_server_start()`
4. **Run**: Server runs asynchronously, dispatching handlers via thread pool
5. **Stop**: Call `ak_tcp_server_stop()` to initiate graceful shutdown
6. **Free**: Release resources with `ak_tcp_server_free()`

### Connection Callbacks

Three lambda callbacks control connection behavior:

Note: In implementation `on_handle` and `on_disconnect` will happen in the same thread, but `on_connect` will happen in the main thread.
This library does NOT make any guarantee tha either of these lambdas will execute in the same thread however, so assume each is a boundary.

- **on_connect**: Called when a connection arrives. Receives `ak_tcp_conn_info_t*` with remote IP/port. Set `accept = false` to reject.
- **on_handle**: Called in a worker thread to process the connection. Receives `ak_tcp_ctx_t*` for I/O operations.
- **on_disconnect**: Called after handler returns for cleanup. Receives the same `ak_tcp_ctx_t*`.

### Connection Context

The `ak_tcp_ctx_t` represents an active connection. It provides:

- Send/receive operations with timeout support
- Delimiter-based line reading
- Connection metadata storage (key-value pairs)
- Keepalive and timeout configuration
- Graceful shutdown control

### Backpressure

When `max_recv_buffer_bytes` is set, the server tracks bytes received but not consumed. If the limit is reached, `ak_tcp_recv()` returns `AK_TCP_ERR_BUFFER_FULL`. Call `ak_tcp_consume_bytes()` after processing data to release backpressure.

### Rate Limiting

Two rate limiting mechanisms protect against abuse:

- **max_connections_per_ip**: Limits concurrent connections from a single IP address
- **connection_rate_limit**: Limits new connections per second from a single IP

Connections exceeding limits are rejected before handler dispatch.

## API Reference

### Error Handling

Error codes distinguish failure modes:

| Code | Meaning |
|------|---------|
| `AK_TCP_OK` | Success |
| `AK_TCP_ERR_TIMEOUT` | Operation timed out |
| `AK_TCP_ERR_CLOSED` | Connection closed gracefully |
| `AK_TCP_ERR_RESET` | Connection reset by peer |
| `AK_TCP_ERR_NETWORK` | Network error |
| `AK_TCP_ERR_MEMORY` | Memory allocation failed |
| `AK_TCP_ERR_INVALID` | Invalid argument |
| `AK_TCP_ERR_LIMIT` | Limit reached (max_bytes in recv_until) |
| `AK_TCP_ERR_WOULDBLOCK` | Operation would block |
| `AK_TCP_ERR_BUFFER_FULL` | Receive buffer full (backpressure) |
| `AK_TCP_ERR_QUEUE_FULL` | Task queue full (server overloaded) |
| `AK_TCP_ERR_RATE_LIMIT` | Rate limit exceeded |

Use `ak_tcp_error_string()` to convert codes to human-readable strings.

### Server Configuration

`ak_tcp_server_config_t` controls server behavior. Use `ak_tcp_server_config_default()` for sensible defaults, then customize:

**Required Fields:**
- `port`: Port number to listen on
- `on_handle`: Lambda for connection handling

**Connection Limits:**
- `max_connections`: Maximum concurrent connections (0 = unlimited)
- `max_pending_tasks`: Maximum queued connection tasks (0 = unlimited)
- `max_connections_per_ip`: Per-IP connection limit (0 = unlimited)
- `connection_rate_limit`: New connections/second per IP (0 = unlimited)

**Buffer Configuration:**
- `max_recv_buffer_bytes`: Per-connection receive buffer limit (0 = unlimited)
- `recv_buffer_size`: Application receive buffer size (default: 4096)
- `send_buffer_size`: Application send buffer size (default: 4096)
- `socket_recv_buffer`: SO_RCVBUF size (0 = system default)
- `socket_send_buffer`: SO_SNDBUF size (0 = system default)

**Timeout Configuration:**
- `default_recv_timeout_ms`: Default receive timeout (0 = no timeout)
- `default_send_timeout_ms`: Default send timeout (0 = no timeout)
- `shutdown_drain_timeout_ms`: Time to wait for handlers during shutdown

**Keepalive Configuration:**
- `enable_keepalive`: Enable TCP keepalive probes
- `keepalive_idle_sec`: Seconds before first probe (default: 60)
- `keepalive_interval_sec`: Seconds between probes (default: 10)
- `keepalive_count`: Probes before declaring dead (default: 5)

**Linger Configuration:**
- `enable_linger`: Enable SO_LINGER on connections
- `linger_timeout_sec`: Seconds to wait for pending data (0 = hard close)

### Server Lifecycle

| Function | Purpose |
|----------|---------|
| `ak_tcp_server_new()` | Create server from configuration |
| `ak_tcp_server_start()` | Begin accepting connections |
| `ak_tcp_server_stop()` | Initiate graceful shutdown |
| `ak_tcp_server_free()` | Release server resources |
| `ak_tcp_server_is_draining()` | Check if shutdown is in progress |

### Connection I/O

| Function | Purpose |
|----------|---------|
| `ak_tcp_send()` | Send buffer data |
| `ak_tcp_recv()` | Receive up to max_bytes |
| `ak_tcp_recv_until()` | Receive until delimiter found |
| `ak_tcp_send_ex()` | Send with error code return |
| `ak_tcp_recv_ex()` | Receive with error code return |
| `ak_tcp_recv_until_ex()` | Receive until delimiter with error code |

The `_ex` variants return `ak_tcp_error_t` instead of -1, providing finer-grained error information.

### Connection Control

| Function | Purpose |
|----------|---------|
| `ak_tcp_is_alive()` | Check if connection is open |
| `ak_tcp_is_tls()` | Check if using TLS encryption |
| `ak_tcp_close()` | Close connection gracefully |
| `ak_tcp_kill()` | Force close (thread-safe) |
| `ak_tcp_shutdown()` | Graceful shutdown with drain |
| `ak_tcp_abort()` | Immediate close (alias for close) |
| `ak_tcp_consume_bytes()` | Release backpressure after processing |

### Connection Configuration

| Function | Purpose |
|----------|---------|
| `ak_tcp_set_timeout()` | Set send/receive timeouts |
| `ak_tcp_get_timeout()` | Get current timeouts |
| `ak_tcp_set_keepalive()` | Configure keepalive probes |
| `ak_tcp_set_meta()` | Store connection metadata |
| `ak_tcp_get_meta()` | Retrieve connection metadata |

### Server Statistics

| Function | Purpose |
|----------|---------|
| `ak_tcp_server_connection_count()` | Get active connection count |
| `ak_tcp_server_get_stats()` | Get comprehensive statistics |

Statistics include:
- `connections_accepted`: Total accepted connections
- `connections_rejected_limit`: Rejected due to connection limit
- `connections_rejected_queue`: Rejected due to full queue
- `connections_rejected_rate`: Rejected due to rate limit
- `connections_rejected_ip_limit`: Rejected due to per-IP limit
- `active_connections`: Current active connections
- `total_bytes_received`: Cumulative bytes received
- `total_bytes_sent`: Cumulative bytes sent

### TLS Support

TLS is available when compiled with `AK24_TLS_ENABLED`. Use `ak_tcp_tls_available()` to check runtime support.

TLS configuration fields:
- `use_tls`: Enable TLS for the server
- `cert_file`: Path to PEM certificate file
- `key_file`: Path to PEM private key file
- `ca_file`: CA certificate for client verification (optional)
- `verify_client`: Require client certificates
- `tls_ciphers`: Cipher suite list (NULL = defaults)
- `tls_min_version`: Minimum TLS version (0 = TLS 1.2)

TLS-specific errors:
- `AK_TCP_ERR_TLS_INIT`: TLS initialization failed
- `AK_TCP_ERR_TLS_CERT`: Certificate error
- `AK_TCP_ERR_TLS_HANDSHAKE`: Handshake failed

### Initialization

The TCP subsystem is initialized automatically by `ak_kernel_init()`. Manual initialization is available:

| Function | Purpose |
|----------|---------|
| `ak_tcp_init()` | Initialize TCP subsystem |
| `ak_tcp_deinit()` | Deinitialize TCP subsystem |

## Implementation Details

### Thread Model

- **Accept Thread**: Single thread accepts incoming connections
- **Worker Pool**: Configurable thread pool dispatches `on_handle` callbacks
- **Task Queue**: Bounded queue between acceptor and workers (configurable)

### Platform Abstraction

Windows uses Winsock2 APIs with IOCP-compatible socket options. POSIX systems use BSD socket APIs with platform-specific keepalive configuration (TCP_KEEPIDLE on Linux, TCP_KEEPALIVE on macOS).

### Memory Management

- Uses `AK24_ALLOC` and `AK24_FREE` for all allocations
- Connection contexts are allocated per-connection
- Metadata uses the map module for key-value storage
- Buffers use `ak_buffer_t` from the buffer module

### Error Propagation

All I/O functions accept an optional `const char **error` parameter for detailed error messages. The message pointer is valid until the next call on the same context.

## Integration

The TCP module is part of the kernel. Include the header:

```c
#include "tcp.h"
```

The module links against `libak24_kernel.a`. On Windows, link `ws2_32.lib`. For TLS support, link OpenSSL libraries.

## Best Practices

1. **Set Timeouts**: Configure receive/send timeouts to prevent hung connections
2. **Enable Keepalive**: Detect dead connections on long-lived sessions
3. **Use Backpressure**: Set `max_recv_buffer_bytes` to prevent memory exhaustion
4. **Rate Limit**: Configure per-IP limits for public-facing servers
5. **Handle Errors**: Check return values and error codes for all I/O
6. **Graceful Shutdown**: Set `shutdown_drain_timeout_ms` for clean exits
7. **Use Metadata**: Store per-connection state via `ak_tcp_set_meta()`
8. **Buffer Sizing**: Tune socket buffers for high-throughput applications
9. **Monitor Stats**: Use `ak_tcp_server_get_stats()` for observability
