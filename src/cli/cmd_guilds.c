#include "cli.h"
#include "discord/discord.h"
#include <cargs.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct cag_option options[] = {{.identifier = 'h',
                                       .access_letters = "h",
                                       .access_name = "help",
                                       .description = "Show this help"}};

int cmd_guilds(SSL *ssl, int argc, char **argv) {
    cag_option_context context;
    cag_option_init(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'h':
            printf("Usage: xcord guilds [OPTION]...\n");
            printf(
                "List the guilds (servers) your account is a member of.\n\n");
            cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
            return 0;
        case '?':
            cag_option_print_error(&context, stdout);
            return 1;
        }
    }
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
