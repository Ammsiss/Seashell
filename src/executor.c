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

static char **create_argv_or_exit(const ps_cmd *cmd) {
    size_t argc = cmd->words.size + 1;

    char **argv = calloc(argc, sizeof(char *));
    if (!argv) {
        LOG_ERRNO("failed to create argv: calloc");
        _exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < cmd->words.size; ++i)
        argv[i] = cmd->words.data[i].arg;

    argv[argc - 1] = NULL;

    return argv;
}

static void exec_cmd_or_exit(const ps_cmd *cmd) {
    char **argv = create_argv_or_exit(cmd);

    LOG_INFO("execing %s", argv[0]);

    xexecvp(argv[0], argv);
    err_exit(127, "seashell: command not found: %s\n", argv[0]);
}

static bool run_pipeline(const ps_pipeline *pipeline) {
    LOG_INFO("running %ld cmd pipeline", pipeline->cmds.size);

    pid_t final_pid;
    pid_t child_pid;

    if (pipeline->cmds.size == 1) {

        /* check if builtin; run if it is */
        if (strcmp("exit", pipeline->cmds.data[0].words.data[0].arg) == 0) {
            LOG_INFO("running builtin exit");
            set_sh_result(SH_EXIT, 0, NULL);
            return true;
        }

        LOG_INFO("forking");

        switch (final_pid = xfork()) {
        case -1:
            set_sh_result(SH_FAIL, SH_ERRSYS, "fork");
            return -1;

        case 0:
            exec_cmd_or_exit(&pipeline->cmds.data[0]);

        default:
            break;
        }
    } else {
        int pfd[2];
        int read_fd;

        for (size_t i = 0; i < pipeline->cmds.size; ++i) {
            if (xpipe(pfd) == -1) {
                set_sh_result(SH_FAIL, SH_ERRSYS, "pipe");
                return -1;
            }

            LOG_INFO("forking");

            switch (child_pid = xfork()) {
            case -1:
                set_sh_result(SH_FAIL, SH_ERRSYS, "fork");
                return -1;

            case 0:

                /* check if builtin; run it if it is */
                if (strcmp("exit", pipeline->cmds.data[i].words.data[0].arg) == 0) {
                    LOG_INFO("running builtin exit");
                    _exit(EXIT_SUCCESS);
                }


                if (xclose(pfd[0]) == -1)
                    err_exit(EXIT_FAILURE, "close %s\n", strerror(errno));
                if (i != 0) {
                    if (xdup2(read_fd, STDIN_FILENO) == -1)
                        err_exit(EXIT_FAILURE, "dup2 %s\n", strerror(errno));
                    if (read_fd != STDIN_FILENO)
                        if (xclose(read_fd) == -1)
                            err_exit(EXIT_FAILURE, "close %s\n", strerror(errno));
                }
                if (i != pipeline->cmds.size - 1)
                    if (xdup2(pfd[1], STDOUT_FILENO) == -1)
                        err_exit(EXIT_FAILURE, "dup2 %s\n", strerror(errno));
                if (pfd[1] != STDOUT_FILENO)
                    if (xclose(pfd[1]) == -1)
                        err_exit(EXIT_FAILURE, "close %s\n", strerror(errno));

                exec_cmd_or_exit(&pipeline->cmds.data[i]);

            default:
                if (xclose(pfd[1]) == -1)
                    set_sh_result(SH_FAIL, SH_ERRSYS, "close");
                if (i != 0)
                    if (xclose(read_fd) == -1)
                        set_sh_result(SH_FAIL, SH_ERRSYS, "close");
                read_fd = pfd[0];
                if (i == pipeline->cmds.size - 1)
                    final_pid = child_pid;
                break;
            }
        }

        if (xclose(pfd[0]) == -1)
            set_sh_result(SH_FAIL, SH_ERRSYS, "close");
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
