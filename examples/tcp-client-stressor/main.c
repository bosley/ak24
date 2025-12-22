#include "kernel/application.h"
#include "threads.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static volatile int keep_running = 1;
static volatile size_t clients_completed = 0;
static volatile size_t clients_failed = 0;

typedef struct {
  const char *host;
  uint16_t port;
  size_t num_chunks;
  size_t chunk_size;
  uint32_t client_id;
} client_task_params_t;

static void update_hash(uint32_t *hash, const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    *hash = ((*hash << 5) + *hash) ^ data[i];
  }
}

static void client_task(void *captured, void *args) {
  (void)args;
  client_task_params_t *params = (client_task_params_t *)captured;

  uint32_t client_id = params->client_id;
  const char *error = NULL;

  ak_tcp_client_config_t cfg = ak_tcp_client_config_default();
  cfg.host = params->host;
  cfg.port = params->port;
  cfg.connect_timeout_ms = 10000;
  cfg.send_timeout_ms = 60000;

  ak_tcp_client_t *client = ak_tcp_client_new(&cfg, &error);
  if (!client) {
    AK24_LOG_ERROR("[ID:%04u] Failed to create client: %s", client_id,
                   error ? error : "Unknown");
    __sync_add_and_fetch((size_t *)&clients_failed, 1);
    return;
  }

  ak_tcp_error_t err = ak_tcp_client_connect(client, &error);
  if (err != AK_TCP_OK) {
    AK24_LOG_ERROR("[ID:%04u] Failed to connect: %s", client_id,
                   error ? error : "Unknown");
    ak_tcp_client_free(client);
    __sync_add_and_fetch((size_t *)&clients_failed, 1);
    return;
  }

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
    __sync_add_and_fetch((size_t *)&clients_failed, 1);
    return;
  }

  size_t chunk_size = params->chunk_size;
  uint8_t *chunk_data = AK24_ALLOC(chunk_size);
  if (!chunk_data) {
    AK24_LOG_ERROR("[ID:%04u] Failed to allocate chunk buffer", client_id);
    ak_tcp_client_disconnect(client);
    ak_tcp_client_free(client);
    __sync_add_and_fetch((size_t *)&clients_failed, 1);
    return;
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
    __sync_add_and_fetch((size_t *)&clients_failed, 1);
    return;
  }

  ak_buffer_copy_to(send_buf, chunk_data, chunk_size);

  uint32_t hash = 5381;
  size_t bytes_sent = 0;
  size_t chunks_sent = 0;
  size_t num_chunks = params->num_chunks;

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
  }

  ak_tcp_ctx_t *tcp_ctx = ak_tcp_client_ctx(client);
  if (tcp_ctx) {
    ak_tcp_shutdown(tcp_ctx);
  }

  ak_tcp_client_disconnect(client);
  ak_tcp_client_free(client);
  ak_buffer_free(send_buf);
  AK24_FREE(chunk_data);

  if (chunks_sent == num_chunks) {
    AK24_LOG_INFO("[ID:%04u] hash=0x%08X bytes=%zu", client_id, hash,
                  bytes_sent);
    __sync_add_and_fetch((size_t *)&clients_completed, 1);
  } else {
    AK24_LOG_WARN("[ID:%04u] incomplete hash=0x%08X bytes=%zu chunks=%zu/%zu",
                  client_id, hash, bytes_sent, chunks_sent, num_chunks);
    __sync_add_and_fetch((size_t *)&clients_failed, 1);
  }
}

static void client_task_params_free(void *ptr) {
  client_task_params_t *params = (client_task_params_t *)ptr;
  AK24_FREE(params);
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
  AK24_LOG_INFO("Stressor runtime: %ld seconds", uptime);

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
      "Usage: %s <host> <port> <num_chunks> <chunk_size> <parallelism>", prog);
  AK24_LOG_ERROR("  host        - Server hostname or IP (e.g., 127.0.0.1)");
  AK24_LOG_ERROR("  port        - Server port (e.g., 9999)");
  AK24_LOG_ERROR("  num_chunks  - Number of data chunks per client");
  AK24_LOG_ERROR("  chunk_size  - Size of each chunk in bytes");
  AK24_LOG_ERROR("  parallelism - Number of concurrent clients");
}

APP_MAIN(app_main) {
  ak_log_set_level(AK24_LOG_LEVEL_INFO);
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  AK24_LOG_INFO("TCP Client Stressor");

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  if (list_count(&ctx->args) < 6) {
    print_usage("tcp-client-stressor");
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

  arg = list_next(&ctx->args, &iter);
  size_t parallelism = (size_t)atol(*arg);
  if (parallelism == 0) {
    AK24_LOG_ERROR("Invalid parallelism: %s", *arg);
    return 1;
  }

  size_t total_bytes = num_chunks * chunk_size * parallelism;
  AK24_LOG_INFO("Target: %s:%d", host, port);
  AK24_LOG_INFO("Config: %zu clients, %zu chunks x %zu bytes each", parallelism,
                num_chunks, chunk_size);
  AK24_LOG_INFO("Total data: %zu bytes (%.2f MB)", total_bytes,
                (double)total_bytes / (1024.0 * 1024.0));

  ak_thread_pool_config_t pool_cfg = ak_thread_pool_config_default();
  pool_cfg.max_workers = parallelism;
  pool_cfg.max_queue_size = parallelism * 2;

  ak_thread_pool_t *pool = ak_thread_pool_new(&pool_cfg);
  if (!pool) {
    AK24_LOG_ERROR("Failed to create thread pool");
    return 1;
  }

  time_t start_time = time(NULL);

  for (size_t i = 0; i < parallelism && keep_running; i++) {
    client_task_params_t *params = AK24_ALLOC(sizeof(client_task_params_t));
    if (!params) {
      AK24_LOG_ERROR("Failed to allocate task params");
      continue;
    }

    params->host = host;
    params->port = (uint16_t)port;
    params->num_chunks = num_chunks;
    params->chunk_size = chunk_size;
    params->client_id = (uint32_t)(i + 1);

    ak_lambda_t *task =
        ak_lambda_new(client_task, params, client_task_params_free);
    if (!task) {
      AK24_LOG_ERROR("Failed to create task lambda");
      AK24_FREE(params);
      continue;
    }

    if (ak_thread_pool_enqueue(pool, task, NULL) != 0) {
      AK24_LOG_ERROR("Failed to enqueue task %zu", i);
      ak_lambda_free(task);
    }
  }

  AK24_LOG_INFO("All tasks enqueued, waiting for completion...");

  ak_thread_pool_wait(pool);
  ak_thread_pool_free(pool);

  time_t end_time = time(NULL);
  time_t elapsed = end_time - start_time;
  if (elapsed == 0)
    elapsed = 1;

  size_t actual_bytes = clients_completed * num_chunks * chunk_size;
  double throughput =
      (double)actual_bytes / (double)elapsed / (1024.0 * 1024.0);

  AK24_LOG_INFO("=== Stress Test Complete ===");
  AK24_LOG_INFO("Clients completed: %zu/%zu", clients_completed, parallelism);
  AK24_LOG_INFO("Clients failed: %zu", clients_failed);
  AK24_LOG_INFO("Time: %ld seconds", elapsed);
  AK24_LOG_INFO("Throughput: %.2f MB/s", throughput);

  return (clients_failed > 0) ? 1 : 0;
}

AK24_APPLICATION("tcp-client-stressor", app_main, on_shutdown)
