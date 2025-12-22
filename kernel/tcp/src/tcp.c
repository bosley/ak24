/**
 * @file tcp.c
 * @brief Core TCP server implementation
 *
 * Implements the TCP server with three-stage lambda system:
 * 1. on_connect: Accept/reject decision (main thread)
 * 2. on_handle: Connection handler (worker thread)
 * 3. on_disconnect: Cleanup callback (worker thread)
 */

#include "tcp.h"
#include "tcp_internal.h"
#include "threads.h"
#include <stdlib.h>
#include <string.h>

// Default buffer sizes
#define TCP_READ_BUFFER_SIZE 4096
#define TCP_WRITE_BUFFER_SIZE 4096
#define TCP_RECV_CHUNK_SIZE 1024

// Default keepalive settings
#define TCP_DEFAULT_KEEPALIVE_IDLE 60
#define TCP_DEFAULT_KEEPALIVE_INTERVAL 10
#define TCP_DEFAULT_KEEPALIVE_COUNT 5

// ============================================================================
// Error Handling
// ============================================================================

const char *ak_tcp_error_string(ak_tcp_error_t err) {
  switch (err) {
  case AK_TCP_OK:
    return "Success";
  case AK_TCP_ERR_TIMEOUT:
    return "Operation timed out";
  case AK_TCP_ERR_CLOSED:
    return "Connection closed";
  case AK_TCP_ERR_RESET:
    return "Connection reset by peer";
  case AK_TCP_ERR_NETWORK:
    return "Network error";
  case AK_TCP_ERR_MEMORY:
    return "Memory allocation failed";
  case AK_TCP_ERR_INVALID:
    return "Invalid argument";
  case AK_TCP_ERR_LIMIT:
    return "Connection limit reached";
  case AK_TCP_ERR_WOULDBLOCK:
    return "Operation would block";
  default:
    return "Unknown error";
  }
}

ak_tcp_server_config_t ak_tcp_server_config_default(void) {
  ak_tcp_server_config_t cfg = {
      .bind_addr = NULL,
      .port = 0,
      .thread_pool_size = 4,
      .backlog = 128,
      .on_connect = NULL,
      .on_handle = NULL,
      .on_disconnect = NULL,
      .max_connections = 0,         // Unlimited
      .default_recv_timeout_ms = 0, // No timeout
      .default_send_timeout_ms = 0, // No timeout
      .enable_keepalive = false,
      .keepalive_idle_sec = TCP_DEFAULT_KEEPALIVE_IDLE,
      .keepalive_interval_sec = TCP_DEFAULT_KEEPALIVE_INTERVAL,
      .keepalive_count = TCP_DEFAULT_KEEPALIVE_COUNT,
      .recv_buffer_size = TCP_READ_BUFFER_SIZE,
      .send_buffer_size = TCP_WRITE_BUFFER_SIZE,
  };
  return cfg;
}

// ============================================================================
// Initialization / Deinitialization
// ============================================================================

void ak_tcp_init(void) { ak_tcp_platform_init(); }

void ak_tcp_deinit(void) { ak_tcp_platform_deinit(); }

// ============================================================================
// Connection Context
// ============================================================================

ak_tcp_ctx_t *ak_tcp_ctx_new(ak_socket_fd_t socket_fd, const char *remote_ip,
                             uint16_t remote_port, size_t recv_buf_size,
                             size_t send_buf_size) {
  ak_tcp_ctx_t *ctx = AK24_ALLOC(sizeof(ak_tcp_ctx_t));
  if (!ctx) {
    return NULL;
  }

  memset(ctx, 0, sizeof(ak_tcp_ctx_t));
  ctx->socket_fd = socket_fd;
  ctx->remote_port = remote_port;
  ctx->alive = true;
  ctx->recv_timeout_ms = 0;
  ctx->send_timeout_ms = 0;

  if (remote_ip) {
    strncpy(ctx->remote_ip, remote_ip, sizeof(ctx->remote_ip) - 1);
    ctx->remote_ip[sizeof(ctx->remote_ip) - 1] = '\0';
  }

  AK24_MUTEX_INIT(&ctx->mutex);

  size_t actual_recv_size =
      recv_buf_size > 0 ? recv_buf_size : TCP_READ_BUFFER_SIZE;
  ctx->read_buffer = ak_buffer_new(actual_recv_size);
  if (!ctx->read_buffer) {
    AK24_MUTEX_DESTROY(&ctx->mutex);
    AK24_FREE(ctx);
    return NULL;
  }

  size_t actual_send_size =
      send_buf_size > 0 ? send_buf_size : TCP_WRITE_BUFFER_SIZE;
  ctx->write_buffer = ak_buffer_new(actual_send_size);
  if (!ctx->write_buffer) {
    ak_buffer_free(ctx->read_buffer);
    AK24_MUTEX_DESTROY(&ctx->mutex);
    AK24_FREE(ctx);
    return NULL;
  }

  ctx->metadata = ak_context_new();
  if (!ctx->metadata) {
    ak_buffer_free(ctx->write_buffer);
    ak_buffer_free(ctx->read_buffer);
    AK24_MUTEX_DESTROY(&ctx->mutex);
    AK24_FREE(ctx);
    return NULL;
  }

  return ctx;
}

