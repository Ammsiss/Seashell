#define _GNU_SOURCE

#include <assert.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <stdlib.h>

#include "utils.h"
#include "executor.h"
#include "executor_types.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "log.h"
#include "shell_state.h"

static sh_env shell_env = { .subshell = false };

int run_exit_builtin(char **argv, sh_env *shell_env) {
    (void) argv; /* no args for now */

    LOG_INFO("running builtin exit");

    if (shell_env->subshell)
        _exit(EXIT_SUCCESS);
    else {
        printf("exit\n");
        exit(EXIT_SUCCESS);
    }

    return EXIT_FAILURE;
}

int run_cd_builtin(char **argv, sh_env *shell_env) {
    (void) shell_env;

    if (!argv || !argv[0]) {
        LOG_ERR("builtin cd received invalid argv structure");
        err_msg("cd: internal error check logs");
        return EXIT_FAILURE;
    }

    if (!argv[1]) {
        err_msg("cd: path required");
        return EXIT_FAILURE;
    }

    if (argv[2]) {
        err_msg("cd: too many arguments");
        return EXIT_FAILURE;
    }

    if (chdir(argv[1]) == -1) {
        perror("cd");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int run_set_builtin(char **argv, sh_env *shell_env) {
    (void) shell_env;

    if (!argv || !argv[0]) {
        LOG_ERR("builtin set received invalid argv structure");
        err_msg("set: internal error check logs");
        return EXIT_FAILURE;
    }

    if (!argv[1] || !argv[2]) {
        err_msg("set: not enough arguments");
        return EXIT_FAILURE;
    }

    if (argv[3]) {
        err_msg("set: too many arguments");
        return EXIT_FAILURE;
    }

    var_pair var = {0};

    if (strlen(argv[1]) >= SHELL_VAR_MAX)
        err_msg("set: key too long: %s", argv[1]);

    if (strlen(argv[2]) >= SHELL_VAR_MAX)
        err_msg("set: value too long: %s", argv[2]);

    strcpy(var.key, argv[1]);
    strcpy(var.value, argv[2]);

    LOG_INFO("saved variable %s=%s", var.key, var.value);

    st_add_var(&var);

    return EXIT_SUCCESS;
}

static sh_builtin builtins[BUILTIN_COUNT] = {
    { .name = "exit", .func = run_exit_builtin },
    { .name = "cd", .func = run_cd_builtin },
    { .name = "set", .func = run_set_builtin }
};

sh_builtin *get_builtin(const ps_cmd *cmd) {
    const char *name = cmd->words.data[0].arg;

    for (size_t i = 0; i < BUILTIN_COUNT; ++i)
        if (strcmp(builtins[i].name, name) == 0)
            return &builtins[i];

    return NULL;
}

static bool try_run_builtin(const ps_cmd *cmd, int *status) {
    sh_builtin *builtin = get_builtin(cmd);
    if (builtin) {
        *status = builtin->func(cmd->argv, &shell_env);
        return true;
    }

    return false;
}

static int wait_for_pids(da_pid *pids) {
    int wstat;
    int last_status;

    for (size_t i = 0; i < pids->size; ++i) {
        pid_t pid = pids->data[i];

        if (xwaitpid(pid, &wstat, 0) == -1) {
            errno_msg("waitpid");
            goto fail;
        }

        if (i == pids->size - 1) {
            if (WIFEXITED(wstat)) {
                last_status = WEXITSTATUS(wstat);
            } else if (WIFSIGNALED(wstat)) {
                int signum = WTERMSIG(wstat);
                fprintf(stderr, "terminated by signal %d (%s)",
                        signum, strsignal(signum));
#ifdef WCOREDUMP
                if (WCOREDUMP(wstat))
                    printf(" (core dumped)");
#endif
                printf("\n");

                last_status = 128 + signum;
            } else
                goto fail;
        }
    }

    da_free(pids);
    return last_status;

fail:
    da_free(pids);
    return -1;
}

static int move_fd(int fd1, int fd2) {
    if (fd1 == fd2)
        return 0;

    if (xdup2(fd1, fd2) == -1) {
        errno_msg("dup2");
        return -1;
    }

    if (xclose(fd1) == -1) {
        errno_msg("close");
        return -1;
    }

    return 0;
}

static int exec_pline(const ps_pline *pline, da_pid *pids, \
        int inputfd, int outputfd) {
    da_init(pids);

    pid_t child_pid;

    int next_pipe[2];
    int prev_read_fd;

    size_t cmd_cnt = pline->cmds.size;

    for (size_t i = 0; i < cmd_cnt; ++i) {
        ps_cmd *cmd = &pline->cmds.data[i];
        bool first = (i == 0);
        bool last = (i == cmd_cnt - 1);

        if (!last) {
            if (xpipe(next_pipe) == -1) {
                errno_msg("pipe");
                goto fail;
            }
        }

        child_pid = xfork();
        if (child_pid == -1) {
            errno_msg("fork");
            goto fail;
        }

        pid_t *pid = da_push(pids);
        if (!pid) {
            err_msg("da_push");
            goto fail;
        }
        *pid = child_pid;

        if (child_pid == 0) {
            shell_env.subshell = true;

            if (first) {
                if (move_fd(inputfd, STDIN_FILENO) == -1)
                    _exit(EXIT_FAILURE);
            } else if (!first)
                if (move_fd(prev_read_fd, STDIN_FILENO) == -1)
                    _exit(EXIT_FAILURE);

            if (last) {
                if (move_fd(outputfd, STDOUT_FILENO) == -1)
                    _exit(EXIT_FAILURE);
            } else if (!last) {
                if (move_fd(next_pipe[1], STDOUT_FILENO) == -1)
                    _exit(EXIT_FAILURE);

                if (close(next_pipe[0]) == -1)
                    err_exit(EXIT_FAILURE, true, "close");
            }

            int status;
            if (try_run_builtin(cmd, &status))
                _exit(status);

            xexecvp(cmd->argv[0], cmd->argv);

            if (errno == ENOENT) {
                err_msg("command not found: %s", cmd->argv[0]);
            } else
                errno_msg("execvp");

            _exit(EXIT_FAILURE);
        }

        if (!first)
            if (xclose(prev_read_fd) == -1) {
                errno_msg("close");
                goto fail;
            }

        if (!last) {
            prev_read_fd = next_pipe[0];

            if (xclose(next_pipe[1]) == -1) {
                errno_msg("close");
                goto fail;
            }
        }
    }

    return 0;

fail:
    da_free(pids);
    return -1;
}

static bool run_pline(const ps_pline *pline, int inputfd, int outputfd) {
    ps_cmd *cmd = &pline->cmds.data[0];

    if (pline->cmds.size == 1) {

        int bltin_status;
        if (try_run_builtin(cmd, &bltin_status))
            return bltin_status == EXIT_SUCCESS;
    }

    da_pid pids;
    if (exec_pline(pline, &pids, inputfd, outputfd) == -1)
        return false;

    int last_status = wait_for_pids(&pids);
    if (last_status == -1)
        fatal("fatal: bad job control state");

    return last_status == EXIT_SUCCESS;
}

void sh_run(const ps_job *job, int inputfd, int outputfd) {
    bool pline_success = true;

    for (size_t i = 0; i < job->andors.size; ++i) {
        const ps_andor *andor = &job->andors.data[i];

        if (andor->op == PS_OR_IF && pline_success) {
            LOG_INFO("skipping next pipeline (||)");
            continue;
        }

        if (andor->op == PS_AND_IF && !pline_success) {
            LOG_INFO("skipping next pipeline (&&)");
            continue;
        }

        pline_success = run_pline(&andor->pline, inputfd, outputfd);
    }
}
