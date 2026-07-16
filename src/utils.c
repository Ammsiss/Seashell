#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "utils.h"
#include "shell_state.h"

/* error functions */

static void output_err(const char *fmt, va_list *va, bool print_err) {
    fflush(stdout);

    char user_msg[BUF_SIZE] = "";
    char err_str[BUF_SIZE] = "";

    vsnprintf(user_msg, BUF_SIZE, fmt, *va);

    if (print_err) {
        strncat(err_str, strerror(errno), BUF_SIZE);
        fprintf(stderr, "seashell: %s: %s\n", user_msg, err_str);
    } else
        fprintf(stderr, "seashell: %s\n", user_msg);

    fflush(stderr);
}

void fatal(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    output_err(fmt, &va, false);
    va_end(va);

    if (get_env()->subshell)
        _exit(EXIT_FAILURE);
    else
        exit(EXIT_FAILURE);
}

void err_exit(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    output_err(fmt, &va, true);
    va_end(va);

    if (get_env()->subshell)
        _exit(EXIT_FAILURE);
    else
        exit(EXIT_FAILURE);
}

void err_msg(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    output_err(fmt, &va, false);
    va_end(va);
}

void errno_msg(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    output_err(fmt, &va, true);
    va_end(va);
}

void usage_err(const char *fmt, ...) {
    fflush(stdout);
    fprintf(stderr, "Usage: ");

    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);

    fprintf(stderr, "\n");
    fflush(stderr);
    exit(EXIT_FAILURE);
}

/* signal functions */

int set_sig_action(int sig, sighandler_t handler, int flags, sigset_t *mask) {
    struct sigaction sa;
    sa.sa_flags = flags;
    sa.sa_handler = handler;

    if (mask) {
        sa.sa_mask = *mask;
    } else {
        if (xsigemptyset(&sa.sa_mask) == -1)
            return -1;
    }

    if (xsigaction(sig, &sa, &old_sa) == -1)
        return -1;

    return 0;
}

int procmask_add(int sig, int how) {
    sigset_t set;

    if (xsigemptyset(&set) == -1)
        return -1;
    if (xsigaddset(&set, sig) == -1)
        return -1;

    if (xsigprocmask(how, &set, &old_set) == -1)
        return -1;

    return 0;
}

int block_sig(int sig) {
    if (procmask_add(sig, SIG_BLOCK) == -1)
        return -1;

    return 0;
}

int unblock_sig(int sig) {
    if (procmask_add(sig, SIG_UNBLOCK) == -1)
        return -1;

    return 0;
}

int make_sigset(int sigs[], sigset_t *set, bool start_empty) {
    if (start_empty) {
        if (sigemptyset(set) == -1)
            return -1;
        for (int *sig = sigs; *sig != -1; ++ sig) {
            if (sigaddset(set, *sig) == -1)
                return -1;
        }
    } else {
        if (sigfillset(set) == -1)
            return -1;
        for (int *sig = sigs; *sig != -1; ++ sig) {
            if (sigdelset(set, *sig) == -1)
                return -1;
        }
    }


    return 0;
}
