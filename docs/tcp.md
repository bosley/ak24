# TCP Module

The AK24 TCP module provides low-level TCP primitives for building networked applications. It handles socket lifecycle, connection threading, and data delivery—leaving application-level concerns (protocol framing, event dispatch, backpressure policies) to the implementer.

## Design Philosophy

This is a **foundational layer**, not a framework. The module provides:

- Clean lifecycle management for sockets
- Thread-per-connection model for servers
- Blocking I/O with configurable timeouts
- Cross-platform abstraction (POSIX/Windows)
- Optional TLS support

What it does **not** impose:

- Event loop architecture
- Protocol framing
- Application-level flow control
- Message queuing patterns

## Server Architecture

The server uses a thread-per-connection model. When a client connects, a dedicated thread is spawned to handle that connection. This gives implementers a straightforward execution context—no callback scheduling, no state machines.

### Lifecycle

1. **Accept** - Main accept thread receives incoming connections
2. **Dispatch** - Connection handed to thread pool
3. **Handle** - `on_handle` callback runs in dedicated thread
4. **Cleanup** - `on_disconnect` fires, thread returns to pool

### Callbacks

| Callback | Thread Context | Purpose |
|----------|----------------|---------|
| `on_connect` | Accept thread | Accept/reject decision, connection metadata |
| `on_handle` | Worker thread | Main connection logic, runs for connection lifetime |
| `on_disconnect` | Worker thread | Resource cleanup after handler exits |

The `on_handle` callback receives an `ak_tcp_ctx_t*` which provides `ak_tcp_recv` and `ak_tcp_send` for I/O. The callback runs until it returns—there is no implicit event loop.

### Threading Model

Each accepted connection gets a worker thread from the pool. The implementer controls what happens in that thread:

- Block on recv and process synchronously
- Dispatch to an application event loop
- Queue work to an actor system
- Hand off to a coroutine scheduler

The module stays out of the way.

## Client Architecture

The client provides a blocking connection abstraction with optional auto-reconnect.

### Lifecycle

1. **Configure** - Set host, port, timeouts, TLS options
2. **Connect** - Establish connection (blocking with timeout)
3. **Send/Recv** - Blocking I/O operations
4. **Disconnect** - Clean shutdown with optional linger

### Operations

| Function | Behavior |
|----------|----------|
| `ak_tcp_client_send` | Send buffer, returns bytes sent or error |
| `ak_tcp_client_recv` | Receive up to N bytes |
| `ak_tcp_client_recv_until` | Receive until delimiter (for text protocols) |

All operations respect configured timeouts and return structured error codes.

## Error Handling

Both server and client use a dual error pattern:

- **Error code** (`ak_tcp_error_t`) for programmatic handling
- **Error string** (`const char **error`) for diagnostics

Error codes distinguish between timeout, connection reset, network failure, and resource exhaustion—allowing appropriate retry/recovery logic.

## Configuration

### Server Limits

| Option | Purpose |
|--------|---------|
| `max_connections` | Bound concurrent connections |
| `max_pending_tasks` | Bound thread pool queue depth |
| `max_connections_per_ip` | Per-IP connection limit |
| `connection_rate_limit` | New connections per second per IP |

### Timeouts

| Option | Default | Notes |
|--------|---------|-------|
| `default_recv_timeout_ms` | 0 (none) | Applied to new connections |
| `default_send_timeout_ms` | 0 (none) | Applied to new connections |
| `shutdown_drain_timeout_ms` | 0 (immediate) | Grace period during shutdown |

### Socket Tuning

Buffer sizes (`socket_recv_buffer`, `socket_send_buffer`) control kernel buffer allocation. Defaults are appropriate for most workloads; tune for high-throughput or low-latency scenarios.

## TLS Support

When compiled with `AK24_TLS_ENABLED`, both server and client support TLS:

**Server:**
- Certificate and key file paths
- Optional client certificate verification
- Cipher suite and minimum version configuration

**Client:**
- CA certificate for server verification
- Optional client certificate (mutual TLS)
- SNI hostname support

TLS is transparent to the handler—`ak_tcp_recv`/`ak_tcp_send` work identically.

## Resource Usage

Tested with 42 concurrent connections streaming 172 GB total:

| Metric | Value |
|--------|-------|
| Server memory | 2.4–3.7 MB |
| Per-connection overhead | ~200 KB |
| Throughput | 1.4 GB/s (loopback-limited) |

Memory usage remains flat regardless of data volume—buffers are fixed-size and reused.


## Examples

See `examples/tcp-server-example/` and `examples/tcp-client-example/` for complete working code demonstrating server setup, client connection, and stress testing patterns.
