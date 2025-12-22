
#include "tcp_client.h"
#include "kernel.h"
#include "tcp_internal.h"
#include "threads.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TCP_CLIENT_DEFAULT_RECV_BUF 4096
#define TCP_CLIENT_DEFAULT_SEND_BUF 4096
#define TCP_CLIENT_DEFAULT_RECONNECT_DELAY 1000
#define TCP_CLIENT_DEFAULT_RECONNECT_MAX_DELAY 30000
#define TCP_CLIENT_DEFAULT_BACKOFF_MULT 2.0f
#define TCP_CLIENT_DEFAULT_KEEPALIVE_IDLE 60
#define TCP_CLIENT_DEFAULT_KEEPALIVE_INTERVAL 10
#define TCP_CLIENT_DEFAULT_KEEPALIVE_COUNT 5

typedef struct ak_tcp_client_internal_s {
  char *host;
  uint16_t port;

  ak_tcp_ctx_t *ctx;
  ak_tcp_client_state_t state;
  AK24_MUTEX mutex;

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
  uint32_t current_reconnect_delay;
  size_t current_attempt;

  bool enable_keepalive;
  int keepalive_idle_sec;
  int keepalive_interval_sec;
  int keepalive_count;

  bool enable_nodelay;

  size_t recv_buffer_size;
  size_t send_buffer_size;
  size_t socket_recv_buffer;
  size_t socket_send_buffer;

  size_t connection_attempts;
  size_t successful_connections;
  size_t total_bytes_sent;
  size_t total_bytes_received;
  size_t reconnect_count;

  AK24_THREAD reconnect_thread;
  bool reconnect_thread_running;
  AK24_COND reconnect_cond;
  bool stop_reconnect;

#if AK24_TLS_ENABLED
  bool use_tls;
  char *ca_file;
  char *cert_file;
  char *key_file;
  bool verify_server;
  char *sni_hostname;
  char *tls_ciphers;
  int tls_min_version;
  SSL_CTX *ssl_ctx;
#endif
} ak_tcp_client_internal_t;

struct ak_tcp_client_s {
  ak_tcp_client_internal_t *internal;
};

const char *ak_tcp_client_state_string(ak_tcp_client_state_t state) {
  switch (state) {
  case AK_TCP_CLIENT_DISCONNECTED:
    return "Disconnected";
  case AK_TCP_CLIENT_CONNECTING:
    return "Connecting";
  case AK_TCP_CLIENT_CONNECTED:
    return "Connected";
  case AK_TCP_CLIENT_RECONNECTING:
    return "Reconnecting";
  default:
    return "Unknown";
  }
}

ak_tcp_client_config_t ak_tcp_client_config_default(void) {
  ak_tcp_client_config_t cfg = {
      .host = NULL,
      .port = 0,
      .on_connect = NULL,
      .on_disconnect = NULL,
      .on_data = NULL,
      .connect_timeout_ms = 0,
      .recv_timeout_ms = 0,
      .send_timeout_ms = 0,
      .auto_reconnect = false,
      .reconnect_delay_ms = TCP_CLIENT_DEFAULT_RECONNECT_DELAY,
      .reconnect_max_delay_ms = TCP_CLIENT_DEFAULT_RECONNECT_MAX_DELAY,
      .reconnect_max_attempts = 0,
      .reconnect_backoff_mult = TCP_CLIENT_DEFAULT_BACKOFF_MULT,
      .enable_keepalive = false,
      .keepalive_idle_sec = TCP_CLIENT_DEFAULT_KEEPALIVE_IDLE,
      .keepalive_interval_sec = TCP_CLIENT_DEFAULT_KEEPALIVE_INTERVAL,
      .keepalive_count = TCP_CLIENT_DEFAULT_KEEPALIVE_COUNT,
      .enable_nodelay = false,
      .recv_buffer_size = TCP_CLIENT_DEFAULT_RECV_BUF,
      .send_buffer_size = TCP_CLIENT_DEFAULT_SEND_BUF,
      .socket_recv_buffer = 0,
      .socket_send_buffer = 0,
  };
  return cfg;
}

