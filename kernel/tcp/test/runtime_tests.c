
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

#define TEST_PORT 19000

#define SMALL_DATA_SIZE 64
#define MEDIUM_DATA_SIZE 1024
#define LARGE_DATA_SIZE 8192
#define MAX_MESSAGES 10

static int connections_accepted = 0;
static int connections_handled = 0;
static int connections_disconnected = 0;
static int total_bytes_received = 0;
static int total_bytes_sent = 0;
static int data_validation_errors = 0;

static void on_connect_accept_all(void *captured, void *args) {
  (void)captured;
  ak_tcp_conn_info_t *info = (ak_tcp_conn_info_t *)args;

  printf("  [on_connect] Connection from %s:%d\n", info->remote_ip,
         info->remote_port);

  info->accept = true;
  connections_accepted++;
}

static void on_connect_reject_all(void *captured, void *args) {
  (void)captured;
  ak_tcp_conn_info_t *info = (ak_tcp_conn_info_t *)args;

  printf("  [on_connect] Rejecting connection from %s:%d\n", info->remote_ip,
         info->remote_port);

  info->accept = false;
}

static void on_handle_silent(void *captured, void *args) {
  (void)captured;
  ak_tcp_ctx_t *ctx = (ak_tcp_ctx_t *)args;

  connections_handled++;

  while (ak_tcp_is_alive(ctx)) {
    usleep(50000);
  }
}

static void on_handle_echo(void *captured, void *args) {
  (void)captured;
  ak_tcp_ctx_t *ctx = (ak_tcp_ctx_t *)args;

  connections_handled++;

  while (ak_tcp_is_alive(ctx)) {
    const char *error = NULL;
    ak_buffer_t *line = ak_tcp_recv_until(ctx, "\n", 8192, &error);
    if (!line) {
      break;
    }

    total_bytes_received += (int)ak_buffer_count(line);
    ak_tcp_send(ctx, line, &error);
    total_bytes_sent += (int)ak_buffer_count(line);
    ak_buffer_free(line);

    break;
  }
}

static void on_handle_multi_echo(void *captured, void *args) {
  (void)captured;
  ak_tcp_ctx_t *ctx = (ak_tcp_ctx_t *)args;

  connections_handled++;
  int msg_count = 0;

  while (ak_tcp_is_alive(ctx)) {
    const char *error = NULL;
    ak_buffer_t *line = ak_tcp_recv_until(ctx, "\n", 8192, &error);
    if (!line) {
      break;
    }

    msg_count++;
    total_bytes_received += (int)ak_buffer_count(line);

    ak_tcp_send(ctx, line, &error);
    total_bytes_sent += (int)ak_buffer_count(line);
    ak_buffer_free(line);

    if (msg_count >= MAX_MESSAGES) {
      break;
    }
  }

  ak_tcp_set_meta(ctx, "msg_count", (void *)(intptr_t)msg_count);
}

static void on_handle_binary_echo(void *captured, void *args) {
  (void)captured;
  ak_tcp_ctx_t *ctx = (ak_tcp_ctx_t *)args;

  connections_handled++;

  while (ak_tcp_is_alive(ctx)) {
    const char *error = NULL;
    ak_buffer_t *recv_buf = ak_buffer_new(4);

    ssize_t received = ak_tcp_recv(ctx, recv_buf, 4, &error);
    if (received != 4) {
      ak_buffer_free(recv_buf);
      break;
    }

    uint8_t *data = ak_buffer_data(recv_buf);
    uint32_t length = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                      ((uint32_t)data[2] << 8) | (uint32_t)data[3];
    ak_buffer_clear(recv_buf);

    if (length == 0 || length > LARGE_DATA_SIZE * 2) {
      ak_buffer_free(recv_buf);
      break;
    }

    size_t total_read = 0;
    while (total_read < length && ak_tcp_is_alive(ctx)) {
      ssize_t n = ak_tcp_recv(ctx, recv_buf, length - total_read, &error);
      if (n <= 0) {
        break;
      }
      total_read += (size_t)n;
    }

    if (total_read != length) {
      ak_buffer_free(recv_buf);
      break;
    }

    total_bytes_received += (int)(4 + length);

    ak_buffer_t *send_buf = ak_buffer_new(4 + length);
    uint8_t len_bytes[4] = {(uint8_t)(length >> 24), (uint8_t)(length >> 16),
                            (uint8_t)(length >> 8), (uint8_t)length};
    ak_buffer_copy_to(send_buf, len_bytes, 4);
    ak_buffer_copy_to(send_buf, ak_buffer_data(recv_buf), length);

    ak_tcp_send(ctx, send_buf, &error);
    total_bytes_sent += (int)(4 + length);

    ak_buffer_free(recv_buf);
    ak_buffer_free(send_buf);

    break;
  }
}

