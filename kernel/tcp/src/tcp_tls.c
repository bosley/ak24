
#if AK24_TLS_ENABLED

#include "tcp_internal.h"
#include <string.h>

static int tls_initialized = 0;
static AK24_MUTEX tls_init_mutex;
static char tls_error_buf[256];

void ak_tcp_tls_init(void) {
  if (tls_initialized) {
    return;
  }

  AK24_MUTEX_INIT(&tls_init_mutex);
  AK24_MUTEX_LOCK(&tls_init_mutex);

  if (!tls_initialized) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    tls_initialized = 1;
  }

  AK24_MUTEX_UNLOCK(&tls_init_mutex);
}

void ak_tcp_tls_deinit(void) {
  if (!tls_initialized) {
    return;
  }

  AK24_MUTEX_LOCK(&tls_init_mutex);
  if (tls_initialized) {
    EVP_cleanup();
    ERR_free_strings();
    tls_initialized = 0;
  }
  AK24_MUTEX_UNLOCK(&tls_init_mutex);
  AK24_MUTEX_DESTROY(&tls_init_mutex);
}

const char *ak_tcp_tls_error_string(void) {
  unsigned long err = ERR_get_error();
  if (err == 0) {
    return "Unknown TLS error";
  }
  ERR_error_string_n(err, tls_error_buf, sizeof(tls_error_buf));
  return tls_error_buf;
}

SSL_CTX *ak_tcp_tls_ctx_new(const char *cert_file, const char *key_file,
                            const char *ca_file, bool verify_client,
                            const char *ciphers, int min_version,
                            const char **error) {
  if (!cert_file || !key_file) {
    if (error) {
      *error = "Certificate and key files are required for TLS";
    }
    return NULL;
  }

  ak_tcp_tls_init();

  const SSL_METHOD *method = TLS_server_method();
  SSL_CTX *ctx = SSL_CTX_new(method);
  if (!ctx) {
    if (error) {
      *error = ak_tcp_tls_error_string();
    }
    return NULL;
  }

  int tls_ver = (min_version > 0) ? min_version : TLS1_2_VERSION;
  if (SSL_CTX_set_min_proto_version(ctx, tls_ver) != 1) {
    if (error) {
      *error = "Failed to set minimum TLS version";
    }
    SSL_CTX_free(ctx);
    return NULL;
  }

  SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                               SSL_OP_NO_COMPRESSION |
                               SSL_OP_CIPHER_SERVER_PREFERENCE);

  if (ciphers) {
    if (SSL_CTX_set_cipher_list(ctx, ciphers) != 1) {
      if (error) {
        *error = "Failed to set cipher list";
      }
      SSL_CTX_free(ctx);
      return NULL;
    }
  }

  if (SSL_CTX_use_certificate_chain_file(ctx, cert_file) != 1) {
    if (error) {
      *error = ak_tcp_tls_error_string();
    }
    SSL_CTX_free(ctx);
    return NULL;
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) != 1) {
    if (error) {
      *error = ak_tcp_tls_error_string();
    }
    SSL_CTX_free(ctx);
    return NULL;
  }

  if (SSL_CTX_check_private_key(ctx) != 1) {
    if (error) {
      *error = "Certificate and private key do not match";
    }
    SSL_CTX_free(ctx);
    return NULL;
  }

  if (ca_file) {
    if (SSL_CTX_load_verify_locations(ctx, ca_file, NULL) != 1) {
      if (error) {
        *error = ak_tcp_tls_error_string();
      }
      SSL_CTX_free(ctx);
      return NULL;
    }
  }

  if (verify_client) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       NULL);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
  }

  return ctx;
}

void ak_tcp_tls_ctx_free(SSL_CTX *ctx) {
  if (ctx) {
    SSL_CTX_free(ctx);
  }
}