static char *client_strdup(const char *s) {
  if (!s) {
    return NULL;
  }
  size_t len = strlen(s) + 1;
  char *copy = AK24_ALLOC(len);
  if (copy) {
    memcpy(copy, s, len);
  }
  return copy;
}

static void client_internal_free(ak_tcp_client_internal_t *internal) {
  if (!internal) {
    return;
  }

  if (internal->ctx) {
    ak_tcp_ctx_free(internal->ctx);
  }

  if (internal->host) {
    AK24_FREE(internal->host);
  }

  if (internal->on_connect) {
    ak_lambda_free(internal->on_connect);
  }
  if (internal->on_disconnect) {
    ak_lambda_free(internal->on_disconnect);
  }
  if (internal->on_data) {
    ak_lambda_free(internal->on_data);
  }

#if AK24_TLS_ENABLED
  if (internal->ssl_ctx) {
    ak_tcp_tls_ctx_free(internal->ssl_ctx);
  }
  if (internal->ca_file) {
    AK24_FREE(internal->ca_file);
  }
  if (internal->cert_file) {
    AK24_FREE(internal->cert_file);
  }
  if (internal->key_file) {
    AK24_FREE(internal->key_file);
  }
  if (internal->sni_hostname) {
    AK24_FREE(internal->sni_hostname);
  }
  if (internal->tls_ciphers) {
    AK24_FREE(internal->tls_ciphers);
  }
#endif

  AK24_MUTEX_DESTROY(&internal->mutex);
  AK24_COND_DESTROY(&internal->reconnect_cond);
  AK24_FREE(internal);
}

static ak_tcp_error_t client_do_connect(ak_tcp_client_internal_t *internal,
                                        const char **error) {
  ak_socket_fd_t fd = ak_tcp_socket_create(error);
  if (fd == AK_INVALID_SOCKET) {
    return AK_TCP_ERR_NETWORK;
  }

  if (ak_tcp_socket_set_reuseaddr(fd, true, error) != 0) {
    AK24_LOG_WARN("Failed to set SO_REUSEADDR: %s", error ? *error : "Unknown");
  }

  if (internal->enable_nodelay) {
    if (ak_tcp_socket_set_nodelay(fd, true, error) != 0) {
      AK24_LOG_WARN("Failed to set TCP_NODELAY: %s",
                    error ? *error : "Unknown");
    }
  }

  if (internal->socket_recv_buffer > 0 || internal->socket_send_buffer > 0) {
    if (ak_tcp_socket_set_buffers(fd, internal->socket_recv_buffer,
                                  internal->socket_send_buffer, error) != 0) {
      AK24_LOG_WARN("Failed to set socket buffers: %s",
                    error ? *error : "Unknown");
    }
  }

  if (ak_tcp_socket_connect(fd, internal->host, internal->port,
                            internal->connect_timeout_ms, error) != 0) {
    ak_tcp_socket_close(fd);
    return AK_TCP_ERR_NETWORK;
  }

  if (internal->recv_timeout_ms > 0 || internal->send_timeout_ms > 0) {
    if (ak_tcp_socket_set_timeout(fd, internal->recv_timeout_ms,
                                  internal->send_timeout_ms, error) != 0) {
      AK24_LOG_WARN("Failed to set timeout: %s", error ? *error : "Unknown");
    }
  }

  if (internal->enable_keepalive) {
    if (ak_tcp_socket_set_keepalive(fd, internal->keepalive_idle_sec,
                                    internal->keepalive_interval_sec,
                                    internal->keepalive_count, error) != 0) {
      AK24_LOG_WARN("Failed to set keepalive: %s", error ? *error : "Unknown");
    }
  }

  ak_tcp_ctx_t *ctx =
      ak_tcp_ctx_new(fd, internal->host, internal->port,
                     internal->recv_buffer_size, internal->send_buffer_size);
  if (!ctx) {
    ak_tcp_socket_close(fd);
    if (error) {
      *error = "Failed to create connection context";
    }
    return AK_TCP_ERR_MEMORY;
  }

  ctx->recv_timeout_ms = internal->recv_timeout_ms;
  ctx->send_timeout_ms = internal->send_timeout_ms;

#if AK24_TLS_ENABLED
  if (internal->use_tls) {
    const char *sni =
        internal->sni_hostname ? internal->sni_hostname : internal->host;
    ctx->ssl = ak_tcp_tls_connect(internal->ssl_ctx, fd, sni, error);
    if (!ctx->ssl) {
      ak_tcp_ctx_free(ctx);
      return AK_TCP_ERR_TLS_HANDSHAKE;
    }
  }
#endif

  AK24_MUTEX_LOCK(&internal->mutex);
  if (internal->ctx) {
    ak_tcp_ctx_free(internal->ctx);
  }
  internal->ctx = ctx;
  internal->state = AK_TCP_CLIENT_CONNECTED;
  internal->successful_connections++;
  internal->current_reconnect_delay = internal->reconnect_delay_ms;
  internal->current_attempt = 0;
  AK24_MUTEX_UNLOCK(&internal->mutex);

  return AK_TCP_OK;
}

