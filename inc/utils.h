#ifndef UTILS_H
#define UTILS_H

#define _GNU_SOURCE

#include <signal.h>
#include <stdarg.h>

#define PFFORMAT(x, y) __attribute__ ((format(printf, (x), (y))))
#define BUF_SIZE 1024

PFFORMAT(1, 2)
void fatal(const char *fmt, ...);

PFFORMAT(2, 3)
void errExit(bool print_err, const char *fmt, ...);

PFFORMAT(2, 3)
void err_exit(bool print_err, const char *fmt, ...);

PFFORMAT(1, 2)
void err_msg(const char *fmt, ...);

PFFORMAT(1, 2)
void errno_msg(const char *fmt, ...);

PFFORMAT(1, 2)
void usage_err(const char *fmt, ...);

static struct sigaction old_sa;
static sigset_t old_set;

int set_sig_action(int sig, sighandler_t handler, int flags, sigset_t *mask);

int block_sig(int sig);
int unblock_sig(int sig);
int make_sigset(int sigs[], sigset_t *set, bool start_empty);

#endif
