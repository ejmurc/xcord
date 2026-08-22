#pragma once
#include "http.h"

void ratelimit_wait(const char *method, const char *path);
void ratelimit_update(const char *method, const char *path, const http_response_t *resp);
void ratelimit_cleanup(void);
