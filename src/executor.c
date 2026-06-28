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
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "log.h"

static int wait_all() {
    int wstat;
    while (true) {
        pid_t pid = waitpid(0, &wstat, 0);
        if (pid == -1) {
            if (errno == ECHILD)
                break;
            else {
                LOG_ERR("waitpid (%s)", strerror(errno));
                return -1;
            }
        }

        LOG_INFO("waited for PID(%d)", pid);
    }

    return 0;
}

static char **create_argv(const ps_cmd *cmd) {
    size_t argc = cmd->words.size + 1;

    char **argv = calloc(argc, sizeof(char *));
    if (!argv)
        return NULL;

    for (size_t i = 0; i < cmd->words.size; ++i)
        argv[i] = cmd->words.data[i].arg;

    argv[argc - 1] = NULL;

    return argv;
}

static int exec_cmd(const ps_cmd *cmd) {
    char **argv = create_argv(cmd);
    if (!argv)
        return -1;

    LOG_INFO("execing %s", cmd->words.data[0].arg);

    execvp(argv[0], argv);

    LOG_ERR("failed to exec %s", cmd->words.data[0].arg);
    _exit(EXIT_FAILURE);
}

static int run_pipeline(const ps_pipeline *pipeline) {
    LOG_INFO("execing %ld cmd pipeline", pipeline->cmds.size);

    pid_t final_pid;
    pid_t child_pid;

    if (pipeline->cmds.size == 1) {

        LOG_INFO("forking");

        switch (final_pid = fork()) {
        case -1:
            LOG_ERR("fork (%s)", strerror(errno));
            return -1;

        case 0:
            exec_cmd(&pipeline->cmds.data[0]);
            LOG_ERR("exec_cmd returned");
            _exit(EXIT_FAILURE);

        default:
            break;
        }
    } else {
        int pfd[2];
        int read_fd;

        for (size_t i = 0; i < pipeline->cmds.size; ++i) {

            if (pipe(pfd) == -1) {
                LOG_ERR("pipe (%s)", strerror(errno));
                return -1;
            }

            LOG_INFO("forking");

            switch (child_pid = fork()) {
            case -1:
                LOG_ERR("fork (%s)", strerror(errno));
                return -1;

            case 0:
                close(pfd[0]);

                if (i != 0) {
                    dup2(read_fd, STDIN_FILENO);
                    if (read_fd != STDIN_FILENO)
                        close(read_fd);
                }

                if (i != pipeline->cmds.size - 1)
                    dup2(pfd[1], STDOUT_FILENO);
                if (pfd[1] != STDOUT_FILENO)
                    close(pfd[1]);

                exec_cmd(&pipeline->cmds.data[i]);

                LOG_ERR("exec_cmd returned");
                _exit(EXIT_FAILURE);

            default:
                close(pfd[1]);
                if (i != 0)
                    close(read_fd);
                read_fd = pfd[0];

                if (i == pipeline->cmds.size - 1)
                    final_pid = child_pid;
                break;
            }
        }

        close(pfd[0]);
    }

    int rv;
    int wstat;
    if (waitpid(final_pid, &wstat, 0) == -1) {
        LOG_ERR("waitpid (%s)", strerror(errno));
        return -1;
    }
    rv = (WIFEXITED(wstat) && WEXITSTATUS(wstat) != 1) ? SUCCESS : FAILURE;

    LOG_INFO("waited for last cmd PID(%d) and exit status %d", final_pid, rv);

    if (wait_all() == -1)
        return -1;

    return rv;
}

int sh_run(const ps_job *job) {
    LOG_INFO("running job");

    int exit_stat = 0;

    for (size_t i = 0; i < job->andors.size; ++i) {
        const ps_andor *andor = &job->andors.data[i];

        switch (andor->op) {
        case PS_NO_IF:
            exit_stat = run_pipeline(&andor->pipeline);
            if (exit_stat == -1)
                return -1;
            break;

        case PS_OR_IF:
            if (exit_stat == SUCCESS) {
                LOG_INFO("|| and last cmd passed; exiting early");
                break;
            }
            exit_stat = run_pipeline(&andor->pipeline);
            if (exit_stat == -1)
                return -1;
            break;

        case PS_AND_IF:
            if (exit_stat == FAILURE) {
                LOG_INFO("&& and last cmd failed; exiting early");
                break;
            }
            exit_stat = run_pipeline(&andor->pipeline);
            if (exit_stat == -1)
                return -1;
            break;
        }
    }

    LOG_INFO("job completed");

    return 0;
}
