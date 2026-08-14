#ifndef LOG_H
#define LOG_H

#define _GNU_SOURCE

#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> // IWYU pragma: keep

#include "llog.h"

void log_setup(void);

#define LOG_ERRNO(fmt, ...) \
    ((void)0)

#define xdup2(fd1, fd2) \
    ({ \
        int rv = dup2(fd1, fd2); \
        if (rv == -1) \
            LOG_ERRNO("dup2"); \
        rv; \
    })

#define xwaitpid(pid, wstat, options) \
    ({ \
        int rv = waitpid(pid, wstat, options); \
        if (rv == -1 && errno != ECHILD) \
            LOG_ERRNO("waitpid"); \
        rv; \
    })

#define xexecvp(file, argv) \
    ({ \
        int rv = execvp(file, argv); \
        if (rv == -1 && errno != EACCES) \
            LOG_ERRNO("execvp"); \
        rv; \
    })

#define xopen(file, oflags, ...) \
    ({ \
        int rv = open(file, oflags __VA_OPT__(,) __VA_ARGS__); \
        if (rv == -1) \
            LOG_ERRNO("open"); \
        rv; \
    })

#define xopendir(path) \
    ({ \
        DIR *rv = opendir(path); \
        if (!rv) \
            LOG_ERRNO("opendir"); \
        rv; \
    })

#define xclosedir(dir) \
    ({ \
        int rv = closedir(dir); \
        if (rv == -1) \
            LOG_ERRNO("closedir"); \
        rv; \
    })

#define xreaddir(dir) \
    ({ \
        struct dirent *rv = readdir(dir); \
        if (!rv && errno != 0) \
            LOG_ERRNO("readdir"); \
        rv; \
    })

#define xstat(name, sb) \
    ({ \
        int rv = stat(name, sb); \
        if (rv == -1) \
            LOG_ERRNO("stat"); \
        rv; \
    })

#define xstatfs(name, sfsb) \
    ({ \
        int rv = statfs(name, sfsb); \
        if (rv == -1) \
            LOG_ERRNO("statfs"); \
        rv; \
    })

#define xsigaction(signum, sa, old) \
    ({ \
        int rv = sigaction(signum, sa, old); \
        if (rv == -1) \
            LOG_ERRNO("sigaction"); \
        rv; \
    })

#define xsigemptyset(set) \
    ({ \
        int rv = sigemptyset(set); \
        if (rv == -1) \
            LOG_ERRNO("sigemptyset"); \
        rv; \
    })

#define xsigaddset(set, signum) \
    ({ \
        int rv = sigaddset(set, signum); \
        if (rv == -1) \
            LOG_ERRNO("sigaddset"); \
        rv; \
    })

#define xsigdelset(set, signum) \
    ({ \
        int rv = sigdelset(set, signum); \
        if (rv == -1) \
            LOG_ERRNO("sigdelset"); \
        rv; \
    })

#define xsigprocmask(how, set, old) \
    ({ \
        int rv = sigprocmask(how, set, old); \
        if (rv == -1) \
            LOG_ERRNO("sigprocmask"); \
        rv; \
    })

#define xsetpgid(pid, pgid) \
    ({ \
        int rv = setpgid(pid, pgid); \
        if (rv == -1) {\
            if (errno == EACCES) {\
                LOG_WARN("setpgid"); \
            } else \
                LOG_ERRNO("setpgid"); \
        } \
        rv; \
    })

#define xisatty(fd) \
    ({ \
        int rv = isatty(fd); \
        if (rv == -1) \
            LOG_ERRNO("isatty"); \
        rv; \
    })

#define xtcgetpgrp(fd) \
    ({ \
        pid_t rv = tcgetpgrp(fd); \
        if (rv == -1) \
            LOG_ERRNO("tcgetpgrp"); \
        rv; \
    })

#define xtcsetpgrp(fd, pgid) \
    ({ \
        int rv = tcsetpgrp(fd, pgid); \
        if (rv == -1) \
            LOG_ERRNO("tcsetpgrp"); \
        rv; \
    })

#define xgetline(line, len, stream) \
    ({ \
        int rv = getline(line, len, stream); \
        if (rv == -1) \
            if (ferror(stream)) \
                LOG_ERRNO("getline"); \
        rv; \
    })