void ak_tcp_ctx_free(ak_tcp_ctx_t *ctx) {
  if (!ctx) {
    return;
  }

  // Close socket if still open
  if (ctx->socket_fd != AK_INVALID_SOCKET) {
    ak_tcp_socket_close(ctx->socket_fd);
  }

  // Free buffers
  if (ctx->read_buffer) {
    ak_buffer_free(ctx->read_buffer);
  }
  if (ctx->write_buffer) {
    ak_buffer_free(ctx->write_buffer);
  }

  // Free metadata
  if (ctx->metadata) {
    ak_context_free(ctx->metadata);
  }

  AK24_MUTEX_DESTROY(&ctx->mutex);
  AK24_FREE(ctx);
}

// ============================================================================
// Connection Context Operations
// ============================================================================

ssize_t ak_tcp_send(ak_tcp_ctx_t *ctx, const ak_buffer_t *data,
                    const char **error) {
  if (!ctx || !data) {
    if (error) {
      *error = "Invalid arguments";
    }
    return -1;
  }

  AK24_MUTEX_LOCK(&ctx->mutex);
  if (!ctx->alive) {
    AK24_MUTEX_UNLOCK(&ctx->mutex);
    if (error) {
      *error = "Connection closed";
    }
    return -1;
  }
  ak_socket_fd_t fd = ctx->socket_fd;
  AK24_MUTEX_UNLOCK(&ctx->mutex);

  size_t total_sent = 0;
  size_t to_send = ak_buffer_count((ak_buffer_t *)data);
  const uint8_t *ptr = ak_buffer_data((ak_buffer_t *)data);

  while (total_sent < to_send) {
    ssize_t sent =
        ak_tcp_socket_send(fd, ptr + total_sent, to_send - total_sent, error);
    if (sent < 0) {
      return -1;
    }
    if (sent == 0) {
      // Connection closed
      AK24_MUTEX_LOCK(&ctx->mutex);
      ctx->alive = false;
      AK24_MUTEX_UNLOCK(&ctx->mutex);
      if (error) {
        *error = "Connection closed by peer";
      }
      return -1;
    }
    total_sent += (size_t)sent;
  }

  return (ssize_t)total_sent;
}

ssize_t ak_tcp_recv(ak_tcp_ctx_t *ctx, ak_buffer_t *buffer, size_t max_bytes,
                    const char **error) {
  if (!ctx || !buffer) {
    if (error) {
      *error = "Invalid arguments";
    }
    return -1;
  }

  AK24_MUTEX_LOCK(&ctx->mutex);
  if (!ctx->alive) {
    AK24_MUTEX_UNLOCK(&ctx->mutex);
    if (error) {
      *error = "Connection closed";
    }
    return -1;
  }
  ak_socket_fd_t fd = ctx->socket_fd;
  AK24_MUTEX_UNLOCK(&ctx->mutex);

  uint8_t temp[TCP_RECV_CHUNK_SIZE];
  size_t to_recv =
      max_bytes < TCP_RECV_CHUNK_SIZE ? max_bytes : TCP_RECV_CHUNK_SIZE;

  ssize_t received = ak_tcp_socket_recv(fd, temp, to_recv, error);
  if (received < 0) {
    return -1;
  }
  if (received == 0) {
    // Connection closed
    AK24_MUTEX_LOCK(&ctx->mutex);
    ctx->alive = false;
    AK24_MUTEX_UNLOCK(&ctx->mutex);
    return 0;
  }

  if (ak_buffer_copy_to(buffer, temp, (size_t)received) != 0) {
    if (error) {
      *error = "Buffer allocation failed";
    }
    return -1;
  }

  return received;
}

