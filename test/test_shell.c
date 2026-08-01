#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "log.h"
#include "unity.h"
#include "pty_test.h"

#define assert_send_string(ptyt, str) \
    TEST_ASSERT_EQUAL_INT(0, send_string(ptyt, str));

#define assert_verify_read(ptyt, exp_str) \
    TEST_ASSERT_EQUAL_INT(0, verify_read(ptyt, exp_str));

#define ARR_SIZE(arr) \
    sizeof(arr) / sizeof(pty_step)

#define PROMPT "> "

#define STEP(_send, _verify) \
    (pty_step){ .send = _send, .verify = _verify }

typedef struct {
    char *send;
    char *verify;
} pty_step;

pty_test ptyt = {0};
char *argv[] = { "/home/juta/Projects/Seashell/seashell" };
pid_t cpid = {0};

void run_pty_script(pty_step *steps, size_t size, void (* sh_exit)(void)) {
    for (size_t i = 0; i < size; ++i) {
        if (steps->send)
            assert_send_string(&ptyt, steps->send);
        if (steps->verify)
            assert_verify_read(&ptyt, steps->verify);
    }

    sh_exit();
}

void setUp(void) {
    cpid = fork_pty_test(&ptyt, argv);
    assert_verify_read(&ptyt, PROMPT);
}

void tearDown(void) {
    int rv;
    int wstat;
    int timeout_count = 0;
    struct timespec ts = { .tv_nsec = 33000000, .tv_sec = 0 };

    while ((rv = xwaitpid(cpid, &wstat, WNOHANG | WUNTRACED | WCONTINUED)) < 1) {
        TEST_ASSERT_GREATER_THAN(-1, rv);
        TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(3, timeout_count++, "waitpid timeout");

        if (xnanosleep(&ts, NULL) == -1)
            TEST_FAIL();
    }

    TEST_ASSERT_EQUAL_INT(cpid, rv);
    TEST_ASSERT_EQUAL_INT(0, pty_test_done(&ptyt));
}

void send_exit(void) {
    assert_send_string(&ptyt, "exit\n");
    assert_verify_read(&ptyt, "exit\n");
}

void test_simple_cmd(void) {
    pty_step steps[] = {
        STEP("echo hi\n", "hi\n" PROMPT),
    };

    run_pty_script(steps, ARR_SIZE(steps), send_exit);
}

void test_interupt_fg_job(void) {
    assert_send_string(&ptyt, "cat\n");
    assert_send_string(&ptyt, "foo\n");
    assert_verify_read(&ptyt, "foo\n");
    send_tty_cc(&ptyt, VINTR);
    assert_verify_read(&ptyt, PROMPT);

    send_exit();
}

void test_jobs_builtin(void) {
    pty_step steps[] = {
        STEP("jobs\n", PROMPT),
        STEP("sleep 100 &\n", "[1] started\n" PROMPT),
        STEP("jobs\n", "[1] running\n" PROMPT),
    };

    run_pty_script(steps, ARR_SIZE(steps), send_exit);
}

int main(void) {
    log_init("/home/juta/Projects/Seashell/test/logs");
    open_pty_test(&ptyt);

    UNITY_BEGIN();

    RUN_TEST(test_simple_cmd);
    RUN_TEST(test_interupt_fg_job);
    RUN_TEST(test_jobs_builtin);

    return UNITY_END();
}