SSL *ak_tcp_tls_accept(SSL_CTX *ssl_ctx, ak_socket_fd_t socket_fd,
                       const char **error) {
  if (!ssl_ctx) {
    if (error) {
      *error = "SSL context is NULL";
    }
    return NULL;
  }

  SSL *ssl = SSL_new(ssl_ctx);
  if (!ssl) {
    if (error) {
      *error = ak_tcp_tls_error_string();
    }
    return NULL;
  }

#ifdef AK24_PLATFORM_WINDOWS
  if (SSL_set_fd(ssl, (int)socket_fd) != 1) {
#else
  if (SSL_set_fd(ssl, socket_fd) != 1) {
#endif
    if (error) {
      *error = ak_tcp_tls_error_string();
    }
    SSL_free(ssl);
    return NULL;
  }

  int result = SSL_accept(ssl);
  if (result != 1) {
    int ssl_err = SSL_get_error(ssl, result);
    switch (ssl_err) {
    case SSL_ERROR_ZERO_RETURN:
      if (error) {
        *error = "TLS handshake: connection closed";
      }
      break;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      if (error) {
        *error = "TLS handshake: would block (timeout?)";
      }
      break;
    case SSL_ERROR_SYSCALL:
      if (error) {
        *error = "TLS handshake: system call error";
      }
      break;
    case SSL_ERROR_SSL:
      if (error) {
        *error = ak_tcp_tls_error_string();
      }
      break;
    default:
      if (error) {
        *error = "TLS handshake failed";
      }
      break;
    }
    SSL_free(ssl);
    return NULL;
  }

  return ssl;
}

ssize_t ak_tcp_tls_send(SSL *ssl, const uint8_t *data, size_t len,
                        const char **error) {
  if (!ssl || !data) {
    if (error) {
      *error = "Invalid arguments";
    }
    return -1;
  }

  ERR_clear_error();
  int result = SSL_write(ssl, data, (int)len);

  if (result <= 0) {
    int ssl_err = SSL_get_error(ssl, result);
    switch (ssl_err) {
    case SSL_ERROR_ZERO_RETURN:
      return 0;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      if (error) {
        *error = "Operation would block";
      }
      return -1;
    case SSL_ERROR_SYSCALL:
      if (error) {
        *error = "TLS send: system call error";
      }
      return -1;
    case SSL_ERROR_SSL:
      if (error) {
        *error = ak_tcp_tls_error_string();
      }
      return -1;
    default:
      if (error) {
        *error = "TLS send failed";
      }
      return -1;
    }
  }

  return (ssize_t)result;
}

ssize_t ak_tcp_tls_recv(SSL *ssl, uint8_t *buffer, size_t len,
                        const char **error) {
  if (!ssl || !buffer) {
    if (error) {
      *error = "Invalid arguments";
    }
    return -1;
  }

  ERR_clear_error();
  int result = SSL_read(ssl, buffer, (int)len);

  if (result <= 0) {
    int ssl_err = SSL_get_error(ssl, result);
    switch (ssl_err) {
    case SSL_ERROR_ZERO_RETURN:
      return 0;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      if (error) {
        *error = "Operation would block";
      }
      return -1;
    case SSL_ERROR_SYSCALL:
      if (error) {
        unsigned long e = ERR_peek_error();
        if (e == 0 && result == 0) {
          return 0;
        }
        *error = "TLS recv: system call error";
      }
      return -1;
    case SSL_ERROR_SSL:
      if (error) {
        *error = ak_tcp_tls_error_string();
      }
      return -1;
    default:
      if (error) {
        *error = "TLS recv failed";
      }
      return -1;
    }
  }

  return (ssize_t)result;
}

void ak_tcp_tls_shutdown(SSL *ssl) {
  if (!ssl) {
    return;
  }

  int result = SSL_shutdown(ssl);
  if (result == 0) {
    SSL_shutdown(ssl);
  }
}

void ak_tcp_tls_free(SSL *ssl) {
  if (ssl) {
    SSL_free(ssl);
  }
}

SSL_CTX *ak_tcp_tls_client_ctx_new(const char *ca_file, const char *cert_file,
                                   const char *key_file, bool verify_server,
                                   const char *ciphers, int min_version,
                                   const char **error) {
  ak_tcp_tls_init();

  const SSL_METHOD *method = TLS_client_method();
  SSL_CTX *ctx = SSL_CTX_new(method);
  if (!ctx) {
    if (error) {
      *error = ak_tcp_tls_error_string();
    }
    return NULL;
  }

  int tls_ver = (min_version > 0) ? min_version : TLS1_2_VERSION;
  if (SSL_CTX_set_min_proto_version(ctx, tls_ver) != 1) {
    if (error) {
      *error = "Failed to set minimum TLS version";
    }
    SSL_CTX_free(ctx);
    return NULL;
  }

  SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                               SSL_OP_NO_COMPRESSION);

  if (ciphers) {
    if (SSL_CTX_set_cipher_list(ctx, ciphers) != 1) {
      if (error) {
        *error = "Failed to set cipher list";
      }
      SSL_CTX_free(ctx);
      return NULL;
    }
  }

  if (ca_file) {
    if (SSL_CTX_load_verify_locations(ctx, ca_file, NULL) != 1) {
      if (error) {
        *error = ak_tcp_tls_error_string();
      }
      SSL_CTX_free(ctx);
      return NULL;
    }
  } else {
    if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
      if (error) {
        *error = "Failed to load system CA certificates";
      }
      SSL_CTX_free(ctx);
      return NULL;
    }
  }

  if (cert_file && key_file) {
    if (SSL_CTX_use_certificate_chain_file(ctx, cert_file) != 1) {
      if (error) {
        *error = ak_tcp_tls_error_string();
      }
      SSL_CTX_free(ctx);
      return NULL;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) != 1) {
      if (error) {
        *error = ak_tcp_tls_error_string();
      }
      SSL_CTX_free(ctx);
      return NULL;
    }

    if (SSL_CTX_check_private_key(ctx) != 1) {
      if (error) {
        *error = "Certificate and private key do not match";
      }
      SSL_CTX_free(ctx);
      return NULL;
    }
  }

  if (verify_server) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
  }

  return ctx;
}

