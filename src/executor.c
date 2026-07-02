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

static sh_env shell_env = { .subshell = true };

void output_err(const char *fmt, va_list *va, bool print_err) {
    fflush(stdout);

    char user_msg[BUF_SIZE] = "";
    char err_str[BUF_SIZE] = "";

    vsnprintf(user_msg, BUF_SIZE, fmt, *va);

    if (print_err) {
        strncat(err_str, strerror(errno), BUF_SIZE);
        fprintf(stderr, "seashell: %s: %s\n", user_msg, err_str);
    } else
        fprintf(stderr, "seashell: %s\n", user_msg);
}

PFFORMAT(3, 4)
void errExit(int exit_code, bool print_err, const char *fmt, ...) {
    va_list va;
    va_start(va);
    output_err(fmt, &va, print_err);
    va_end(va);

    exit(exit_code);
}

PFFORMAT(3, 4)
void err_exit(int exit_code, bool print_err, const char *fmt, ...) {
    va_list va;
    va_start(va);
    output_err(fmt, &va, print_err);
    va_end(va);

    _exit(exit_code);
}

PFFORMAT(2, 3)
void err_msg(bool print_err, const char *fmt, ...) {
    va_list va;
    va_start(va);
    output_err(fmt, &va, print_err);
    va_end(va);
}

/* TODO: include more info from wstat in log */
static int wait_for_all() {
    while (true) {
        if (xwaitpid(0, NULL, 0) == -1) {
            if (errno == ECHILD) {
                break;
            } else {
                err_msg(true, "waitpid");
                return -1;
            }
        }
    }

    return 0;
}

int run_exit_builtin(char **argv, sh_env *shell_env) {
    (void) argv; /* no args for now */
    (void) shell_env;

    LOG_INFO("running builtin exit");

    if (shell_env->subshell)
        _exit(EXIT_SUCCESS);
    else {
        printf("exit\n");
        exit(EXIT_SUCCESS);
    }

    return 0;
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

int try_run_builtin(const ps_cmd *cmd) {
    sh_builtin *builtin = get_builtin(cmd);
    if (builtin)
        return builtin->func(cmd->argv, &shell_env);

    return -1;
}

static void dup_fd_or_exit(int fd1, int fd2) {
    if (xdup2(fd1, fd2) == -1)
        err_exit(EXIT_FAILURE, true, "dup2");

    if (fd1 == fd2)
        return;

    if (xclose(fd1) == -1)
        err_exit(EXIT_FAILURE, true, "close");
}

static pid_t exec_pipeline(const ps_pipeline *pipeline) {
    pid_t child_pid;
    pid_t final_pid;

    int next_pipe[2];
    int prev_read_fd;

    size_t cmd_cnt = pipeline->cmds.size;

    for (size_t i = 0; i < cmd_cnt; ++i) {
        ps_cmd *cmd = &pipeline->cmds.data[i];
        bool first = (i == 0);
        bool last = (i == cmd_cnt - 1);

        if (!last) {
            LOG_INFO("calling pipe2");
            if (xpipe2(next_pipe, O_CLOEXEC) == -1) {
                err_msg(true, "pipe2");
                goto fail;
            }
        }

        LOG_INFO("forking");

        if ((child_pid = xfork()) == -1) {
            err_msg(true, "fork");
            goto fail;
        }

        if (child_pid == 0) {
            shell_env.subshell = true;

            if (!first)
                dup_fd_or_exit(prev_read_fd, STDIN_FILENO);

            if (!last) {
                dup_fd_or_exit(next_pipe[1], STDOUT_FILENO);

                if (xclose(next_pipe[0]) == -1)
                    err_exit(EXIT_FAILURE, true, "close");
            }

            if (try_run_builtin(&pipeline->cmds.data[i]) != -1)
                _exit(EXIT_SUCCESS); /* child always exits after builtin */

            LOG_INFO("execing %s", cmd->argv[0]);
            xexecvp(cmd->argv[0], cmd->argv);
            err_exit(127, false, "command not found: %s", cmd->argv[0]);
        }

        if (!first)
            if (xclose(prev_read_fd) == -1) {
                err_msg(true, "close");
                goto fail;
            }

        if (!last) {
            prev_read_fd = next_pipe[0];

            if (xclose(next_pipe[1]) == -1) {
                err_msg(true, "close");
                goto fail;
            }
        }

        if (last) /* final child determines pipe exit status */
            final_pid = child_pid;
    }

    int wstat;
    if (xwaitpid(final_pid, &wstat, 0) == -1) {
        err_msg(true, "waitpid");
        goto fail;
    }

    if (wait_for_all() == -1)
        return -1;

    if (WIFEXITED(wstat))
        return WEXITSTATUS(wstat);

fail:
    wait_for_all();
    return -1;
}

static int run_pipeline(const ps_pipeline *pipeline) {
    LOG_INFO("running %ld cmd pipeline", pipeline->cmds.size);

    ps_cmd *cmd = &pipeline->cmds.data[0];

    if (pipeline->cmds.size == 1) {
        int builtin_status = try_run_builtin(cmd);
        if (builtin_status != -1)
            return builtin_status;
    }

    return exec_pipeline(pipeline);
}

int sh_run(const ps_job *job) {
    int pipeline_status = EXIT_SUCCESS;

    for (size_t i = 0; i < job->andors.size; ++i) {
        const ps_andor *andor = &job->andors.data[i];

        if (andor->op == PS_OR_IF && pipeline_status == EXIT_SUCCESS)
            continue;

        if (andor->op == PS_AND_IF && pipeline_status != EXIT_SUCCESS)
            continue;

        pipeline_status = run_pipeline(&andor->pipeline);
        if (pipeline_status == -1)
            goto done;
    }

done:
    return pipeline_status;
}
