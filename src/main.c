#include "cli/cli.h"
#include "net/err.h"
#include "net/ssl.h"
#include <openssl/ssl.h>
#include <sodium.h>
#include <stdio.h>
#include <string.h>

typedef int (*cmd_fn)(SSL *ssl, int argc, char **argv);

typedef struct {
    const char *name;
    cmd_fn fn;
} cli_command_t;

static const cli_command_t commands[] = {
    {"login", cmd_login},
    {"whoami", cmd_whoami},
    {"guilds", cmd_guilds},
};

int main(int argc, char **argv) {
    if (sodium_init() == -1) {
        fprintf(stderr, "failed to initialize sodium");
        return 1;
    }
    if (argc < 2) {
        fprintf(stderr, "usage: xcord <command> [args]\n");
        return 1;
    }
    cmd_fn fn = NULL;
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(argv[1], commands[i].name) == 0) {
            fn = commands[i].fn;
            break;
        }
    }
    if (!fn) {
        fprintf(stderr, "unknown command: %s\n", argv[1]);
        return 1;
    }
    SSL_CTX *ctx = ssl_ctx_new();
    if (!ctx) {
        fprintf(stderr, "failed to create ssl context: %s\n",
                net_strerror(net_errno));
        return 1;
    }
    SSL *ssl = ssl_connect(ctx, "discord.com", 443);
    if (!ssl) {
        fprintf(stderr, "failed to connect: %s\n", net_strerror(net_errno));
        SSL_CTX_free(ctx);
        return 1;
    }
    int result = fn(ssl, argc - 2, argv + 2);
    ssl_disconnect(ssl);
    SSL_CTX_free(ctx);
    return result;
}
