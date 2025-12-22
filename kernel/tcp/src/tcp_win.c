/**
 * @file tcp_win.c
 * @brief Windows socket operations for TCP module
 *
 * Implements platform-specific socket functions for Windows using Winsock2.
 */

#ifdef AK24_PLATFORM_WINDOWS

#include "tcp_internal.h"
#include <mstcpip.h> // For SIO_KEEPALIVE_VALS and tcp_keepalive
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// Track WSA initialization
static int wsa_initialized = 0;
static WSADATA wsa_data;

void ak_tcp_platform_init(void) {
  if (!wsa_initialized) {
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result == 0) {
      wsa_initialized = 1;
    }
  }
}

void ak_tcp_platform_deinit(void) {
  if (wsa_initialized) {
    WSACleanup();
    wsa_initialized = 0;
  }
}

static const char *wsa_error_string(int error_code) {
  static char error_buf[256];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 error_buf, sizeof(error_buf), NULL);
  return error_buf;
}

static int ak_tcp_detect_addr_family(const char *addr) {
  if (addr == NULL || strcmp(addr, "0.0.0.0") == 0) {
    return AF_INET;
  }
  if (strcmp(addr, "::") == 0 || strcmp(addr, "::0") == 0) {
    return AF_INET6;
  }
  if (strchr(addr, ':') != NULL) {
    return AF_INET6;
  }
  return AF_INET;
}

ak_socket_fd_t ak_tcp_socket_create(const char **error) {
  return ak_tcp_socket_create_for_addr(NULL, error);
}

ak_socket_fd_t ak_tcp_socket_create_for_addr(const char *addr,
                                              const char **error) {
  int family = ak_tcp_detect_addr_family(addr);
  SOCKET sock = socket(family, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return AK_INVALID_SOCKET;
  }

  BOOL optval = TRUE;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval,
                 sizeof(optval)) == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    closesocket(sock);
    return AK_INVALID_SOCKET;
  }

  if (family == AF_INET6) {
    DWORD v6only = 0;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&v6only,
                   sizeof(v6only)) == SOCKET_ERROR) {
      if (error) {
        *error = wsa_error_string(WSAGetLastError());
      }
      closesocket(sock);
      return AK_INVALID_SOCKET;
    }
  }

  return sock;
}

int ak_tcp_socket_bind(ak_socket_fd_t fd, const char *addr, uint16_t port,
                       const char **error) {
  int family = ak_tcp_detect_addr_family(addr);

  if (family == AF_INET6) {
    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);

    if (addr == NULL || strcmp(addr, "::") == 0 || strcmp(addr, "::0") == 0) {
      server_addr.sin6_addr = in6addr_any;
    } else {
      if (inet_pton(AF_INET6, addr, &server_addr.sin6_addr) <= 0) {
        if (error) {
          *error = "Invalid IPv6 address format";
        }
        return -1;
      }
    }

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
        SOCKET_ERROR) {
      if (error) {
        *error = wsa_error_string(WSAGetLastError());
      }
      return -1;
    }
  } else {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (addr == NULL || strcmp(addr, "0.0.0.0") == 0) {
      server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
      if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
        if (error) {
          *error = "Invalid IPv4 address format";
        }
        return -1;
      }
    }

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
        SOCKET_ERROR) {
      if (error) {
        *error = wsa_error_string(WSAGetLastError());
      }
      return -1;
    }
  }

  return 0;
}

int ak_tcp_socket_listen(ak_socket_fd_t fd, int backlog, const char **error) {
  if (backlog <= 0) {
    backlog = 128;
  }

  if (listen(fd, backlog) == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }

  return 0;
}