static void *reconnect_loop(void *arg) {
  ak_tcp_client_internal_t *internal = (ak_tcp_client_internal_t *)arg;

  while (true) {
    AK24_MUTEX_LOCK(&internal->mutex);
    if (internal->stop_reconnect) {
      internal->reconnect_thread_running = false;
      AK24_MUTEX_UNLOCK(&internal->mutex);
      break;
    }

    if (internal->state == AK_TCP_CLIENT_CONNECTED) {
      internal->reconnect_thread_running = false;
      AK24_MUTEX_UNLOCK(&internal->mutex);
      break;
    }

    if (internal->reconnect_max_attempts > 0 &&
        internal->current_attempt >= internal->reconnect_max_attempts) {
      internal->state = AK_TCP_CLIENT_DISCONNECTED;
      internal->reconnect_thread_running = false;
      AK24_MUTEX_UNLOCK(&internal->mutex);

      if (internal->on_disconnect) {
        ak_tcp_client_disconnect_event_t event = {
            .reason = AK_TCP_ERR_LIMIT,
            .reason_msg = "Max reconnection attempts reached",
            .will_reconnect = false,
        };
        ak_lambda_invoke(internal->on_disconnect, &event);
      }
      break;
    }

    uint32_t delay = internal->current_reconnect_delay;
    AK24_MUTEX_UNLOCK(&internal->mutex);

    int wait_result = 0;
    AK24_MUTEX_LOCK(&internal->mutex);
    if (!internal->stop_reconnect) {
      wait_result = AK24_COND_TIMEDWAIT(&internal->reconnect_cond,
                                        &internal->mutex, delay);
    }
    if (internal->stop_reconnect) {
      internal->reconnect_thread_running = false;
      AK24_MUTEX_UNLOCK(&internal->mutex);
      break;
    }
    AK24_MUTEX_UNLOCK(&internal->mutex);

    (void)wait_result;

    AK24_MUTEX_LOCK(&internal->mutex);
    internal->state = AK_TCP_CLIENT_RECONNECTING;
    internal->current_attempt++;
    internal->connection_attempts++;
    internal->reconnect_count++;
    size_t attempt = internal->current_attempt;
    AK24_MUTEX_UNLOCK(&internal->mutex);

    const char *error = NULL;
    ak_tcp_error_t result = client_do_connect(internal, &error);

    if (result == AK_TCP_OK) {
      if (internal->on_connect) {
        ak_tcp_client_conn_event_t event = {
            .local_ip = "",
            .local_port = 0,
            .remote_ip = internal->host,
            .remote_port = internal->port,
            .error = AK_TCP_OK,
            .error_msg = NULL,
            .reconnect_attempt = true,
            .attempt_number = attempt,
        };
        ak_lambda_invoke(internal->on_connect, &event);
      }
      AK24_MUTEX_LOCK(&internal->mutex);
      internal->reconnect_thread_running = false;
      AK24_MUTEX_UNLOCK(&internal->mutex);
      break;
    }

    if (internal->on_connect) {
      ak_tcp_client_conn_event_t event = {
          .local_ip = "",
          .local_port = 0,
          .remote_ip = internal->host,
          .remote_port = internal->port,
          .error = result,
          .error_msg = error,
          .reconnect_attempt = true,
          .attempt_number = attempt,
      };
      ak_lambda_invoke(internal->on_connect, &event);
    }

    AK24_MUTEX_LOCK(&internal->mutex);
    internal->current_reconnect_delay =
        (uint32_t)(internal->current_reconnect_delay *
                   internal->reconnect_backoff_mult);
    if (internal->current_reconnect_delay > internal->reconnect_max_delay_ms) {
      internal->current_reconnect_delay = internal->reconnect_max_delay_ms;
    }
    AK24_MUTEX_UNLOCK(&internal->mutex);
  }

  return NULL;
}

