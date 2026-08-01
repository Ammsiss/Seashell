#define _GNU_SOURCE

#include <time.h>
#include <wait.h>
#include <stdlib.h> // IWYU pragma: keep
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "pty_test.h"
#include "log.h"
#include "dyn_str.h"

#define MTTY_BUF 4096

#define TS \
    (struct timespec){ .tv_nsec = 330000000, .tv_sec = 0 }

static int pty_master_open(pty_test *ptyt) {
    assert(ptyt);
        /* passing O_NONBLOCK directly is linux specific */
    ptyt->mfd = xposix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (ptyt->mfd == -1)
        return -1;

        /* granpt not actually required on linux */
    if (xgrantpt(ptyt->mfd) == -1)
        return -1;

    if (xunlockpt(ptyt->mfd) == -1)
        return -1;

    ptyt->slave_name = xptsname(ptyt->mfd);
    if (!ptyt->slave_name)
        return -1;

    return 0;
}

static int pty_fork(pty_test *ptyt) {
    assert(ptyt);
    assert(ptyt->slave_name);

    int cpid = xfork();
    if (cpid == -1)
        return -1;

    if (cpid != 0)
        return cpid;

    /* child continues... */

    if (xsetsid() == (pid_t) -1)
        return -1;

    if (xclose(ptyt->mfd) == -1)
        return -1;

        /* on BSD you need to preform TIOCSCTTY ioctl operation */
    int sfd = xopen(ptyt->slave_name, O_RDWR);
    if (sfd == -1)
        return -1;

    if (xdup2(sfd, STDIN_FILENO) != STDIN_FILENO)
        return -1;
    if (xdup2(sfd, STDOUT_FILENO) != STDOUT_FILENO)
        return -1;
    if (xdup2(sfd, STDERR_FILENO) != STDERR_FILENO)
        return -1;

    if (sfd > STDERR_FILENO)
        if (xclose(sfd) == -1)
            return -1;

    return 0;
}

static int init_termios(pty_test *ptyt) {
    assert(ptyt);

    if (xtcgetattr(ptyt->mfd, &ptyt->tp) == -1)
        return -1;

    ptyt->tp.c_lflag &= ~ECHO; /* don't echo, non-interactive */
    ptyt->tp.c_oflag &= ~OPOST; /* no post processing, \r\n -> \n */

    if (xtcsetattr(ptyt->mfd, 0, &ptyt->tp) == -1)
        return -1;

    return 0;
}

int open_pty_test(pty_test *ptyt) {
    assert(ptyt);
    assert(log_is_open());

    *ptyt = (pty_test){0};

    if (pty_master_open(ptyt) == -1)
        return -1;
    if (init_termios(ptyt) == -1)
        return -1;

    return 0;
}

void close_pty_test(pty_test *ptyt) {
    assert(ptyt);

    close(ptyt->mfd);

    *ptyt = (pty_test){0};
}

int fork_pty_test(pty_test *ptyt, char **argv) {
    int pfd[2];
    if (xpipe(pfd) == -1)
        return -1;

    int cpid = pty_fork(ptyt);

    if (cpid == -1)
        return -1;

    if (cpid == 0) {
        if (xclose(pfd[0]) == -1)
            _exit(EXIT_FAILURE);

        if (xwrite(pfd[1], &(char){'x'}, 1) != 1)
            _exit(EXIT_FAILURE);

        if (xclose(pfd[1]) == -1)
            _exit(EXIT_FAILURE);

        xexecvp(argv[0], argv);
        _exit(EXIT_FAILURE);
    }

    if (xclose(pfd[1]) == -1)
        return -1;

    if (xread(pfd[0], &(char){0}, 1) != 1)
        return -1;

    if (xclose(pfd[0]) == -1)
        return -1;

    return cpid;
}

int pty_test_done(pty_test *ptyt) {
    char c;
    int num_read = read(ptyt->mfd, &c, 1);
        /* EIO on pty closure is linux specific */
    if (!(num_read == -1 && errno == EIO))
        return -1;

    return 0;
}

int send_string(pty_test *ptyt, const char *cmd) {
    assert(ptyt && cmd);

    int num_write = xwrite(ptyt->mfd,  cmd, strlen(cmd));

    if (num_write == -1)
        return -1;

    if (num_write != (int) strlen(cmd)) {
        LOG_ERR("partial write");
        return -1;
    }

    return 0;
}

int send_tty_cc(pty_test *ptyt, int cc_code) {
    assert(ptyt);

    char buf[2];
    buf[0] = ptyt->tp.c_cc[cc_code];
    buf[1] = '\0';

    return send_string(ptyt, buf);
}

int verify_read(pty_test *ptyt, char *exp_str) {
    assert(ptyt && exp_str);

    d_str got;
    if (d_str_init(&got) == -1) {
        LOG_ERR("d_str_init");
        return -1;
    }

    int exp_len = strlen(exp_str);
    char buf[MTTY_BUF];

    for (int i = 0; i < 3; ++i) {
        int num_read = xread(ptyt->mfd, buf, exp_len);

        if (num_read == -1) {
            if (errno != EAGAIN)
                goto fail;

            if (i == 2) {
                LOG_ERR("timeout: exp: \"%s\", got: %s", exp_str, got.c_str);
                goto fail;
            }

            if (xnanosleep(&TS, NULL) == -1)
                goto fail;

            continue;
        }

        buf[num_read] = '\0';

        if (d_strcat(&got, buf) == -1) {
            LOG_ERR("d_strcat");
            goto fail;
        }

        if (exp_len != num_read) {
            exp_len -= num_read;
            continue;

        } else {
            if (strcmp(exp_str, got.c_str) != 0) {
                LOG_ERR("exp: \"%s\", got: \"%s\"", exp_str, got.c_str);
                goto fail;
            }

            break;
        }
    }

    d_str_free(&got);
    return 0;

fail:
    d_str_free(&got);
    return -1;
}
