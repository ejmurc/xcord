#include "credentials.h"
#include <sodium.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/* file layout: [salt: 16][nonce: 24][ciphertext+mac: plaintext_len+32] */

static int derive_key(const char *password, const unsigned char *salt,
                      unsigned char *key) {
    return crypto_pwhash(key, crypto_secretbox_KEYBYTES, password,
                         strlen(password), salt,
                         crypto_pwhash_OPSLIMIT_INTERACTIVE,
                         crypto_pwhash_MEMLIMIT_INTERACTIVE,
                         crypto_pwhash_ALG_ARGON2ID13) == 0;
}

static unsigned char *encrypt_credentials(const char *plaintext,
                                          const char *password,
                                          size_t *out_len) {
    size_t plaintext_len = strlen(plaintext);
    size_t total = crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES +
                   plaintext_len + crypto_secretbox_MACBYTES;

    unsigned char *output = malloc(total);
    if (!output)
        return NULL;

    unsigned char *salt = output;
    unsigned char *nonce = output + crypto_pwhash_SALTBYTES;
    unsigned char *ct = nonce + crypto_secretbox_NONCEBYTES;

    randombytes_buf(salt, crypto_pwhash_SALTBYTES);
    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);

    unsigned char key[crypto_secretbox_KEYBYTES];
    if (!derive_key(password, salt, key)) {
        sodium_memzero(key, sizeof key);
        free(output);
        return NULL;
    }

    crypto_secretbox_easy(ct, (const unsigned char *)plaintext, plaintext_len,
                          nonce, key);

    sodium_memzero(key, sizeof key);
    *out_len = total;
    return output;
}

static char *decrypt_credentials(const unsigned char *encrypted,
                                 size_t encrypted_len, const char *password) {
    size_t min_len = crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES +
                     crypto_secretbox_MACBYTES;
    if (encrypted_len < min_len)
        return NULL;

    const unsigned char *salt = encrypted;
    const unsigned char *nonce = encrypted + crypto_pwhash_SALTBYTES;
    const unsigned char *ct = nonce + crypto_secretbox_NONCEBYTES;
    size_t ct_len =
        encrypted_len - crypto_pwhash_SALTBYTES - crypto_secretbox_NONCEBYTES;
    size_t plaintext_len = ct_len - crypto_secretbox_MACBYTES;

    unsigned char key[crypto_secretbox_KEYBYTES];
    if (!derive_key(password, salt, key)) {
        sodium_memzero(key, sizeof key);
        return NULL;
    }

    char *plaintext = malloc(plaintext_len + 1);
    if (!plaintext) {
        sodium_memzero(key, sizeof key);
        return NULL;
    }

    if (crypto_secretbox_open_easy((unsigned char *)plaintext, ct, ct_len,
                                   nonce, key) != 0) {
        sodium_memzero(key, sizeof key);
        sodium_memzero(plaintext, plaintext_len);
        free(plaintext);
        return NULL;
    }

    plaintext[plaintext_len] = '\0';
    sodium_memzero(key, sizeof key);
    return plaintext;
}

static int mkdir_p(const char *path) {
    if (!path || strlen(path) == 0) {
        return 1;
    }
#if defined(_WIN32)
    if (_access(path, 0) == 0) {
        return 0;
    }
#else
    if (access(path, 0) == 0) {
        return 0;
    }
#endif
    char *p = malloc(strlen(path) + 1);
    if (!p) {
        return 1;
    }
    strcpy(p, path);
    char *separator = strrchr(p, '/');
    if (!separator) {
        separator = strrchr(p, '\\');
    }
    if (separator && separator != p) {
        *separator = '\0';
        if (mkdir_p(p)) {
            free(p);
            return 1;
        }
    }
    free(p);
#if defined(_WIN32)
    if (_mkdir(path) != 0) {
        return 1;
    }
#else
    if (mkdir(path, 0700) != 0) {
        return 1;
    }
#endif
    return 0;
}

static char *get_config_dir(const char *appname) {
    const char *home = getenv("HOME");
#if defined(_WIN32)
    if (!home) {
        home = getenv("USERPROFILE");
    }
#endif
    if (!home) {
        return NULL;
    }
    size_t len = strlen(home) + strlen("/.config/") + strlen(appname) + 2;
    char *path = malloc(len);
    if (!path) {
        return NULL;
    }
    int n = snprintf(path, len, "%s/.config/%s", home, appname);
    if (n < 0 || (size_t)n >= len) {
        free(path);
        return NULL;
    }
    if (mkdir_p(path) != 0) {
        free(path);
        return NULL;
    }
    return path;
}

char *get_credentials_filepath(const char *appname) {
    char *dir = get_config_dir(appname);
    if (!dir) {
        return NULL;
    }
    int len = snprintf(NULL, 0, "%s/credentials.enc", dir);
    if (len < 0) {
        free(dir);
        return NULL;
    }
    char *filepath = malloc((size_t)len + 1);
    if (!filepath) {
        free(dir);
        return NULL;
    }
    snprintf(filepath, (size_t)len + 1, "%s/credentials.enc", dir);
    free(dir);
    return filepath;
}

int save_credentials(const char *filepath, const char *credentials,
                     const char *password) {
    if (!filepath || !credentials || !password) {
        return 1;
    }
    size_t encrypted_len;
    unsigned char *encrypted =
        encrypt_credentials(credentials, password, &encrypted_len);
    if (!encrypted) {
        return 1;
    }
#if defined(_WIN32)
    FILE *f = fopen(filepath, "wb");
#else
    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    FILE *f = fd >= 0 ? fdopen(fd, "wb") : NULL;
    if (fd >= 0 && !f) {
        close(fd);
    }
#endif
    if (!f) {
        sodium_memzero(encrypted, encrypted_len);
        free(encrypted);
        return 1;
    }
    if (fwrite(encrypted, 1, encrypted_len, f) != encrypted_len) {
        sodium_memzero(encrypted, encrypted_len);
        free(encrypted);
        fclose(f);
        return 1;
    }
    sodium_memzero(encrypted, encrypted_len);
    free(encrypted);
    fclose(f);
    return 0;
}

char *load_credentials(const char *filepath, const char *password) {
    if (!filepath || !password) {
        return NULL;
    }
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long encrypted_len = ftell(f);
    if (encrypted_len < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    unsigned char *encrypted = malloc((size_t)encrypted_len);
    if (!encrypted) {
        fclose(f);
        return NULL;
    }
    if (fread(encrypted, 1, (size_t)encrypted_len, f) !=
        (size_t)encrypted_len) {
        sodium_memzero(encrypted, (size_t)encrypted_len);
        free(encrypted);
        fclose(f);
        return NULL;
    }
    fclose(f);
    char *credentials =
        decrypt_credentials(encrypted, (size_t)encrypted_len, password);
    sodium_memzero(encrypted, (size_t)encrypted_len);
    free(encrypted);
    return credentials;
}
