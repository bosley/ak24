#ifndef AK24_TCP_H
#define AK24_TCP_H

#include "kernel.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct ak_tcp_ctx_s ak_tcp_ctx_t;
typedef struct ak_tcp_server_s ak_tcp_server_t;

/**
 * @brief TCP error codes for structured error handling
 */
typedef enum {
  AK_TCP_OK = 0,         /**< Success */
  AK_TCP_ERR_TIMEOUT,    /**< Operation timed out */
  AK_TCP_ERR_CLOSED,     /**< Connection closed gracefully */
  AK_TCP_ERR_RESET,      /**< Connection reset by peer */
  AK_TCP_ERR_NETWORK,    /**< Network error */
  AK_TCP_ERR_MEMORY,     /**< Memory allocation failed */
  AK_TCP_ERR_INVALID,    /**< Invalid argument */
  AK_TCP_ERR_LIMIT,      /**< Connection limit reached */
  AK_TCP_ERR_WOULDBLOCK, /**< Operation would block (non-blocking mode) */
} ak_tcp_error_t;

/**
 * @brief Convert error code to string
 *
 * @param err Error code
 * @return Human-readable error string
 */
const char *ak_tcp_error_string(ak_tcp_error_t err);

/**
 * @brief Connection info passed to on_connect callback
 *
 * The on_connect lambda receives this struct as invoke_args.
 * Set accept to false to reject the connection.
 */
typedef struct {
  const char *remote_ip; /**< Remote IP address string */
  uint16_t remote_port;  /**< Remote port number */
  bool accept;           /**< Set to false to reject (default: true) */
} ak_tcp_conn_info_t;

/**
 * @brief Server configuration
 */
typedef struct {
  const char *bind_addr;   /**< Bind address (NULL or "0.0.0.0" for any) */
  uint16_t port;           /**< Port number */
  size_t thread_pool_size; /**< Number of worker threads (default: 4) */
  int backlog;             /**< Listen backlog (default: 128) */
  ak_lambda_t
      *on_connect; /**< Accept decision: void(*)(void*, ak_tcp_conn_info_t*) */
  ak_lambda_t
      *on_handle; /**< Connection handler: void(*)(void*, ak_tcp_ctx_t*) */
  ak_lambda_t
      *on_disconnect; /**< Cleanup callback: void(*)(void*, ak_tcp_ctx_t*) */

  // Connection limits
  size_t max_connections; /**< Max concurrent connections (0 = unlimited) */

  // Timeout defaults (milliseconds, 0 = no timeout)
  uint32_t default_recv_timeout_ms; /**< Default receive timeout for new
                                       connections */
  uint32_t
      default_send_timeout_ms; /**< Default send timeout for new connections */

  // Keepalive defaults (0 = system defaults or disabled)
  bool enable_keepalive;      /**< Enable TCP keepalive on connections */
  int keepalive_idle_sec;     /**< Seconds before first probe (default: 60) */
  int keepalive_interval_sec; /**< Seconds between probes (default: 10) */
  int keepalive_count;        /**< Probes before declaring dead (default: 5) */

  // Buffer sizes (0 = use defaults)
  size_t recv_buffer_size; /**< Receive buffer size (default: 4096) */
  size_t send_buffer_size; /**< Send buffer size (default: 4096) */
} ak_tcp_server_config_t;

/**
 * @brief Get default server configuration
 *
 * @return Configuration with sensible defaults
 */
ak_tcp_server_config_t ak_tcp_server_config_default(void);

// Server lifecycle
/**
 * @brief Create a new TCP server
 *
 * @param cfg Server configuration (required)
 * @param error Output error message on failure
 * @return New server handle or NULL on failure
 */
ak_tcp_server_t *ak_tcp_server_new(const ak_tcp_server_config_t *cfg,
                                   const char **error);

/**
 * @brief Start the server
 *
 * Spawns accept thread and worker pool, returns immediately.
 *
 * @param server Server handle
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_server_start(ak_tcp_server_t *server, const char **error);

/**
 * @brief Stop the server
 *
 * Stops accepting connections, interrupts active connections, waits for
 * threads.
 *
 * @param server Server handle
 */
void ak_tcp_server_stop(ak_tcp_server_t *server);

/**
 * @brief Free server resources
 *
 * @param server Server handle (NULL is safe)
 */
void ak_tcp_server_free(ak_tcp_server_t *server);

// Connection context operations
/**
 * @brief Send data on connection
 *
 * @param ctx Connection context
 * @param data Buffer to send
 * @param error Output error message on failure
 * @return Bytes sent or -1 on failure
 */
ssize_t ak_tcp_send(ak_tcp_ctx_t *ctx, const ak_buffer_t *data,
                    const char **error);

/**
 * @brief Receive data from connection
 *
 * @param ctx Connection context
 * @param buffer Buffer to receive into (appends data)
 * @param max_bytes Maximum bytes to read
 * @param error Output error message on failure
 * @return Bytes received, 0 on close, or -1 on failure
 */
ssize_t ak_tcp_recv(ak_tcp_ctx_t *ctx, ak_buffer_t *buffer, size_t max_bytes,
                    const char **error);

/**
 * @brief Receive until delimiter found
 *
 * @param ctx Connection context
 * @param delim Delimiter string to search for
 * @param error Output error message on failure
 * @return New buffer with data up to and including delimiter, or NULL on
 * failure
 */
ak_buffer_t *ak_tcp_recv_until(ak_tcp_ctx_t *ctx, const char *delim,
                               const char **error);