#define xread(fd, buf, size) \
    ({ \
        int rv = read(fd, buf, size); \
        if (rv == -1 && errno != EAGAIN) \
            LOG_ERRNO("read"); \
        rv; \
    })

#define xgetcwd(buf, size) \
    ({ \
        char *rv = getcwd(buf, size); \
        if (!rv) \
            LOG_ERRNO("getcwd"); \
        rv; \
    })

#define xppoll(fds, nfds, timeout, ss) \
    ({ \
        int rv = ppoll(fds, nfds, timeout, ss); \
        if (rv == -1 && errno != EINTR) \
            LOG_ERRNO("ppoll"); \
        rv; \
    })

#define xmalloc(size) \
    ({ \
        void *rv = malloc(size); \
        if (!rv) \
            LOG_ERRNO("malloc"); \
        rv; \
    })

#define xgetpgid(pid) \
    ({ \
        int rv = getpgid(pid); \
        if (rv == -1) \
            LOG_ERRNO("getpgid"); \
        rv; \
    })

#define xwrite(fd, buf, size) \
    ({ \
        int rv = write(fd, buf, size); \
        if (rv == -1) \
            LOG_ERRNO("write"); \
        rv; \
    })

#define xkill(pid, sig) \
    ({ \
        int rv = kill(pid, sig); \
        if (rv == -1) \
            LOG_ERRNO("kill"); \
        rv; \
    })

#define xsymlink(from, to) \
    ({ \
        int rv = symlink(from, to); \
        if (rv == -1) \
            LOG_ERRNO("symlink"); \
        rv; \
    })

#define xunlink(name) \
    ({ \
        int rv = unlink(name); \
        if (rv == -1) \
            LOG_ERRNO("unlink"); \
        rv; \
    })

#define xatexit(func) \
    ({ \
        int rv = atexit(func); \
        if (rv == -1) \
            LOG_ERRNO("atexit"); \
        rv; \
    })

#define xfflush(stream) \
    ({ \
        int rv = fflush(stream); \
        if (rv == -1) \
            LOG_ERRNO("fflush"); \
        rv; \
    })

#define xposix_openpt(flags) \
    ({ \
        int rv = posix_openpt(flags); \
        if (rv == -1) \
            LOG_ERRNO("posix_openpt"); \
        rv; \
    })

#define xgrantpt(fd) \
    ({ \
        int rv = grantpt(fd); \
        if (rv == -1) \
            LOG_ERRNO("grantpt"); \
        rv; \
    })

#define xptsname(fd) \
    ({ \
        char *rv = ptsname(fd); \
        if (!rv) \
            LOG_ERRNO("ptsname"); \
        rv; \
    })

#define xunlockpt(fd) \
    ({ \
        int rv = unlockpt(fd); \
        if (rv == -1) \
            LOG_ERRNO("unlockpt"); \
        rv; \
    })

#define xsetsid() \
    ({ \
        int rv = setsid(); \
        if (rv == (pid_t) -1) \
            LOG_ERRNO("setsid"); \
        rv; \
    })

#define xtcgetattr(fd, termios) \
    ({ \
        int rv = tcgetattr(fd, termios); \
        if (rv == -1) \
            LOG_ERRNO("tcgetattr"); \
        rv; \
    })

#define xtcsetattr(fd, optional_actions, termios) \
    ({ \
        int rv = tcsetattr(fd, optional_actions, termios); \
        if (rv == -1) \
            LOG_ERRNO("tcsetattr"); \
        rv; \
    })

#define xioctl(fd, op, ...) \
    ({ \
        int rv = ioctl(fd, op __VA_OPT__(,) __VA_ARGS__); \
        if (rv == -1) \
            LOG_ERRNO("ioctl"); \
        rv; \
    })

#define xnanosleep(ts, remaining) \
    ({ \
        int rv = nanosleep(ts, remaining); \
        if (rv == -1) \
            LOG_ERRNO("nanosleep"); \
        rv; \
    })

#define xpipe(pfd) \
    ({ \
        int rv = pipe(pfd); \
        if (rv == -1) \
            LOG_ERRNO("pipe"); \
        rv; \
    })

#define xfork() \
    ({ \
        int rv = fork(); \
        if (rv == -1) \
            LOG_ERRNO("fork"); \
        rv; \
     })

#define xclose(fd) \
    ({ \
        int rv = close(fd); \
        if (rv == -1) \
            LOG_ERRNO("close"); \
        rv; \
    })

#endif