ak_buffer_t *ak_tcp_recv_until(ak_tcp_ctx_t *ctx, const char *delim,
                               const char **error) {
  if (!ctx || !delim) {
    if (error) {
      *error = "Invalid arguments";
    }
    return NULL;
  }

  size_t delim_len = strlen(delim);
  if (delim_len == 0) {
    if (error) {
      *error = "Empty delimiter";
    }
    return NULL;
  }

  ak_buffer_t *result = ak_buffer_new(256);
  if (!result) {
    if (error) {
      *error = "Buffer allocation failed";
    }
    return NULL;
  }

  while (ak_tcp_is_alive(ctx)) {
    // Check if delimiter is already in buffer
    size_t count = ak_buffer_count(result);
    uint8_t *data = ak_buffer_data(result);

    if (count >= delim_len) {
      for (size_t i = 0; i <= count - delim_len; i++) {
        if (memcmp(data + i, delim, delim_len) == 0) {
          // Found delimiter - truncate result at delimiter
          result->count = i + delim_len;
          return result;
        }
      }
    }

    // Read more data
    AK24_MUTEX_LOCK(&ctx->mutex);
    if (!ctx->alive) {
      AK24_MUTEX_UNLOCK(&ctx->mutex);
      break;
    }
    ak_socket_fd_t fd = ctx->socket_fd;
    AK24_MUTEX_UNLOCK(&ctx->mutex);

    uint8_t temp[TCP_RECV_CHUNK_SIZE];
    ssize_t received = ak_tcp_socket_recv(fd, temp, TCP_RECV_CHUNK_SIZE, error);
    if (received < 0) {
      ak_buffer_free(result);
      return NULL;
    }
    if (received == 0) {
      // Connection closed
      AK24_MUTEX_LOCK(&ctx->mutex);
      ctx->alive = false;
      AK24_MUTEX_UNLOCK(&ctx->mutex);
      break;
    }

    if (ak_buffer_copy_to(result, temp, (size_t)received) != 0) {
      if (error) {
        *error = "Buffer allocation failed";
      }
      ak_buffer_free(result);
      return NULL;
    }
  }

  // Connection closed without finding delimiter
  if (ak_buffer_count(result) == 0) {
    ak_buffer_free(result);
    return NULL;
  }

  return result;
}

bool ak_tcp_is_alive(ak_tcp_ctx_t *ctx) {
  if (!ctx) {
    return false;
  }

  AK24_MUTEX_LOCK(&ctx->mutex);

  // Quick check: already marked dead?
  if (!ctx->alive || ctx->socket_fd == AK_INVALID_SOCKET) {
    AK24_MUTEX_UNLOCK(&ctx->mutex);
    return false;
  }

  // Probe socket to detect peer close
  if (ak_tcp_socket_peer_closed(ctx->socket_fd)) {
    ctx->alive = false;
    AK24_MUTEX_UNLOCK(&ctx->mutex);
    return false;
  }

  AK24_MUTEX_UNLOCK(&ctx->mutex);
  return true;
}

void ak_tcp_close(ak_tcp_ctx_t *ctx) {
  if (!ctx) {
    return;
  }

  AK24_MUTEX_LOCK(&ctx->mutex);
  if (ctx->alive) {
    ctx->alive = false;
    if (ctx->socket_fd != AK_INVALID_SOCKET) {
      ak_tcp_socket_close(ctx->socket_fd);
      ctx->socket_fd = AK_INVALID_SOCKET;
    }
  }
  AK24_MUTEX_UNLOCK(&ctx->mutex);
}

void ak_tcp_kill(ak_tcp_ctx_t *ctx) {
  // Thread-safe forced close
  ak_tcp_close(ctx);
}

void ak_tcp_set_meta(ak_tcp_ctx_t *ctx, const char *key, void *value) {
  if (!ctx || !key) {
    return;
  }
  ak_context_set(ctx->metadata, key, value);
}

void *ak_tcp_get_meta(ak_tcp_ctx_t *ctx, const char *key) {
  if (!ctx || !key) {
    return NULL;
  }
  return ak_context_get(ctx->metadata, key);
}

