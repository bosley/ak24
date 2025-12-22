/**
 * @file tcp_types.h
 * @brief Shared types for TCP server and client modules
 *
 * Contains forward declarations, error codes, and type definitions
 * shared between tcp.h and tcp_client.h. This header has no dependencies
 * on kernel.h to avoid circular includes.
 */

#ifndef AK24_TCP_TYPES_H
#define AK24_TCP_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET ak_socket_fd_t;
#define AK_INVALID_SOCKET INVALID_SOCKET
#else
typedef int ak_socket_fd_t;
#define AK_INVALID_SOCKET -1
#endif

typedef struct ak_tcp_ctx_s ak_tcp_ctx_t;
typedef struct ak_tcp_server_s ak_tcp_server_t;
typedef struct ak_tcp_client_s ak_tcp_client_t;
typedef struct ak_buffer_s ak_buffer_t;
typedef struct ak_lambda_t ak_lambda_t;

/**
 * @brief TCP error codes for structured error handling
 */
typedef enum {
  AK_TCP_OK = 0,            /**< Success */
  AK_TCP_ERR_TIMEOUT,       /**< Operation timed out */
  AK_TCP_ERR_CLOSED,        /**< Connection closed gracefully */
  AK_TCP_ERR_RESET,         /**< Connection reset by peer */
  AK_TCP_ERR_NETWORK,       /**< Network error */
  AK_TCP_ERR_MEMORY,        /**< Memory allocation failed */
  AK_TCP_ERR_INVALID,       /**< Invalid argument */
  AK_TCP_ERR_LIMIT,         /**< Limit reached (recv_until max_bytes, etc.) */
  AK_TCP_ERR_WOULDBLOCK,    /**< Operation would block (non-blocking mode) */
  AK_TCP_ERR_BUFFER_FULL,   /**< Receive buffer full (backpressure) */
  AK_TCP_ERR_QUEUE_FULL,    /**< Task queue full (server overloaded) */
  AK_TCP_ERR_RATE_LIMIT,    /**< Rate limit exceeded */
  AK_TCP_ERR_TLS_INIT,      /**< TLS initialization failed */
  AK_TCP_ERR_TLS_CERT,      /**< TLS certificate error */
  AK_TCP_ERR_TLS_HANDSHAKE, /**< TLS handshake failed */
} ak_tcp_error_t;

/**
 * @brief Convert error code to string
 *
 * @param err Error code
 * @return Human-readable error string
 */
const char *ak_tcp_error_string(ak_tcp_error_t err);

/**
 * @brief Check if TLS support is available
 *
 * @return true if compiled with TLS support, false otherwise
 */
bool ak_tcp_tls_available(void);

#ifdef __cplusplus
}
#endif

#endif // AK24_TCP_TYPES_H
