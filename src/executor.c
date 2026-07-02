#define _GNU_SOURCE

#include <assert.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <stdlib.h>

#include "executor.h"
#include "executor_types.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "log.h"

static sh_result result = { SH_OK, 0, "" };

PFFORMAT(3, 4)
static void set_sh_result(int exit_code, int err_code, \
        const char *fmt, ...) {

    result = (sh_result) {
        .exit_code = exit_code,
        .err_code = err_code,
        .msg = ""
    };

    if (exit_code == SH_OK || exit_code == SH_EXIT)
        return;

    va_list va;
    va_start(va);

    switch (err_code) {
    case SH_ERRSYS:
        result.err_code = SH_ERRSYS;
        int num_written = snprintf(result.msg, 128, fmt, va);
        snprintf(result.msg + num_written, 128, ": %s", strerror(errno));
        break;
    case SH_ERRREG:
        result.err_code = SH_ERRSYS;
        snprintf(result.msg, 128, fmt, va);
        break;
    default:
        break;
    }

    va_end(va);
    return;
}

PFFORMAT(2, 3)
void err_exit(int exit_code, const char *fmt, ...) {
    va_list va;
    va_start(va);
    vfprintf(stderr, fmt, va);
    va_end(va);

    _exit(exit_code);
}

/* TODO: include more info from wstat in log */
static int wait_for_all() {
    while (true) {
        if (xwaitpid(0, NULL, 0) == -1) {
            if (errno == ECHILD) {
                break;
            } else {
                return -1;
            }
        }
    }

    return 0;
}

void run_exit_builtin(char **argv, sh_builtin_data *data) {
    (void) argv; /* no args for now */
    (void) data;
    LOG_INFO("running builtin exit");
    _exit(EXIT_SUCCESS);
}

static sh_builtin builtins[BUILTIN_COUNT] = {
    { .name = "exit", .func = run_exit_builtin }
};

sh_builtin *get_builtin(const ps_cmd *cmd) {
    const char *name = cmd->words.data[0].arg;

    for (size_t i = 0; i < BUILTIN_COUNT; ++i)
        if (strcmp(builtins[i].name, name) == 0)
            return &builtins[i];

    return NULL;
}

static void exec_or_exit(const ps_cmd *cmd) {
    sh_builtin *builtin = get_builtin(cmd);
    if (builtin)/* builtins shouldn't return */
        builtin->func(cmd->argv, &(sh_builtin_data){ .from_parent = false });

    LOG_INFO("execing %s", cmd->argv[0]);
    xexecvp(cmd->argv[0], cmd->argv);
    err_exit(127, "seashell: command not found: %s\n", cmd->argv[0]);
}

static void dup_fd_or_exit(int fd1, int fd2) {
    if (xdup2(fd1, fd2) == -1)
        _exit(EXIT_FAILURE);

    if (fd1 == fd2)
        return;

    if (xclose(fd1) == -1)
        _exit(EXIT_FAILURE);
}

static bool run_pipeline(const ps_pipeline *pipeline) {
    LOG_INFO("running %ld cmd pipeline", pipeline->cmds.size);

    pid_t final_pid;
    pid_t child_pid;

    int next_pipe[2];
    int prev_read_fd;

    size_t cmd_cnt = pipeline->cmds.size;

    for (size_t i = 0; i < cmd_cnt; ++i) {

        bool first = (i == 0);
        bool last = (i == cmd_cnt - 1);

        if (!last) {
            LOG_INFO("calling pipe2");
            if (xpipe2(next_pipe, O_CLOEXEC) == -1) {
                set_sh_result(SH_FAIL, SH_ERRSYS, "pipe");
                return -1;
            }
        }

        LOG_INFO("forking");

        if ((child_pid = xfork()) == -1) {
            set_sh_result(SH_FAIL, SH_ERRSYS, "fork");
            return -1;
        }

        if (child_pid == 0) {
            if (!first)
                dup_fd_or_exit(prev_read_fd, STDIN_FILENO);

            if (!last) {
                dup_fd_or_exit(next_pipe[1], STDOUT_FILENO);

                if (xclose(next_pipe[0]) == -1)
                    err_exit(EXIT_FAILURE, "close %s\n", strerror(errno));
            }

            exec_or_exit(&pipeline->cmds.data[i]);
        }

        if (!first)
            if (xclose(prev_read_fd) == -1)
                set_sh_result(SH_FAIL, SH_ERRSYS, "close");

        if (!last) {
            prev_read_fd = next_pipe[0];

            if (xclose(next_pipe[1]) == -1)
                set_sh_result(SH_FAIL, SH_ERRSYS, "close");
        }

        if (last) /* final child determines pipe exit status */
            final_pid = child_pid;
    }

    int wstat;
    if (xwaitpid(final_pid, &wstat, 0) == -1) {
        set_sh_result(SH_FAIL, SH_ERRSYS, "waitpid");
        return -1;
    }

    if (wait_for_all() == -1)
        return -1;

    return WEXITSTATUS(wstat) ? false : true;
}

sh_result sh_run(const ps_job *job) {
    int pipeline_succeeded = 0;

    for (size_t i = 0; i < job->andors.size; ++i) {
        const ps_andor *andor = &job->andors.data[i];

        switch (andor->op) {
        case PS_NO_IF:
            pipeline_succeeded = run_pipeline(&andor->pipeline);
            if (!pipeline_succeeded)
                goto done;
            break;

        case PS_OR_IF:
            if (pipeline_succeeded) {
                LOG_INFO("|| and last cmd passed; finishing early");
                goto done;
            }
            pipeline_succeeded = run_pipeline(&andor->pipeline);
            if (!pipeline_succeeded)
                goto done;
            break;

        case PS_AND_IF:
            if (!pipeline_succeeded) {
                LOG_INFO("&& and last cmd failed; finishing early");
                goto done;
            }
            pipeline_succeeded = run_pipeline(&andor->pipeline);
            if (!pipeline_succeeded)
                goto done;
            break;
        }
    }

done:
    return result;
}
