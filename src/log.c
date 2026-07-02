#define _GNU_SOURCE

#include <limits.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

int log_output_fd;

int log_init() {
    time_t t = time(NULL);
    struct tm *time = localtime(&t);

    char date[50];
    strftime(date, sizeof(date), "%d-%m-%y_%H:%M:%S", time);

    char filename[256];
    strcpy(filename, "log.");
    strcat(filename, date);

    char fullpath[512];
    strcpy(fullpath, "./logs/");
    strcat(fullpath, filename);

    log_output_fd = open(fullpath, O_CREAT | O_RDWR, 0600);
    if (log_output_fd == -1) {
        fprintf(stderr, "failed to create log file %s\n", filename);
        return -1;
    }

    unlink("./logs/latest");

    if (symlink(filename, "./logs/latest") == -1) {
        fprintf(stderr, "error: failed to create sym link %s -> %s\n",
                fullpath, "./logs/latest");
        return -1;
    }

    return 0;
}

/*
  Best effort; drops message on any failure

  level: file:line:func:pid msg[: errstr]
*/
void log_msg(log_level level, const char *errstr, const char *file, int line, \
        const char *func, const char *fmt, ...) {

    int saved_errno = errno;

    char level_str[LOG_BUF_SIZE] = "";
    char file_str[PATH_MAX] = "";
    char func_str[LOG_BUF_SIZE] = "";
    char msg[LOG_BUF_SIZE] = "";

    if (level == L_INFO)
        strcpy(level_str, CGREEN "info" CCL);
    else if (level == L_ERR)
        strcpy(level_str, CRED "error" CCL);
    else
        strcpy(level_str, "???");

    strncpy(file_str, file, PATH_MAX);
    strncpy(func_str, func, LOG_BUF_SIZE);

    va_list va;
    va_start(va, fmt);
    vsnprintf(msg, LOG_BUF_SIZE, fmt, va);
    va_end(va);

    char output_str[OUTPUT_SIZE] = "";

    if (errstr) {
        snprintf(output_str, OUTPUT_SIZE, \
            "%s: " CDIM "%s:%d:%s:%d " CCL "%s: %s\n", \
            level_str, basename(file_str), line, func_str, getpid(), msg, errstr);
    } else {
        snprintf(output_str, OUTPUT_SIZE, \
            "%s: " CDIM "%s:%d:%s:%d " CCL "%s\n", \
            level_str, basename(file_str), line, func_str, getpid(), msg);
    }

    write(log_output_fd, output_str, strlen(output_str));

    errno = saved_errno;
}

int xpipe2(int pipefd[2], int flags) {
    if (pipe2(pipefd, flags) == -1) {
        LOG_ERRNO("pipe2");
        return -1;
    }
    return 0;
}

int xfork(void) {
    pid_t child_pid = fork();

    switch (child_pid) {
    case -1:
        LOG_ERRNO("fork");
        return -1;
    case 0:
        return 0;
    default:
        return child_pid;
    }
}

int xdup2(int oldfd, int newfd) {
    if (dup2(oldfd, newfd) == -1) {
        LOG_ERRNO("dup2");
        return -1;
    }
    return newfd;
}

int xclose(int fd) {
    if (close(fd) == -1) {
        LOG_ERRNO("close");
        return -1;
    }
    return 0;
}

void xexecvp(const char *file, char *const argv[]) {
    execvp(file, argv);
    LOG_ERRNO("execvp");
}

pid_t xwaitpid(pid_t pid, int *wstatus, int options) {
    int wstat;

    pid_t child_pid = waitpid(pid, &wstat, options);
    if (child_pid == -1) {
        if (errno != ECHILD) {
            LOG_ERRNO("waitpid(%d)", pid);
        } else {
            return -1;
        }
    }

    if (wstatus)
        *wstatus = wstat;

    if (WIFEXITED(wstat))
        LOG_INFO("waited for pid=%d (status %d)", child_pid, WEXITSTATUS(wstat));
    else
        LOG_INFO("waited for pid=%d (bad exit)", child_pid);

    return child_pid;
}

