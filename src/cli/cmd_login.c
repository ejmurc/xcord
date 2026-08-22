#include "cli.h"
#include "common/tty.h"
#include "credentials/credentials.h"
#include "discord/discord.h"
#include <cargs.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XCORD_APP_NAME "xcord"

static struct cag_option options[] = {{.identifier = 'h',
                                       .access_letters = "h",
                                       .access_name = "help",
                                       .description = "Show this help"}};

static char *prompt_line(const char *label) {
    printf("%s", label);
    fflush(stdout);
    size_t cap = 64;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        return NULL;
    }
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) {
                free(buf);
                return NULL;
            }
            buf = tmp;
        }
        buf[len++] = (char)ch;
    }
    if (ch == EOF && len == 0) {
        free(buf);
        return NULL;
    }
    buf[len] = '\0';
    return buf;
}

static const char *pick_mfa_method(const DiscordLoginResult *result) {
    if (result->mfa_totp_available) {
        return "totp";
    }
    if (result->mfa_sms_available) {
        return "sms";
    }
    if (result->mfa_backup_available) {
        return "backup";
    }
    return NULL;
}

int cmd_login(SSL *ssl, int argc, char **argv) {
    cag_option_context context;
    cag_option_init(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'h':
            printf("Usage: xcord login [OPTION]...\n");
            printf("Log in to Discord and save an encrypted local token.\n\n");
            cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
            return 0;
        case '?':
            cag_option_print_error(&context, stdout);
            return 1;
        }
    }
    char *email = prompt_line("email: ");
    if (!email) {
        fprintf(stderr, "failed to read email\n");
        return 1;
    }
    printf("password: ");
    char *password = get_password();
    if (!password) {
        fprintf(stderr, "failed to read password\n");
        free(email);
        return 1;
    }
    DiscordLoginResult login_result;
    DiscordStatus status = discord_login(ssl, email, password, &login_result);
    free(email);
    sodium_memzero(password, strlen(password));
    free(password);
    if (status == DISCORD_ERR_MFA_REQUIRED) {
        const char *method = pick_mfa_method(&login_result);
        if (!method) {
            fprintf(
                stderr,
                "account requires mfa but no supported method was reported\n");
            discord_login_result_free(&login_result);
            return 1;
        }
        int label_len = snprintf(NULL, 0, "%s code: ", method);
        char *label = label_len >= 0 ? malloc((size_t)label_len + 1) : NULL;
        char *code = NULL;
        if (label) {
            snprintf(label, (size_t)label_len + 1, "%s code: ", method);
            code = prompt_line(label);
            free(label);
        }
        if (!code) {
            fprintf(stderr, "failed to read mfa code\n");
            discord_login_result_free(&login_result);
            return 1;
        }
        DiscordLoginResult mfa_result;
        status = discord_verify_mfa(ssl, method, login_result.mfa_ticket,
                                    login_result.mfa_login_instance_id, code,
                                    &mfa_result);
        free(code);
        discord_login_result_free(&login_result);
        login_result = mfa_result;
    }
    if (status != DISCORD_OK || !login_result.token) {
        fprintf(stderr, "login failed\n");
        discord_login_result_free(&login_result);
        return 1;
    }
    printf("choose a local passphrase to encrypt the saved token\n");
    printf("passphrase: ");
    char *passphrase = get_password();
    if (!passphrase) {
        fprintf(stderr, "failed to read passphrase\n");
        discord_login_result_free(&login_result);
        return 1;
    }
    printf("confirm passphrase: ");
    char *confirm = get_password();
    if (!confirm) {
        fprintf(stderr, "failed to read passphrase\n");
        sodium_memzero(passphrase, strlen(passphrase));
        free(passphrase);
        discord_login_result_free(&login_result);
        return 1;
    }
    int mismatch = strcmp(passphrase, confirm) != 0;
    sodium_memzero(confirm, strlen(confirm));
    free(confirm);
    if (mismatch) {
        fprintf(stderr, "passphrases did not match\n");
        sodium_memzero(passphrase, strlen(passphrase));
        free(passphrase);
        discord_login_result_free(&login_result);
        return 1;
    }
    char *filepath = get_credentials_filepath(XCORD_APP_NAME);
    int save_status =
        filepath ? save_credentials(filepath, login_result.token, passphrase)
                 : 1;
    sodium_memzero(passphrase, strlen(passphrase));
    free(passphrase);
    free(filepath);
    if (save_status != 0) {
        fprintf(stderr, "failed to save credentials\n");
        discord_login_result_free(&login_result);
        return 1;
    }
    printf("logged in as %s\n",
           login_result.user_id ? login_result.user_id : "(unknown)");
    discord_login_result_free(&login_result);
    return 0;
}
