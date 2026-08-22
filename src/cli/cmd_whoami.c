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

int cmd_whoami(SSL *ssl, int argc, char **argv) {
    cag_option_context context;
    cag_option_init(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'h':
            printf("Usage: xcord whoami [OPTION]...\n");
            printf("Show the currently logged in Discord account.\n\n");
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
