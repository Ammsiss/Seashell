#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "unity_fixture.h"
#include "llog.h"

#define DEFAULT_LOG_DIR "/home/juta/Projects/Seashell/logs/test/"

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

void run_all(void) {
    llog_set_fd(open_log_file(DEFAULT_LOG_DIR));

    RUN_TEST_GROUP(array);
    RUN_TEST_GROUP(string);
    RUN_TEST_GROUP(lexer);
    RUN_TEST_GROUP(parser);
    RUN_TEST_GROUP(jobs);
    RUN_TEST_GROUP(shell);
}

int main(int argc, const char **argv) {
    return UnityMain(argc, argv, run_all);
}
