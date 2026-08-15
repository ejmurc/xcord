#include "base64.h"
#include <stdlib.h>

static const char table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(const unsigned char *data, size_t len) {
    size_t out_len = ((len + 2) / 3) * 4;
    char *out = malloc(out_len + 1);
    if (!out) {
        return NULL;
    }
    size_t i = 0;
    size_t j = 0;
    while (i + 3 <= len) {
        unsigned int n = ((unsigned int)data[i] << 16) |
                         ((unsigned int)data[i + 1] << 8) |
                         (unsigned int)data[i + 2];
        out[j++] = table[(n >> 18) & 0x3f];
        out[j++] = table[(n >> 12) & 0x3f];
        out[j++] = table[(n >> 6) & 0x3f];
        out[j++] = table[n & 0x3f];
        i += 3;
    }
    size_t remaining = len - i;
    if (remaining == 1) {
        unsigned int n = (unsigned int)data[i] << 16;
        out[j++] = table[(n >> 18) & 0x3f];
        out[j++] = table[(n >> 12) & 0x3f];
        out[j++] = '=';
        out[j++] = '=';
    } else if (remaining == 2) {
        unsigned int n =
            ((unsigned int)data[i] << 16) | ((unsigned int)data[i + 1] << 8);
        out[j++] = table[(n >> 18) & 0x3f];
        out[j++] = table[(n >> 12) & 0x3f];
        out[j++] = table[(n >> 6) & 0x3f];
        out[j++] = '=';
    }
    out[j] = '\0';
    return out;
}
