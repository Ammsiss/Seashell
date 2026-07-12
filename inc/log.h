#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define STR_IMP(x) #x
#define STR(x) STR_IMP(x)

#define LOG_MSG(type, perrno, fmt, ...) \
    log_msg(type, perrno, __FILE__, __LINE__, __func__, \
            fmt __VA_OPT__(,) __VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    LOG_MSG(L_INFO, NULL, fmt __VA_OPT__(,) __VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    LOG_MSG(L_WARN, NULL, fmt __VA_OPT__(,) __VA_ARGS__)

#define LOG_ERR(fmt, ...) \
    LOG_MSG(L_ERR, NULL, fmt __VA_OPT__(,) __VA_ARGS__)

#define LOG_ERRNO(fmt, ...) \
    do { \
        int saved_errno = errno; \
        LOG_MSG(L_ERR, strerror(saved_errno), (fmt) __VA_OPT__(,) __VA_ARGS__); \
        errno = saved_errno; \
    } while (false)

#define PFFORMAT(x, y) __attribute__ ((format(printf, (x), (y))))

/* +100 for format chars, pid, and line number */
#define OUTPUT_SIZE (LOG_BUF_SIZE * 3 + PATH_MAX + ERRSTR_SIZE + 100)
#define ERRSTR_SIZE 1024
#define LOG_BUF_SIZE 128

#define CGREEN   "\033[2;36m"
#define CRED     "\033[91m"
#define CBLACK   "\033[30m"
#define CYELLOW  "\033[33m"
#define CBLUE    "\033[34m"
#define CMAGENTA "\033[35m"
#define CCYAN    "\033[36m"
#define CWHITE   "\033[37m"

#define CBRIGHTBLACK   "\033[90m"
#define CBRIGHTRED     "\033[91m"
#define CBRIGHTGREEN   "\033[92m"
#define CBRIGHTYELLOW  "\033[93m"
#define CBRIGHTBLUE    "\033[94m"
#define CBRIGHTMAGENTA "\033[95m"
#define CBRIGHTCYAN    "\033[96m"
#define CBRIGHTWHITE   "\033[97m"

#define CDIM    "\033[90m"
#define CBOLD   "\033[1m"
#define CUNDER  "\033[4m"
#define CREV    "\033[7m"

#define CCL     "\033[m"

typedef enum {
    L_INFO,
    L_WARN,
    L_ERR,
} log_level;

extern int log_output_fd;

int log_init();

PFFORMAT(6, 7)
void log_msg(log_level type, const char *errstr, const char *file, \
    int line, const char *function, const char *fmt, ...);

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
        if (rv == -1) \
            LOG_ERRNO("waitpid"); \
        rv; \
    })

#define xexecvp(file, argv) \
    ({ \
        int rv = execvp(file, argv); \
        if (rv == -1) \
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
        if (rv == -1) \
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

#endif