static void start_reconnect_thread(ak_tcp_client_internal_t *internal) {
  AK24_MUTEX_LOCK(&internal->mutex);
  if (internal->reconnect_thread_running) {
    AK24_MUTEX_UNLOCK(&internal->mutex);
    return;
  }
  internal->reconnect_thread_running = true;
  internal->stop_reconnect = false;
  internal->state = AK_TCP_CLIENT_RECONNECTING;
  AK24_MUTEX_UNLOCK(&internal->mutex);

  AK24_THREAD_CREATE(&internal->reconnect_thread, reconnect_loop, internal);
}

static void stop_reconnect_thread(ak_tcp_client_internal_t *internal) {
  AK24_MUTEX_LOCK(&internal->mutex);
  if (!internal->reconnect_thread_running) {
    AK24_MUTEX_UNLOCK(&internal->mutex);
    return;
  }
  internal->stop_reconnect = true;
  AK24_COND_SIGNAL(&internal->reconnect_cond);
  AK24_MUTEX_UNLOCK(&internal->mutex);

  AK24_THREAD_JOIN(internal->reconnect_thread);
}

ak_tcp_client_t *ak_tcp_client_new(const ak_tcp_client_config_t *cfg,
                                   const char **error) {
  if (!cfg) {
    if (error) {
      *error = "Configuration is required";
    }
    return NULL;
  }

  if (!cfg->host || cfg->host[0] == '\0') {
    if (error) {
      *error = "Host is required";
    }
    return NULL;
  }

  if (cfg->port == 0) {
    if (error) {
      *error = "Port must be non-zero";
    }
    return NULL;
  }

  ak_tcp_client_t *client = AK24_ALLOC(sizeof(ak_tcp_client_t));
  if (!client) {
    if (error) {
      *error = "Memory allocation failed";
    }
    return NULL;
  }

  ak_tcp_client_internal_t *internal =
      AK24_ALLOC(sizeof(ak_tcp_client_internal_t));
  if (!internal) {
    AK24_FREE(client);
    if (error) {
      *error = "Memory allocation failed";
    }
    return NULL;
  }

  memset(internal, 0, sizeof(ak_tcp_client_internal_t));
  client->internal = internal;

  internal->host = client_strdup(cfg->host);
  if (!internal->host) {
    AK24_FREE(internal);
    AK24_FREE(client);
    if (error) {
      *error = "Memory allocation failed";
    }
    return NULL;
  }

  internal->port = cfg->port;
  internal->state = AK_TCP_CLIENT_DISCONNECTED;

  internal->on_connect = cfg->on_connect;
  internal->on_disconnect = cfg->on_disconnect;
  internal->on_data = cfg->on_data;

  internal->connect_timeout_ms = cfg->connect_timeout_ms;
  internal->recv_timeout_ms = cfg->recv_timeout_ms;
  internal->send_timeout_ms = cfg->send_timeout_ms;

  internal->auto_reconnect = cfg->auto_reconnect;
  internal->reconnect_delay_ms = cfg->reconnect_delay_ms > 0
                                     ? cfg->reconnect_delay_ms
                                     : TCP_CLIENT_DEFAULT_RECONNECT_DELAY;
  internal->reconnect_max_delay_ms =
      cfg->reconnect_max_delay_ms > 0 ? cfg->reconnect_max_delay_ms
                                      : TCP_CLIENT_DEFAULT_RECONNECT_MAX_DELAY;
  internal->reconnect_max_attempts = cfg->reconnect_max_attempts;
  internal->reconnect_backoff_mult = cfg->reconnect_backoff_mult > 0.0f
                                         ? cfg->reconnect_backoff_mult
                                         : TCP_CLIENT_DEFAULT_BACKOFF_MULT;
  internal->current_reconnect_delay = internal->reconnect_delay_ms;
  internal->current_attempt = 0;

  internal->enable_keepalive = cfg->enable_keepalive;
  internal->keepalive_idle_sec = cfg->keepalive_idle_sec > 0
                                     ? cfg->keepalive_idle_sec
                                     : TCP_CLIENT_DEFAULT_KEEPALIVE_IDLE;
  internal->keepalive_interval_sec =
      cfg->keepalive_interval_sec > 0 ? cfg->keepalive_interval_sec
                                      : TCP_CLIENT_DEFAULT_KEEPALIVE_INTERVAL;
  internal->keepalive_count = cfg->keepalive_count > 0
                                  ? cfg->keepalive_count
                                  : TCP_CLIENT_DEFAULT_KEEPALIVE_COUNT;

  internal->enable_nodelay = cfg->enable_nodelay;

  internal->recv_buffer_size = cfg->recv_buffer_size > 0
                                   ? cfg->recv_buffer_size
                                   : TCP_CLIENT_DEFAULT_RECV_BUF;
  internal->send_buffer_size = cfg->send_buffer_size > 0
                                   ? cfg->send_buffer_size
                                   : TCP_CLIENT_DEFAULT_SEND_BUF;

  internal->socket_recv_buffer = cfg->socket_recv_buffer;
  internal->socket_send_buffer = cfg->socket_send_buffer;

  AK24_MUTEX_INIT(&internal->mutex);
  AK24_COND_INIT(&internal->reconnect_cond);

#if AK24_TLS_ENABLED
  if (cfg->use_tls) {
    internal->use_tls = true;
    internal->verify_server = cfg->verify_server;
    internal->tls_min_version = cfg->tls_min_version;

    if (cfg->ca_file) {
      internal->ca_file = client_strdup(cfg->ca_file);
    }
    if (cfg->cert_file) {
      internal->cert_file = client_strdup(cfg->cert_file);
    }
    if (cfg->key_file) {
      internal->key_file = client_strdup(cfg->key_file);
    }
    if (cfg->sni_hostname) {
      internal->sni_hostname = client_strdup(cfg->sni_hostname);
    }
    if (cfg->tls_ciphers) {
      internal->tls_ciphers = client_strdup(cfg->tls_ciphers);
    }

    internal->ssl_ctx = ak_tcp_tls_client_ctx_new(
        internal->ca_file, internal->cert_file, internal->key_file,
        internal->verify_server, internal->tls_ciphers,
        internal->tls_min_version, error);
    if (!internal->ssl_ctx) {
      client_internal_free(internal);
      AK24_FREE(client);
      return NULL;
    }
  }
#endif

  return client;
}

