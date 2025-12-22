/**
 * @file tcp_internal.h
 * @brief Internal structures and platform abstraction for TCP module
 *
 * Contains internal data structures and platform-specific socket
 * function declarations. Not for public API use.
 */

#ifndef AK24_TCP_INTERNAL_H
#define AK24_TCP_INTERNAL_H

#include "kernel.h"
#include "tcp.h"
#include "threads.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef AK24_PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET ak_socket_fd_t;
#define AK_INVALID_SOCKET INVALID_SOCKET
#else
typedef int ak_socket_fd_t;
#define AK_INVALID_SOCKET -1
#endif

// ============================================================================
// Internal Data Structures
// ============================================================================

/**
 * @brief TCP connection context (full structure)
 */
struct ak_tcp_ctx_s {
  ak_socket_fd_t socket_fd;  /**< Platform socket handle */
  char remote_ip[46];        /**< IPv4 or IPv6 address string */
  uint16_t remote_port;      /**< Remote port number */
  ak_buffer_t *read_buffer;  /**< Incoming data buffer */
  ak_buffer_t *write_buffer; /**< Outgoing data buffer */
  bool alive;                /**< Connection active flag */
  AK24_MUTEX mutex;          /**< For thread-safe state access */
  ak_context_t *metadata;    /**< User metadata storage */

  // Timeout settings (milliseconds, 0 = no timeout)
  uint32_t recv_timeout_ms; /**< Receive timeout */
  uint32_t send_timeout_ms; /**< Send timeout */
};

/**
 * @brief IP tracker for rate limiting (Feature 3)
 */
typedef struct ak_tcp_ip_tracker_s {
  char ip[46];                /**< IP address string */
  size_t active_count;        /**< Active connections from this IP */
  time_t last_connect_time;   /**< Last connection timestamp */
  size_t connects_this_second;/**< Connections in current second */
  struct ak_tcp_ip_tracker_s *next; /**< Next in linked list */
} ak_tcp_ip_tracker_t;

/**
 * @brief Internal server state
 */
typedef struct ak_tcp_server_internal_s {
  ak_socket_fd_t listen_fd;      /**< Listening socket */
  ak_thread_pool_t *thread_pool; /**< Worker thread pool */
  AK24_THREAD accept_thread;     /**< Accept loop thread */
  bool running;                  /**< Server running flag */
  bool draining;                 /**< Graceful shutdown in progress */
  AK24_MUTEX mutex;              /**< For thread-safe state access */
  AK24_COND shutdown_cond;       /**< Shutdown signal */

  // Configuration (copied from config)
  char *bind_addr; /**< Bind address (owned copy) */
  uint16_t port;   /**< Port number */
  int backlog;     /**< Listen backlog */

  // Connection limits
  size_t max_connections;    /**< Max concurrent connections (0 = unlimited) */
  size_t active_connections; /**< Current active connection count */

  // Thread pool queue limits (Feature 2)
  size_t max_pending_tasks; /**< Max queued tasks (0 = unlimited) */

  // Backpressure handling (Feature 1)
  size_t max_recv_buffer_bytes; /**< Max bytes buffered per conn */

  // Rate limiting (Feature 3)
  size_t max_connections_per_ip; /**< Max concurrent from same IP */
  size_t connection_rate_limit;  /**< Max new conn/sec per IP */
  ak_tcp_ip_tracker_t *ip_trackers; /**< Linked list of IP trackers */
  AK24_MUTEX ip_tracker_mutex;   /**< Mutex for IP tracker access */

  // Timeout defaults
  uint32_t default_recv_timeout_ms; /**< Default receive timeout */
  uint32_t default_send_timeout_ms; /**< Default send timeout */

  // Graceful shutdown (Feature 7)
  uint32_t shutdown_drain_timeout_ms; /**< Drain timeout */

  // Linger control (Feature 5)
  bool enable_linger;     /**< Enable SO_LINGER */
  int linger_timeout_sec; /**< Linger timeout */

  // Keepalive defaults
  bool enable_keepalive;      /**< Enable keepalive on connections */
  int keepalive_idle_sec;     /**< Keepalive idle time */
  int keepalive_interval_sec; /**< Keepalive interval */
  int keepalive_count;        /**< Keepalive probe count */

  // Buffer sizes
  size_t recv_buffer_size; /**< Connection receive buffer size */
  size_t send_buffer_size; /**< Connection send buffer size */

  // Socket buffer tuning (Feature 9)
  size_t socket_recv_buffer; /**< SO_RCVBUF size */
  size_t socket_send_buffer; /**< SO_SNDBUF size */

  // Statistics
  size_t connections_accepted;           /**< Total accepted */
  size_t connections_rejected_limit;     /**< Rejected: conn limit */
  size_t connections_rejected_queue;     /**< Rejected: queue full */
  size_t connections_rejected_rate;      /**< Rejected: rate limit */
  size_t connections_rejected_ip_limit;  /**< Rejected: per-IP limit */
  size_t total_bytes_received;           /**< Total bytes received */
  size_t total_bytes_sent;               /**< Total bytes sent */

  // Lambdas (owned)
  ak_lambda_t *on_connect;    /**< Connection accept callback */
  ak_lambda_t *on_handle;     /**< Connection handler callback */
  ak_lambda_t *on_disconnect; /**< Disconnect callback */
} ak_tcp_server_internal_t;

