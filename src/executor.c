#include <stdarg.h>
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <stdlib.h>

#include "executor.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes

int log_fd;

#define PFFORMAT(x, y) __attribute__ ((format(printf, (x), (y))))

// TODO: error reporting
PFFORMAT(1, 2) void log_msg(const char *fmt, ...) {
    va_list va;
    char msg[256];

    va_start(va, fmt);
    vsnprintf(msg, 256, fmt, va);
    va_end(va);

    const char *header = "info: ";
    write(log_fd, header, strlen(header));
    write(log_fd, msg, strlen(msg));
    write(log_fd, "\n", 1);
}

char **create_argv(const ps_cmd *cmd) {
    size_t argc = cmd->words.size + 1;

    char **argv = calloc(argc, sizeof(char *));
    if (!argv)
        return NULL;

    for (size_t i = 0; i < cmd->words.size; ++i)
        argv[i] = cmd->words.data[i].arg;

    argv[argc - 1] = NULL;

    return argv;
}

/* Assumes fork already happend */
int exec_cmd(const ps_cmd *cmd) {
    char **argv = create_argv(cmd);
    if (!argv)
        return -1;

    log_msg("PID(%d) execing %s", getpid(), cmd->words.data[0].arg);

    execvp(argv[0], argv);
    _exit(EXIT_FAILURE);;
}

int pipe_fork(void) {

    int pfd[2];
    if (pipe(pfd) == -1)
        return -1;

    log_msg("PID(%d) pipe forking", getpid());

    pid_t child_pid;
    switch (child_pid = fork()) {
    case -1:
        return -1;

    case 0: /* child */
        if (close(pfd[1]) == -1)
            return -1;
        if (dup2(pfd[0], STDIN_FILENO) == -1)
            return -1;
        if (STDIN_FILENO != pfd[0])
            if (close(pfd[0]) == -1)
                return -1;

        return 0;

    default: /* parent */
        if (close(pfd[0]) == -1)
            return -1;
        if (dup2(pfd[1], STDOUT_FILENO) == -1)
            return -1;
        if (STDOUT_FILENO != pfd[1])
            if (close(pfd[1]) == -1)
                return -1;

        return child_pid;
    }
}

int run_pipeline(const ps_pipeline *pipeline, size_t i) {
    if (i == pipeline->cmds.size - 1) {
        exec_cmd(&pipeline->cmds.data[i]);
        return -1;
    }

    switch (pipe_fork()) {
    case -1:
        return -1;

    case 0: /* child */
        log_msg("PID(%d) PPID(%d) started", getpid(), getppid());

        if (run_pipeline(pipeline, ++i) == -1)
            return -1;
    default: /* parent */
        exec_cmd(&pipeline->cmds.data[i]);
        return -1;
    }
}

int sh_run(const ps_job *job) {
    char template[] = "log.XXXXXX";
    log_fd = mkstemp(template);

    if (log_fd == -1) {
        fprintf(stderr, "waitpid (%s)", strerror(errno));
        return -1;
    }

    log_msg("PID(%d) running job", getpid());

    for (size_t i = 0; i < job->andors.size; ++i) {
        const ps_andor *andor = &job->andors.data[i];

        log_msg("PID(%d) forking", getpid());

        switch (andor->op) {
        case PS_NO_IF:
            switch (fork()) {
            case -1:
                return -1;
            case 0:
                log_msg("PID(%d) running %ld cmd pipeline", getpid(),
                        andor->pipeline.cmds.size);
                run_pipeline(&andor->pipeline, 0);
                return -1; /* No child process should return here */
            default:
                break;
        }

        case PS_OR_IF:
        case PS_AND_IF:
        }
    }

    log_msg("PID(%d) waiting...", getpid());

    int wstat;
    while (true) {
        pid_t pid = waitpid(0, &wstat, 0);
        if (pid == -1) {
            if (errno == ECHILD)
                break;
            else {
                fprintf(stderr, "waitpid (%s)", strerror(errno));
                return -1;
            }
        }

        log_msg("PID(%d) waited for PID(%d)", getpid(), pid);
    }

    log_msg("PID(%d) finished waiting", getpid());

    return 0;
}