// ============================================================================
// Timeout Operations
// ============================================================================

ak_tcp_error_t ak_tcp_set_timeout(ak_tcp_ctx_t *ctx, uint32_t recv_timeout_ms,
                                  uint32_t send_timeout_ms) {
  if (!ctx) {
    return AK_TCP_ERR_INVALID;
  }

  AK24_MUTEX_LOCK(&ctx->mutex);
  if (!ctx->alive || ctx->socket_fd == AK_INVALID_SOCKET) {
    AK24_MUTEX_UNLOCK(&ctx->mutex);
    return AK_TCP_ERR_CLOSED;
  }

  const char *error = NULL;
  if (ak_tcp_socket_set_timeout(ctx->socket_fd, recv_timeout_ms,
                                send_timeout_ms, &error) != 0) {
    AK24_MUTEX_UNLOCK(&ctx->mutex);
    AK24_LOG_WARN("Failed to set timeout: %s", error ? error : "Unknown");
    return AK_TCP_ERR_NETWORK;
  }

  ctx->recv_timeout_ms = recv_timeout_ms;
  ctx->send_timeout_ms = send_timeout_ms;
  AK24_MUTEX_UNLOCK(&ctx->mutex);

  return AK_TCP_OK;
}

ak_tcp_error_t ak_tcp_get_timeout(ak_tcp_ctx_t *ctx, uint32_t *recv_timeout_ms,
                                  uint32_t *send_timeout_ms) {
  if (!ctx) {
    return AK_TCP_ERR_INVALID;
  }

  AK24_MUTEX_LOCK(&ctx->mutex);
  if (recv_timeout_ms) {
    *recv_timeout_ms = ctx->recv_timeout_ms;
  }
  if (send_timeout_ms) {
    *send_timeout_ms = ctx->send_timeout_ms;
  }
  AK24_MUTEX_UNLOCK(&ctx->mutex);

  return AK_TCP_OK;
}

// ============================================================================
// Keepalive Operations
// ============================================================================

ak_tcp_error_t ak_tcp_set_keepalive(ak_tcp_ctx_t *ctx, int idle_sec,
                                    int interval_sec, int probe_count) {
  if (!ctx) {
    return AK_TCP_ERR_INVALID;
  }

  AK24_MUTEX_LOCK(&ctx->mutex);
  if (!ctx->alive || ctx->socket_fd == AK_INVALID_SOCKET) {
    AK24_MUTEX_UNLOCK(&ctx->mutex);
    return AK_TCP_ERR_CLOSED;
  }

  const char *error = NULL;
  if (ak_tcp_socket_set_keepalive(ctx->socket_fd, idle_sec, interval_sec,
                                  probe_count, &error) != 0) {
    AK24_MUTEX_UNLOCK(&ctx->mutex);
    AK24_LOG_WARN("Failed to set keepalive: %s", error ? error : "Unknown");
    return AK_TCP_ERR_NETWORK;
  }

  AK24_MUTEX_UNLOCK(&ctx->mutex);
  return AK_TCP_OK;
}

// ============================================================================
// Graceful Shutdown
// ============================================================================

void ak_tcp_shutdown(ak_tcp_ctx_t *ctx) {
  if (!ctx) {
    return;
  }

  AK24_MUTEX_LOCK(&ctx->mutex);
  if (ctx->alive && ctx->socket_fd != AK_INVALID_SOCKET) {
    // Graceful shutdown - half-close the write side
    ak_tcp_socket_shutdown(ctx->socket_fd);
    // Mark as not alive for further operations
    ctx->alive = false;
  }
  AK24_MUTEX_UNLOCK(&ctx->mutex);
}

void ak_tcp_abort(ak_tcp_ctx_t *ctx) {
  // Abort is the same as close (immediate termination)
  ak_tcp_close(ctx);
}

// ============================================================================
// Extended Error-Code Operations
// ============================================================================

