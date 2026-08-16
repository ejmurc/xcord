#include "cli.h"
#include "common/tty.h"
#include "credentials/credentials.h"
#include "discord/discord.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XCORD_APP_NAME "xcord"

static int prompt_line(const char *label, char *buf, size_t buf_size) {
    printf("%s", label);
    fflush(stdout);
    if (!fgets(buf, (int)buf_size, stdin)) {
        return -1;
    }
    if (!strchr(buf, '\n')) {
        flush_stdin();
    }
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
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
    (void)argc;
    (void)argv;
    char email[256];
    if (prompt_line("email: ", email, sizeof(email)) != 0) {
        return 1;
    }
    printf("password: ");
    char *password = get_password();
    if (!password) {
        fprintf(stderr, "failed to read password\n");
        return 1;
    }
    DiscordLoginResult login_result;
    DiscordStatus status = discord_login(ssl, email, password, &login_result);
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
        char code[16];
        char prompt[32];
        snprintf(prompt, sizeof(prompt), "%s code: ", method);
        if (prompt_line(prompt, code, sizeof(code)) != 0) {
            discord_login_result_free(&login_result);
            return 1;
        }
        DiscordLoginResult mfa_result;
        status = discord_verify_mfa(ssl, method, login_result.mfa_ticket,
                                    login_result.mfa_login_instance_id, code,
                                    &mfa_result);
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
