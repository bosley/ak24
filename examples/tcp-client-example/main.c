#include "kernel/application.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 9999
#define DEFAULT_CLIENT_ID 1

static volatile int keep_running = 1;

static void update_hash(uint32_t *hash, const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    *hash = ((*hash << 5) + *hash) ^ data[i];
  }
}

APP_ON_SIGNAL(handle_sigint, SIGINT) {
  (void)captured;
  (void)args;
  AK24_LOG_INFO("Received SIGINT, aborting...");
  keep_running = 0;
}

APP_ON_SIGNAL(handle_sigterm, SIGTERM) {
  (void)captured;
  (void)args;
  AK24_LOG_INFO("Received SIGTERM, aborting...");
  keep_running = 0;
}

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  AK24_LOG_INFO("Client runtime: %ld seconds", uptime);

#if AK24_BUILD_DEBUG_MEMORY
  AK24_LOG_DEBUG("Memory stats: allocs=%zu frees=%zu current=%zu peak=%zu",
                 ctx->shutdown_info->memory_stats.total_allocations,
                 ctx->shutdown_info->memory_stats.total_frees,
                 ctx->shutdown_info->memory_stats.current_bytes,
                 ctx->shutdown_info->memory_stats.peak_bytes);
#endif
}

static void print_usage(const char *prog) {
  AK24_LOG_ERROR(
      "Usage: %s <host> <port> <num_chunks> <chunk_size> [client_id]", prog);
  AK24_LOG_ERROR("  host       - Server hostname or IP (e.g., 127.0.0.1)");
  AK24_LOG_ERROR("  port       - Server port (e.g., 9999)");
  AK24_LOG_ERROR("  num_chunks - Number of data chunks to send (N)");
  AK24_LOG_ERROR("  chunk_size - Size of each chunk in bytes (X)");
  AK24_LOG_ERROR("  client_id  - Optional client identifier (default: 1)");
}