ak_tcp_error_t ak_tcp_send_ex(ak_tcp_ctx_t *ctx, const ak_buffer_t *data,
                              size_t *bytes_sent, const char **error) {
  if (bytes_sent) {
    *bytes_sent = 0;
  }

  ssize_t result = ak_tcp_send(ctx, data, error);
  if (result < 0) {
    // Determine specific error
    if (!ctx) {
      return AK_TCP_ERR_INVALID;
    }
    if (!ak_tcp_is_alive(ctx)) {
      return AK_TCP_ERR_CLOSED;
    }
    return AK_TCP_ERR_NETWORK;
  }

  if (bytes_sent) {
    *bytes_sent = (size_t)result;
  }
  return AK_TCP_OK;
}

ak_tcp_error_t ak_tcp_recv_ex(ak_tcp_ctx_t *ctx, ak_buffer_t *buffer,
                              size_t max_bytes, size_t *bytes_received,
                              const char **error) {
  if (bytes_received) {
    *bytes_received = 0;
  }

  ssize_t result = ak_tcp_recv(ctx, buffer, max_bytes, error);
  if (result < 0) {
    if (!ctx) {
      return AK_TCP_ERR_INVALID;
    }
    if (!ak_tcp_is_alive(ctx)) {
      return AK_TCP_ERR_CLOSED;
    }
    // Check if it was a timeout
    if (error && *error && strstr(*error, "timed out")) {
      return AK_TCP_ERR_TIMEOUT;
    }
    return AK_TCP_ERR_NETWORK;
  }

  if (result == 0) {
    return AK_TCP_ERR_CLOSED;
  }

  if (bytes_received) {
    *bytes_received = (size_t)result;
  }
  return AK_TCP_OK;
}

ak_tcp_error_t ak_tcp_recv_until_ex(ak_tcp_ctx_t *ctx, const char *delim,
                                    ak_buffer_t **result, const char **error) {
  if (result) {
    *result = NULL;
  }

  if (!ctx || !delim || !result) {
    if (error) {
      *error = "Invalid arguments";
    }
    return AK_TCP_ERR_INVALID;
  }

  ak_buffer_t *buf = ak_tcp_recv_until(ctx, delim, error);
  if (!buf) {
    if (!ak_tcp_is_alive(ctx)) {
      return AK_TCP_ERR_CLOSED;
    }
    if (error && *error && strstr(*error, "timed out")) {
      return AK_TCP_ERR_TIMEOUT;
    }
    return AK_TCP_ERR_NETWORK;
  }

  *result = buf;
  return AK_TCP_OK;
}

// ============================================================================
// Server Connection Count
// ============================================================================

size_t ak_tcp_server_connection_count(ak_tcp_server_t *server) {
  if (!server || !server->internal) {
    return 0;
  }

  AK24_MUTEX_LOCK(&server->internal->mutex);
  size_t count = server->internal->active_connections;
  AK24_MUTEX_UNLOCK(&server->internal->mutex);

  return count;
}

// ============================================================================
// Worker Thread Handler
// ============================================================================

static void worker_task_fn(void *captured, void *args) {
  (void)args;
  ak_tcp_worker_task_t *task = (ak_tcp_worker_task_t *)captured;
  if (!task) {
    return;
  }

  ak_tcp_ctx_t *ctx = task->ctx;
  ak_tcp_server_internal_t *server = task->server;

  // Stage 2: Call on_handle
  if (server->on_handle) {
    ak_lambda_invoke(server->on_handle, ctx);
  }

  // Stage 3: Call on_disconnect
  if (server->on_disconnect) {
    ak_lambda_invoke(server->on_disconnect, ctx);
  }

  // Decrement active connection count
  AK24_MUTEX_LOCK(&server->mutex);
  if (server->active_connections > 0) {
    server->active_connections--;
  }
  AK24_MUTEX_UNLOCK(&server->mutex);

  // Cleanup
  ak_tcp_ctx_free(ctx);
  AK24_FREE(task);
}

// ============================================================================
// Accept Loop
// ============================================================================

