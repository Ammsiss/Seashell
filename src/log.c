#define _GNU_SOURCE

#include <limits.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

int log_output_fd;

int log_init() {
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

    log_output_fd = open(fullpath, O_CREAT | O_RDWR, 0600);
    if (log_output_fd == -1) {
        perror("in log_init: open");
        return -1;
    }

    unlink("./logs/latest");

    if (symlink(filename, "./logs/latest") == -1) {
        perror("in logsinit: symlink");
        return -1;
    }

    return 0;
}

/* level: file:line:func:pid msg[: errstr] */
void log_msg(log_level level, const char *errstr, const char *file, int line, \
        const char *func, const char *fmt, ...) {

    int saved_errno = errno;

    char level_str[LOG_BUF_SIZE] = "";
    char file_str[PATH_MAX] = "";
    char func_str[LOG_BUF_SIZE] = "";
    char msg[LOG_BUF_SIZE] = "";

    if (level == L_INFO)
        strcpy(level_str, CGREEN "info" CCL);
    else if (level == L_ERR)
        strcpy(level_str, CRED "error" CCL);
    else
        strcpy(level_str, "???");

    strncpy(file_str, file, PATH_MAX);
    strncpy(func_str, func, LOG_BUF_SIZE);

    va_list va;
    va_start(va, fmt);
    vsnprintf(msg, LOG_BUF_SIZE, fmt, va);
    va_end(va);

    char output_str[OUTPUT_SIZE] = "";

    if (errstr) {
        snprintf(output_str, OUTPUT_SIZE, \
            "%s: " CDIM "%s:%d:%s:%d " CCL "%s: %s\n", \
            level_str, basename(file_str), line, func_str, getpid(), msg, errstr);
    } else {
        snprintf(output_str, OUTPUT_SIZE, \
            "%s: " CDIM "%s:%d:%s:%d " CCL "%s\n", \
            level_str, basename(file_str), line, func_str, getpid(), msg);
    }

    write(log_output_fd, output_str, strlen(output_str));

    errno = saved_errno;
}
