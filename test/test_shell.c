#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "log.h"
#include "unity.h"
#include "pty_test.h"
#include "proc_view.h"

#define PROMPT "> "
#define BUF_SIZE 8192

typedef struct {
    size_t len;
    char buf[BUF_SIZE];
    size_t n;
} pty_io;

char *argv[] = { "/home/juta/Projects/Seashell/seashell" };
pty_test ptyt = {0};
pid_t cpid = {0};
da_pstat pstats;

bool read_until(void *context) {
    pty_io *iost = context;

    if (iost->len > BUF_SIZE - 1 || iost->n > iost->len)
        return false;

    int num_read = xread(ptyt.mfd, &iost->buf[iost->n], iost->len - iost->n);

    if (num_read == -1) {
        if (errno == EAGAIN) {
            return false;
        } else
            TEST_FAIL();
    }

    iost->n += num_read;

    iost->buf[iost->n] = '\0';

    return iost->len == iost->n;
}

bool write_until(void *context) {
    pty_io *iost = context;

    if (iost->n > BUF_SIZE -1 || iost->n > iost->len)
        return false;

    int num_write = xwrite(ptyt.mfd, &iost->buf[iost->n], iost->len - iost->n);

    if (num_write == -1) {
        if (errno == EAGAIN) {
            return false;
        } else
            TEST_FAIL();
    }

    iost->n += num_write;

    return iost->len == iost->n;
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

bool proc_in_fg(void *context) {
    pid_t pid = *(pid_t *)context;

    pid_t fg_pgid = xtcgetpgrp(ptyt.mfd);
    if (fg_pgid == -1)
        TEST_FAIL();

    pid_t pgid = getpgid(pid);
    if (pgid == -1)
        TEST_FAIL();

    return fg_pgid == pgid;
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
    int checks = 1000;
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
    pty_io rst = {
        .len = strlen(exp_str),
        .buf = {0},
        .n = 0,
    };

    if (rst.len >= BUF_SIZE)
        TEST_FAIL_MESSAGE("expected string too large");

    TEST_ASSERT(wait_for(read_until, &rst, 1000));
    TEST_ASSERT_EQUAL_STRING(exp_str, rst.buf);
}

void write_verify(char *str) {
    pty_io wst;

    wst.len = strlen(str);

    if (wst.len > BUF_SIZE - 1)
        TEST_FAIL_MESSAGE("string too large to write");

    strncpy(wst.buf, str, wst.len);
    wst.n = 0;

    TEST_ASSERT(wait_for(write_until, &wst, 1000));
}

void send_tty_cc(int cc_code) {
    char tty_cc_str[2];
    tty_cc_str[0] = ptyt.tp.c_cc[cc_code];
    tty_cc_str[1] = '\0';

    write_verify(tty_cc_str);
}

void verify_proc_in_fg(char *name) {
    if (!wait_for(names_present, (char *[]){ name, NULL }, 1000))
        TEST_FAIL();

    ps_pstat *pstat = lookup_pstat(&pstats, name);
    if (!pstat)
        TEST_FAIL();

    if (!wait_for(proc_in_fg, &(pid_t){ pstat->pid }, 1000))
        TEST_FAIL();
}

int flush_pty(void) {
    int num_read;
    char buf[BUF_SIZE];

    while ((num_read = read(ptyt.mfd, buf, BUF_SIZE - 1)) > 0) {
        buf[num_read] = '\0';
        LOG_INFO("%s: \"%s\"", Unity.CurrentTestName, buf);
    }

    if (num_read == -1 && errno != EIO && errno != EAGAIN)
        TEST_FAIL();

    return 0;
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

    flush_pty();
    da_free(&pstats);
}

void test_interupt_fg_job(void) {
    write_verify("cat\n");
    write_verify("foo\n");
    verify_proc_in_fg("cat");
    send_tty_cc(VINTR);

    read_verify("foo\n");
    read_verify(PROMPT);
}

void test_bg_jobs_pid_state(void) {
    write_verify("sleep 10&\n");
    write_verify("sleep 10&\n");

    if (!wait_for(pstat_size, &(size_t){2}, 1000))
        TEST_FAIL();

    read_verify("[1] started\n");
    read_verify(PROMPT);

    read_verify("[2] started\n");
    read_verify(PROMPT);
}

void test_rapid_set_up_tear_down(void) {
}

int main(void) {
    log_init("/home/juta/Projects/Seashell/test/logs");
    open_pty_test(&ptyt);

    UNITY_BEGIN();

    for (int i = 0; i < 300; ++i)
        RUN_TEST(test_interupt_fg_job);
    for (int i = 0; i < 100; ++i)
        RUN_TEST(test_bg_jobs_pid_state);
    for (int i = 0; i < 100; ++i)
        RUN_TEST(test_rapid_set_up_tear_down);

    return UNITY_END();
}
