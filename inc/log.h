#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#define STR_IMP(x) #x
#define STR(x) STR_IMP(x)

#define LOG_MSG(type, perrno, fmt, ...) \
    log_msg(type, perrno, __FILE__, __LINE__, __func__, \
            fmt __VA_OPT__(,) __VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    LOG_MSG(L_INFO, NULL, fmt __VA_OPT__(,) __VA_ARGS__)

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
#define OUTPUT_SIZE (BUF_SIZE * 3 + PATH_MAX + ERRSTR_SIZE + 100)
#define ERRSTR_SIZE 1024
#define BUF_SIZE 128

#define CGREEN "\033[2;36m"
#define CRED "\033[91m"
#define CDIM "\033[90m"
#define CCL "\033[m"

typedef enum {
    L_INFO,
    L_ERR,
} log_level;

extern int log_output_fd;

int log_init();
PFFORMAT(6, 7) void log_msg(log_level type, const char *errstr, const char *file, \
    int line, const char *function, const char *fmt, ...);
int xpipe(int pipefd[2]);
int xfork(void);
int xdup2(int oldfd, int newfd);
int xclose(int fd);
void xexecvp(const char *file, char *const argv[]);
pid_t xwaitpid(pid_t pid, int *wstatus, int options);

#endif
