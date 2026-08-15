#ifndef XFUNCS_H
#define XFUNCS_H

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "utils.h" // IWYU pragma: export

/* Macro implementation is nice for open()'s weird 2,3 argument inerface */
#define xopen(file, oflags, ...) \
    ({ \
        int rv = open(file, oflags __VA_OPT__(,) __VA_ARGS__); \
        if (rv == -1) { \
            llog_log(LLOG_ERR, __FILE__, __LINE__, "open: %m"); \
            err_exit("open"); \
        } \
        rv; \
    })

#define xdup2(...) xdup2_at(__FILE__, __LINE__, __VA_ARGS__)
int xdup2_at(const char *file, int line, int oldfd, int newfd);

#define xclose(...) xclose_at(__FILE__, __LINE__, __VA_ARGS__)
int xclose_at(const char *file, int line, int fd);

#define xmalloc(...) xmalloc_at(__FILE__, __LINE__, __VA_ARGS__)
void *xmalloc_at(const char *file, int line, int size);

#define xsigaction(...) xsigaction_at(__FILE__, __LINE__, __VA_ARGS__)
int xsigaction_at(const char *file, int line, int signum,
        const struct sigaction *act, struct sigaction *oldact);

#define xsigemptyset(...) xsigemptyset_at(__FILE__, __LINE__, __VA_ARGS__)
int xsigemptyset_at(const char *file, int line, sigset_t *set);

#define xsigaddset(...) xsigaddset_at(__FILE__, __LINE__, __VA_ARGS__)
int xsigaddset_at(const char *file, int line, sigset_t *set, int signum);

#define xsigdelset(...) xsigdelset_at(__FILE__, __LINE__, __VA_ARGS__)
int xsigdelset_at(const char *file, int line, sigset_t *set, int signum);

#define xsigprocmask(...) xsigprocmask_at(__FILE__, __LINE__, __VA_ARGS__)
int xsigprocmask_at(const char *file, int line, int how, const sigset_t *set,
        sigset_t *oldset);

#define xsetpgid(...) xsetpgid_at(__FILE__, __LINE__, __VA_ARGS__)
int xsetpgid_at(const char *file, int line, pid_t pid, pid_t pgid);

#define xtcsetpgrp(...) xtcsetpgrp_at(__FILE__, __LINE__, __VA_ARGS__)
int xtcsetpgrp_at(const char *file, int line, int fd, pid_t pgrp);

#define xgetcwd(...) xgetcwd_at(__FILE__, __LINE__, __VA_ARGS__)
char *xgetcwd_at(const char *file, int line, char *buf, size_t size);

#define xkill(...) xkill_at(__FILE__, __LINE__, __VA_ARGS__)
int xkill_at(const char *file, int line, pid_t pid, int sig);

#define xatexit(...) xatexit_at(__FILE__, __LINE__, __VA_ARGS__)
int xatexit_at(const char *file, int line, void (*function)(void));

#define xpipe(...) xpipe_at(__FILE__, __LINE__, __VA_ARGS__)
int xpipe_at(const char *file, int line, int pipefd[2]);

#define xfork() xfork_at(__FILE__, __LINE__)
int xfork_at(const char *file, int line);

/*
#define x(...) x_at(__FILE__, __LINE__, __VA_ARGS__)
int x_at(const char *file, int line, );
*/

#endif
