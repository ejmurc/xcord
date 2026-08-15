#include "strbuf.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int strbuf_appendf(strbuf_t *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (needed < 0) {
        return needed;
    }
    size_t new_size = (size_t)needed + buf->size + 1;
    char *new_data = realloc(buf->data, new_size);
    if (!new_data) {
        return -1;
    }
    buf->data = new_data;
    va_start(ap, fmt);
    int written = vsnprintf(buf->data + buf->size, (size_t)needed + 1, fmt, ap);
    va_end(ap);
    if (written < 0) {
        return -1;
    }
    buf->size = new_size - 1;
    return needed;
}

void strbuf_free(strbuf_t *buf) {
    if (buf) {
        free(buf->data);
    }
}