ak_socket_fd_t ak_tcp_socket_accept(ak_socket_fd_t fd, char *remote_ip,
                                    uint16_t *remote_port, const char **error) {
  struct sockaddr_storage client_addr;
  int client_len = sizeof(client_addr);

  SOCKET client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
  if (client_fd == INVALID_SOCKET) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return AK_INVALID_SOCKET;
  }

  if (client_addr.ss_family == AF_INET6) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&client_addr;
    if (remote_ip) {
      if (IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr)) {
        struct in_addr addr4;
        memcpy(&addr4, &addr6->sin6_addr.s6_addr[12], 4);
        inet_ntop(AF_INET, &addr4, remote_ip, 46);
      } else {
        inet_ntop(AF_INET6, &addr6->sin6_addr, remote_ip, 46);
      }
    }
    if (remote_port) {
      *remote_port = ntohs(addr6->sin6_port);
    }
  } else {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)&client_addr;
    if (remote_ip) {
      inet_ntop(AF_INET, &addr4->sin_addr, remote_ip, 46);
    }
    if (remote_port) {
      *remote_port = ntohs(addr4->sin_port);
    }
  }

  return client_fd;
}

ssize_t ak_tcp_socket_send(ak_socket_fd_t fd, const uint8_t *data, size_t len,
                           const char **error) {
  int sent = send(fd, (const char *)data, (int)len, 0);
  if (sent == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }
  return (ssize_t)sent;
}

ssize_t ak_tcp_socket_recv(ak_socket_fd_t fd, uint8_t *buffer, size_t len,
                           const char **error) {
  int received = recv(fd, (char *)buffer, (int)len, 0);
  if (received == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }
  return (ssize_t)received;
}

void ak_tcp_socket_close(ak_socket_fd_t fd) {
  if (fd != AK_INVALID_SOCKET) {
    shutdown(fd, SD_BOTH);
    closesocket(fd);
  }
}

int ak_tcp_socket_set_nonblocking(ak_socket_fd_t fd, bool nonblocking,
                                  const char **error) {
  u_long mode = nonblocking ? 1 : 0;
  if (ioctlsocket(fd, FIONBIO, &mode) == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }
  return 0;
}

int ak_tcp_socket_set_reuseaddr(ak_socket_fd_t fd, bool enable,
                                const char **error) {
  BOOL optval = enable ? TRUE : FALSE;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval,
                 sizeof(optval)) == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }
  return 0;
}

int ak_tcp_socket_set_nodelay(ak_socket_fd_t fd, bool enable,
                              const char **error) {
  BOOL optval = enable ? TRUE : FALSE;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&optval,
                 sizeof(optval)) == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }
  return 0;
}

int ak_tcp_socket_set_timeout(ak_socket_fd_t fd, uint32_t recv_timeout_ms,
                              uint32_t send_timeout_ms, const char **error) {
  DWORD timeout;

  // Set receive timeout
  timeout = (DWORD)recv_timeout_ms;
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout,
                 sizeof(timeout)) == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }

  // Set send timeout
  timeout = (DWORD)send_timeout_ms;
  if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout,
                 sizeof(timeout)) == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }

  return 0;
}

int ak_tcp_socket_get_timeout(ak_socket_fd_t fd, uint32_t *recv_timeout_ms,
                              uint32_t *send_timeout_ms, const char **error) {
  DWORD timeout;
  int len = sizeof(timeout);

  if (recv_timeout_ms) {
    if (getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, &len) ==
        SOCKET_ERROR) {
      if (error) {
        *error = wsa_error_string(WSAGetLastError());
      }
      return -1;
    }
    *recv_timeout_ms = (uint32_t)timeout;
  }

  if (send_timeout_ms) {
    len = sizeof(timeout);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, &len) ==
        SOCKET_ERROR) {
      if (error) {
        *error = wsa_error_string(WSAGetLastError());
      }
      return -1;
    }
    *send_timeout_ms = (uint32_t)timeout;
  }

  return 0;
}