SSL *ak_tcp_tls_connect(SSL_CTX *ssl_ctx, ak_socket_fd_t socket_fd,
                        const char *sni_hostname, const char **error) {
  if (!ssl_ctx) {
    if (error) {
      *error = "SSL context is NULL";
    }
    return NULL;
  }

  SSL *ssl = SSL_new(ssl_ctx);
  if (!ssl) {
    if (error) {
      *error = ak_tcp_tls_error_string();
    }
    return NULL;
  }

  if (sni_hostname) {
    if (SSL_set_tlsext_host_name(ssl, sni_hostname) != 1) {
      if (error) {
        *error = "Failed to set SNI hostname";
      }
      SSL_free(ssl);
      return NULL;
    }
  }

#ifdef AK24_PLATFORM_WINDOWS
  if (SSL_set_fd(ssl, (int)socket_fd) != 1) {
#else
  if (SSL_set_fd(ssl, socket_fd) != 1) {
#endif
    if (error) {
      *error = ak_tcp_tls_error_string();
    }
    SSL_free(ssl);
    return NULL;
  }

  int result = SSL_connect(ssl);
  if (result != 1) {
    int ssl_err = SSL_get_error(ssl, result);
    switch (ssl_err) {
    case SSL_ERROR_ZERO_RETURN:
      if (error) {
        *error = "TLS handshake: connection closed";
      }
      break;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      if (error) {
        *error = "TLS handshake: would block (timeout?)";
      }
      break;
    case SSL_ERROR_SYSCALL:
      if (error) {
        *error = "TLS handshake: system call error";
      }
      break;
    case SSL_ERROR_SSL:
      if (error) {
        *error = ak_tcp_tls_error_string();
      }
      break;
    default:
      if (error) {
        *error = "TLS handshake failed";
      }
      break;
    }
    SSL_free(ssl);
    return NULL;
  }

  return ssl;
}

#endif // AK24_TLS_ENABLED