APP_MAIN(app_main) {
  ak_log_set_level(AK24_LOG_LEVEL_INFO);
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  AK24_LOG_INFO("TCP Client Example");

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  if (list_count(&ctx->args) < 5) {
    print_usage("tcp-client-example");
    return 1;
  }

  list_iter_t iter = list_iter(&ctx->args);
  char **arg;

  list_next(&ctx->args, &iter);
  arg = list_next(&ctx->args, &iter);
  const char *host = *arg;

  arg = list_next(&ctx->args, &iter);
  int port = atoi(*arg);
  if (port <= 0 || port > 65535) {
    AK24_LOG_ERROR("Invalid port: %s", *arg);
    return 1;
  }

  arg = list_next(&ctx->args, &iter);
  size_t num_chunks = (size_t)atol(*arg);
  if (num_chunks == 0) {
    AK24_LOG_ERROR("Invalid num_chunks: %s", *arg);
    return 1;
  }

  arg = list_next(&ctx->args, &iter);
  size_t chunk_size = (size_t)atol(*arg);
  if (chunk_size == 0) {
    AK24_LOG_ERROR("Invalid chunk_size: %s", *arg);
    return 1;
  }

  uint32_t client_id = DEFAULT_CLIENT_ID;
  arg = list_next(&ctx->args, &iter);
  if (arg) {
    client_id = (uint32_t)atol(*arg);
  }

  size_t total_bytes = num_chunks * chunk_size;
  AK24_LOG_INFO("[ID:%04u] Connecting to %s:%d", client_id, host, port);
  AK24_LOG_INFO(
      "[ID:%04u] Sending %zu chunks of %zu bytes each (%zu bytes total)",
      client_id, num_chunks, chunk_size, total_bytes);

  const char *error = NULL;

  ak_tcp_client_config_t cfg = ak_tcp_client_config_default();
  cfg.host = host;
  cfg.port = (uint16_t)port;
  cfg.connect_timeout_ms = 5000;
  cfg.send_timeout_ms = 30000;

  ak_tcp_client_t *client = ak_tcp_client_new(&cfg, &error);
  if (!client) {
    AK24_LOG_ERROR("[ID:%04u] Failed to create client: %s", client_id,
                   error ? error : "Unknown");
    return 1;
  }

  ak_tcp_error_t err = ak_tcp_client_connect(client, &error);
  if (err != AK_TCP_OK) {
    AK24_LOG_ERROR("[ID:%04u] Failed to connect: %s", client_id,
                   error ? error : "Unknown");
    ak_tcp_client_free(client);
    return 1;
  }

  AK24_LOG_DEBUG("[ID:%04u] Connected to server", client_id);

  uint8_t id_bytes[4] = {(uint8_t)(client_id >> 24), (uint8_t)(client_id >> 16),
                         (uint8_t)(client_id >> 8), (uint8_t)client_id};

  ak_buffer_t *id_buf = ak_buffer_new(4);
  ak_buffer_copy_to(id_buf, id_bytes, 4);
  ssize_t sent = ak_tcp_client_send(client, id_buf, &error);
  ak_buffer_free(id_buf);

  if (sent != 4) {
    AK24_LOG_ERROR("[ID:%04u] Failed to send client ID", client_id);
    ak_tcp_client_disconnect(client);
    ak_tcp_client_free(client);
    return 1;
  }

  uint8_t *chunk_data = AK24_ALLOC(chunk_size);
  if (!chunk_data) {
    AK24_LOG_ERROR("[ID:%04u] Failed to allocate chunk buffer", client_id);
    ak_tcp_client_disconnect(client);
    ak_tcp_client_free(client);
    return 1;
  }

  for (size_t i = 0; i < chunk_size; i++) {
    chunk_data[i] = (uint8_t)((i * 7 + 13) & 0xFF);
  }

  ak_buffer_t *send_buf = ak_buffer_new(chunk_size);
  if (!send_buf) {
    AK24_LOG_ERROR("[ID:%04u] Failed to allocate send buffer", client_id);
    AK24_FREE(chunk_data);
    ak_tcp_client_disconnect(client);
    ak_tcp_client_free(client);
    return 1;
  }

  ak_buffer_copy_to(send_buf, chunk_data, chunk_size);

  uint32_t hash = 5381;
  size_t bytes_sent = 0;
  size_t chunks_sent = 0;

  time_t start_time = time(NULL);

  for (size_t i = 0; i < num_chunks && keep_running; i++) {
    sent = ak_tcp_client_send(client, send_buf, &error);
    if (sent < 0) {
      AK24_LOG_ERROR("[ID:%04u] Send failed at chunk %zu: %s", client_id, i,
                     error ? error : "Unknown");
      break;
    }

    update_hash(&hash, chunk_data, chunk_size);
    bytes_sent += (size_t)sent;
    chunks_sent++;

    if ((i + 1) % 1000 == 0 || i == num_chunks - 1) {
      AK24_LOG_DEBUG("[ID:%04u] Progress: %zu/%zu chunks sent", client_id,
                     i + 1, num_chunks);
    }
  }

  time_t end_time = time(NULL);
  time_t elapsed = end_time - start_time;
  if (elapsed == 0)
    elapsed = 1;

  ak_tcp_ctx_t *tcp_ctx = ak_tcp_client_ctx(client);
  if (tcp_ctx) {
    ak_tcp_shutdown(tcp_ctx);
  }

  ak_tcp_client_disconnect(client);
  ak_tcp_client_free(client);
  ak_buffer_free(send_buf);
  AK24_FREE(chunk_data);

  double throughput = (double)bytes_sent / (double)elapsed / (1024.0 * 1024.0);

  AK24_LOG_INFO("[ID:%04u] hash=0x%08X bytes=%zu chunks=%zu/%zu time=%lds "
                "throughput=%.2fMB/s",
                client_id, hash, bytes_sent, chunks_sent, num_chunks, elapsed,
                throughput);

  return 0;
}

AK24_APPLICATION("tcp-client-example", app_main, on_shutdown)