ak_tcp_error_t ak_tcp_client_connect(ak_tcp_client_t *client,
                                     const char **error) {
  if (!client || !client->internal) {
    if (error) {
      *error = "Invalid client";
    }
    return AK_TCP_ERR_INVALID;
  }

  ak_tcp_client_internal_t *internal = client->internal;

  AK24_MUTEX_LOCK(&internal->mutex);
  if (internal->state == AK_TCP_CLIENT_CONNECTED) {
    AK24_MUTEX_UNLOCK(&internal->mutex);
    return AK_TCP_OK;
  }
  if (internal->state == AK_TCP_CLIENT_CONNECTING ||
      internal->state == AK_TCP_CLIENT_RECONNECTING) {
    AK24_MUTEX_UNLOCK(&internal->mutex);
    if (error) {
      *error = "Connection already in progress";
    }
    return AK_TCP_ERR_INVALID;
  }
  internal->state = AK_TCP_CLIENT_CONNECTING;
  internal->connection_attempts++;
  internal->current_attempt = 1;
  AK24_MUTEX_UNLOCK(&internal->mutex);

  ak_tcp_error_t result = client_do_connect(internal, error);

  if (result == AK_TCP_OK) {
    if (internal->on_connect) {
      ak_tcp_client_conn_event_t event = {
          .local_ip = "",
          .local_port = 0,
          .remote_ip = internal->host,
          .remote_port = internal->port,
          .error = AK_TCP_OK,
          .error_msg = NULL,
          .reconnect_attempt = false,
          .attempt_number = 1,
      };
      ak_lambda_invoke(internal->on_connect, &event);
    }
    return AK_TCP_OK;
  }

  if (internal->on_connect) {
    ak_tcp_client_conn_event_t event = {
        .local_ip = "",
        .local_port = 0,
        .remote_ip = internal->host,
        .remote_port = internal->port,
        .error = result,
        .error_msg = error ? *error : NULL,
        .reconnect_attempt = false,
        .attempt_number = 1,
    };
    ak_lambda_invoke(internal->on_connect, &event);
  }

  if (internal->auto_reconnect) {
    start_reconnect_thread(internal);
    return result;
  }

  AK24_MUTEX_LOCK(&internal->mutex);
  internal->state = AK_TCP_CLIENT_DISCONNECTED;
  AK24_MUTEX_UNLOCK(&internal->mutex);

  return result;
}

