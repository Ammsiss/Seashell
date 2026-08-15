#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>

#include "log.h"
#include "llog.h"
#include "opts.h"

#define DEFAULT_LOG_DIR "/home/juta/Projects/Seashell/logs/shell/"

static int log_fd;

static int open_log_file(const char *log_dir) {
    char log_path[PATH_MAX] = "";
    char link_path[PATH_MAX] = "";

    strcpy(log_path, log_dir);
    strcat(log_path, "/log-XXXXXX");
    int fd = mkstemp(log_path);

    if (log_fd == -1) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }

    strcpy(link_path, log_dir);
    strcat(link_path, "/latest");

    if (unlink(link_path) == -1 && errno != ENOENT) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }

    char *abs_log_path = realpath(log_path, NULL);

    if (!abs_log_path) {
        perror("realpath");
        exit(EXIT_FAILURE);
    }

    if (symlink(abs_log_path, link_path) == -1) {
        perror("symlink");
        exit(EXIT_FAILURE);
    }

    free(abs_log_path);

    return fd;
}

void log_setup(void) {
    if (opts[LOGFD].found) {
        if (fcntl(opts[LOGFD].val_int, F_GETFD) == -1) {
            perror("Error using log fd");
            exit(EXIT_FAILURE);
        }

        log_fd = opts[LOGFD].val_int;

    } else if (opts[LOGDIR].found) {
        log_fd = open_log_file(opts[LOGDIR].val_str);

    } else
        log_fd = open_log_file(DEFAULT_LOG_DIR);

    llog_set_fd(log_fd);
}

