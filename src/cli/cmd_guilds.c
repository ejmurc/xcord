#include "cli.h"
#include "discord/discord.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_guilds(SSL *ssl, int argc, char **argv) {
    (void)argc;
    (void)argv;
    char *token = cli_load_token();
    if (!token) {
        return 1;
    }
    DiscordGuildList guilds;
    DiscordStatus status = discord_get_guilds(ssl, token, &guilds);
    sodium_memzero(token, strlen(token));
    free(token);
    if (status != DISCORD_OK) {
        fprintf(stderr, "failed to list guilds\n");
        return 1;
    }
    for (size_t i = 0; i < guilds.count; i++) {
        printf("%s  %s\n", guilds.items[i].id,
               guilds.items[i].name ? guilds.items[i].name : "(unnamed)");
    }
    discord_guild_list_free(&guilds);
    return 0;
}
