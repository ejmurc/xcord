#include "cli.h"
#include "common/tty.h"
#include "credentials/credentials.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#define XCORD_APP_NAME "xcord"

static int file_exists(const char *path) {
#if defined(_WIN32)
    return _access(path, 0) == 0;
#else
    return access(path, 0) == 0;
#endif
}

char *cli_load_token(void) {
    char *filepath = get_credentials_filepath(XCORD_APP_NAME);
    if (!filepath) {
        fprintf(stderr, "could not resolve credentials path\n");
        return NULL;
    }
    if (!file_exists(filepath)) {
        fprintf(stderr, "not logged in, run `xcord login`\n");
        free(filepath);
        return NULL;
    }
    printf("passphrase: ");
    char *passphrase = get_password();
    if (!passphrase) {
        fprintf(stderr, "failed to read passphrase\n");
        free(filepath);
        return NULL;
    }
    char *token = load_credentials(filepath, passphrase);
    sodium_memzero(passphrase, strlen(passphrase));
    free(passphrase);
    free(filepath);
    if (!token) {
        fprintf(stderr, "incorrect passphrase or corrupted credentials\n");
        return NULL;
    }
    return token;
}
