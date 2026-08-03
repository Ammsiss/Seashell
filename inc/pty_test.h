#ifndef PTY_TEST_H
#define PTY_TEST_H

#define _GNU_SOURCE

#include <unistd.h>
#include <limits.h>
#include <termios.h>
#include <sys/ioctl.h>

typedef struct pty_test pty_test;

struct pty_test {
    int mfd;
    char *slave_name;
    struct termios tp;
};

int open_pty_test(pty_test *ptyt);
void close_pty_test(pty_test *ptyt);

int fork_pty_test(pty_test *ptyt, char **argv);
int pty_test_done(pty_test *ptyt);

#endif
