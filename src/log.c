#define _GNU_SOURCE

#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

int stored_fd;

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

    stored_fd = open(fullpath, O_CREAT | O_RDWR, 0600);
    if (stored_fd == -1) {
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
1. Split building the message with delivering the message
2. Use a data structure as the base for a log module to represent log messages
3. Define a "max log" size so len does not have to be dynamically created
4. Provide macros for source context like file number, function...
5. Color should be logically associated with log level not encoded into the header
*/

/* Best effort; drops message on any failure */
void log_msg(const char *header, exit_type how_exit, bool print_errno, \
        const char *fmt, ...) {
    int saved_errno = errno;

    int mlen = 0;
    char *out_msg = NULL;

    /* header */

    int hlen = strlen(header);
    mlen += hlen;

    /* pid */

    pid_t pid = getpid();
    int pid_len = snprintf(NULL, 0, "pid=%d ", pid);
    mlen += pid_len;

    /* variadic message */

    va_list va;
    va_start(va, fmt);
    va_list vlen;
    va_copy(vlen, va);

    mlen += vsnprintf(NULL, 0, fmt, vlen);
    if (mlen < 0)
        goto fail;

    /* optional strerror */

    errno = 0;
    const char *err_msg = strerror(saved_errno);
    if (errno != 0)
        goto fail;

    if (print_errno)
        mlen += /*: */2 + strlen(err_msg);

    /* final newline */

    mlen += 1;

    /* construct message */

    out_msg = malloc(mlen + /*\0*/1);
    if (!out_msg)
        goto fail;

    strcpy(out_msg, header);

    snprintf(out_msg + hlen, pid_len + 1, "pid=%d ", pid);

    if (vsprintf(out_msg + hlen + pid_len, fmt, va) <= 0)
        goto fail;

    if (print_errno) {
        strcat(out_msg, ": ");
        strcat(out_msg, err_msg);
    }

    strcat(out_msg, "\n");

    if (write(stored_fd, out_msg, mlen) != mlen)
        goto fail;

    va_end(vlen);
    va_end(va);
    free(out_msg);
    errno = saved_errno;

    switch (how_exit) {
    case EXIT_U:
        _exit(EXIT_FAILURE);
    case EXIT:
        exit(EXIT_FAILURE);
    case NO_EXIT:
        break;
    }

    return;

fail:
    va_end(vlen);
    va_end(va);
    free(out_msg);
    errno = saved_errno;
}

int xpipe(int pipefd[2]) {
    if (pipe(pipefd) == -1) {
        LOG_ERRNO("pipe");
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
    pid_t child_pid = waitpid(pid, wstatus, options);
    if (child_pid == -1) {
        if (errno != ECHILD)
            LOG_ERRNO("waitpid");
        return -1;
    }

    return child_pid;
}

