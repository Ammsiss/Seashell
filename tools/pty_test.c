#define _GNU_SOURCE

#include <stdio.h>
#include <time.h>
#include <wait.h>
#include <stdlib.h> // IWYU pragma: keep
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "wait_stat.h"
#include "utils.h"
#include "log.h"
#include "dyn_str.h"

#define PROJ_DIR "/home/juta/Projects/Seashell"

struct termios tp;

void transfer_fd(int fd1, int fd2) {
    if (fd1 == fd2)
        return;

    if (xdup2(fd1, fd2) == -1)
        err_exit("dup2");

    if (xclose(fd1) == -1)
        err_exit("close");
}

int pty_master_open(char **slave_name) {
    assert(slave_name);

    int master_fd = xposix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd == -1)
        err_exit("posix_openpt");

    if (xgrantpt(master_fd) == -1) /* not actually required on linux */
        err_exit("grantpt");

    if (xunlockpt(master_fd) == -1)
        err_exit("unlockpt");

    *slave_name = xptsname(master_fd);
    if (!slave_name)
        err_exit("ptsname");

    return master_fd;
}

int pty_fork(int master_fd, const char *slave_name,
        const struct termios *slave_termios, const struct winsize *slave_ws) {

    int cpid = xfork();
    if (cpid == -1)
        err_exit("fork");

    if (cpid != 0)
        return cpid;

    /* child continues... */

    if (xsetsid() == (pid_t) -1)
        err_exit("setsid");

    if (xclose(master_fd) == -1)
        err_exit("close");

    /* on BSD you need to preform TIOCSCTTY ioctl operation */

    int sfd = xopen(slave_name, O_RDWR);
    if (sfd == -1)
        err_exit("open");

    if (slave_termios)
        if (xtcsetattr(sfd, 0, slave_termios) == -1)
            err_exit("tcsetattr");

    if (slave_ws)
        if (xioctl(sfd, TIOCSWINSZ, slave_ws) == -1)
            err_exit("ioctl");

    if (xdup2(sfd, STDIN_FILENO) != STDIN_FILENO)
        err_exit("dup2");
    if (xdup2(sfd, STDOUT_FILENO) != STDOUT_FILENO)
        err_exit("dup2");
    if (xdup2(sfd, STDERR_FILENO) != STDERR_FILENO)
        err_exit("dup2");

    if (sfd > STDERR_FILENO)
        if (xclose(sfd) == -1)
            err_exit("close");

    return 0;
}

#define MTTY_BUF 8096

#define TS \
    (struct timespec){ .tv_nsec = 330000000, .tv_sec = 0 }

void verify_read(int fd, char *exp_str) {
    assert(exp_str);

    d_str actual;
    if (d_str_init(&actual) == -1)
        exit(EXIT_FAILURE);

    int exp_len = strlen(exp_str);
    char buf[MTTY_BUF];
    int num_read;

    for (int i = 0; i < 3; ++i) {
        num_read = xread(fd, buf, exp_len);
        if (num_read == -1 && errno != EAGAIN)
            exit(EXIT_FAILURE);

        if (exp_len == num_read) {
            buf[exp_len] = '\0';
            break;
        }

        if (i == 2) {
            LOG_ERR("read timed out: \"%s\"", exp_str);
            exit(EXIT_FAILURE);
        }

        if (xnanosleep(&TS, NULL) == -1)
            exit(EXIT_FAILURE);
    }

    if (strcmp(buf, exp_str) != 0) {
        LOG_ERR("exp: \"%s\" | got: \"%s\"", exp_str, buf);
        exit(EXIT_FAILURE);
    }
}

void send_string(int fd, const char *cmd) {
    if (xwrite(fd, cmd, strlen(cmd)) != (int) strlen(cmd)) {
        LOG_ERR("failed/partial write");
        exit(EXIT_FAILURE);
    }
}

void send_tty_cc(int fd, int cc_code) {
    char buf[2];
    buf[0] = tp.c_cc[cc_code];
    buf[1] = '\0';

    send_string(fd, buf);
}

int main(void) {
    log_init(PROJ_DIR "/tools/logs");

    int tty_fd = xopen("/dev/tty", O_RDWR);
    if (tty_fd == -1)
        err_exit("open");

    char *argv[] = {
        PROJ_DIR "/seashell",
        NULL
    };

    char *slave_name;
    int mfd = pty_master_open(&slave_name);

    /* make pty fds non blocking */

    int flags = fcntl(mfd, F_GETFL, 0);
    if (flags == -1)
        exit(EXIT_FAILURE);

    if (fcntl(mfd, F_SETFL, flags | O_NONBLOCK) == -1)
        exit(EXIT_FAILURE);

    /* modify pty flags */

    if (xtcgetattr(tty_fd, &tp) == -1)
        exit(EXIT_FAILURE);

    tp.c_lflag &= ~ECHO; /* don't echo, non-interactive */
    tp.c_oflag &= ~OPOST; /* no post processing, \r\n -> \n */

    if (xtcsetattr(mfd, 0, &tp) == -1)
        exit(EXIT_FAILURE);

    /* fork and exec seashell */

    int cpid = pty_fork(mfd, slave_name, NULL, NULL);

    if (cpid == 0) {
        xexecvp(PROJ_DIR "/seashell", argv);
        exit(EXIT_FAILURE);
    }

    /* run end to end tests */

    verify_read(mfd, "> ");

    send_string(mfd, "echo hi | wc -c | grep 3\n");
    verify_read(mfd, "3\n");
    verify_read(mfd, "> ");

    send_string(mfd, "jobs\n");
    verify_read(mfd, "> ");

    send_string(mfd, "cat\n");
    send_string(mfd, "hello\n");
    verify_read(mfd, "hello\n");
    send_tty_cc(mfd, VINTR);
    verify_read(mfd, "> ");

    send_string(mfd, "sleep 100 &\n");
    verify_read(mfd, "[1] started\n");
    verify_read(mfd, "> ");

    send_string(mfd, "jobs\n");
    verify_read(mfd, "[1] running\n");
    verify_read(mfd, "> ");

    /* tell seashell to exit, then wait */

    send_string(mfd, "exit\n");
    verify_read(mfd, "exit\n");

    int wstat;
    if (xwaitpid(cpid, &wstat, WUNTRACED | WCONTINUED) == -1)
        exit(EXIT_FAILURE);

    LOG_INFO("seashell %s", get_wstat_str(cpid, wstat));

    /* ensure theres nothing unexpected in the master pty */

    char c;               /* EIO on pty closure is linux specific */
    if (read(mfd, &c, 1) != -1 || errno != EIO) {
        LOG_ERR("extra bytes in pty");
        exit(EXIT_FAILURE);
    }

    printf("Success! All tests passed\n");
    LOG_INFO("all tests passed. exiting");
    return EXIT_SUCCESS;
}