/**
 * @brief TCP server handle
 */
struct ak_tcp_server_s {
  ak_tcp_server_internal_t *internal;
};

/**
 * @brief Worker task context (passed to thread pool)
 */
typedef struct ak_tcp_worker_task_s {
  ak_tcp_ctx_t *ctx;                /**< Connection context */
  ak_tcp_server_internal_t *server; /**< Server reference */
} ak_tcp_worker_task_t;

// ============================================================================
// Platform-Specific Functions
// ============================================================================

/**
 * @brief Platform-specific initialization
 *
 * POSIX: Sets up SIGPIPE handling
 * Windows: Calls WSAStartup
 */
void ak_tcp_platform_init(void);

/**
 * @brief Platform-specific cleanup
 *
 * POSIX: No-op
 * Windows: Calls WSACleanup
 */
void ak_tcp_platform_deinit(void);

/**
 * @brief Create a TCP socket
 *
 * @param error Output error message on failure
 * @return Socket descriptor or AK_INVALID_SOCKET on failure
 */
ak_socket_fd_t ak_tcp_socket_create(const char **error);

/**
 * @brief Create a TCP socket for a specific address family
 *
 * Detects IPv4/IPv6 based on address format and creates appropriate socket.
 * For IPv6, enables dual-stack mode (accepts both IPv4 and IPv6).
 *
 * @param addr Bind address (NULL for IPv4 any, "::" for IPv6 dual-stack)
 * @param error Output error message on failure
 * @return Socket descriptor or AK_INVALID_SOCKET on failure
 */
ak_socket_fd_t ak_tcp_socket_create_for_addr(const char *addr,
                                              const char **error);

/**
 * @brief Bind socket to address and port
 *
 * Supports both IPv4 and IPv6 addresses.
 *
 * @param fd Socket descriptor
 * @param addr Bind address (NULL/"0.0.0.0" for IPv4 any, "::" for IPv6 any)
 * @param port Port number
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_bind(ak_socket_fd_t fd, const char *addr, uint16_t port,
                       const char **error);

/**
 * @brief Set socket to listening mode
 *
 * @param fd Socket descriptor
 * @param backlog Connection backlog
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_listen(ak_socket_fd_t fd, int backlog, const char **error);

/**
 * @brief Accept incoming connection
 *
 * @param fd Listening socket
 * @param remote_ip Buffer for remote IP (at least 46 bytes)
 * @param remote_port Output for remote port
 * @param error Output error message on failure
 * @return Client socket or AK_INVALID_SOCKET on failure
 */
ak_socket_fd_t ak_tcp_socket_accept(ak_socket_fd_t fd, char *remote_ip,
                                    uint16_t *remote_port, const char **error);

/**
 * @brief Send data on socket
 *
 * @param fd Socket descriptor
 * @param data Data to send
 * @param len Data length
 * @param error Output error message on failure
 * @return Bytes sent or -1 on failure
 */
ssize_t ak_tcp_socket_send(ak_socket_fd_t fd, const uint8_t *data, size_t len,
                           const char **error);

/**
 * @brief Receive data from socket
 *
 * @param fd Socket descriptor
 * @param buffer Receive buffer
 * @param len Buffer length
 * @param error Output error message on failure
 * @return Bytes received, 0 on close, or -1 on failure
 */
ssize_t ak_tcp_socket_recv(ak_socket_fd_t fd, uint8_t *buffer, size_t len,
                           const char **error);

/**
 * @brief Close socket
 *
 * @param fd Socket descriptor
 */
void ak_tcp_socket_close(ak_socket_fd_t fd);

