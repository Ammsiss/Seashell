#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#include "shell_state.h"
#include "sig_funcs.h"
#include "xfuncs.h"

#define DEFAULT_LOG_DIR "/home/juta/Projects/Seashell/logs/shell/"

shell_env sh_env = {0};

opt_data opts[] = {
    [LOGFD] = {
        .long_name = "logfd",
        .short_name = 'l',
        .has_arg = REQUIRED_ARG,
        .arg_type = INT_ARG
    },
    [LOGDIR] = {
        .long_name = "logdir",
        .short_name = 'p',
        .has_arg = REQUIRED_ARG,
        .arg_type = STR_ARG
    },
    (opt_data){0}
};

static int open_log_file(const char *log_dir) {
    char log_path[PATH_MAX] = "";
    char link_path[PATH_MAX] = "";

    strcpy(log_path, log_dir);
    strcat(log_path, "/log-XXXXXX");
    int fd = mkstemp(log_path);

    if (fd == -1) {
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

static void log_setup(int *log_fd) {
    if (opts[LOGFD].found) {
        if (fcntl(opts[LOGFD].val_int, F_GETFD) == -1) {
            perror("Error using log fd");
            exit(EXIT_FAILURE);
        }

        *log_fd = opts[LOGFD].val_int;

    } else if (opts[LOGDIR].found) {
        *log_fd = open_log_file(opts[LOGDIR].val_str);

    } else
        *log_fd = open_log_file(DEFAULT_LOG_DIR);

    llog_set_fd(*log_fd);
}

bool shell_in_fg(void) {
    return sh_env.fg_jid == NOFG;
}

void env_setup(shell_env *env) {
    log_setup(&env->log_fd);
    sig_setup(&env->og_mask);

    env->subshell = false;
    env->fg_jid = NOFG;
    env->tty_fd = xopen("/dev/tty", O_RDWR | O_CLOEXEC);

    LOG_INFO("seashell PID(%d)", getpid());
}
