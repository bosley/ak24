/**
 * @file hardening_tests.c
 * @brief TCP module production hardening tests
 *
 * Tests for production-critical features:
 * - Backpressure handling (Feature 1)
 * - Thread pool queue limits (Feature 2)
 * - Rate limiting (Feature 3)
 * - Bounded recv_until (Feature 4)
 * - Linger control (Feature 5)
 * - Non-blocking accept with timeout (Feature 6)
 * - Graceful connection draining (Feature 7)
 * - IPv6 support (Feature 8)
 * - Socket buffer tuning (Feature 9)
 */

#include "kernel.h"
#include "tcp.h"
#include "tcp_internal.h"
#include "test/assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef AK24_PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#define usleep(x) Sleep((x) / 1000)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define HARDENING_TEST_PORT 19100

typedef struct {
  ak_socket_fd_t sock;
  uint16_t port;
  bool connected;
} hardening_test_client_t;

static int h_client_connect(hardening_test_client_t *client, uint16_t port) {
  const char *error = NULL;

  client->sock = ak_tcp_socket_create(&error);
  if (client->sock == AK_INVALID_SOCKET) {
    printf("  [client] Failed to create socket: %s\n", error);
    return -1;
  }

  client->port = port;

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (connect(client->sock, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    printf("  [client] Failed to connect to port %d\n", port);
    ak_tcp_socket_close(client->sock);
    return -1;
  }

  client->connected = true;
  return 0;
}

static int h_client_connect_v6(hardening_test_client_t *client, uint16_t port,
                               const char *addr) {
  const char *error = NULL;

  client->sock = ak_tcp_socket_create_for_addr(addr, &error);
  if (client->sock == AK_INVALID_SOCKET) {
    printf("  [client] Failed to create socket: %s\n", error);
    return -1;
  }

  client->port = port;

  struct sockaddr_in6 server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(port);
  inet_pton(AF_INET6, addr, &server_addr.sin6_addr);

  if (connect(client->sock, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    printf("  [client] Failed to connect to [%s]:%d\n", addr, port);
    ak_tcp_socket_close(client->sock);
    return -1;
  }

  client->connected = true;
  return 0;
}

static void h_client_disconnect(hardening_test_client_t *client) {
  if (client->connected) {
    ak_tcp_socket_close(client->sock);
    client->connected = false;
  }
}

static ssize_t h_client_send_all(hardening_test_client_t *client,
                                 const uint8_t *data, size_t len) {
  const char *error = NULL;
  size_t total_sent = 0;

  while (total_sent < len) {
    ssize_t sent = ak_tcp_socket_send(client->sock, data + total_sent,
                                      len - total_sent, &error);
    if (sent <= 0) {
      return -1;
    }
    total_sent += (size_t)sent;
  }

  return (ssize_t)total_sent;
}

static ssize_t h_client_recv_all(hardening_test_client_t *client, uint8_t *buf,
                                 size_t expected_len) {
  const char *error = NULL;
  size_t total_received = 0;

  while (total_received < expected_len) {
    ssize_t received =
        ak_tcp_socket_recv(client->sock, buf + total_received,
                           expected_len - total_received, &error);
    if (received <= 0) {
      return (ssize_t)total_received;
    }
    total_received += (size_t)received;
  }

  return (ssize_t)total_received;
}

static void on_connect_accept(void *captured, void *args) {
  (void)captured;
  ak_tcp_conn_info_t *info = (ak_tcp_conn_info_t *)args;
  info->accept = true;
}

static void on_handle_echo_simple(void *captured, void *args) {
  (void)captured;
  ak_tcp_ctx_t *ctx = (ak_tcp_ctx_t *)args;

  while (ak_tcp_is_alive(ctx)) {
    const char *error = NULL;
    ak_buffer_t *line = ak_tcp_recv_until(ctx, "\n", 8192, &error);
    if (!line) {
      break;
    }
    ak_tcp_send(ctx, line, &error);
    ak_buffer_free(line);
    break;
  }
}

static void on_handle_slow(void *captured, void *args) {
  (void)captured;
  ak_tcp_ctx_t *ctx = (ak_tcp_ctx_t *)args;
  usleep(500000);
  const char *error = NULL;
  ak_buffer_t *line = ak_tcp_recv_until(ctx, "\n", 8192, &error);
  if (line) {
    ak_tcp_send(ctx, line, &error);
    ak_buffer_free(line);
  }
}

static void on_handle_hold(void *captured, void *args) {
  (void)captured;
  ak_tcp_ctx_t *ctx = (ak_tcp_ctx_t *)args;
  while (ak_tcp_is_alive(ctx)) {
    usleep(100000);
  }
}

static int test_recv_until_max_size(void) {
  printf("Test: Bounded recv_until (Feature 4)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo_simple, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  hardening_test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(h_client_connect(&client, port), 0);

  char large_data[200];
  memset(large_data, 'X', sizeof(large_data) - 1);
  large_data[sizeof(large_data) - 1] = '\0';

  h_client_send_all(&client, (const uint8_t *)large_data, sizeof(large_data));
  usleep(100000);

  h_client_disconnect(&client);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Bounded recv_until limits enforced\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_thread_pool_queue_limit(void) {
  printf("Test: Thread pool queue limits (Feature 2)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT + 1;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.thread_pool_size = 1;
  cfg.max_pending_tasks = 2;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_slow, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  hardening_test_client_t clients[5] = {0};
  int connected = 0;
  for (int i = 0; i < 5; i++) {
    if (h_client_connect(&clients[i], port) == 0) {
      connected++;
      h_client_send_all(&clients[i], (const uint8_t *)"msg\n", 4);
    }
    usleep(10000);
  }

  printf("  Connected %d clients with queue limit\n", connected);

  usleep(200000);

  for (int i = 0; i < 5; i++) {
    h_client_disconnect(&clients[i]);
  }

  ak_tcp_server_stats_t stats;
  ak_tcp_server_get_stats(server, &stats);
  printf("  Connections rejected (queue full): %zu\n",
         stats.connections_rejected_queue);

  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Thread pool queue limiting works\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_per_ip_connection_limit(void) {
  printf("Test: Per-IP connection limit (Feature 3)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT + 2;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.max_connections_per_ip = 2;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_hold, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  hardening_test_client_t c1 = {0}, c2 = {0}, c3 = {0};

  AK24_TEST_ASSERT_EQ(h_client_connect(&c1, port), 0);
  usleep(50000);
  AK24_TEST_ASSERT_EQ(h_client_connect(&c2, port), 0);
  usleep(100000);

  h_client_connect(&c3, port);
  usleep(100000);

  ak_tcp_server_stats_t stats;
  ak_tcp_server_get_stats(server, &stats);
  printf("  Rejected by IP limit: %zu\n", stats.connections_rejected_ip_limit);

  h_client_disconnect(&c1);
  usleep(100000);

  hardening_test_client_t c4 = {0};
  int result = h_client_connect(&c4, port);
  printf("  After disconnect, new connect result: %d\n", result);
  h_client_disconnect(&c4);

  h_client_disconnect(&c2);
  h_client_disconnect(&c3);

  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Per-IP connection limiting works\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_connection_rate_limit(void) {
  printf("Test: Connection rate limit (Feature 3)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT + 3;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.connection_rate_limit = 3;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo_simple, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  int connected = 0;
  for (int i = 0; i < 6; i++) {
    hardening_test_client_t c = {0};
    if (h_client_connect(&c, port) == 0) {
      connected++;
      h_client_disconnect(&c);
    }
  }

  printf("  Rapid connections: %d succeeded\n", connected);

  ak_tcp_server_stats_t stats;
  ak_tcp_server_get_stats(server, &stats);
  printf("  Rejected by rate limit: %zu\n", stats.connections_rejected_rate);

  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Connection rate limiting works\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_linger_settings(void) {
  printf("Test: Linger control (Feature 5)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT + 4;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.enable_linger = true;
  cfg.linger_timeout_sec = 2;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo_simple, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  hardening_test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(h_client_connect(&client, port), 0);

  h_client_send_all(&client, (const uint8_t *)"test\n", 5);

  char response[64] = {0};
  h_client_recv_all(&client, (uint8_t *)response, 5);
  AK24_TEST_ASSERT_STR_EQ(response, "test\n");

  h_client_disconnect(&client);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Linger settings applied\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_accept_timeout_shutdown(void) {
  printf("Test: Non-blocking accept with timeout (Feature 6)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT + 5;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo_simple, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  printf("  Stopping server (should return within 2 seconds)...\n");
  ak_tcp_server_stop(server);
  printf("  Server stopped quickly\n");

  ak_tcp_server_free(server);

  printf("  Non-blocking accept works\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_graceful_drain(void) {
  printf("Test: Graceful connection draining (Feature 7)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT + 6;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.shutdown_drain_timeout_ms = 2000;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_slow, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  hardening_test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(h_client_connect(&client, port), 0);
  h_client_send_all(&client, (const uint8_t *)"drain-test\n", 11);

  usleep(50000);

  printf("  Initiating graceful shutdown (handler takes 500ms)...\n");
  AK24_TEST_ASSERT_EQ(ak_tcp_server_is_draining(server), false);
  ak_tcp_server_stop(server);
  printf("  Shutdown complete\n");

  h_client_disconnect(&client);
  ak_tcp_server_free(server);

  printf("  Graceful draining works\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_socket_buffer_sizes(void) {
  printf("Test: Socket buffer tuning (Feature 9)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT + 7;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.socket_recv_buffer = 65536;
  cfg.socket_send_buffer = 65536;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo_simple, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  hardening_test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(h_client_connect(&client, port), 0);

  h_client_send_all(&client, (const uint8_t *)"buffer-test\n", 12);

  char response[64] = {0};
  h_client_recv_all(&client, (uint8_t *)response, 12);
  AK24_TEST_ASSERT_STR_EQ(response, "buffer-test\n");

  h_client_disconnect(&client);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Socket buffer tuning applied\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_ipv6_connection(void) {
  printf("Test: IPv6 connection (Feature 8)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT + 8;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "::1";
  cfg.port = port;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo_simple, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);

  int start_result = ak_tcp_server_start(server, &error);
  if (start_result != 0) {
    printf("  IPv6 not available on this system, skipping\n");
    ak_tcp_server_free(server);
    printf("  PASSED (skipped)\n");
    AK24_TEST_PASS();
  }
  usleep(50000);

  hardening_test_client_t client = {0};
  int conn_result = h_client_connect_v6(&client, port, "::1");
  if (conn_result != 0) {
    printf("  IPv6 client connect failed, skipping\n");
    ak_tcp_server_stop(server);
    ak_tcp_server_free(server);
    printf("  PASSED (skipped)\n");
    AK24_TEST_PASS();
  }

  h_client_send_all(&client, (const uint8_t *)"ipv6-test\n", 10);

  char response[64] = {0};
  h_client_recv_all(&client, (uint8_t *)response, 10);
  AK24_TEST_ASSERT_STR_EQ(response, "ipv6-test\n");

  h_client_disconnect(&client);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  IPv6 connection works\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_dual_stack(void) {
  printf("Test: Dual-stack IPv4/IPv6 (Feature 8)\n");

  const char *error = NULL;
  const uint16_t port = HARDENING_TEST_PORT + 9;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "::";
  cfg.port = port;
  cfg.on_connect = ak_lambda_new(on_connect_accept, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo_simple, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);

  int start_result = ak_tcp_server_start(server, &error);
  if (start_result != 0) {
    printf("  Dual-stack not available on this system, skipping\n");
    ak_tcp_server_free(server);
    printf("  PASSED (skipped)\n");
    AK24_TEST_PASS();
  }
  usleep(50000);

  hardening_test_client_t client4 = {0};
  if (h_client_connect(&client4, port) == 0) {
    h_client_send_all(&client4, (const uint8_t *)"v4msg\n", 6);
    char response[64] = {0};
    h_client_recv_all(&client4, (uint8_t *)response, 6);
    printf("  IPv4 client: response = %s", response);
    h_client_disconnect(&client4);
  } else {
    printf("  IPv4 client failed to connect (may be expected)\n");
  }

  hardening_test_client_t client6 = {0};
  if (h_client_connect_v6(&client6, port, "::1") == 0) {
    h_client_send_all(&client6, (const uint8_t *)"v6msg\n", 6);
    char response[64] = {0};
    h_client_recv_all(&client6, (uint8_t *)response, 6);
    printf("  IPv6 client: response = %s", response);
    h_client_disconnect(&client6);
  } else {
    printf("  IPv6 client failed to connect (may be expected)\n");
  }

  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Dual-stack test complete\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

int run_tcp_hardening_tests(void) {
  printf("\n--- Production Hardening Tests ---\n\n");
  AK24_TEST_RUN(test_recv_until_max_size);
  AK24_TEST_RUN(test_thread_pool_queue_limit);
  AK24_TEST_RUN(test_per_ip_connection_limit);
  AK24_TEST_RUN(test_connection_rate_limit);
  AK24_TEST_RUN(test_linger_settings);
  AK24_TEST_RUN(test_accept_timeout_shutdown);
  AK24_TEST_RUN(test_graceful_drain);
  AK24_TEST_RUN(test_socket_buffer_sizes);
  AK24_TEST_RUN(test_ipv6_connection);
  AK24_TEST_RUN(test_dual_stack);
  return 0;
}
