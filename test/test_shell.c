#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "log.h"
#include "unity.h"
#include "pty_test.h"
#include "proc_view.h"

#define assert_send_string(ptyt, str) \
    TEST_ASSERT_EQUAL_INT(0, send_string(ptyt, str));

#define PROMPT "> "
#define BUF_SIZE 8192

typedef struct {
    size_t n;
    char buf[BUF_SIZE];
    size_t len;
} pty_read;

char *argv[] = { "/home/juta/Projects/Seashell/seashell" };
pty_test ptyt = {0};
pid_t cpid = {0};
da_pstat pstats;

bool read_until(void *context) {
    pty_read *rst = context;

    if (rst->n >= BUF_SIZE - 1 || rst->len >= BUF_SIZE - 1)
        return false;

    int num_read = xread(ptyt.mfd, &rst->buf[rst->len], rst->n - rst->len);

    if (num_read == -1) {
        if (errno == EAGAIN) {
            return false;

        } else
            TEST_FAIL();
    }

    rst->len += num_read;

    rst->buf[rst->len] = '\0';

    return rst->n == rst->len;
}

bool shell_exited(void *_) {
    pid_t pid = xwaitpid(cpid, NULL, WNOHANG);
    return pid > 0;
}

bool pstat_size(void *context) {
    size_t exp_size = *(size_t *)context;

    if (child_pstat(cpid, &pstats) == -1)
        TEST_FAIL();

    if (pstats.size != exp_size) {
        da_free(&pstats);
        return false;
    }

    da_free(&pstats);
    return true;
}

bool names_present(void *context) {
    char **names = context;

    TEST_ASSERT_NOT_EQUAL_INT(-1, child_pstat(cpid, &pstats));

    if (pstats.size == 0)
        return false;

    for (char **name = names; *name != NULL; ++name) {
        if (!lookup_pstat(&pstats, *name)) {
            da_free(&pstats);
            return false;
        }
    }

    return true;
}

bool wait_for(bool (* pred)(void *context), void *context, int max_ms) {
    int checks = 500;
    int sleep_time = max_ms / checks;

    int sec = sleep_time / 1000;
    int ns = (sleep_time % 1000) * 1000000;

    struct timespec ts = { sec, ns };

    while (!pred(context)) {
        if (checks-- < 1)
            return false;

        if (xnanosleep(&ts, NULL) == -1)
            TEST_FAIL();
    }

    return true;
}

void read_verify(char *exp_str) {
    pty_read rst = {
        .buf = {0},
        .len = 0,
        .n = strlen(exp_str)
    };

    if (rst.n >= BUF_SIZE)
        TEST_FAIL_MESSAGE("expected string too large");

    wait_for(read_until, &rst, 1000);
    TEST_ASSERT_EQUAL_STRING(exp_str, rst.buf);
}

void verify_proc_in_fg(char *name) {
    if (!wait_for(names_present, (char *[]){ name, NULL }, 1000))
        TEST_FAIL();

    ps_pstat *pstat = lookup_pstat(&pstats, name);

    pid_t fg_pgid = xtcgetpgrp(ptyt.mfd);
    if (fg_pgid == -1)
        TEST_FAIL();

    if (fg_pgid != pstat->pid)
        TEST_FAIL_MESSAGE("unexpected pgid");
}

void setUp(void) {
    cpid = fork_pty_test(&ptyt, argv);

    read_verify(PROMPT);

    if (child_pstat(cpid, &pstats) == -1)
        TEST_FAIL();

    TEST_ASSERT_EQUAL_size_t(0, pstats.size);
}

void tearDown(void) {
    if (xkill(cpid, SIGHUP) == -1)
        TEST_FAIL();

    bool sigkill_sent = false;

    if (!wait_for(shell_exited, NULL, 1000)) {
        if (sigkill_sent)
            TEST_FAIL_MESSAGE("failed to wait for seashell");

        if (xkill(cpid, SIGKILL) == -1)
            TEST_FAIL();

        sigkill_sent = true;
    }

    da_free(&pstats);

    TEST_ASSERT_EQUAL_INT(0, pty_test_done(&ptyt));
}

void test_interupt_fg_job(void) {
    assert_send_string(&ptyt, "cat\n");
    assert_send_string(&ptyt, "foo\n");

    verify_proc_in_fg("cat");
    send_tty_cc(&ptyt, VINTR);

    read_verify("foo\n");
    read_verify(PROMPT);
}

void test_bg_jobs_pid_state(void) {
    assert_send_string(&ptyt, "sleep 10&\n");
    assert_send_string(&ptyt, "sleep 10&\n");
    assert_send_string(&ptyt, "sleep 10&\n");
    assert_send_string(&ptyt, "sleep 10&\n");

    if (!wait_for(pstat_size, &(size_t){4}, 1000))
        TEST_FAIL();
}

void test_rapid_set_up_tear_down(void) {
}

int main(void) {
    log_init("/home/juta/Projects/Seashell/test/logs");
    open_pty_test(&ptyt);

    UNITY_BEGIN();

    RUN_TEST(test_interupt_fg_job);
    RUN_TEST(test_bg_jobs_pid_state);
    RUN_TEST(test_rapid_set_up_tear_down);

    return UNITY_END();
}
