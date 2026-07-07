#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

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

    exit(EXIT_FAILURE);
}

void errExit(int exit_code, bool print_err, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    output_err(fmt, &va, print_err);
    va_end(va);

    exit(exit_code);
}

void err_exit(int exit_code, bool print_err, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    output_err(fmt, &va, print_err);
    va_end(va);

    _exit(exit_code);
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