static void *accept_loop(void *arg) {
  ak_tcp_server_internal_t *server = (ak_tcp_server_internal_t *)arg;

  while (true) {
    AK24_MUTEX_LOCK(&server->mutex);
    bool running = server->running;
    AK24_MUTEX_UNLOCK(&server->mutex);

    if (!running) {
      break;
    }

    // Accept connection
    char remote_ip[46] = {0};
    uint16_t remote_port = 0;
    const char *error = NULL;

    ak_socket_fd_t client_fd = ak_tcp_socket_accept(
        server->listen_fd, remote_ip, &remote_port, &error);

    if (client_fd == AK_INVALID_SOCKET) {
      // Check if we're shutting down
      AK24_MUTEX_LOCK(&server->mutex);
      running = server->running;
      AK24_MUTEX_UNLOCK(&server->mutex);
      if (!running) {
        break;
      }
      // Otherwise, log and continue
      AK24_LOG_WARN("Accept failed: %s", error ? error : "Unknown error");
      continue;
    }

    // Check connection limit
    AK24_MUTEX_LOCK(&server->mutex);
    bool limit_reached =
        (server->max_connections > 0 &&
         server->active_connections >= server->max_connections);
    AK24_MUTEX_UNLOCK(&server->mutex);

    if (limit_reached) {
      ak_tcp_socket_close(client_fd);
      AK24_LOG_WARN("Connection limit reached, rejecting %s:%d", remote_ip,
                    remote_port);
      continue;
    }

    // Stage 1: Check on_connect decision
    bool accept_connection = true;
    if (server->on_connect) {
      ak_tcp_conn_info_t info = {
          .remote_ip = remote_ip,
          .remote_port = remote_port,
          .accept = true // Default to accept
      };
      ak_lambda_invoke(server->on_connect, &info);
      accept_connection = info.accept;
    }

    if (!accept_connection) {
      ak_tcp_socket_close(client_fd);
      AK24_LOG_DEBUG("Rejected connection from %s:%d", remote_ip, remote_port);
      continue;
    }

    // Apply socket options before creating context

    // Apply timeouts if configured
    if (server->default_recv_timeout_ms > 0 ||
        server->default_send_timeout_ms > 0) {
      if (ak_tcp_socket_set_timeout(client_fd, server->default_recv_timeout_ms,
                                    server->default_send_timeout_ms,
                                    &error) != 0) {
        AK24_LOG_WARN("Failed to set timeout: %s", error ? error : "Unknown");
      }
    }

    // Apply keepalive if configured
    if (server->enable_keepalive) {
      if (ak_tcp_socket_set_keepalive(client_fd, server->keepalive_idle_sec,
                                      server->keepalive_interval_sec,
                                      server->keepalive_count, &error) != 0) {
        AK24_LOG_WARN("Failed to set keepalive: %s", error ? error : "Unknown");
      }
    }

    // Create connection context with configured buffer sizes
    ak_tcp_ctx_t *ctx =
        ak_tcp_ctx_new(client_fd, remote_ip, remote_port,
                       server->recv_buffer_size, server->send_buffer_size);
    if (!ctx) {
      ak_tcp_socket_close(client_fd);
      AK24_LOG_ERROR("Failed to create connection context");
      continue;
    }

    // Store timeout settings in context
    ctx->recv_timeout_ms = server->default_recv_timeout_ms;
    ctx->send_timeout_ms = server->default_send_timeout_ms;

    // Increment active connection count
    AK24_MUTEX_LOCK(&server->mutex);
    server->active_connections++;
    AK24_MUTEX_UNLOCK(&server->mutex);

    // Create worker task
    ak_tcp_worker_task_t *task = AK24_ALLOC(sizeof(ak_tcp_worker_task_t));
    if (!task) {
      AK24_MUTEX_LOCK(&server->mutex);
      if (server->active_connections > 0) {
        server->active_connections--;
      }
      AK24_MUTEX_UNLOCK(&server->mutex);
      ak_tcp_ctx_free(ctx);
      AK24_LOG_ERROR("Failed to create worker task");
      continue;
    }
    task->ctx = ctx;
    task->server = server;

    // Create lambda for worker and enqueue to thread pool
    ak_lambda_t *worker_lambda = ak_lambda_new(worker_task_fn, task, NULL);
    if (!worker_lambda) {
      AK24_MUTEX_LOCK(&server->mutex);
      if (server->active_connections > 0) {
        server->active_connections--;
      }
      AK24_MUTEX_UNLOCK(&server->mutex);
      AK24_FREE(task);
      ak_tcp_ctx_free(ctx);
      AK24_LOG_ERROR("Failed to create worker lambda");
      continue;
    }

    if (ak_thread_pool_enqueue(server->thread_pool, worker_lambda, NULL) != 0) {
      AK24_MUTEX_LOCK(&server->mutex);
      if (server->active_connections > 0) {
        server->active_connections--;
      }
      AK24_MUTEX_UNLOCK(&server->mutex);
      ak_lambda_free(worker_lambda);
      AK24_FREE(task);
      ak_tcp_ctx_free(ctx);
      AK24_LOG_ERROR("Failed to enqueue worker task");
      continue;
    }

    AK24_LOG_DEBUG("Accepted connection from %s:%d", remote_ip, remote_port);
  }

  return NULL;
}

