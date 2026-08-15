#include "ssl.h"
#include "err.h"
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

SSL_CTX *ssl_ctx_new(void) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        net_errno = NET_ECTX;
        return NULL;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
        net_errno = NET_ECTX;
        SSL_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

SSL *ssl_connect(SSL_CTX *ctx, const char *hostname, uint16_t port) {
    if (!ctx || !hostname) {
        net_errno = NET_EARGS;
        return NULL;
    }
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned int)port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *results;
    if (getaddrinfo(hostname, port_str, &hints, &results) != 0) {
        net_errno = NET_ERESOLVE;
        return NULL;
    }
    int sockfd = -1;
    for (struct addrinfo *addr = results; addr; addr = addr->ai_next) {
        sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sockfd < 0) {
            continue;
        }
        if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }
        close(sockfd);
        sockfd = -1;
    }
    freeaddrinfo(results);
    if (sockfd < 0) {
        net_errno = NET_ECONNECT;
        return NULL;
    }
    struct timeval tv = {.tv_sec = 30, .tv_usec = 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    int flag = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) ==
        -1) {
        net_errno = NET_ESOCK;
        goto fail_sock;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)) ==
        -1) {
        net_errno = NET_ESOCK;
        goto fail_sock;
    }
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        net_errno = NET_ESSL;
        goto fail_sock;
    }
    SSL_set_fd(ssl, sockfd);
    if (!SSL_set_tlsext_host_name(ssl, hostname)) {
        net_errno = NET_ESSL;
        goto fail_ssl;
    }
    X509_VERIFY_PARAM *param = SSL_get0_param(ssl);
    X509_VERIFY_PARAM_set_hostflags(param,
                                    X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    if (!X509_VERIFY_PARAM_set1_host(param, hostname, 0)) {
        net_errno = NET_ESSL;
        goto fail_ssl;
    }
    if (SSL_connect(ssl) <= 0) {
        net_errno = NET_EHANDSHAKE;
        goto fail_ssl;
    }
    if (SSL_get_verify_result(ssl) != X509_V_OK) {
        net_errno = NET_EVERIFY;
        SSL_shutdown(ssl);
        goto fail_ssl;
    }
    return ssl;
fail_ssl:
    SSL_free(ssl);
fail_sock:
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
    return NULL;
}

void ssl_disconnect(SSL *ssl) {
#ifdef _WIN32
    SOCKET sockfd = SSL_get_fd(ssl);
#else
    int sockfd = SSL_get_fd(ssl);
#endif
    SSL_shutdown(ssl);
    SSL_free(ssl);
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}