/**
 * @brief Set socket blocking mode
 *
 * @param fd Socket descriptor
 * @param nonblocking true for non-blocking, false for blocking
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_set_nonblocking(ak_socket_fd_t fd, bool nonblocking,
                                  const char **error);

/**
 * @brief Enable/disable address reuse
 *
 * @param fd Socket descriptor
 * @param enable true to enable, false to disable
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_set_reuseaddr(ak_socket_fd_t fd, bool enable,
                                const char **error);

/**
 * @brief Enable/disable TCP_NODELAY (disable Nagle's algorithm)
 *
 * @param fd Socket descriptor
 * @param enable true to enable, false to disable
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_set_nodelay(ak_socket_fd_t fd, bool enable,
                              const char **error);

/**
 * @brief Set socket receive and send timeouts
 *
 * @param fd Socket descriptor
 * @param recv_timeout_ms Receive timeout in milliseconds (0 = no timeout)
 * @param send_timeout_ms Send timeout in milliseconds (0 = no timeout)
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_set_timeout(ak_socket_fd_t fd, uint32_t recv_timeout_ms,
                              uint32_t send_timeout_ms, const char **error);

/**
 * @brief Get socket receive and send timeouts
 *
 * @param fd Socket descriptor
 * @param recv_timeout_ms Output for receive timeout in ms
 * @param send_timeout_ms Output for send timeout in ms
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_get_timeout(ak_socket_fd_t fd, uint32_t *recv_timeout_ms,
                              uint32_t *send_timeout_ms, const char **error);

/**
 * @brief Configure TCP keepalive on socket
 *
 * @param fd Socket descriptor
 * @param idle_sec Seconds before first probe (0 = disable keepalive)
 * @param interval_sec Seconds between probes
 * @param probe_count Number of probes before declaring dead
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_set_keepalive(ak_socket_fd_t fd, int idle_sec,
                                int interval_sec, int probe_count,
                                const char **error);

/**
 * @brief Set SO_LINGER option on socket
 *
 * @param fd Socket descriptor
 * @param enable Enable linger
 * @param timeout_sec Linger timeout (0 = hard close)
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_set_linger(ak_socket_fd_t fd, bool enable, int timeout_sec,
                             const char **error);

/**
 * @brief Set socket buffer sizes
 *
 * @param fd Socket descriptor
 * @param recv_size SO_RCVBUF size (0 = don't change)
 * @param send_size SO_SNDBUF size (0 = don't change)
 * @param error Output error message on failure
 * @return 0 on success, -1 on failure
 */
int ak_tcp_socket_set_buffers(ak_socket_fd_t fd, size_t recv_size,
                              size_t send_size, const char **error);

/**
 * @brief Poll socket for read readiness with timeout
 *
 * @param fd Socket descriptor
 * @param timeout_ms Timeout in milliseconds
 * @return 1 = ready, 0 = timeout, -1 = error
 */
int ak_tcp_socket_poll_read(ak_socket_fd_t fd, int timeout_ms);

/**
 * @brief Graceful shutdown (half-close write side)
 *
 * @param fd Socket descriptor
 */
void ak_tcp_socket_shutdown(ak_socket_fd_t fd);

/**
 * @brief Check if socket peer has closed the connection
 *
 * Uses non-blocking peek to detect if the peer has closed.
 *
 * @param fd Socket descriptor
 * @return true if peer closed or error, false if still connected
 */
bool ak_tcp_socket_peer_closed(ak_socket_fd_t fd);

/**
 * @brief Map platform errno to ak_tcp_error_t
 *
 * @param platform_errno Platform-specific error code
 * @return Mapped error code
 */
ak_tcp_error_t ak_tcp_map_error(int platform_errno);

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Create a new connection context
 *
 * @param socket_fd Client socket
 * @param remote_ip Remote IP address
 * @param remote_port Remote port
 * @param recv_buf_size Receive buffer size (0 = default 4096)
 * @param send_buf_size Send buffer size (0 = default 4096)
 * @return New context or NULL on failure
 */
ak_tcp_ctx_t *ak_tcp_ctx_new(ak_socket_fd_t socket_fd, const char *remote_ip,
                             uint16_t remote_port, size_t recv_buf_size,
                             size_t send_buf_size);

/**
 * @brief Free connection context
 *
 * @param ctx Context to free
 */
void ak_tcp_ctx_free(ak_tcp_ctx_t *ctx);

#endif // AK24_TCP_INTERNAL_H