int ak_tcp_socket_set_keepalive(ak_socket_fd_t fd, int idle_sec,
                                int interval_sec, int probe_count,
                                const char **error) {
  (void)probe_count; // Windows doesn't support setting probe count directly

  BOOL optval = (idle_sec > 0) ? TRUE : FALSE;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&optval,
                 sizeof(optval)) == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }

  if (idle_sec <= 0) {
    return 0;
  }

  // Use SIO_KEEPALIVE_VALS ioctl for detailed keepalive settings
  struct tcp_keepalive ka;
  ka.onoff = 1;
  ka.keepalivetime = idle_sec * 1000;         // milliseconds
  ka.keepaliveinterval = interval_sec * 1000; // milliseconds

  DWORD bytes_returned;
  if (WSAIoctl(fd, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), NULL, 0,
               &bytes_returned, NULL, NULL) == SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }

  return 0;
}

int ak_tcp_socket_set_linger(ak_socket_fd_t fd, bool enable, int timeout_sec,
                             const char **error) {
  struct linger lg;
  lg.l_onoff = enable ? 1 : 0;
  lg.l_linger = (u_short)timeout_sec;

  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&lg, sizeof(lg)) ==
      SOCKET_ERROR) {
    if (error) {
      *error = wsa_error_string(WSAGetLastError());
    }
    return -1;
  }
  return 0;
}

int ak_tcp_socket_set_buffers(ak_socket_fd_t fd, size_t recv_size,
                              size_t send_size, const char **error) {
  if (recv_size > 0) {
    int size = (int)recv_size;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *)&size,
                   sizeof(size)) == SOCKET_ERROR) {
      if (error) {
        *error = wsa_error_string(WSAGetLastError());
      }
      return -1;
    }
  }
  if (send_size > 0) {
    int size = (int)send_size;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&size,
                   sizeof(size)) == SOCKET_ERROR) {
      if (error) {
        *error = wsa_error_string(WSAGetLastError());
      }
      return -1;
    }
  }
  return 0;
}

int ak_tcp_socket_poll_read(ak_socket_fd_t fd, int timeout_ms) {
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(fd, &read_fds);

  struct timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  int ret = select(0, &read_fds, NULL, NULL, &tv);
  if (ret == SOCKET_ERROR) {
    return -1;
  }
  if (ret == 0) {
    return 0; // Timeout
  }
  return 1; // Ready
}

void ak_tcp_socket_shutdown(ak_socket_fd_t fd) {
  if (fd != AK_INVALID_SOCKET) {
    shutdown(fd, SD_SEND);
  }
}

bool ak_tcp_socket_peer_closed(ak_socket_fd_t fd) {
  if (fd == AK_INVALID_SOCKET) {
    return true;
  }

  // Use select with 0 timeout to check socket state
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(fd, &read_fds);

  struct timeval tv = {0, 0}; // 0 timeout = poll
  int ret = select(0, &read_fds, NULL, NULL, &tv);

  if (ret < 0) {
    // Error - treat as closed
    return true;
  }

  if (ret == 0) {
    // No events - connection still alive
    return false;
  }

  // There's data to read - peek to check if it's EOF
  if (FD_ISSET(fd, &read_fds)) {
    char buf;
    int n = recv(fd, &buf, 1, MSG_PEEK);
    if (n == 0) {
      // EOF - peer closed
      return true;
    }
    if (n == SOCKET_ERROR) {
      int err = WSAGetLastError();
      if (err != WSAEWOULDBLOCK) {
        // Error - treat as closed
        return true;
      }
    }
  }

  return false;
}

ak_tcp_error_t ak_tcp_map_error(int platform_errno) {
  switch (platform_errno) {
  case 0:
    return AK_TCP_OK;
  case WSAETIMEDOUT:
  case WSAEWOULDBLOCK:
    return AK_TCP_ERR_TIMEOUT;
  case WSAECONNRESET:
    return AK_TCP_ERR_RESET;
  case WSAENOTCONN:
  case WSAECONNREFUSED:
  case WSAECONNABORTED:
    return AK_TCP_ERR_CLOSED;
  case WSA_NOT_ENOUGH_MEMORY:
    return AK_TCP_ERR_MEMORY;
  case WSAEINVAL:
  case WSAEBADF:
    return AK_TCP_ERR_INVALID;
  default:
    return AK_TCP_ERR_NETWORK;
  }
}

#endif // AK24_PLATFORM_WINDOWS