void ak_tcp_client_disconnect(ak_tcp_client_t *client) {
  if (!client || !client->internal) {
    return;
  }

  ak_tcp_client_internal_t *internal = client->internal;

  stop_reconnect_thread(internal);

  AK24_MUTEX_LOCK(&internal->mutex);
  internal->auto_reconnect = false;

  ak_tcp_client_state_t prev_state = internal->state;
  if (internal->ctx) {
    ak_tcp_close(internal->ctx);
    ak_tcp_ctx_free(internal->ctx);
    internal->ctx = NULL;
  }
  internal->state = AK_TCP_CLIENT_DISCONNECTED;
  AK24_MUTEX_UNLOCK(&internal->mutex);

  if (prev_state == AK_TCP_CLIENT_CONNECTED && internal->on_disconnect) {
    ak_tcp_client_disconnect_event_t event = {
        .reason = AK_TCP_OK,
        .reason_msg = "Disconnected by user",
        .will_reconnect = false,
    };
    ak_lambda_invoke(internal->on_disconnect, &event);
  }
}

void ak_tcp_client_free(ak_tcp_client_t *client) {
  if (!client) {
    return;
  }

  ak_tcp_client_disconnect(client);
  client_internal_free(client->internal);
  AK24_FREE(client);
}

ak_tcp_client_state_t ak_tcp_client_state(ak_tcp_client_t *client) {
  if (!client || !client->internal) {
    return AK_TCP_CLIENT_DISCONNECTED;
  }

  AK24_MUTEX_LOCK(&client->internal->mutex);
  ak_tcp_client_state_t state = client->internal->state;
  AK24_MUTEX_UNLOCK(&client->internal->mutex);

  return state;
}

bool ak_tcp_client_is_connected(ak_tcp_client_t *client) {
  if (!client || !client->internal) {
    return false;
  }

  AK24_MUTEX_LOCK(&client->internal->mutex);
  bool connected =
      (client->internal->state == AK_TCP_CLIENT_CONNECTED &&
       client->internal->ctx != NULL && ak_tcp_is_alive(client->internal->ctx));
  AK24_MUTEX_UNLOCK(&client->internal->mutex);

  return connected;
}

ak_tcp_ctx_t *ak_tcp_client_ctx(ak_tcp_client_t *client) {
  if (!client || !client->internal) {
    return NULL;
  }

  AK24_MUTEX_LOCK(&client->internal->mutex);
  ak_tcp_ctx_t *ctx = client->internal->ctx;
  AK24_MUTEX_UNLOCK(&client->internal->mutex);

  return ctx;
}

ssize_t ak_tcp_client_send(ak_tcp_client_t *client, const ak_buffer_t *data,
                           const char **error) {
  ak_tcp_ctx_t *ctx = ak_tcp_client_ctx(client);
  if (!ctx) {
    if (error) {
      *error = "Not connected";
    }
    return -1;
  }

  ssize_t result = ak_tcp_send(ctx, data, error);
  if (result > 0 && client->internal) {
    AK24_MUTEX_LOCK(&client->internal->mutex);
    client->internal->total_bytes_sent += (size_t)result;
    AK24_MUTEX_UNLOCK(&client->internal->mutex);
  }
  return result;
}

