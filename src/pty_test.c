#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <wait.h>
#include <stdlib.h> // IWYU pragma: keep
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "pty_test.h"

#define TS \
    (struct timespec){ .tv_nsec = 330000000, .tv_sec = 0 }

static void err_exit(char *msg) {
    fprintf(stderr, "pty_test: %s: %s", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

static void pty_master_open(pty_test *ptyt) {
    assert(ptyt);
        /* passing O_NONBLOCK directly is linux specific */
    ptyt->mfd = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (ptyt->mfd == -1)
        err_exit("posix_openpt");

        /* granpt not actually required on linux */
    if (grantpt(ptyt->mfd) == -1)
        err_exit("grantpt");

    if (unlockpt(ptyt->mfd) == -1)
        err_exit("unlockpt");

    ptyt->slave_name = ptsname(ptyt->mfd);
    if (!ptyt->slave_name)
        err_exit("ptsname");
}

static int pty_fork(pty_test *ptyt) {
    assert(ptyt);
    assert(ptyt->slave_name);

    int cpid = fork();
    if (cpid == -1)
        err_exit("fork");

    if (cpid != 0)
        return cpid;

    /* child continues... */

    if (setsid() == (pid_t) -1)
        _exit(EXIT_FAILURE);

    if (close(ptyt->mfd) == -1)
        _exit(EXIT_FAILURE);

        /* on BSD you need to preform TIOCSCTTY ioctl operation */
    int sfd = open(ptyt->slave_name, O_RDWR);
    if (sfd == -1)
        _exit(EXIT_FAILURE);

    if (dup2(sfd, STDIN_FILENO) != STDIN_FILENO)
        _exit(EXIT_FAILURE);
    if (dup2(sfd, STDOUT_FILENO) != STDOUT_FILENO)
        _exit(EXIT_FAILURE);
    if (dup2(sfd, STDERR_FILENO) != STDERR_FILENO)
        _exit(EXIT_FAILURE);

    if (sfd > STDERR_FILENO)
        if (close(sfd) == -1)
            _exit(EXIT_FAILURE);

    return 0;
}

static void init_termios(pty_test *ptyt) {
    assert(ptyt);

    if (tcgetattr(ptyt->mfd, &ptyt->tp) == -1)
        err_exit("tcgetattr");

    ptyt->tp.c_lflag &= ~ECHO; /* don't echo, non-interactive */
    ptyt->tp.c_oflag &= ~OPOST; /* no post processing, \r\n -> \n */

    if (tcsetattr(ptyt->mfd, 0, &ptyt->tp) == -1)
        err_exit("tcsetattr");
}

void open_pty_test(pty_test *ptyt) {
    assert(ptyt);

    *ptyt = (pty_test){0};

    pty_master_open(ptyt);
    init_termios(ptyt);
}

void close_pty_test(pty_test *ptyt) {
    assert(ptyt);

    close(ptyt->mfd);

    *ptyt = (pty_test){0};
}

int fork_pty_test(pty_test *ptyt, char **argv) {
    int pfd[2];
    if (pipe(pfd) == -1)
        err_exit("pipe");

    int cpid = pty_fork(ptyt);

    if (cpid == 0) {
        if (close(pfd[0]) == -1)
            _exit(EXIT_FAILURE);

        if (write(pfd[1], &(char){'x'}, 1) != 1)
            _exit(EXIT_FAILURE);

        if (close(pfd[1]) == -1)
            _exit(EXIT_FAILURE);

        execvp(argv[0], argv);
        _exit(EXIT_FAILURE);
    }

    if (close(pfd[1]) == -1)
        err_exit("close");

    if (read(pfd[0], &(char){0}, 1) != 1)
        err_exit("read");

    if (close(pfd[0]) == -1)
        err_exit("close");

    return cpid;
}
