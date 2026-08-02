#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "dyn_str.h"

static int log_fd = -1;

bool log_is_open(void) {
    return log_fd != -1;
}

char *date_str(void) {
    static char date[256];

    time_t t = time(NULL);
    if (t == ((time_t) -1))
        err_exit("time");

    struct tm *time = localtime(&t);
    if (!time)
        err_exit("localtime");

    if (strftime(date, sizeof(date), "%d-%m-%y", time) == 0)
        err_exit("strftime");

    return date;
}

void log_init(char *dir_path) {
    static int log_id = 1;
    char log_path[PATH_MAX];
    char link_path[PATH_MAX];

    do {
        snprintf(log_path, PATH_MAX, "%s/log.%s.%d", dir_path, date_str(), log_id);
        snprintf(link_path, PATH_MAX, "%s/latest", dir_path);

        log_fd = open(log_path, O_CREAT | O_EXCL | O_RDWR, 0600);

        if (log_fd == -1 && errno != EEXIST) {
            perror("log: open");
            exit(EXIT_FAILURE);
        }
    } while (log_fd == -1 && ++log_id);

    if (unlink(link_path) == -1)
        if (errno != ENOENT) {
            perror("unlink");
            exit(EXIT_FAILURE);
        }

    if (symlink(log_path, link_path) == -1) {
        perror("symlink");
        exit(EXIT_FAILURE);
    }
}

void log_free() {
    xclose(log_fd);
    log_fd = -1;

    log_fd = (int){0};
}

char *convert_newlines(char *s) {
    d_str out;
    if (d_str_init(&out) == -1)
        fatal("d_str_init");

    for (char *c = s; *c != '\0'; ++c) {
        if (*c == '\n') {
            if (d_strcat(&out, "\\n") == -1)
                fatal("d_str_push");

        } else if (d_str_push(&out, *c) == -1)
            fatal("d_str_push");
    }

    d_str_push(&out, '\n');
    return out.c_str;
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
        strcpy(level_str, CGREEN "INFO" CCL);
    else if (level == L_ERR)
        strcpy(level_str, CRED "ERROR" CCL);
    else if (level == L_WARN)
        strcpy(level_str, CYELLOW "WARN" CCL);
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
            "%s " CDIM "%s:%d:%s:%d " CCL "%s: %s", \
            level_str, basename(file_str), line, func_str, \
            getpid(), msg, errstr);
    } else {
        snprintf(output_str, OUTPUT_SIZE, \
            "%s " CDIM "%s:%d:%s:%d " CCL "%s", \
            level_str, basename(file_str), line, func_str, getpid(), msg);
    }

    char *log = convert_newlines(output_str);
    write(log_fd, log, strlen(log));

    free(log);

    errno = saved_errno;
}
