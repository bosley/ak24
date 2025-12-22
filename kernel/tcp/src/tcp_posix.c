
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

static int sigpipe_handled = 0;

void ak_tcp_platform_init(void) {
  if (!sigpipe_handled) {
    signal(SIGPIPE, SIG_IGN);
    sigpipe_handled = 1;
  }
}

void ak_tcp_platform_deinit(void) {}

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
  int fd = socket(family, SOCK_STREAM, 0);
  if (fd < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return AK_INVALID_SOCKET;
  }

  int optval = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    close(fd);
    return AK_INVALID_SOCKET;
  }

  if (family == AF_INET6) {
    int v6only = 0;
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only)) <
        0) {
      if (error) {
        *error = strerror(errno);
      }
      close(fd);
      return AK_INVALID_SOCKET;
    }
  }

  return fd;
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

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      if (error) {
        *error = strerror(errno);
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

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      if (error) {
        *error = strerror(errno);
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
  struct sockaddr_storage client_addr;
  socklen_t client_len = sizeof(client_addr);

  int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
  if (client_fd < 0) {
    if (error) {
      *error = strerror(errno);
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

  optval = (idle_sec > 0) ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  if (idle_sec <= 0) {

    return 0;
  }

#ifdef __APPLE__

  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle_sec, sizeof(idle_sec)) <
      0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }
#else

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

int ak_tcp_socket_set_linger(ak_socket_fd_t fd, bool enable, int timeout_sec,
                             const char **error) {
  struct linger lg;
  lg.l_onoff = enable ? 1 : 0;
  lg.l_linger = timeout_sec;

  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg)) < 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }
  return 0;
}

int ak_tcp_socket_set_buffers(ak_socket_fd_t fd, size_t recv_size,
                              size_t send_size, const char **error) {
  if (recv_size > 0) {
    int size = (int)recv_size;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0) {
      if (error) {
        *error = strerror(errno);
      }
      return -1;
    }
  }
  if (send_size > 0) {
    int size = (int)send_size;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0) {
      if (error) {
        *error = strerror(errno);
      }
      return -1;
    }
  }
  return 0;
}

int ak_tcp_socket_poll_read(ak_socket_fd_t fd, int timeout_ms) {
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;

  int ret = poll(&pfd, 1, timeout_ms);
  if (ret < 0) {
    return -1;
  }
  if (ret == 0) {
    return 0;
  }
  return 1;
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

  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;

  int ret = poll(&pfd, 1, 0);
  if (ret < 0) {

    return true;
  }

  if (ret == 0) {

    return false;
  }

  if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
    return true;
  }

  if (pfd.revents & POLLIN) {
    char buf;
    ssize_t n = recv(fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    if (n == 0) {

      return true;
    }
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {

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

#include <netdb.h>

int ak_tcp_socket_connect(ak_socket_fd_t fd, const char *host, uint16_t port,
                          uint32_t timeout_ms, const char **error) {
  if (!host) {
    if (error) {
      *error = "Host is required";
    }
    return -1;
  }

  struct addrinfo hints, *result, *rp;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  char port_str[16];
  snprintf(port_str, sizeof(port_str), "%u", port);

  int gai_result = getaddrinfo(host, port_str, &hints, &result);
  if (gai_result != 0) {
    if (error) {
      *error = gai_strerror(gai_result);
    }
    return -1;
  }

  bool was_blocking = true;
  if (timeout_ms > 0) {
    if (ak_tcp_socket_set_nonblocking(fd, true, error) != 0) {
      freeaddrinfo(result);
      return -1;
    }
    was_blocking = false;
  }

  int connect_result = -1;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    connect_result = connect(fd, rp->ai_addr, rp->ai_addrlen);

    if (connect_result == 0) {
      break;
    }

    if (errno == EINPROGRESS && timeout_ms > 0) {
      struct pollfd pfd;
      pfd.fd = fd;
      pfd.events = POLLOUT;
      pfd.revents = 0;

      int poll_result = poll(&pfd, 1, (int)timeout_ms);
      if (poll_result < 0) {
        if (error) {
          *error = strerror(errno);
        }
        freeaddrinfo(result);
        if (!was_blocking) {
          ak_tcp_socket_set_nonblocking(fd, false, NULL);
        }
        return -1;
      }
      if (poll_result == 0) {
        if (error) {
          *error = "Connection timed out";
        }
        freeaddrinfo(result);
        if (!was_blocking) {
          ak_tcp_socket_set_nonblocking(fd, false, NULL);
        }
        return -1;
      }

      int so_error = 0;
      socklen_t len = sizeof(so_error);
      if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
        if (error) {
          *error = strerror(errno);
        }
        freeaddrinfo(result);
        if (!was_blocking) {
          ak_tcp_socket_set_nonblocking(fd, false, NULL);
        }
        return -1;
      }

      if (so_error == 0) {
        connect_result = 0;
        break;
      }

      errno = so_error;
    }
  }

  freeaddrinfo(result);

  if (!was_blocking) {
    ak_tcp_socket_set_nonblocking(fd, false, NULL);
  }

  if (connect_result != 0) {
    if (error) {
      *error = strerror(errno);
    }
    return -1;
  }

  return 0;
}

#endif
