#ifndef LOG_H
#define LOG_H

#define _GNU_SOURCE

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define LOG_INFO(msg, ...) \
    log_info("PID(%d) " msg, getpid() __VA_OPT__(,) __VA_ARGS__)

#define LOG_ERR(msg, ...) \
    log_err("PID(%d) " msg, getpid() __VA_OPT__(,) __VA_ARGS__)

#define PFFORMAT(x, y) __attribute__ ((format(printf, (x), (y))))

#define SUCCESS 0
#define FAILURE 1

extern int stored_fd;

static inline int log_init() {
    time_t t = time(NULL);
    struct tm *time = localtime(&t);

    char date[50];
    strftime(date, sizeof(date), "%d-%m-%y_%H:%M:%S", time);

    char filename[256];
    strcpy(filename, "log.");
    strcat(filename, date);

    char fullpath[512];
    strcpy(fullpath, "./logs/");
    strcat(fullpath, filename);

    stored_fd = open(fullpath, O_CREAT | O_RDWR, 0600);
    if (stored_fd == -1) {
        fprintf(stderr, "error: failed to create log file (%s)\n", filename);
        return -1;
    }

    unlink("./logs/latest");

    if (symlink(filename, "./logs/latest") == -1) {
        fprintf(stderr, "error: failed to create sym link %s -> %s\n",
                fullpath, "./logs/latest");
        return -1;
    }

    return 0;
}

static inline void log_msg(const char *header, const char *fmt, va_list va) {
    int hlen = strlen(header);

    va_list va_len;
    va_copy(va_len, va);

    int mlen = vsnprintf(NULL, 0, fmt, va_len);
    assert(mlen >= 0);

    mlen += strlen(header) + 1;

    char *msg = malloc(mlen + 1);
    assert(msg);

    strcpy(msg, header);
    vsprintf(msg + hlen, fmt, va);
    strcat(msg, "\n");

    va_end(va_len);

    assert(write(stored_fd, msg, mlen) != -1);

    free(msg);
}

PFFORMAT(1, 2) static inline void log_info(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    log_msg("\033[2;36minfo\033[m:  ", fmt, va);

    va_end(va);
}

PFFORMAT(1, 2) static inline void log_err(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    log_msg("\033[91merror\033[m: ", fmt, va);

    va_end(va);
}

PFFORMAT(1, 2) static inline void log_trace(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    log_msg("error: ", fmt, va);

    va_end(va);
}

#endif
