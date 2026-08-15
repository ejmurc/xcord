#include "common/tty.h"
#include "discord/discord.h"
#include "net/err.h"
#include "net/ssl.h"
#include <openssl/ssl.h>
#include <sodium.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "libsodium failed to initialize");
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
    char email[256];
    printf("email: ");
    fflush(stdout);
    if (!fgets(email, sizeof(email), stdin)) {
        return 1;
    }
    if (!strchr(email, '\n')) {
        flush_stdin();
    }
    email[strcspn(email, "\n")] = '\0';
    printf("password: ");
    char *password = get_password();
    if (!password) {
        fprintf(stderr, "failed to read password\n");
        ssl_disconnect(ssl);
        SSL_CTX_free(ctx);
        return 1;
    }
    DiscordLoginResult login_result;
    DiscordStatus status = discord_login(ssl, email, password, &login_result);
    sodium_memzero(password, strlen(password));
    free(password);
    if (status == DISCORD_ERR_MFA_REQUIRED) {
        char code[16];
        printf("2fa code: ");
        fflush(stdout);
        if (!fgets(code, sizeof(code), stdin)) {
            return 1;
        }
        if (!strchr(code, '\n')) {
            flush_stdin();
        }
        code[strcspn(code, "\n")] = '\0';
        DiscordLoginResult mfa_result;
        status = discord_verify_mfa(ssl, "totp", login_result.mfa_ticket,
                                    login_result.mfa_login_instance_id, code,
                                    &mfa_result);
        discord_login_result_free(&login_result);
        login_result = mfa_result;
    }
    if (status != DISCORD_OK) {
        fprintf(stderr, "login failed, status=%d\n", status);
        discord_login_result_free(&login_result);
        ssl_disconnect(ssl);
        SSL_CTX_free(ctx);
        return 1;
    }
    printf("logged in as user_id=%s\n",
           login_result.user_id ? login_result.user_id : "(unknown)");
    DiscordGuildList guilds;
    if (discord_get_guilds(ssl, login_result.token, &guilds) == DISCORD_OK) {
        for (size_t i = 0; i < guilds.count; i++) {
            printf("guild: %s (%s)\n", guilds.items[i].name,
                   guilds.items[i].id);
        }
        discord_guild_list_free(&guilds);
    }
    discord_login_result_free(&login_result);
    ssl_disconnect(ssl);
    SSL_CTX_free(ctx);
    return 0;
}
