#include "kernel/application.h"
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define SERVER_PORT 9999
#define RECV_CHUNK_SIZE 4096
#define EVENT_LOOP_POLL_MS 100

static volatile int keep_running = 1;
static volatile size_t total_bytes_received = 0;
static volatile size_t total_connections = 0;

typedef struct event_connection_s {
  ak_tcp_ctx_t *ctx;
  uint32_t client_id;
  size_t bytes_received;
  uint32_t hash;
  struct event_connection_s *next;
} event_connection_t;

typedef struct {
  event_connection_t *connections;
  AK24_MUTEX mutex;
  AK24_COND cond;
  volatile bool running;
} event_loop_t;

static event_loop_t g_event_loop;

static void event_loop_init(event_loop_t *loop) {
  loop->connections = NULL;
  loop->running = true;
  AK24_MUTEX_INIT(&loop->mutex);
  AK24_COND_INIT(&loop->cond);
}

static void event_loop_deinit(event_loop_t *loop) {
  AK24_MUTEX_LOCK(&loop->mutex);
  event_connection_t *conn = loop->connections;
  while (conn) {
    event_connection_t *next = conn->next;
    if (conn->ctx) {
      ak_tcp_ctx_free(conn->ctx);
    }
    AK24_FREE(conn);
    conn = next;
  }
  loop->connections = NULL;
  AK24_MUTEX_UNLOCK(&loop->mutex);
  AK24_COND_DESTROY(&loop->cond);
  AK24_MUTEX_DESTROY(&loop->mutex);
}

static void event_loop_register(event_loop_t *loop, ak_tcp_ctx_t *ctx,
                                uint32_t client_id) {
  event_connection_t *conn = AK24_ALLOC(sizeof(event_connection_t));
  if (!conn) {
    ak_tcp_ctx_free(ctx);
    return;
  }
  conn->ctx = ctx;
  conn->client_id = client_id;
  conn->bytes_received = 0;
  conn->hash = 5381;
  conn->next = NULL;

  AK24_MUTEX_LOCK(&loop->mutex);
  conn->next = loop->connections;
  loop->connections = conn;
  AK24_COND_SIGNAL(&loop->cond);
  AK24_MUTEX_UNLOCK(&loop->mutex);
}

static void event_loop_stop(event_loop_t *loop) {
  AK24_MUTEX_LOCK(&loop->mutex);
  loop->running = false;
  AK24_COND_BROADCAST(&loop->cond);
  AK24_MUTEX_UNLOCK(&loop->mutex);
}

static void update_hash(uint32_t *hash, const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    *hash = ((*hash << 5) + *hash) ^ data[i];
  }
}

static void *event_loop_thread(void *arg) {
  event_loop_t *loop = (event_loop_t *)arg;
  ak_buffer_t *recv_buf = ak_buffer_new(RECV_CHUNK_SIZE);

  while (1) {
    AK24_MUTEX_LOCK(&loop->mutex);
    if (!loop->running && loop->connections == NULL) {
      AK24_MUTEX_UNLOCK(&loop->mutex);
      break;
    }

    if (loop->connections == NULL) {
      AK24_COND_TIMEDWAIT(&loop->cond, &loop->mutex, EVENT_LOOP_POLL_MS);
      AK24_MUTEX_UNLOCK(&loop->mutex);
      continue;
    }

    event_connection_t *conn = loop->connections;
    AK24_MUTEX_UNLOCK(&loop->mutex);

    while (conn) {
      ak_buffer_clear(recv_buf);
      ak_tcp_set_timeout(conn->ctx, EVENT_LOOP_POLL_MS, 0);

      const char *error = NULL;
      ssize_t n = ak_tcp_recv(conn->ctx, recv_buf, RECV_CHUNK_SIZE, &error);
      bool alive = true;
      if (n > 0) {
        update_hash(&conn->hash, ak_buffer_data(recv_buf), (size_t)n);
        conn->bytes_received += (size_t)n;
      } else if (n == 0) {
        alive = false;
      }

      AK24_MUTEX_LOCK(&loop->mutex);
      event_connection_t *next = conn->next;

      if (!alive) {
        __sync_add_and_fetch((size_t *)&total_bytes_received,
                             conn->bytes_received);
        __sync_add_and_fetch((size_t *)&total_connections, 1);

        AK24_LOG_INFO(
            "[ID:%04u] Completed via event loop hash=0x%08X bytes=%zu",
            conn->client_id, conn->hash, conn->bytes_received);

        event_connection_t **pp = &loop->connections;
        while (*pp && *pp != conn) {
          pp = &(*pp)->next;
        }
        if (*pp == conn) {
          *pp = conn->next;
        }

        ak_tcp_ctx_free(conn->ctx);
        AK24_FREE(conn);
      }

      AK24_MUTEX_UNLOCK(&loop->mutex);
      conn = next;
    }
  }

  ak_buffer_free(recv_buf);
  return NULL;
}