ssize_t ak_tcp_client_recv(ak_tcp_client_t *client, ak_buffer_t *buffer,
                           size_t max_bytes, const char **error) {
  ak_tcp_ctx_t *ctx = ak_tcp_client_ctx(client);
  if (!ctx) {
    if (error) {
      *error = "Not connected";
    }
    return -1;
  }

  ssize_t result = ak_tcp_recv(ctx, buffer, max_bytes, error);
  if (result > 0 && client->internal) {
    AK24_MUTEX_LOCK(&client->internal->mutex);
    client->internal->total_bytes_received += (size_t)result;
    AK24_MUTEX_UNLOCK(&client->internal->mutex);
  }

  if (result == 0 || (result < 0 && !ak_tcp_is_alive(ctx))) {
    if (client->internal->auto_reconnect) {
      if (client->internal->on_disconnect) {
        ak_tcp_client_disconnect_event_t event = {
            .reason = result == 0 ? AK_TCP_ERR_CLOSED : AK_TCP_ERR_NETWORK,
            .reason_msg =
                result == 0 ? "Connection closed by peer" : "Network error",
            .will_reconnect = true,
        };
        ak_lambda_invoke(client->internal->on_disconnect, &event);
      }
      start_reconnect_thread(client->internal);
    }
  }

  return result;
}

ak_buffer_t *ak_tcp_client_recv_until(ak_tcp_client_t *client,
                                      const char *delim, size_t max_bytes,
                                      const char **error) {
  ak_tcp_ctx_t *ctx = ak_tcp_client_ctx(client);
  if (!ctx) {
    if (error) {
      *error = "Not connected";
    }
    return NULL;
  }

  ak_buffer_t *result = ak_tcp_recv_until(ctx, delim, max_bytes, error);
  if (result && client->internal) {
    AK24_MUTEX_LOCK(&client->internal->mutex);
    client->internal->total_bytes_received += ak_buffer_count(result);
    AK24_MUTEX_UNLOCK(&client->internal->mutex);
  }
  return result;
}

ak_tcp_error_t ak_tcp_client_send_ex(ak_tcp_client_t *client,
                                     const ak_buffer_t *data,
                                     size_t *bytes_sent, const char **error) {
  ak_tcp_ctx_t *ctx = ak_tcp_client_ctx(client);
  if (!ctx) {
    if (error) {
      *error = "Not connected";
    }
    return AK_TCP_ERR_CLOSED;
  }

  ak_tcp_error_t result = ak_tcp_send_ex(ctx, data, bytes_sent, error);
  if (result == AK_TCP_OK && bytes_sent && *bytes_sent > 0 &&
      client->internal) {
    AK24_MUTEX_LOCK(&client->internal->mutex);
    client->internal->total_bytes_sent += *bytes_sent;
    AK24_MUTEX_UNLOCK(&client->internal->mutex);
  }
  return result;
}

ak_tcp_error_t ak_tcp_client_recv_ex(ak_tcp_client_t *client,
                                     ak_buffer_t *buffer, size_t max_bytes,
                                     size_t *bytes_received,
                                     const char **error) {
  ak_tcp_ctx_t *ctx = ak_tcp_client_ctx(client);
  if (!ctx) {
    if (error) {
      *error = "Not connected";
    }
    return AK_TCP_ERR_CLOSED;
  }

  ak_tcp_error_t result =
      ak_tcp_recv_ex(ctx, buffer, max_bytes, bytes_received, error);
  if (result == AK_TCP_OK && bytes_received && *bytes_received > 0 &&
      client->internal) {
    AK24_MUTEX_LOCK(&client->internal->mutex);
    client->internal->total_bytes_received += *bytes_received;
    AK24_MUTEX_UNLOCK(&client->internal->mutex);
  }
  return result;
}

