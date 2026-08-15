#include "tty.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <conio.h>
#else
#include <signal.h>
#include <termios.h>
#include <unistd.h>
static struct termios g_oldt;
static volatile sig_atomic_t g_restore = 0;
static void restore_terminal(int sig) {
    if (g_restore)
        tcsetattr(STDIN_FILENO, TCSANOW, &g_oldt);
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif
char *get_password(void) {
    size_t capacity = 64;
    size_t length = 0;
    int ch;
    char *password = malloc(capacity);
    if (!password)
        return NULL;
    fflush(stdout);
#ifdef _WIN32
    while ((ch = _getch()) != '\r') {
        if (ch == 0 || ch == 0xE0) {
            _getch();
            continue;
        }
        if (ch == 8) {
            if (length > 0)
                length--;
        } else if (ch >= 32) {
            if (length >= capacity - 1) {
                capacity *= 2;
                char *temp = realloc(password, capacity);
                if (!temp) {
                    free(password);
                    return NULL;
                }
                password = temp;
            }
            password[length++] = (char)ch;
        }
    }
#else
    struct termios newt;
    int is_tty = tcgetattr(STDIN_FILENO, &g_oldt) == 0;
    if (is_tty) {
        struct sigaction sa = {0};
        sa.sa_handler = restore_terminal;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        newt = g_oldt;
        newt.c_lflag &= (tcflag_t)~ECHO;
        g_restore = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }
    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (length >= capacity - 1) {
            capacity *= 2;
            char *temp = realloc(password, capacity);
            if (!temp) {
                if (is_tty) {
                    tcsetattr(STDIN_FILENO, TCSANOW, &g_oldt);
                    g_restore = 0;
                }
                free(password);
                return NULL;
            }
            password = temp;
        }
        password[length++] = (char)ch;
    }
    if (is_tty) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_oldt);
        g_restore = 0;
    }
#endif
    password[length] = '\0';
    printf("\n");
    return password;
}
void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}
