#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>

#include "log.h"
#include "llog.h"

#define LOG_DIR "/home/juta/Projects/Seashell/logs/shell/"

static int log_fd;

void log_setup(void) {
    char path[] = LOG_DIR "log-XXXXXX";

    log_fd = mkstemp(path);

    if (log_fd == -1) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }

    if (unlink(LOG_DIR "latest") == -1 && errno != ENOENT) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }

    if (symlink(path, LOG_DIR "latest") == -1) {
        perror("symlink");
        exit(EXIT_FAILURE);
    }

    llog_set_fd(log_fd);
}

