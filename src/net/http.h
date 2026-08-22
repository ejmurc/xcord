#pragma once
#include <openssl/ssl.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *hostname;
    const char *method;
    const char *path;
    const char *body;
    const char *content_type;
    const char **headers;
} http_request_t;

typedef struct {
    int    status;
    char  *body;
    size_t body_len;
    long   retry_after_ms;
    long   rl_remaining;
    long   rl_reset_ms;
    char  *bucket;
    bool   should_close;
} http_response_t;

int http_request(SSL *ssl, const http_request_t *req, http_response_t *resp);
