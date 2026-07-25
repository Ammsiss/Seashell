#ifndef UTILS_H
#define UTILS_H

#define PFFORMAT(x, y) __attribute__ ((format(printf, (x), (y))))
#define BUF_SIZE 1024

PFFORMAT(1, 2)
void fatal(const char *fmt, ...);

PFFORMAT(1, 2)
void err_exit(const char *fmt, ...);

PFFORMAT(1, 2)
void err_msg(const char *fmt, ...);

PFFORMAT(1, 2)
void errno_msg(const char *fmt, ...);

PFFORMAT(1, 2)
void usage_err(const char *fmt, ...);

#endif