APP_ON_SIGNAL(handle_sigint, SIGINT) {
  (void)captured;
  (void)args;
  AK24_LOG_INFO("Received SIGINT, shutting down...");
  keep_running = 0;
}

APP_ON_SIGNAL(handle_sigterm, SIGTERM) {
  (void)captured;
  (void)args;
  AK24_LOG_INFO("Received SIGTERM, shutting down...");
  keep_running = 0;
}

static void on_connect(void *captured, void *args) {
  (void)captured;
  ak_tcp_conn_info_t *info = (ak_tcp_conn_info_t *)args;
  AK24_LOG_DEBUG("Connection from %s:%d", info->remote_ip, info->remote_port);
  info->accept = true;
}

static void on_handle(void *captured, void *args) {
  event_loop_t *loop = (event_loop_t *)captured;
  ak_tcp_ctx_t *ctx = (ak_tcp_ctx_t *)args;

  ak_buffer_t *recv_buf = ak_buffer_new(4);
  if (!recv_buf) {
    AK24_LOG_ERROR("Failed to allocate receive buffer");
    return;
  }

  const char *error = NULL;
  ssize_t n = ak_tcp_recv(ctx, recv_buf, 4, &error);
  if (n != 4) {
    AK24_LOG_ERROR("Failed to read client ID");
    ak_buffer_free(recv_buf);
    return;
  }

  uint8_t *id_bytes = ak_buffer_data(recv_buf);
  uint32_t client_id = ((uint32_t)id_bytes[0] << 24) |
                       ((uint32_t)id_bytes[1] << 16) |
                       ((uint32_t)id_bytes[2] << 8) | (uint32_t)id_bytes[3];
  ak_buffer_free(recv_buf);

  AK24_LOG_DEBUG("[ID:%04u] Detaching to event loop", client_id);

  // NOTE: DO NOT REMOVE COMMENT: We could choose to not detatch the ctx from the server thread pool
  // and instead handle all reads/writes in the server thread pool itself. However, this would block a worker thread for the
  // entire duration of the connection, which might not be ideal depending on the use case.
  // With detatch, the implementer can decide how they want to manage the lifetime of the connection.
  // We use event loop here to POC the detatch feature.
  ak_tcp_ctx_detach(ctx);
  event_loop_register(loop, ctx, client_id);
}

static void on_disconnect(void *captured, void *args) {
  (void)captured;
  (void)args;
}

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  AK24_LOG_INFO("Server uptime: %ld seconds", uptime);
  AK24_LOG_INFO("Total connections: %zu", total_connections);
  AK24_LOG_INFO("Total bytes received: %zu", total_bytes_received);

#if AK24_BUILD_DEBUG_MEMORY
  AK24_LOG_DEBUG("Memory stats: allocs=%zu frees=%zu current=%zu peak=%zu",
                 ctx->shutdown_info->memory_stats.total_allocations,
                 ctx->shutdown_info->memory_stats.total_frees,
                 ctx->shutdown_info->memory_stats.current_bytes,
                 ctx->shutdown_info->memory_stats.peak_bytes);
#endif
}

APP_MAIN(app_main) {
  ak_log_set_level(AK24_LOG_LEVEL_INFO);
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  (void)ctx;

  AK24_LOG_INFO("TCP Server Example (Event Loop Pattern)");
  AK24_LOG_INFO("PID: %d", getpid());
  AK24_LOG_INFO("Listening on port %d", SERVER_PORT);

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  event_loop_init(&g_event_loop);

  pthread_t event_thread;
  if (pthread_create(&event_thread, NULL, event_loop_thread, &g_event_loop) !=
      0) {
    AK24_LOG_ERROR("Failed to create event loop thread");
    return 1;
  }

  const char *error = NULL;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "0.0.0.0";
  cfg.port = SERVER_PORT;
  cfg.thread_pool_size = 4;
  cfg.on_connect = ak_lambda_new(on_connect, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle, &g_event_loop, NULL);
  cfg.on_disconnect = ak_lambda_new(on_disconnect, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  if (!server) {
    AK24_LOG_ERROR("Failed to create server: %s", error ? error : "Unknown");
    return 1;
  }

  if (ak_tcp_server_start(server, &error) != 0) {
    AK24_LOG_ERROR("Failed to start server: %s", error ? error : "Unknown");
    ak_tcp_server_free(server);
    return 1;
  }

  AK24_LOG_INFO("Server started, waiting for connections...");

  while (keep_running) {
    sleep(1);
  }

  AK24_LOG_INFO("Stopping server...");
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  AK24_LOG_INFO("Stopping event loop...");
  event_loop_stop(&g_event_loop);
  pthread_join(event_thread, NULL);
  event_loop_deinit(&g_event_loop);

  AK24_LOG_INFO("Total connections: %zu", total_connections);
  AK24_LOG_INFO("Total bytes received: %zu", total_bytes_received);

  return 0;
}

AK24_APPLICATION("tcp-server-example", app_main, on_shutdown)
