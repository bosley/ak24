/**
 * @file tcp_client.h
 * @brief TCP client for connecting to TCP servers
 *
 * Provides a cross-platform TCP client abstraction for Windows and POSIX
 * systems. Supports connection lifecycle management, protocol layering via
 * lambda callbacks, automatic reconnection, and optional TLS encryption.
 */

#ifndef AK24_TCP_CLIENT_H
#define AK24_TCP_CLIENT_H

#include "tcp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Client connection state
 */
typedef enum {
  AK_TCP_CLIENT_DISCONNECTED = 0,
  AK_TCP_CLIENT_CONNECTING,
  AK_TCP_CLIENT_CONNECTED,
  AK_TCP_CLIENT_RECONNECTING,
} ak_tcp_client_state_t;

/**
 * @brief Convert client state to string
 */
const char *ak_tcp_client_state_string(ak_tcp_client_state_t state);

/**
 * @brief Connection event info passed to on_connect callback
 */
typedef struct {
  const char *local_ip;
  uint16_t local_port;
  const char *remote_ip;
  uint16_t remote_port;
  ak_tcp_error_t error;
  const char *error_msg;
  bool reconnect_attempt;
  size_t attempt_number;
} ak_tcp_client_conn_event_t;

/**
 * @brief Disconnect event info passed to on_disconnect callback
 */
typedef struct {
  ak_tcp_error_t reason;
  const char *reason_msg;
  bool will_reconnect;
} ak_tcp_client_disconnect_event_t;

/**
 * @brief Client configuration
 */
typedef struct {
  const char *host;
  uint16_t port;

  ak_lambda_t *on_connect;
  ak_lambda_t *on_disconnect;
  ak_lambda_t *on_data;

  uint32_t connect_timeout_ms;
  uint32_t recv_timeout_ms;
  uint32_t send_timeout_ms;

  bool auto_reconnect;
  uint32_t reconnect_delay_ms;
  uint32_t reconnect_max_delay_ms;
  size_t reconnect_max_attempts;
  float reconnect_backoff_mult;

  bool enable_keepalive;
  int keepalive_idle_sec;
  int keepalive_interval_sec;
  int keepalive_count;

  bool enable_nodelay;

  size_t recv_buffer_size;
  size_t send_buffer_size;
  size_t socket_recv_buffer;
  size_t socket_send_buffer;

#if AK24_TLS_ENABLED
  bool use_tls;
  const char *ca_file;
  const char *cert_file;
  const char *key_file;
  bool verify_server;
  const char *sni_hostname;
  const char *tls_ciphers;
  int tls_min_version;
#endif
} ak_tcp_client_config_t;

ak_tcp_client_config_t ak_tcp_client_config_default(void);

ak_tcp_client_t *ak_tcp_client_new(const ak_tcp_client_config_t *cfg,
                                   const char **error);

ak_tcp_error_t ak_tcp_client_connect(ak_tcp_client_t *client,
                                     const char **error);

void ak_tcp_client_disconnect(ak_tcp_client_t *client);

void ak_tcp_client_free(ak_tcp_client_t *client);

ak_tcp_client_state_t ak_tcp_client_state(ak_tcp_client_t *client);

bool ak_tcp_client_is_connected(ak_tcp_client_t *client);

ak_tcp_ctx_t *ak_tcp_client_ctx(ak_tcp_client_t *client);

ssize_t ak_tcp_client_send(ak_tcp_client_t *client, const ak_buffer_t *data,
                           const char **error);

ssize_t ak_tcp_client_recv(ak_tcp_client_t *client, ak_buffer_t *buffer,
                           size_t max_bytes, const char **error);

ak_buffer_t *ak_tcp_client_recv_until(ak_tcp_client_t *client,
                                      const char *delim, size_t max_bytes,
                                      const char **error);

ak_tcp_error_t ak_tcp_client_send_ex(ak_tcp_client_t *client,
                                     const ak_buffer_t *data,
                                     size_t *bytes_sent, const char **error);

ak_tcp_error_t ak_tcp_client_recv_ex(ak_tcp_client_t *client,
                                     ak_buffer_t *buffer, size_t max_bytes,
                                     size_t *bytes_received,
                                     const char **error);

ak_tcp_error_t ak_tcp_client_recv_until_ex(ak_tcp_client_t *client,
                                           const char *delim, size_t max_bytes,
                                           ak_buffer_t **result,
                                           const char **error);

ak_tcp_error_t ak_tcp_client_set_timeout(ak_tcp_client_t *client,
                                         uint32_t recv_timeout_ms,
                                         uint32_t send_timeout_ms);

ak_tcp_error_t ak_tcp_client_get_timeout(ak_tcp_client_t *client,
                                         uint32_t *recv_timeout_ms,
                                         uint32_t *send_timeout_ms);

void ak_tcp_client_set_meta(ak_tcp_client_t *client, const char *key,
                            void *value);

void *ak_tcp_client_get_meta(ak_tcp_client_t *client, const char *key);

bool ak_tcp_client_is_tls(ak_tcp_client_t *client);

typedef struct {
  size_t connection_attempts;
  size_t successful_connections;
  size_t total_bytes_sent;
  size_t total_bytes_received;
  size_t reconnect_count;
} ak_tcp_client_stats_t;

int ak_tcp_client_get_stats(ak_tcp_client_t *client,
                            ak_tcp_client_stats_t *stats);

void ak_tcp_client_set_auto_reconnect(ak_tcp_client_t *client, bool enable);

const char *ak_tcp_client_remote_host(ak_tcp_client_t *client);

uint16_t ak_tcp_client_remote_port(ak_tcp_client_t *client);

#ifdef __cplusplus
}
#endif

#endif // AK24_TCP_CLIENT_H
