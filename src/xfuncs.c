#include <stdlib.h>

#include "xfuncs.h"
#include "llog.h"
#include "utils.h"

int xdup2_at(const char *file, int line, int oldfd, int newfd) {
    int rv = dup2(oldfd, newfd);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "dup2: %m");
        err_exit("dup2");
    }

    return rv;
}

int xclose_at(const char *file, int line, int fd) {
    int rv = close(fd);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "close: %m");
        err_exit("close");
    }

    return rv;
}

void *xmalloc_at(const char *file, int line, int size) {
    void *rv = malloc(size);
    if (!rv) {
        llog_log(LLOG_ERR, file, line, "malloc: %m");
        err_exit("malloc");
    }

    return rv;
}

int xsigaction_at(const char *file, int line, int signum,
    const struct sigaction *act, struct sigaction *oldact) {
    int rv = sigaction(signum, act, oldact);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "sigaction: %m");
        err_exit("sigaction");
    }

    return rv;
}

int xsigemptyset_at(const char *file, int line, sigset_t *set) {
    int rv = sigemptyset(set);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "sigemptyset: %m");
        err_exit("sigemptyset");
    }

    return rv;
}

int xsigaddset_at(const char *file, int line, sigset_t *set, int signum) {
    int rv = sigaddset(set, signum);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "sigaddset: %m");
        err_exit("sigaddset");
    }

    return rv;
}

int xsigdelset_at(const char *file, int line, sigset_t *set, int signum) {
    int rv = sigdelset(set, signum);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "sigdelset: %m");
        err_exit("sigdelset");
    }

    return rv;
}

int xsigprocmask_at(const char *file, int line, int how, const sigset_t *set,
        sigset_t *oldset) {
    int rv = sigprocmask(how, set, oldset);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "sigprocmask: %m");
        err_exit("sigprocmask");
    }

    return rv;
}

int xsetpgid_at(const char *file, int line, pid_t pid, pid_t pgid) {
    int rv = setpgid(pid, pgid);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "setpgid: %m");
        err_exit("setpgid");
    }

    return rv;
}

int xtcsetpgrp_at(const char *file, int line, int fd, pid_t pgrp) {
    int rv = tcsetpgrp(fd, pgrp);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "tcsetpgrp: %m");
        err_exit("tcsetpgrp");
    }

    return rv;
}

char *xgetcwd_at(const char *file, int line, char *buf, size_t size) {
    char *rv = getcwd(buf, size);
    if (!rv) {
        llog_log(LLOG_ERR, file, line, "getcwd: %m");
        err_exit("getcwd");
    }

    return rv;
}

int xkill_at(const char *file, int line, pid_t pid, int sig) {
    int rv = kill(pid, sig);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "kill: %m");
        err_exit("kill");
    }

    return rv;
}

int xatexit_at(const char *file, int line, void (*function)(void)) {
    int rv = atexit(function);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "atexit: %m");
        err_exit("atexit");
    }

    return rv;
}

int xpipe_at(const char *file, int line, int pipefd[2]) {
    int rv = pipe(pipefd);
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "pipe: %m");
        err_exit("pipe");
    }

    return rv;
}

int xfork_at(const char *file, int line) {
    int rv = fork();
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, "fork: %m");
        err_exit("fork");
    }

    return rv;
}

/*
    int rv = ();
    if (rv == -1) {
        llog_log(LLOG_ERR, file, line, ": %m");
        err_exit("");
    }

    return rv;
}
*/
