#pragma once

#include <stddef.h>

typedef struct {
  char *data;
  size_t size;
} strbuf_t;

int strbuf_appendf(strbuf_t *buf, const char *fmt, ...);
void strbuf_free(strbuf_t *buf);
