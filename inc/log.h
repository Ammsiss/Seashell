#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#define LOG_INFO(msg, ...) \
    log_msg("\033[2;36minfo\033[m: ", NO_EXIT, false, msg __VA_OPT__(,) \
            __VA_ARGS__)

#define LOG_ERR(msg, ...) \
    log_msg("\033[91merror\033[m: ", NO_EXIT, false, msg __VA_OPT__(,) \
            __VA_ARGS__)

#define LOG_ERRNO(msg, ...) \
    log_msg("\033[91merror\033[m: ", NO_EXIT, true, msg __VA_OPT__(,) \
            __VA_ARGS__)

#define PFFORMAT(x, y) __attribute__ ((format(printf, (x), (y))))

#define SUCCESS 0
#define FAILURE 1

typedef enum {
    EXIT_U,
    EXIT,
    NO_EXIT,
} exit_type;

extern int stored_fd;

int log_init();

PFFORMAT(4, 5)
void log_msg(const char *header, exit_type should_exit, bool print_errno, \
        const char *fmt, ...);

int xpipe(int pipefd[2]);
int xfork(void);
int xdup2(int oldfd, int newfd);
int xclose(int fd);
void xexecvp(const char *file, char *const argv[]);
pid_t xwaitpid(pid_t pid, int *wstatus, int options);

#endif
