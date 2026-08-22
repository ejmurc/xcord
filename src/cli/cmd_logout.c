#include "cli.h"
#include "credentials/credentials.h"
#include <cargs.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XCORD_APP_NAME "xcord"

static struct cag_option options[] = {{.identifier = 'h',
                                       .access_letters = "h",
                                       .access_name = "help",
                                       .description = "Show this help"}};

int cmd_logout(SSL *ssl, int argc, char **argv) {
    (void)ssl;
    cag_option_context context;
    cag_option_init(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'h':
            printf("Usage: xcord logout [OPTION]...\n");
            printf("Remove the locally saved, encrypted Discord token.\n\n");
            cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
            return 0;
        case '?':
            cag_option_print_error(&context, stdout);
            return 1;
        }
    }
    if (!cli_credentials_exist()) {
        printf("not logged in\n");
        return 0;
    }
    char *filepath = get_credentials_filepath(XCORD_APP_NAME);
    if (!filepath) {
        fprintf(stderr, "could not resolve credentials path\n");
        return 1;
    }
    if (remove(filepath) != 0) {
        fprintf(stderr, "failed to remove credentials: %s\n", strerror(errno));
        free(filepath);
        return 1;
    }
    free(filepath);
    printf("logged out\n");
    return 0;
}
