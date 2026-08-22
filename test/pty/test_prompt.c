#define _GNU_SOURCE

#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <sys/wait.h>

#include "unity_fixture.h"
#include "xfuncs.h"
#include "pty_test.h"

TEST_GROUP(prompt);

/************ Shared utils ************/

static size_t pred_i;
static size_t ptybuf_len;
static char ptybuf[PTY_RBUF];
static char outbuf[PTY_RBUF];

static int cpid;
static int mfd;
static struct termios tio;
static struct pollfd pollfds[2];

static bool pty_read(void *context) {
    ssize_t nchars = *(ssize_t *)context;

    if (nchars <= 0)
        return true;

    while (true) {
        int num_read = read(mfd, ptybuf + ptybuf_len, nchars);

        if (num_read == -1) {
            if (errno == EAGAIN) { /* need to make mfd nonblocking */
                return false;
            } else
                TEST_FAIL();
        }

        ptybuf_len += num_read;

        if (num_read >= nchars)
            return true;

        nchars -= num_read;
    }
}

static bool wait_for(bool (* pred)(void *context), void *context, int max_ms) {
    int checks = 1000;
    int sleep_time = max_ms / checks;

    int sec = sleep_time / 1000;
    int ns = (sleep_time % 1000) * 1000000;

    struct timespec ts = { sec, ns };

    while (!pred(context)) {
        if (checks-- < 1)
            return false;

        if (nanosleep(&ts, NULL) == -1)
            TEST_FAIL();
    }

    return true;
}

static char *request_chars(size_t nchars, int ms) {
    if (ptybuf_len - pred_i >= nchars) {
        strncpy(outbuf, &ptybuf[pred_i], nchars);
        pred_i += nchars;

        return outbuf;
    }

    if (wait_for(pty_read, &(ssize_t){nchars}, ms)) {
        strncpy(outbuf, &ptybuf[pred_i], nchars);
        pred_i += nchars;

        return outbuf;

    } else
        TEST_FAIL_MESSAGE("wait_for(pty_read): timed out");
}

/************ Fixture ************/

TEST_SETUP(prompt) {
    pred_i = 0;
    ptybuf_len = 0;
    ptybuf[0] = '\0';

    cpid = xforkpty(&mfd, NULL, &tio, NULL);

    if (cpid == 0) {
        char *bin = "/home/juta/Projects/Seashell/seashell";

        char *lfd_arg;
        if (asprintf(&lfd_arg, "--logfd=%d", seashell_pipe[1]) < 0)
            TEST_FAIL_MESSAGE("asprintf: input output error");

        char *argv[] = { bin, lfd_arg, NULL };

        execvp(argv[0], argv);
        _exit(EXIT_FAILURE);
    }

    pollfds[0].events = POLLIN;
    pollfds[0].fd = mfd;

    pollfds[1].events = POLLIN;
    pollfds[1].fd = seashell_pipe[1];
}

TEST_TEAR_DOWN(prompt) {
    xkill(cpid, SIGHUP);

    if (waitpid(cpid, NULL, 0) == -1)
        TEST_FAIL_MESSAGE("waitpid failed");

    close(mfd);
}

/************ Tests ************/

TEST(prompt, prompt_displayed_at_startup) {
    char *pty_chars = request_chars(2, 1000);

    TEST_ASSERT_EQUAL_STRING("> ", pty_chars);
    LOG_INFO("read: \"%s\"", pty_chars);
}

/************ Test runner ************/

TEST_GROUP_RUNNER(prompt) {
    xtcgetattr(STDERR_FILENO, &tio);
    tio.c_lflag &= ~ECHO; /* don't echo, non-interactive */
    tio.c_oflag &= ~OPOST; /* no post processing, \r\n -> \n */

    RUN_TEST_CASE(prompt, prompt_displayed_at_startup);
}