/**
 * @brief Check if connection is alive
 *
 * Thread-safe check.
 *
 * @param ctx Connection context
 * @return true if alive, false otherwise
 */
bool ak_tcp_is_alive(ak_tcp_ctx_t *ctx);

/**
 * @brief Close connection gracefully
 *
 * @param ctx Connection context
 */
void ak_tcp_close(ak_tcp_ctx_t *ctx);

/**
 * @brief Force close connection (can be called from another thread)
 *
 * @param ctx Connection context
 */
void ak_tcp_kill(ak_tcp_ctx_t *ctx);

/**
 * @brief Set metadata on connection
 *
 * @param ctx Connection context
 * @param key Metadata key
 * @param value Metadata value
 */
void ak_tcp_set_meta(ak_tcp_ctx_t *ctx, const char *key, void *value);

/**
 * @brief Get metadata from connection
 *
 * @param ctx Connection context
 * @param key Metadata key
 * @return Metadata value or NULL if not found
 */
void *ak_tcp_get_meta(ak_tcp_ctx_t *ctx, const char *key);

// Timeout operations
/**
 * @brief Set socket timeouts for a connection
 *
 * @param ctx Connection context
 * @param recv_timeout_ms Receive timeout in milliseconds (0 = no timeout)
 * @param send_timeout_ms Send timeout in milliseconds (0 = no timeout)
 * @return AK_TCP_OK on success, error code on failure
 */
ak_tcp_error_t ak_tcp_set_timeout(ak_tcp_ctx_t *ctx, uint32_t recv_timeout_ms,
                                  uint32_t send_timeout_ms);

/**
 * @brief Get current socket timeouts
 *
 * @param ctx Connection context
 * @param recv_timeout_ms Output for receive timeout (can be NULL)
 * @param send_timeout_ms Output for send timeout (can be NULL)
 * @return AK_TCP_OK on success, error code on failure
 */
ak_tcp_error_t ak_tcp_get_timeout(ak_tcp_ctx_t *ctx, uint32_t *recv_timeout_ms,
                                  uint32_t *send_timeout_ms);

// Keepalive operations
/**
 * @brief Configure TCP keepalive for a connection
 *
 * @param ctx Connection context
 * @param idle_sec Seconds before first probe (0 = disable keepalive)
 * @param interval_sec Seconds between probes
 * @param probe_count Number of probes before declaring dead
 * @return AK_TCP_OK on success, error code on failure
 */
ak_tcp_error_t ak_tcp_set_keepalive(ak_tcp_ctx_t *ctx, int idle_sec,
                                    int interval_sec, int probe_count);

// Graceful shutdown
/**
 * @brief Gracefully shutdown connection (finish pending sends, then close)
 *
 * Unlike ak_tcp_close() which does immediate close, this function:
 * 1. Stops accepting new data
 * 2. Finishes sending data in the kernel buffer
 * 3. Waits for peer to close their end
 * 4. Closes the connection
 *
 * @param ctx Connection context
 */
void ak_tcp_shutdown(ak_tcp_ctx_t *ctx);

/**
 * @brief Abort connection immediately (same as ak_tcp_close)
 *
 * @param ctx Connection context
 */
void ak_tcp_abort(ak_tcp_ctx_t *ctx);

// Server stats
/**
 * @brief Get current active connection count
 *
 * @param server Server handle
 * @return Number of active connections
 */
size_t ak_tcp_server_connection_count(ak_tcp_server_t *server);

// Extended error-code based operations
/**
 * @brief Send data with error code return
 *
 * @param ctx Connection context
 * @param data Buffer to send
 * @param bytes_sent Output for bytes actually sent (can be NULL)
 * @param error Output error message on failure (can be NULL)
 * @return AK_TCP_OK on success, error code on failure
 */
ak_tcp_error_t ak_tcp_send_ex(ak_tcp_ctx_t *ctx, const ak_buffer_t *data,
                              size_t *bytes_sent, const char **error);

/**
 * @brief Receive data with error code return
 *
 * @param ctx Connection context
 * @param buffer Buffer to receive into (appends data)
 * @param max_bytes Maximum bytes to read
 * @param bytes_received Output for bytes actually received (can be NULL)
 * @param error Output error message on failure (can be NULL)
 * @return AK_TCP_OK on success, AK_TCP_ERR_CLOSED on close, error code on
 * failure
 */
ak_tcp_error_t ak_tcp_recv_ex(ak_tcp_ctx_t *ctx, ak_buffer_t *buffer,
                              size_t max_bytes, size_t *bytes_received,
                              const char **error);

/**
 * @brief Receive until delimiter with error code return
 *
 * @param ctx Connection context
 * @param delim Delimiter string to search for
 * @param result Output for result buffer (caller must free)
 * @param error Output error message on failure (can be NULL)
 * @return AK_TCP_OK on success, AK_TCP_ERR_TIMEOUT on timeout, error code on
 * failure
 */
ak_tcp_error_t ak_tcp_recv_until_ex(ak_tcp_ctx_t *ctx, const char *delim,
                                    ak_buffer_t **result, const char **error);

// Initialization/Deinit (called by ak_kernel_init/deinit)
/**
 * @brief Initialize TCP subsystem
 *
 * Called automatically by ak_kernel_init().
 */
void ak_tcp_init(void);

/**
 * @brief Deinitialize TCP subsystem
 *
 * Called automatically by ak_kernel_deinit().
 */
void ak_tcp_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // AK24_TCP_H