ak_tcp_error_t ak_tcp_client_recv_until_ex(ak_tcp_client_t *client,
                                           const char *delim, size_t max_bytes,
                                           ak_buffer_t **result,
                                           const char **error) {
  ak_tcp_ctx_t *ctx = ak_tcp_client_ctx(client);
  if (!ctx) {
    if (error) {
      *error = "Not connected";
    }
    return AK_TCP_ERR_CLOSED;
  }

  ak_tcp_error_t err =
      ak_tcp_recv_until_ex(ctx, delim, max_bytes, result, error);
  if (err == AK_TCP_OK && result && *result && client->internal) {
    AK24_MUTEX_LOCK(&client->internal->mutex);
    client->internal->total_bytes_received += ak_buffer_count(*result);
    AK24_MUTEX_UNLOCK(&client->internal->mutex);
  }
  return err;
}

ak_tcp_error_t ak_tcp_client_set_timeout(ak_tcp_client_t *client,
                                         uint32_t recv_timeout_ms,
                                         uint32_t send_timeout_ms) {
  if (!client || !client->internal) {
    return AK_TCP_ERR_INVALID;
  }

  AK24_MUTEX_LOCK(&client->internal->mutex);
  client->internal->recv_timeout_ms = recv_timeout_ms;
  client->internal->send_timeout_ms = send_timeout_ms;

  ak_tcp_ctx_t *ctx = client->internal->ctx;
  AK24_MUTEX_UNLOCK(&client->internal->mutex);

  if (ctx) {
    return ak_tcp_set_timeout(ctx, recv_timeout_ms, send_timeout_ms);
  }

  return AK_TCP_OK;
}

ak_tcp_error_t ak_tcp_client_get_timeout(ak_tcp_client_t *client,
                                         uint32_t *recv_timeout_ms,
                                         uint32_t *send_timeout_ms) {
  if (!client || !client->internal) {
    return AK_TCP_ERR_INVALID;
  }

  AK24_MUTEX_LOCK(&client->internal->mutex);
  if (recv_timeout_ms) {
    *recv_timeout_ms = client->internal->recv_timeout_ms;
  }
  if (send_timeout_ms) {
    *send_timeout_ms = client->internal->send_timeout_ms;
  }
  AK24_MUTEX_UNLOCK(&client->internal->mutex);

  return AK_TCP_OK;
}

void ak_tcp_client_set_meta(ak_tcp_client_t *client, const char *key,
                            void *value) {
  ak_tcp_ctx_t *ctx = ak_tcp_client_ctx(client);
  if (ctx) {
    ak_tcp_set_meta(ctx, key, value);
  }
}

void *ak_tcp_client_get_meta(ak_tcp_client_t *client, const char *key) {
  ak_tcp_ctx_t *ctx = ak_tcp_client_ctx(client);
  if (ctx) {
    return ak_tcp_get_meta(ctx, key);
  }
  return NULL;
}

bool ak_tcp_client_is_tls(ak_tcp_client_t *client) {
  ak_tcp_ctx_t *ctx = ak_tcp_client_ctx(client);
  if (ctx) {
    return ak_tcp_is_tls(ctx);
  }
  return false;
}

int ak_tcp_client_get_stats(ak_tcp_client_t *client,
                            ak_tcp_client_stats_t *stats) {
  if (!client || !client->internal || !stats) {
    return -1;
  }

  AK24_MUTEX_LOCK(&client->internal->mutex);
  stats->connection_attempts = client->internal->connection_attempts;
  stats->successful_connections = client->internal->successful_connections;
  stats->total_bytes_sent = client->internal->total_bytes_sent;
  stats->total_bytes_received = client->internal->total_bytes_received;
  stats->reconnect_count = client->internal->reconnect_count;
  AK24_MUTEX_UNLOCK(&client->internal->mutex);

  return 0;
}

void ak_tcp_client_set_auto_reconnect(ak_tcp_client_t *client, bool enable) {
  if (!client || !client->internal) {
    return;
  }

  AK24_MUTEX_LOCK(&client->internal->mutex);
  client->internal->auto_reconnect = enable;
  AK24_MUTEX_UNLOCK(&client->internal->mutex);

  if (!enable) {
    stop_reconnect_thread(client->internal);
  }
}

const char *ak_tcp_client_remote_host(ak_tcp_client_t *client) {
  if (!client || !client->internal) {
    return NULL;
  }
  return client->internal->host;
}

uint16_t ak_tcp_client_remote_port(ak_tcp_client_t *client) {
  if (!client || !client->internal) {
    return 0;
  }
  return client->internal->port;
}