// ============================================================================
// Server Lifecycle
// ============================================================================

ak_tcp_server_t *ak_tcp_server_new(const ak_tcp_server_config_t *cfg,
                                   const char **error) {
  if (!cfg) {
    if (error) {
      *error = "Configuration is required";
    }
    return NULL;
  }

  if (!cfg->on_handle) {
    if (error) {
      *error = "on_handle lambda is required";
    }
    return NULL;
  }

  if (cfg->port == 0) {
    if (error) {
      *error = "Port must be non-zero";
    }
    return NULL;
  }

  // Allocate server
  ak_tcp_server_t *server = AK24_ALLOC(sizeof(ak_tcp_server_t));
  if (!server) {
    if (error) {
      *error = "Memory allocation failed";
    }
    return NULL;
  }

  // Allocate internal
  ak_tcp_server_internal_t *internal =
      AK24_ALLOC(sizeof(ak_tcp_server_internal_t));
  if (!internal) {
    AK24_FREE(server);
    if (error) {
      *error = "Memory allocation failed";
    }
    return NULL;
  }

  memset(internal, 0, sizeof(ak_tcp_server_internal_t));
  server->internal = internal;

  // Copy configuration
  internal->port = cfg->port;
  internal->backlog = cfg->backlog > 0 ? cfg->backlog : 128;
  internal->listen_fd = AK_INVALID_SOCKET;
  internal->running = false;

  if (cfg->bind_addr) {
    size_t addr_len = strlen(cfg->bind_addr) + 1;
    internal->bind_addr = AK24_ALLOC(addr_len);
    if (!internal->bind_addr) {
      AK24_FREE(internal);
      AK24_FREE(server);
      if (error) {
        *error = "Memory allocation failed";
      }
      return NULL;
    }
    memcpy(internal->bind_addr, cfg->bind_addr, addr_len);
  }

  // Store lambdas (ownership transferred)
  internal->on_connect = cfg->on_connect;
  internal->on_handle = cfg->on_handle;
  internal->on_disconnect = cfg->on_disconnect;

  // Copy connection limits and options
  internal->max_connections = cfg->max_connections;
  internal->active_connections = 0;

  // Copy timeout defaults
  internal->default_recv_timeout_ms = cfg->default_recv_timeout_ms;
  internal->default_send_timeout_ms = cfg->default_send_timeout_ms;

  // Copy keepalive defaults
  internal->enable_keepalive = cfg->enable_keepalive;
  internal->keepalive_idle_sec = cfg->keepalive_idle_sec > 0
                                     ? cfg->keepalive_idle_sec
                                     : TCP_DEFAULT_KEEPALIVE_IDLE;
  internal->keepalive_interval_sec = cfg->keepalive_interval_sec > 0
                                         ? cfg->keepalive_interval_sec
                                         : TCP_DEFAULT_KEEPALIVE_INTERVAL;
  internal->keepalive_count = cfg->keepalive_count > 0
                                  ? cfg->keepalive_count
                                  : TCP_DEFAULT_KEEPALIVE_COUNT;

  // Copy buffer sizes
  internal->recv_buffer_size =
      cfg->recv_buffer_size > 0 ? cfg->recv_buffer_size : TCP_READ_BUFFER_SIZE;
  internal->send_buffer_size =
      cfg->send_buffer_size > 0 ? cfg->send_buffer_size : TCP_WRITE_BUFFER_SIZE;

  // Initialize mutex and condition
  AK24_MUTEX_INIT(&internal->mutex);
  AK24_COND_INIT(&internal->shutdown_cond);

  // Create thread pool
  size_t pool_size = cfg->thread_pool_size > 0 ? cfg->thread_pool_size : 4;
  ak_thread_pool_config_t pool_cfg = ak_thread_pool_config_default();
  pool_cfg.max_workers = pool_size;

  internal->thread_pool = ak_thread_pool_new(&pool_cfg);
  if (!internal->thread_pool) {
    if (internal->bind_addr) {
      AK24_FREE(internal->bind_addr);
    }
    AK24_MUTEX_DESTROY(&internal->mutex);
    AK24_COND_DESTROY(&internal->shutdown_cond);
    AK24_FREE(internal);
    AK24_FREE(server);
    if (error) {
      *error = "Failed to create thread pool";
    }
    return NULL;
  }

  return server;
}

