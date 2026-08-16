#include "cli.h"
#include "discord/discord.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_whoami(SSL *ssl, int argc, char **argv) {
    (void)argc;
    (void)argv;
    char *token = cli_load_token();
    if (!token) {
        return 1;
    }
    char *user_id;
    char *username;
    DiscordStatus status =
        discord_get_current_user(ssl, token, &user_id, &username);
    sodium_memzero(token, strlen(token));
    free(token);
    if (status != DISCORD_OK) {
        fprintf(stderr, "whoami failed\n");
        return 1;
    }
    printf("%s (%s)\n", username ? username : "(unknown)",
           user_id ? user_id : "(unknown)");
    free(user_id);
    free(username);
    return 0;
}
