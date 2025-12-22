/**
 * @file tcp_posix.c
 * @brief POSIX socket operations for TCP module
 *
 * Implements platform-specific socket functions for POSIX systems
 * (Linux, macOS, BSD).
 */

#ifndef AK24_PLATFORM_WINDOWS

#include "tcp_internal.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// Static flag to track SIGPIPE handling
static int sigpipe_handled = 0;

void ak_tcp_platform_init(void) {
  // Ignore SIGPIPE to prevent crashes when writing to closed sockets
  if (!sigpipe_handled) {
    signal(SIGPIPE, SIG_IGN);
    sigpipe_handled = 1;
  }
}

void ak_tcp_platform_deinit(void) {
  // Nothing to clean up on POSIX
}

ak_socket_fd_t ak_tcp_socket_create(const char **error) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return AK_INVALID_SOCKET;
  }

  // Enable address reuse
  int optval = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    close(fd);
    return AK_INVALID_SOCKET;
  }

  return fd;
}

int ak_tcp_socket_bind(ak_socket_fd_t fd, const char *addr, uint16_t port,
                       const char **error) {
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (addr == NULL || strcmp(addr, "0.0.0.0") == 0) {
    server_addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
      if (error) {
        *error = "Invalid address format";
      }
      return -1;
    }
  }

  if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  return 0;
}

int ak_tcp_socket_listen(ak_socket_fd_t fd, int backlog, const char **error) {
  if (backlog <= 0) {
    backlog = 128;
  }

  if (listen(fd, backlog) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  return 0;
}

ak_socket_fd_t ak_tcp_socket_accept(ak_socket_fd_t fd, char *remote_ip,
                                    uint16_t *remote_port, const char **error) {
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
  if (client_fd < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return AK_INVALID_SOCKET;
  }

  // Extract remote address info
  if (remote_ip) {
    inet_ntop(AF_INET, &client_addr.sin_addr, remote_ip, 46);
  }
  if (remote_port) {
    *remote_port = ntohs(client_addr.sin_port);
  }

  return client_fd;
}

ssize_t ak_tcp_socket_send(ak_socket_fd_t fd, const uint8_t *data, size_t len,
                           const char **error) {
  ssize_t sent = send(fd, data, len, MSG_NOSIGNAL);
  if (sent < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }
  return sent;
}

ssize_t ak_tcp_socket_recv(ak_socket_fd_t fd, uint8_t *buffer, size_t len,
                           const char **error) {
  ssize_t received = recv(fd, buffer, len, 0);
  if (received < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }
  return received;
}

void ak_tcp_socket_close(ak_socket_fd_t fd) {
  if (fd != AK_INVALID_SOCKET) {
    shutdown(fd, SHUT_RDWR);
    close(fd);
  }
}

int ak_tcp_socket_set_nonblocking(ak_socket_fd_t fd, bool nonblocking,
                                  const char **error) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  if (nonblocking) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }

  if (fcntl(fd, F_SETFL, flags) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  return 0;
}

int ak_tcp_socket_set_reuseaddr(ak_socket_fd_t fd, bool enable,
                                const char **error) {
  int optval = enable ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }
  return 0;
}

int ak_tcp_socket_set_nodelay(ak_socket_fd_t fd, bool enable,
                              const char **error) {
  int optval = enable ? 1 : 0;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }
  return 0;
}

int ak_tcp_socket_set_timeout(ak_socket_fd_t fd, uint32_t recv_timeout_ms,
                              uint32_t send_timeout_ms, const char **error) {
  struct timeval tv;

  // Set receive timeout
  if (recv_timeout_ms > 0) {
    tv.tv_sec = recv_timeout_ms / 1000;
    tv.tv_usec = (recv_timeout_ms % 1000) * 1000;
  } else {
    tv.tv_sec = 0;
    tv.tv_usec = 0;
  }
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  // Set send timeout
  if (send_timeout_ms > 0) {
    tv.tv_sec = send_timeout_ms / 1000;
    tv.tv_usec = (send_timeout_ms % 1000) * 1000;
  } else {
    tv.tv_sec = 0;
    tv.tv_usec = 0;
  }
  if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  return 0;
}

int ak_tcp_socket_get_timeout(ak_socket_fd_t fd, uint32_t *recv_timeout_ms,
                              uint32_t *send_timeout_ms, const char **error) {
  struct timeval tv;
  socklen_t len = sizeof(tv);

  if (recv_timeout_ms) {
    if (getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, &len) < 0) {
      if (error) {
        *error = strerror(errno);
      }
      return -1;
    }
    *recv_timeout_ms = (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
  }

  if (send_timeout_ms) {
    len = sizeof(tv);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, &len) < 0) {
      if (error) {
        *error = strerror(errno);
      }
      return -1;
    }
    *send_timeout_ms = (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
  }

  return 0;
}

int ak_tcp_socket_set_keepalive(ak_socket_fd_t fd, int idle_sec,
                                int interval_sec, int probe_count,
                                const char **error) {
  int optval;

  // Enable or disable keepalive
  optval = (idle_sec > 0) ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  if (idle_sec <= 0) {
    // Keepalive disabled, we're done
    return 0;
  }

  // Set keepalive parameters
#ifdef __APPLE__
  // macOS uses TCP_KEEPALIVE instead of TCP_KEEPIDLE
  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle_sec, sizeof(idle_sec)) <
      0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }
#else
  // Linux uses TCP_KEEPIDLE
  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle_sec, sizeof(idle_sec)) <
      0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }
#endif

  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval_sec,
                 sizeof(interval_sec)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &probe_count,
                 sizeof(probe_count)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  return 0;
}

void ak_tcp_socket_shutdown(ak_socket_fd_t fd) {
  if (fd != AK_INVALID_SOCKET) {
    shutdown(fd, SHUT_WR);
  }
}

bool ak_tcp_socket_peer_closed(ak_socket_fd_t fd) {
  if (fd == AK_INVALID_SOCKET) {
    return true;
  }

  // Use poll with 0 timeout to check if there's data or if peer closed
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;

  int ret = poll(&pfd, 1, 0);
  if (ret < 0) {
    // Error - treat as closed
    return true;
  }

  if (ret == 0) {
    // No events - connection still alive, just no data
    return false;
  }

  // Check for hangup or error
  if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
    return true;
  }

  // There's data to read - peek to check if it's EOF
  if (pfd.revents & POLLIN) {
    char buf;
    ssize_t n = recv(fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    if (n == 0) {
      // EOF - peer closed
      return true;
    }
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      // Error - treat as closed
      return true;
    }
  }

  return false;
}

ak_tcp_error_t ak_tcp_map_error(int platform_errno) {
  switch (platform_errno) {
  case 0:
    return AK_TCP_OK;
  case ETIMEDOUT:
  case EAGAIN:
#if EAGAIN != EWOULDBLOCK
  case EWOULDBLOCK:
#endif
    return AK_TCP_ERR_TIMEOUT;
  case ECONNRESET:
  case EPIPE:
    return AK_TCP_ERR_RESET;
  case ENOTCONN:
  case ECONNREFUSED:
  case ECONNABORTED:
    return AK_TCP_ERR_CLOSED;
  case ENOMEM:
    return AK_TCP_ERR_MEMORY;
  case EINVAL:
  case EBADF:
    return AK_TCP_ERR_INVALID;
  default:
    return AK_TCP_ERR_NETWORK;
  }
}

#endif // !AK24_PLATFORM_WINDOWS
