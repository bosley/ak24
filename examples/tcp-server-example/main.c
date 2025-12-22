#include "kernel/application.h"
#include <signal.h>
#include <unistd.h>

#define SERVER_PORT 9999
#define RECV_CHUNK_SIZE 4096

static volatile int keep_running = 1;
static volatile size_t total_bytes_received = 0;
static volatile size_t total_connections = 0;

static void update_hash(uint32_t *hash, const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    *hash = ((*hash << 5) + *hash) ^ data[i];
  }
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

typedef struct {
  uint32_t client_id;
  size_t bytes_received;
  uint32_t hash;
} connection_state_t;

static void on_connect(void *captured, void *args) {
  (void)captured;
  ak_tcp_conn_info_t *info = (ak_tcp_conn_info_t *)args;
  AK24_LOG_DEBUG("Connection from %s:%d", info->remote_ip, info->remote_port);
  info->accept = true;
}

static void on_handle(void *captured, void *args) {
  (void)captured;
  ak_tcp_ctx_t *tcp_ctx = (ak_tcp_ctx_t *)args;

  connection_state_t state = {
      .client_id = 0, .bytes_received = 0, .hash = 5381};

  ak_buffer_t *recv_buf = ak_buffer_new(RECV_CHUNK_SIZE);
  if (!recv_buf) {
    AK24_LOG_ERROR("Failed to allocate receive buffer");
    return;
  }

  const char *error = NULL;
  ssize_t n = ak_tcp_recv(tcp_ctx, recv_buf, 4, &error);
  if (n != 4) {
    AK24_LOG_ERROR("Failed to read client ID");
    ak_buffer_free(recv_buf);
    return;
  }

  uint8_t *id_bytes = ak_buffer_data(recv_buf);
  state.client_id = ((uint32_t)id_bytes[0] << 24) |
                    ((uint32_t)id_bytes[1] << 16) |
                    ((uint32_t)id_bytes[2] << 8) | (uint32_t)id_bytes[3];

  AK24_LOG_DEBUG("[ID:%04u] Client connected", state.client_id);

  for (;;) {
    ak_buffer_clear(recv_buf);

    n = ak_tcp_recv(tcp_ctx, recv_buf, RECV_CHUNK_SIZE, &error);
    if (n <= 0) {
      break;
    }

    update_hash(&state.hash, ak_buffer_data(recv_buf), (size_t)n);
    state.bytes_received += (size_t)n;
  }

  __sync_add_and_fetch((size_t *)&total_bytes_received, state.bytes_received);
  __sync_add_and_fetch((size_t *)&total_connections, 1);

  AK24_LOG_INFO("[ID:%04u] hash=0x%08X bytes=%zu", state.client_id, state.hash,
                state.bytes_received);

  ak_buffer_free(recv_buf);
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

  AK24_LOG_INFO("TCP Server Example");
  AK24_LOG_INFO("PID: %d", getpid());
  AK24_LOG_INFO("Listening on port %d", SERVER_PORT);

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  const char *error = NULL;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "0.0.0.0";
  cfg.port = SERVER_PORT;
  cfg.thread_pool_size = 8;
  cfg.on_connect = ak_lambda_new(on_connect, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle, NULL, NULL);
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

  AK24_LOG_INFO("Total connections: %zu", total_connections);
  AK24_LOG_INFO("Total bytes received: %zu", total_bytes_received);

  return 0;
}

AK24_APPLICATION("tcp-server-example", app_main, on_shutdown)