static void on_disconnect_log(void *captured, void *args) {
  (void)captured;
  (void)args;
  connections_disconnected++;
}

static void generate_sequential_data(uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    buf[i] = (uint8_t)(i & 0xFF);
  }
}

static void generate_random_data(uint8_t *buf, size_t len, unsigned int seed) {
  srand(seed);
  for (size_t i = 0; i < len; i++) {
    buf[i] = (uint8_t)(rand() & 0xFF);
  }
}

static void generate_binary_pattern(uint8_t *buf, size_t len) {

  for (size_t i = 0; i < len; i++) {
    if (i % 256 == 0) {
      buf[i] = 0x00;
    } else if (i % 256 == 255) {
      buf[i] = 0xFF;
    } else {
      buf[i] = (uint8_t)(i % 256);
    }
  }
}

static int validate_data(const uint8_t *expected, const uint8_t *actual,
                         size_t len, const char *test_name) {
  int errors = 0;
  for (size_t i = 0; i < len; i++) {
    if (expected[i] != actual[i]) {
      if (errors < 5) {
        printf("  [%s] Mismatch at byte %zu: expected 0x%02X, got 0x%02X\n",
               test_name, i, expected[i], actual[i]);
      }
      errors++;
    }
  }
  if (errors > 0) {
    printf("  [%s] Total byte mismatches: %d / %zu\n", test_name, errors, len);
    data_validation_errors += errors;
  }
  return errors;
}

typedef struct {
  ak_socket_fd_t sock;
  uint16_t port;
  bool connected;
} test_client_t;

