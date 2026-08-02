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

#define assert_verify_read(ptyt, exp_str) \
    TEST_ASSERT_EQUAL_INT(0, verify_read(ptyt, exp_str));

#define PROMPT "> "

char *argv[] = { "/home/juta/Projects/Seashell/seashell" };
pty_test ptyt = {0};
pid_t cpid = {0};
da_pstat pstats;

bool shell_exited(void *_) {
    pid_t pid = xwaitpid(cpid, NULL, WNOHANG);
    return pid > 0;
}

bool pstat_size(void *exp_size) {
    TEST_ASSERT_NOT_EQUAL_INT(-1, child_pstat(cpid, &pstats));

    LOG_INFO("exp %ld, real %ld", *(size_t *)exp_size, pstats.size);

    if (pstats.size != *(size_t *)exp_size) {
        da_free(&pstats);
        return false;
    }

    return true;
}

bool names_present(void *names) {
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
        LOG_INFO("pred failed");

        if (checks-- < 1)
            return false;

        if (xnanosleep(&ts, NULL) == -1)
            TEST_FAIL();
    }

    return true;
}

void setUp(void) {
    cpid = fork_pty_test(&ptyt, argv);
    assert_verify_read(&ptyt, PROMPT);

    TEST_ASSERT_EQUAL_INT(0, child_pstat(cpid, &pstats));
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

void test_interupt_fg_job(void) {
    assert_send_string(&ptyt, "cat\n");
    assert_send_string(&ptyt, "foo\n");

    verify_proc_in_fg("cat");
    send_tty_cc(&ptyt, VINTR);

    assert_verify_read(&ptyt, "foo\n");
    assert_verify_read(&ptyt, PROMPT);
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

    // RUN_TEST(test_interupt_fg_job);
    // RUN_TEST(test_bg_jobs_pid_state);

    // for (int i = 0; i < 100; ++i)
    RUN_TEST(test_rapid_set_up_tear_down);

    return UNITY_END();
}