int ak_tcp_server_start(ak_tcp_server_t *server, const char **error) {
  if (!server || !server->internal) {
    if (error) {
      *error = "Invalid server";
    }
    return -1;
  }

  ak_tcp_server_internal_t *internal = server->internal;

  AK24_MUTEX_LOCK(&internal->mutex);
  if (internal->running) {
    AK24_MUTEX_UNLOCK(&internal->mutex);
    if (error) {
      *error = "Server already running";
    }
    return -1;
  }
  AK24_MUTEX_UNLOCK(&internal->mutex);

  // Create listening socket
  internal->listen_fd = ak_tcp_socket_create(error);
  if (internal->listen_fd == AK_INVALID_SOCKET) {
    return -1;
  }

  // Bind
  if (ak_tcp_socket_bind(internal->listen_fd, internal->bind_addr,
                         internal->port, error) != 0) {
    ak_tcp_socket_close(internal->listen_fd);
    internal->listen_fd = AK_INVALID_SOCKET;
    return -1;
  }

  // Listen
  if (ak_tcp_socket_listen(internal->listen_fd, internal->backlog, error) !=
      0) {
    ak_tcp_socket_close(internal->listen_fd);
    internal->listen_fd = AK_INVALID_SOCKET;
    return -1;
  }

  // Mark as running
  AK24_MUTEX_LOCK(&internal->mutex);
  internal->running = true;
  AK24_MUTEX_UNLOCK(&internal->mutex);

  // Start accept thread
  if (AK24_THREAD_CREATE(&internal->accept_thread, accept_loop, internal) !=
      0) {
    AK24_MUTEX_LOCK(&internal->mutex);
    internal->running = false;
    AK24_MUTEX_UNLOCK(&internal->mutex);
    ak_tcp_socket_close(internal->listen_fd);
    internal->listen_fd = AK_INVALID_SOCKET;
    if (error) {
      *error = "Failed to create accept thread";
    }
    return -1;
  }

  return 0;
}

void ak_tcp_server_stop(ak_tcp_server_t *server) {
  if (!server || !server->internal) {
    return;
  }

  ak_tcp_server_internal_t *internal = server->internal;

  // Signal shutdown
  AK24_MUTEX_LOCK(&internal->mutex);
  if (!internal->running) {
    AK24_MUTEX_UNLOCK(&internal->mutex);
    return;
  }
  internal->running = false;
  AK24_MUTEX_UNLOCK(&internal->mutex);

  // Close listening socket to unblock accept
  if (internal->listen_fd != AK_INVALID_SOCKET) {
    ak_tcp_socket_close(internal->listen_fd);
    internal->listen_fd = AK_INVALID_SOCKET;
  }

  // Wait for accept thread to finish
  AK24_THREAD_JOIN(internal->accept_thread);

  // Wait for thread pool to drain
  ak_thread_pool_wait(internal->thread_pool);
}

void ak_tcp_server_free(ak_tcp_server_t *server) {
  if (!server) {
    return;
  }

  if (server->internal) {
    ak_tcp_server_internal_t *internal = server->internal;

    // Stop if still running
    ak_tcp_server_stop(server);

    // Free thread pool
    if (internal->thread_pool) {
      ak_thread_pool_free(internal->thread_pool);
    }

    // Free lambdas
    if (internal->on_connect) {
      ak_lambda_free(internal->on_connect);
    }
    if (internal->on_handle) {
      ak_lambda_free(internal->on_handle);
    }
    if (internal->on_disconnect) {
      ak_lambda_free(internal->on_disconnect);
    }

    // Free bind address
    if (internal->bind_addr) {
      AK24_FREE(internal->bind_addr);
    }

    // Destroy synchronization primitives
    AK24_MUTEX_DESTROY(&internal->mutex);
    AK24_COND_DESTROY(&internal->shutdown_cond);

    AK24_FREE(internal);
  }

  AK24_FREE(server);
}