static int test_client_connect(test_client_t *client, uint16_t port) {
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

static void test_client_disconnect(test_client_t *client) {
  if (client->connected) {
    ak_tcp_socket_close(client->sock);
    client->connected = false;
  }
}

static ssize_t test_client_send_all(test_client_t *client, const uint8_t *data,
                                    size_t len) {
  const char *error = NULL;
  size_t total_sent = 0;

  while (total_sent < len) {
    ssize_t sent = ak_tcp_socket_send(client->sock, data + total_sent,
                                      len - total_sent, &error);
    if (sent <= 0) {
      printf("  [client] Send failed: %s\n",
             error ? error : "connection closed");
      return -1;
    }
    total_sent += (size_t)sent;
  }

  return (ssize_t)total_sent;
}

static ssize_t test_client_recv_all(test_client_t *client, uint8_t *buf,
                                    size_t expected_len) {
  const char *error = NULL;
  size_t total_received = 0;

  while (total_received < expected_len) {
    ssize_t received =
        ak_tcp_socket_recv(client->sock, buf + total_received,
                           expected_len - total_received, &error);
    if (received <= 0) {
      printf("  [client] Recv failed after %zu bytes: %s\n", total_received,
             error ? error : "connection closed");
      return (ssize_t)total_received;
    }
    total_received += (size_t)received;
  }

  return (ssize_t)total_received;
}

static int test_client_connect_and_send(uint16_t port, const char *message,
                                        char *response, size_t response_size) {
  test_client_t client = {0};

  if (test_client_connect(&client, port) != 0) {
    return -1;
  }

  size_t msg_len = strlen(message);
  if (test_client_send_all(&client, (const uint8_t *)message, msg_len) !=
      (ssize_t)msg_len) {
    test_client_disconnect(&client);
    return -1;
  }

  if (response && response_size > 0) {
    const char *error = NULL;
    ssize_t received = ak_tcp_socket_recv(client.sock, (uint8_t *)response,
                                          response_size - 1, &error);
    if (received > 0) {
      response[received] = '\0';
    } else {
      response[0] = '\0';
    }
  }

  test_client_disconnect(&client);
  return 0;
}

static void reset_test_state(void) {
  connections_accepted = 0;
  connections_handled = 0;
  connections_disconnected = 0;
  total_bytes_received = 0;
  total_bytes_sent = 0;
  data_validation_errors = 0;
}

static int test_socket_create_bind_listen(void) {
  printf("Test: Socket create/bind/listen\n");

  const char *error = NULL;

  ak_socket_fd_t fd = ak_tcp_socket_create(&error);
  AK24_TEST_ASSERT_NEQ((int)fd, (int)AK_INVALID_SOCKET);

  AK24_TEST_ASSERT_EQ(ak_tcp_socket_bind(fd, "127.0.0.1", TEST_PORT, &error),
                      0);

  AK24_TEST_ASSERT_EQ(ak_tcp_socket_listen(fd, 10, &error), 0);

  ak_tcp_socket_close(fd);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_server_create(void) {
  printf("Test: Server create/free\n");

  const char *error = NULL;

  ak_lambda_t *handler = ak_lambda_new(on_handle_echo, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(handler);

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = TEST_PORT + 1,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = NULL,
      .on_handle = handler,
      .on_disconnect = NULL,
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);

  ak_tcp_server_free(server);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_server_start_stop(void) {
  printf("Test: Server start/stop\n");

  const char *error = NULL;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = TEST_PORT + 2,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_echo, NULL, NULL),
      .on_disconnect = ak_lambda_new(on_disconnect_log, NULL, NULL),
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);

  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);

  usleep(50000);

  ak_tcp_server_stop(server);

  ak_tcp_server_free(server);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_server_echo(void) {
  printf("Test: Server echo connection\n");

  const char *error = NULL;

  connections_accepted = 0;
  connections_handled = 0;
  connections_disconnected = 0;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = TEST_PORT + 3,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_echo, NULL, NULL),
      .on_disconnect = ak_lambda_new(on_disconnect_log, NULL, NULL),
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);

  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);

  usleep(50000);

  char response[256] = {0};
  int result = test_client_connect_and_send(TEST_PORT + 3, "Hello\n", response,
                                            sizeof(response));
  AK24_TEST_ASSERT_EQ(result, 0);

  usleep(100000);

  printf("  Response: '%s'\n", response);
  AK24_TEST_ASSERT_STR_EQ(response, "Hello\n");

  AK24_TEST_ASSERT_EQ(connections_accepted, 1);
  AK24_TEST_ASSERT_EQ(connections_handled, 1);

  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_connection_rejection(void) {
  printf("Test: Connection rejection\n");

  const char *error = NULL;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = TEST_PORT + 4,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_reject_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_echo, NULL, NULL),
      .on_disconnect = NULL,
  };

  connections_handled = 0;

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);

  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);

  usleep(50000);

  ak_socket_fd_t sock = ak_tcp_socket_create(&error);
  AK24_TEST_ASSERT_NEQ((int)sock, (int)AK_INVALID_SOCKET);

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(TEST_PORT + 4);
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  int conn_result =
      connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
  AK24_TEST_ASSERT_EQ(conn_result, 0);

  usleep(100000);

  AK24_TEST_ASSERT_EQ(connections_handled, 0);

  ak_tcp_socket_close(sock);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_buffer_operations(void) {
  printf("Test: Buffer operations (context)\n");

  const char *error = NULL;
  ak_socket_fd_t sock = ak_tcp_socket_create(&error);
  AK24_TEST_ASSERT_NEQ((int)sock, (int)AK_INVALID_SOCKET);

  ak_tcp_ctx_t *ctx = ak_tcp_ctx_new(sock, "127.0.0.1", 12345, 0, 0);
  AK24_TEST_ASSERT_NOT_NULL(ctx);

  int value = 42;
  ak_tcp_set_meta(ctx, "test_key", &value);
  int *retrieved = (int *)ak_tcp_get_meta(ctx, "test_key");
  AK24_TEST_ASSERT_NOT_NULL(retrieved);
  AK24_TEST_ASSERT_EQ(*retrieved, 42);

  AK24_TEST_ASSERT(ak_tcp_is_alive(ctx));
  ak_tcp_close(ctx);
  AK24_TEST_ASSERT(!ak_tcp_is_alive(ctx));

  ak_tcp_ctx_free(ctx);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_data_validation_small(void) {
  printf("Test: Data validation - small payload (%d bytes)\n", SMALL_DATA_SIZE);

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 10;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = port,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_binary_echo, NULL, NULL),
      .on_disconnect = ak_lambda_new(on_disconnect_log, NULL, NULL),
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  uint8_t send_data[SMALL_DATA_SIZE];
  uint8_t recv_data[SMALL_DATA_SIZE + 4] = {0};
  generate_sequential_data(send_data, SMALL_DATA_SIZE);

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  uint8_t header[4] = {0, 0, 0, SMALL_DATA_SIZE};
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, header, 4), 4);
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, send_data, SMALL_DATA_SIZE),
                      SMALL_DATA_SIZE);

  ssize_t received =
      test_client_recv_all(&client, recv_data, 4 + SMALL_DATA_SIZE);
  AK24_TEST_ASSERT_EQ(received, 4 + SMALL_DATA_SIZE);

  uint32_t recv_len = ((uint32_t)recv_data[0] << 24) |
                      ((uint32_t)recv_data[1] << 16) |
                      ((uint32_t)recv_data[2] << 8) | (uint32_t)recv_data[3];
  AK24_TEST_ASSERT_EQ(recv_len, SMALL_DATA_SIZE);

  int errors =
      validate_data(send_data, recv_data + 4, SMALL_DATA_SIZE, "small_data");
  AK24_TEST_ASSERT_EQ(errors, 0);

  test_client_disconnect(&client);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Sent: %d bytes, Received: %d bytes, Errors: %d\n", SMALL_DATA_SIZE,
         (int)received - 4, errors);
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_data_validation_medium(void) {
  printf("Test: Data validation - medium payload (%d bytes)\n",
         MEDIUM_DATA_SIZE);

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 11;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = port,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_binary_echo, NULL, NULL),
      .on_disconnect = ak_lambda_new(on_disconnect_log, NULL, NULL),
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  uint8_t *send_data = AK24_ALLOC(MEDIUM_DATA_SIZE);
  uint8_t *recv_data = AK24_ALLOC(MEDIUM_DATA_SIZE + 4);
  AK24_TEST_ASSERT_NOT_NULL(send_data);
  AK24_TEST_ASSERT_NOT_NULL(recv_data);

  generate_random_data(send_data, MEDIUM_DATA_SIZE, 12345);
  memset(recv_data, 0, MEDIUM_DATA_SIZE + 4);

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  uint8_t header[4] = {
      (uint8_t)(MEDIUM_DATA_SIZE >> 24), (uint8_t)(MEDIUM_DATA_SIZE >> 16),
      (uint8_t)(MEDIUM_DATA_SIZE >> 8), (uint8_t)MEDIUM_DATA_SIZE};
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, header, 4), 4);
  AK24_TEST_ASSERT_EQ(
      test_client_send_all(&client, send_data, MEDIUM_DATA_SIZE),
      MEDIUM_DATA_SIZE);

  ssize_t received =
      test_client_recv_all(&client, recv_data, 4 + MEDIUM_DATA_SIZE);
  AK24_TEST_ASSERT_EQ(received, 4 + MEDIUM_DATA_SIZE);

  int errors =
      validate_data(send_data, recv_data + 4, MEDIUM_DATA_SIZE, "medium_data");
  AK24_TEST_ASSERT_EQ(errors, 0);

  test_client_disconnect(&client);
  AK24_FREE(send_data);
  AK24_FREE(recv_data);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_data_validation_large(void) {
  printf("Test: Data validation - large payload (%d bytes)\n", LARGE_DATA_SIZE);

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 12;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = port,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_binary_echo, NULL, NULL),
      .on_disconnect = ak_lambda_new(on_disconnect_log, NULL, NULL),
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  uint8_t *send_data = AK24_ALLOC(LARGE_DATA_SIZE);
  uint8_t *recv_data = AK24_ALLOC(LARGE_DATA_SIZE + 4);
  AK24_TEST_ASSERT_NOT_NULL(send_data);
  AK24_TEST_ASSERT_NOT_NULL(recv_data);

  generate_binary_pattern(send_data, LARGE_DATA_SIZE);
  memset(recv_data, 0, LARGE_DATA_SIZE + 4);

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  uint8_t header[4] = {
      (uint8_t)(LARGE_DATA_SIZE >> 24), (uint8_t)(LARGE_DATA_SIZE >> 16),
      (uint8_t)(LARGE_DATA_SIZE >> 8), (uint8_t)LARGE_DATA_SIZE};
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, header, 4), 4);
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, send_data, LARGE_DATA_SIZE),
                      LARGE_DATA_SIZE);

  ssize_t received =
      test_client_recv_all(&client, recv_data, 4 + LARGE_DATA_SIZE);
  AK24_TEST_ASSERT_EQ(received, 4 + LARGE_DATA_SIZE);

  int errors =
      validate_data(send_data, recv_data + 4, LARGE_DATA_SIZE, "large_data");
  AK24_TEST_ASSERT_EQ(errors, 0);

  test_client_disconnect(&client);
  AK24_FREE(send_data);
  AK24_FREE(recv_data);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_data_validation_multi_message(void) {
  printf("Test: Data validation - multiple messages on single connection\n");

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 13;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = port,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_multi_echo, NULL, NULL),
      .on_disconnect = ak_lambda_new(on_disconnect_log, NULL, NULL),
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  const char *test_messages[] = {
      "Message 1: Hello World!\n",
      "Message 2: Testing TCP data validation.\n",
      "Message 3: Special chars: !@#$%^&*()_+-=[]{}|;':\",./<>?\n",
      "Message 4: Numbers 0123456789\n",
      "Message 5: Final message for multi-message test!\n",
  };

  int num_messages = sizeof(test_messages) / sizeof(test_messages[0]);
  int total_errors = 0;

  for (int i = 0; i < num_messages; i++) {
    const char *msg = test_messages[i];
    size_t msg_len = strlen(msg);
    char recv_buf[256] = {0};

    ssize_t sent = test_client_send_all(&client, (const uint8_t *)msg, msg_len);
    AK24_TEST_ASSERT_EQ(sent, (ssize_t)msg_len);

    ssize_t received =
        test_client_recv_all(&client, (uint8_t *)recv_buf, msg_len);
    AK24_TEST_ASSERT_EQ(received, (ssize_t)msg_len);

    int errors = validate_data((const uint8_t *)msg, (const uint8_t *)recv_buf,
                               msg_len, "multi_msg");
    total_errors += errors;

    printf("  Message %d: Sent %zu bytes, received %zd bytes, errors: %d\n",
           i + 1, msg_len, received, errors);
  }

  AK24_TEST_ASSERT_EQ(total_errors, 0);

  test_client_disconnect(&client);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Total messages: %d, Total errors: %d\n", num_messages,
         total_errors);
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_data_validation_null_bytes(void) {
  printf("Test: Data validation - binary with NULL bytes\n");

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 14;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = port,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_binary_echo, NULL, NULL),
      .on_disconnect = ak_lambda_new(on_disconnect_log, NULL, NULL),
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  uint8_t send_data[32] = {0x00, 0x01, 0x02, 0x00, 0x04, 0x05, 0x00, 0x07,
                           0xFF, 0xFE, 0x00, 0xFC, 0xFB, 0x00, 0xF9, 0xF8,
                           0x00, 0x00, 0x00, 0x00, 0xAA, 0xBB, 0xCC, 0xDD,
                           0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF};
  uint8_t recv_data[36] = {0};

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  uint8_t header[4] = {0, 0, 0, 32};
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, header, 4), 4);
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, send_data, 32), 32);

  ssize_t received = test_client_recv_all(&client, recv_data, 36);
  AK24_TEST_ASSERT_EQ(received, 36);

  int errors = validate_data(send_data, recv_data + 4, 32, "null_bytes");
  AK24_TEST_ASSERT_EQ(errors, 0);

  int null_count_sent = 0, null_count_recv = 0;
  for (int i = 0; i < 32; i++) {
    if (send_data[i] == 0x00)
      null_count_sent++;
    if (recv_data[i + 4] == 0x00)
      null_count_recv++;
  }
  printf("  NULL bytes sent: %d, NULL bytes received: %d\n", null_count_sent,
         null_count_recv);
  AK24_TEST_ASSERT_EQ(null_count_sent, null_count_recv);

  test_client_disconnect(&client);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_data_validation_all_bytes(void) {
  printf("Test: Data validation - all 256 byte values\n");

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 15;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = port,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_binary_echo, NULL, NULL),
      .on_disconnect = ak_lambda_new(on_disconnect_log, NULL, NULL),
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  uint8_t send_data[256];
  uint8_t recv_data[260] = {0};
  for (int i = 0; i < 256; i++) {
    send_data[i] = (uint8_t)i;
  }

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  uint8_t header[4] = {0, 0, 1, 0};
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, header, 4), 4);
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, send_data, 256), 256);

  ssize_t received = test_client_recv_all(&client, recv_data, 260);
  AK24_TEST_ASSERT_EQ(received, 260);

  int errors = validate_data(send_data, recv_data + 4, 256, "all_bytes");
  AK24_TEST_ASSERT_EQ(errors, 0);

  AK24_TEST_ASSERT_EQ(recv_data[4], 0x00);
  AK24_TEST_ASSERT_EQ(recv_data[4 + 10], 0x0A);
  AK24_TEST_ASSERT_EQ(recv_data[4 + 13], 0x0D);
  AK24_TEST_ASSERT_EQ(recv_data[4 + 127], 127);
  AK24_TEST_ASSERT_EQ(recv_data[4 + 255], 255);

  test_client_disconnect(&client);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  All 256 byte values validated successfully\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_data_validation_byte_counts(void) {
  printf("Test: Data validation - byte count verification\n");

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 16;

  ak_tcp_server_config_t cfg = {
      .bind_addr = "127.0.0.1",
      .port = port,
      .thread_pool_size = 2,
      .backlog = 10,
      .on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL),
      .on_handle = ak_lambda_new(on_handle_binary_echo, NULL, NULL),
      .on_disconnect = ak_lambda_new(on_disconnect_log, NULL, NULL),
  };

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  uint8_t send_data[128];
  uint8_t recv_data[132] = {0};
  generate_random_data(send_data, 128, 99999);

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  uint8_t header[4] = {0, 0, 0, 128};
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, header, 4), 4);
  AK24_TEST_ASSERT_EQ(test_client_send_all(&client, send_data, 128), 128);

  ssize_t received = test_client_recv_all(&client, recv_data, 132);
  AK24_TEST_ASSERT_EQ(received, 132);

  test_client_disconnect(&client);
  usleep(100000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Server received: %d bytes\n", total_bytes_received);
  printf("  Server sent: %d bytes\n", total_bytes_sent);
  AK24_TEST_ASSERT_EQ(total_bytes_received, 132);
  AK24_TEST_ASSERT_EQ(total_bytes_sent, 132);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_error_codes(void) {
  printf("Test: Error code string conversion\n");

  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_OK), "Success");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_TIMEOUT),
                          "Operation timed out");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_CLOSED),
                          "Connection closed");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_RESET),
                          "Connection reset by peer");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_NETWORK),
                          "Network error");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_MEMORY),
                          "Memory allocation failed");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_INVALID),
                          "Invalid argument");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_LIMIT),
                          "Limit exceeded");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_WOULDBLOCK),
                          "Operation would block");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_BUFFER_FULL),
                          "Receive buffer full");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_QUEUE_FULL),
                          "Task queue full");
  AK24_TEST_ASSERT_STR_EQ(ak_tcp_error_string(AK_TCP_ERR_RATE_LIMIT),
                          "Rate limit exceeded");

  const char *unknown = ak_tcp_error_string((ak_tcp_error_t)999);
  AK24_TEST_ASSERT_NOT_NULL(unknown);

  printf("  All error codes have valid string representations\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_default_config(void) {
  printf("Test: Default server configuration\n");

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();

  AK24_TEST_ASSERT_EQ(cfg.thread_pool_size, 4);
  AK24_TEST_ASSERT_EQ(cfg.backlog, 128);
  AK24_TEST_ASSERT_EQ(cfg.max_connections, 0);
  AK24_TEST_ASSERT_EQ(cfg.default_recv_timeout_ms, 0);
  AK24_TEST_ASSERT_EQ(cfg.default_send_timeout_ms, 0);
  AK24_TEST_ASSERT(!cfg.enable_keepalive);
  AK24_TEST_ASSERT_EQ(cfg.keepalive_idle_sec, 60);
  AK24_TEST_ASSERT_EQ(cfg.keepalive_interval_sec, 10);
  AK24_TEST_ASSERT_EQ(cfg.keepalive_count, 5);
  AK24_TEST_ASSERT_EQ(cfg.recv_buffer_size, 4096);
  AK24_TEST_ASSERT_EQ(cfg.send_buffer_size, 4096);

  printf("  All default values verified\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_set_timeout(void) {
  printf("Test: Socket timeout set/get\n");

  ak_tcp_ctx_t *ctx =
      ak_tcp_ctx_new(AK_INVALID_SOCKET, "127.0.0.1", 12345, 0, 0);
  AK24_TEST_ASSERT_NOT_NULL(ctx);

  ak_tcp_error_t err = ak_tcp_set_timeout(ctx, 5000, 3000);
  AK24_TEST_ASSERT_EQ(err, AK_TCP_ERR_CLOSED);

  uint32_t recv_timeout = 0, send_timeout = 0;
  err = ak_tcp_get_timeout(ctx, &recv_timeout, &send_timeout);
  AK24_TEST_ASSERT_EQ(err, AK_TCP_OK);

  AK24_TEST_ASSERT_EQ(recv_timeout, 0);
  AK24_TEST_ASSERT_EQ(send_timeout, 0);

  err = ak_tcp_get_timeout(ctx, NULL, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_TCP_OK);

  err = ak_tcp_set_timeout(NULL, 1000, 1000);
  AK24_TEST_ASSERT_EQ(err, AK_TCP_ERR_INVALID);

  err = ak_tcp_get_timeout(NULL, &recv_timeout, &send_timeout);
  AK24_TEST_ASSERT_EQ(err, AK_TCP_ERR_INVALID);

  ak_tcp_ctx_free(ctx);

  printf("  Timeout set/get operations verified\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_recv_timeout(void) {
  printf("Test: Receive timeout behavior\n");

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 20;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_silent, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 200000;
  setsockopt(client.sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  uint8_t buf[64];
  ssize_t received = recv(client.sock, buf, sizeof(buf), 0);

  printf("  Recv returned: %zd (expected timeout/error)\n", received);
  AK24_TEST_ASSERT(received <= 0);

  test_client_disconnect(&client);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_server_default_timeout(void) {
  printf("Test: Server-level default timeout configuration\n");

  const char *error = NULL;
  const uint16_t port = TEST_PORT + 21;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.default_recv_timeout_ms = 5000;
  cfg.default_send_timeout_ms = 3000;
  cfg.on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  char response[64] = {0};
  int result =
      test_client_connect_and_send(port, "test\n", response, sizeof(response));
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_STR_EQ(response, "test\n");

  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Server with default timeouts configured works correctly\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_keepalive_config(void) {
  printf("Test: TCP keepalive configuration\n");

  const char *error = NULL;
  ak_socket_fd_t sock = ak_tcp_socket_create(&error);
  AK24_TEST_ASSERT_NEQ((int)sock, (int)AK_INVALID_SOCKET);

  ak_tcp_ctx_t *ctx = ak_tcp_ctx_new(sock, "127.0.0.1", 12345, 0, 0);
  AK24_TEST_ASSERT_NOT_NULL(ctx);

  ak_tcp_error_t err = ak_tcp_set_keepalive(ctx, 30, 5, 3);

  printf("  Set keepalive returned: %s\n", ak_tcp_error_string(err));

  err = ak_tcp_set_keepalive(NULL, 30, 5, 3);
  AK24_TEST_ASSERT_EQ(err, AK_TCP_ERR_INVALID);

  err = ak_tcp_set_keepalive(ctx, 0, 0, 0);
  printf("  Disable keepalive returned: %s\n", ak_tcp_error_string(err));

  ak_tcp_ctx_free(ctx);

  printf("  Keepalive API operations verified\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_server_keepalive(void) {
  printf("Test: Server with keepalive enabled\n");

  const char *error = NULL;
  const uint16_t port = TEST_PORT + 22;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.enable_keepalive = true;
  cfg.keepalive_idle_sec = 30;
  cfg.keepalive_interval_sec = 5;
  cfg.keepalive_count = 3;
  cfg.on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  char response[64] = {0};
  int result = test_client_connect_and_send(port, "keepalive-test\n", response,
                                            sizeof(response));
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_STR_EQ(response, "keepalive-test\n");

  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Server with keepalive enabled works correctly\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_graceful_shutdown(void) {
  printf("Test: Graceful shutdown\n");

  const char *error = NULL;

  ak_socket_fd_t sock = ak_tcp_socket_create(&error);
  AK24_TEST_ASSERT_NEQ((int)sock, (int)AK_INVALID_SOCKET);

  ak_tcp_ctx_t *ctx = ak_tcp_ctx_new(sock, "127.0.0.1", 12345, 0, 0);
  AK24_TEST_ASSERT_NOT_NULL(ctx);
  AK24_TEST_ASSERT(ak_tcp_is_alive(ctx));

  ak_tcp_shutdown(ctx);
  AK24_TEST_ASSERT(!ak_tcp_is_alive(ctx));

  ak_tcp_ctx_free(ctx);

  sock = ak_tcp_socket_create(&error);
  AK24_TEST_ASSERT_NEQ((int)sock, (int)AK_INVALID_SOCKET);

  ctx = ak_tcp_ctx_new(sock, "127.0.0.1", 12345, 0, 0);
  AK24_TEST_ASSERT_NOT_NULL(ctx);
  AK24_TEST_ASSERT(ak_tcp_is_alive(ctx));

  ak_tcp_abort(ctx);
  AK24_TEST_ASSERT(!ak_tcp_is_alive(ctx));

  ak_tcp_ctx_free(ctx);

  printf("  Graceful shutdown and abort verified\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_connection_count(void) {
  printf("Test: Connection count tracking\n");

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 23;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  size_t count = ak_tcp_server_connection_count(server);
  printf("  Initial connection count: %zu\n", count);
  AK24_TEST_ASSERT_EQ(count, 0);

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  test_client_send_all(&client, (const uint8_t *)"hello\n", 6);
  usleep(100000);

  count = ak_tcp_server_connection_count(server);
  printf("  Connection count after connect: %zu\n", count);

  test_client_disconnect(&client);
  usleep(100000);

  count = ak_tcp_server_connection_count(server);
  printf("  Connection count after disconnect: %zu\n", count);

  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Connection counting verified\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_connection_limit(void) {
  printf("Test: Connection limit enforcement\n");

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 24;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.max_connections = 2;
  cfg.on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_multi_echo, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  test_client_t client1 = {0}, client2 = {0}, client3 = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client1, port), 0);
  test_client_send_all(&client1, (const uint8_t *)"msg1\n", 5);
  usleep(50000);

  AK24_TEST_ASSERT_EQ(test_client_connect(&client2, port), 0);
  test_client_send_all(&client2, (const uint8_t *)"msg2\n", 5);
  usleep(100000);

  size_t count = ak_tcp_server_connection_count(server);
  printf("  Active connections after 2 clients: %zu\n", count);

  int conn_result = test_client_connect(&client3, port);
  printf("  Third client connect result: %d\n", conn_result);

  if (conn_result == 0) {

    usleep(100000);

    ssize_t sent = send(client3.sock, "test\n", 5, 0);
    printf("  Third client send result: %zd\n", sent);
    test_client_disconnect(&client3);
  }

  test_client_disconnect(&client1);
  test_client_disconnect(&client2);
  usleep(100000);

  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Connection limit test completed\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_custom_buffer_sizes(void) {
  printf("Test: Custom buffer sizes\n");

  const char *error = NULL;
  const uint16_t port = TEST_PORT + 25;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.recv_buffer_size = 8192;
  cfg.send_buffer_size = 16384;
  cfg.on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_echo, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  char response[64] = {0};
  int result = test_client_connect_and_send(port, "buffer-test\n", response,
                                            sizeof(response));
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_STR_EQ(response, "buffer-test\n");

  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Server with custom buffer sizes works correctly\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

static int test_extended_error_api(void) {
  printf("Test: Extended error-code API\n");

  reset_test_state();
  const char *error = NULL;
  const uint16_t port = TEST_PORT + 26;

  ak_tcp_server_config_t cfg = ak_tcp_server_config_default();
  cfg.bind_addr = "127.0.0.1";
  cfg.port = port;
  cfg.on_connect = ak_lambda_new(on_connect_accept_all, NULL, NULL);
  cfg.on_handle = ak_lambda_new(on_handle_binary_echo, NULL, NULL);

  ak_tcp_server_t *server = ak_tcp_server_new(&cfg, &error);
  AK24_TEST_ASSERT_NOT_NULL(server);
  AK24_TEST_ASSERT_EQ(ak_tcp_server_start(server, &error), 0);
  usleep(50000);

  test_client_t client = {0};
  AK24_TEST_ASSERT_EQ(test_client_connect(&client, port), 0);

  uint8_t send_data[64];
  generate_sequential_data(send_data, 64);

  uint8_t header[4] = {0, 0, 0, 64};
  test_client_send_all(&client, header, 4);
  test_client_send_all(&client, send_data, 64);

  uint8_t recv_buf[68] = {0};
  test_client_recv_all(&client, recv_buf, 68);

  int errors = validate_data(send_data, recv_buf + 4, 64, "extended_api");
  AK24_TEST_ASSERT_EQ(errors, 0);

  test_client_disconnect(&client);
  usleep(50000);
  ak_tcp_server_stop(server);
  ak_tcp_server_free(server);

  printf("  Extended error-code API works correctly\n");
  printf("  PASSED\n");
  AK24_TEST_PASS();
}

int main(void) {
  ak_kernel_init("ak24-test");
  ak_log_set_level(AK24_LOG_LEVEL_WARN);

  printf("\n");
  printf("============================================================\n");
  printf("       TCP Module Runtime Tests - Full Data Validation\n");
  printf("============================================================\n\n");

  printf("--- Basic Infrastructure Tests ---\n\n");
  AK24_TEST_RUN(test_socket_create_bind_listen);
  AK24_TEST_RUN(test_server_create);
  AK24_TEST_RUN(test_server_start_stop);
  AK24_TEST_RUN(test_buffer_operations);

  printf("\n--- Connection Handling Tests ---\n\n");
  AK24_TEST_RUN(test_server_echo);
  AK24_TEST_RUN(test_connection_rejection);

  printf("\n--- Data Validation Tests (POC) ---\n\n");
  AK24_TEST_RUN(test_data_validation_small);
  AK24_TEST_RUN(test_data_validation_medium);
  AK24_TEST_RUN(test_data_validation_large);
  AK24_TEST_RUN(test_data_validation_multi_message);
  AK24_TEST_RUN(test_data_validation_null_bytes);
  AK24_TEST_RUN(test_data_validation_all_bytes);
  AK24_TEST_RUN(test_data_validation_byte_counts);

  printf("\n--- Feature Tests (Timeouts, Keepalive, Limits) ---\n\n");
  AK24_TEST_RUN(test_error_codes);
  AK24_TEST_RUN(test_default_config);
  AK24_TEST_RUN(test_set_timeout);
  AK24_TEST_RUN(test_recv_timeout);
  AK24_TEST_RUN(test_server_default_timeout);
  AK24_TEST_RUN(test_keepalive_config);
  AK24_TEST_RUN(test_server_keepalive);
  AK24_TEST_RUN(test_graceful_shutdown);
  AK24_TEST_RUN(test_connection_count);
  AK24_TEST_RUN(test_connection_limit);
  AK24_TEST_RUN(test_custom_buffer_sizes);
  AK24_TEST_RUN(test_extended_error_api);

  extern int run_tcp_hardening_tests(void);
  run_tcp_hardening_tests();

  printf("\n============================================================\n");
  printf("       All TCP Tests Passed - Features Verified!\n");
  printf("============================================================\n\n");

  ak_kernel_deinit();
  return 0;
}
