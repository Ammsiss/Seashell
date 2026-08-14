#define _GNU_SOURCE

#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "unity_fixture.h"
#include "pty_test.h"
#include "proc_view.h"
#include "log.h"

TEST_GROUP(shell);

/************ Shared utils ************/

#define PROMPT "> "
#define BUF_SIZE 8192

typedef struct {
    size_t len;
    char buf[BUF_SIZE];
    size_t n;
} pty_io;

static char *argv[] = { "/home/juta/Projects/Seashell/seashell", NULL };

static pty_test ptyt = {0};
static pid_t cpid = {0};
static da_pstat pstats;

static bool read_until(void *context) {
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

static bool write_until(void *context) {
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

static bool shell_exited(void *_) {
    pid_t pid = xwaitpid(cpid, NULL, WNOHANG);
    return pid > 0;
}

static bool pstat_size(void *context) {
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

static bool proc_in_fg(void *context) {
    pid_t pid = *(pid_t *)context;

    pid_t fg_pgid = xtcgetpgrp(ptyt.mfd);
    if (fg_pgid == -1)
        TEST_FAIL();

    pid_t pgid = getpgid(pid);
    if (pgid == -1)
        TEST_FAIL();

    return fg_pgid == pgid;
}

static bool names_present(void *context) {
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

static bool wait_for(bool (* pred)(void *context), void *context, int max_ms) {
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

static void read_verify(char *exp_str) {
    pty_io rst = {
        .len = strlen(exp_str),
        .buf = "",
        .n = 0,
    };

    if (rst.len >= BUF_SIZE)
        TEST_FAIL_MESSAGE("expected string too large");

    if (!wait_for(read_until, &rst, 1000)) {
        LOG_ERR("exp: %s got: %s", exp_str, rst.buf);
        TEST_FAIL();
    }

    if (strcmp(exp_str, rst.buf) != 0) {
        LOG_ERR("exp: %s got: %s", exp_str, rst.buf);
        TEST_FAIL();
    }
}

static void write_verify(char *str) {
    pty_io wst;

    wst.len = strlen(str);

    if (wst.len > BUF_SIZE - 1)
        TEST_FAIL_MESSAGE("string too large to write");

    strncpy(wst.buf, str, wst.len);
    wst.n = 0;

    TEST_ASSERT(wait_for(write_until, &wst, 1000));
}

static void send_tty_cc(int cc_code) {
    char tty_cc_str[2];
    tty_cc_str[0] = ptyt.tp.c_cc[cc_code];
    tty_cc_str[1] = '\0';

    write_verify(tty_cc_str);
}

static void verify_proc_in_fg(char *name) {
    if (!wait_for(names_present, (char *[]){ name, NULL }, 1000))
        TEST_FAIL();

    ps_pstat *pstat = lookup_pstat(&pstats, name);
    if (!pstat)
        TEST_FAIL();

    if (!wait_for(proc_in_fg, &(pid_t){ pstat->pid }, 1000))
        TEST_FAIL();
}

static int flush_pty(void) {
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

/************ Fixture ************/

TEST_SETUP(shell) {
    cpid = fork_pty_test(&ptyt, argv);

    read_verify(PROMPT);

    if (child_pstat(cpid, &pstats) == -1)
        TEST_FAIL();

    TEST_ASSERT_EQUAL_size_t(0, pstats.size);
}

TEST_TEAR_DOWN(shell) {
    if (xkill(cpid, SIGHUP) == -1)
        TEST_FAIL();

    bool sigkill_sent = false;

    while (!wait_for(shell_exited, NULL, 1000)) {
        if (sigkill_sent)
            TEST_FAIL_MESSAGE("failed to wait for seashell");

        if (xkill(cpid, SIGKILL) == -1)
            TEST_FAIL();

        sigkill_sent = true;
    }

    flush_pty();
    da_free(&pstats);
}

/************ Tests ************/

TEST(shell, prompt_after_reclaim_tty) {
    write_verify("/bin/echo x\n");

    read_verify("x\n" PROMPT);
}

TEST(shell, prompt_after_launch_bg_cmd) {
    write_verify("sleep 100 &\n");

    read_verify("[1] started\n" PROMPT);
}

TEST(shell, prompt_after_bg_job_done) {
    write_verify("sleep 0.05 &\n");

    read_verify("[1] started\n" PROMPT "\n[1] exited\n" PROMPT);
}

TEST(shell, short_lived_bg_cmd) {
    write_verify("/bin/echo x &\n");
            /* echo prints this newline --v */
    read_verify("[1] started\n" PROMPT "x\n\n[1] exited\n" PROMPT);
}

TEST(shell, int_fg_job) {
    write_verify("cat\n");
    write_verify("foo\n");

    verify_proc_in_fg("cat");
    send_tty_cc(VINTR);

    read_verify("foo\n" PROMPT);
}

TEST(shell, stop_fg_job) {
    write_verify("sleep 100\n");

    verify_proc_in_fg("sleep");
    send_tty_cc(VSUSP);

    read_verify("\n[1] stopped\n" PROMPT);
}

TEST(shell, bg_jobs_pid_state) {
    write_verify("sleep 100 &\n");
    write_verify("sleep 100 &\n");

    if (!wait_for(pstat_size, &(size_t){2}, 1000))
        TEST_FAIL();

    read_verify("[1] started\n" PROMPT "[2] started\n" PROMPT);
}

TEST(shell, unknown_cmd) {
    write_verify("zoobar\n"); /* hopefully no-one ever makes this... */

    read_verify("seashell: command not found: zoobar\n" PROMPT);
}

TEST(shell, pipeline) {
    write_verify("/bin/echo hi | grep h | wc -c\n");
        /* wc outputs 3 because it includes the newline from grep */
    read_verify("3\n" PROMPT);
}

TEST(shell, bg_job_done_with_fg_job) {

    /* launch 2 bg jobs to determine if the prompt was not printed.
     * If the prompt does not print between job exit messages it was
     * correctly skipped.
     *
     * flush_pty should also have no output. */

    write_verify("sleep 0.05 &\n");
    write_verify("sleep 0.1 &\n");
    write_verify("sleep 1\n");

    read_verify("[1] started\n" PROMPT "[2] started\n" PROMPT
                "[1] exited\n[2] exited\n");
}

TEST(shell, builtin_in_bg) {
    write_verify("jobs &\n");

    read_verify("[1] started\n" PROMPT "jobs: no job control in this shell\n"
                "\n[1] exited\n" PROMPT);
}

TEST(shell, pipeline_of_builtins_does_not_duplicate_prompt) {

    /* Regression: after adding display_prompt() to try_run_builtin()
     * and neglecting to confirm if we are in the fg, and not calling
     * from a subshell, the prompt was duplicated. */

    write_verify("jobs | cd .\n");

    read_verify("jobs: no job control in this shell\n" PROMPT);
}

TEST(shell, simple_andor_chain) {
    write_verify("sleep 0.05 && /bin/echo x\n");

    read_verify("x\n" PROMPT);
}

TEST(shell, simple_bg_andor_chain) {
    write_verify("sleep 0.05 && /bin/echo x &\n");

    read_verify("[1] started\n" PROMPT "x\n" "\n[1] exited\n" PROMPT);
}

TEST(shell, and_if_logic) {
    write_verify("true && /bin/echo x\n");
    read_verify("x\n" PROMPT);

    write_verify("false && /bin/echo x\n");
    read_verify(PROMPT);
}

TEST(shell, or_if_logic) {
    write_verify("false || /bin/echo y\n");
    read_verify("y\n" PROMPT);

    write_verify("true || /bin/echo y\n");
    read_verify(PROMPT);
}

TEST(shell, and_if_logic_bg) {
    write_verify("true && /bin/echo x &\n");
    read_verify("[1] started\n" PROMPT "x\n\n[1] exited\n" PROMPT);

    write_verify("false && /bin/echo x &\n");
    read_verify("[1] started\n" PROMPT "\n[1] exited\n" PROMPT);
}

TEST(shell, or_if_logic_bg) {
    write_verify("false || /bin/echo y &\n");
    read_verify("[1] started\n" PROMPT "y\n\n[1] exited\n" PROMPT);

    write_verify("true || /bin/echo y &\n");
    read_verify("[1] started\n" PROMPT "\n[1] exited\n" PROMPT);
}

TEST(shell, and_or_chain_with_builtins) {
    write_verify("echo x && echo y\n");

    read_verify("x\ny\n" PROMPT);
}

TEST(shell, prompt_after_launch_fg_builtin) {
    write_verify("cd .\n");

    read_verify(PROMPT);
}

TEST(shell, jobs_builtin) {
    write_verify("jobs\n");
    write_verify("sleep 100 &\n");
    write_verify("jobs\n");

    read_verify(PROMPT "[1] started\n" PROMPT "[1] running\n" PROMPT);
}

TEST(shell, builtin_and_or_logic) {
    write_verify("cd /non-existant || echo ?\n");
    read_verify("cd: chdir: No such file or directory\n?\n" PROMPT);

    write_verify("cd /non-existant && echo ?\n");
    read_verify("cd: chdir: No such file or directory\n" PROMPT);
}

/************ Test runner ************/

TEST_GROUP_RUNNER(shell) {
    open_pty_test(&ptyt);

    RUN_TEST_CASE(shell, prompt_after_reclaim_tty);
    RUN_TEST_CASE(shell, prompt_after_launch_bg_cmd);
    RUN_TEST_CASE(shell, prompt_after_bg_job_done);
    RUN_TEST_CASE(shell, short_lived_bg_cmd);
    RUN_TEST_CASE(shell, int_fg_job);
    RUN_TEST_CASE(shell, stop_fg_job);
    RUN_TEST_CASE(shell, bg_jobs_pid_state);
    RUN_TEST_CASE(shell, unknown_cmd);
    RUN_TEST_CASE(shell, pipeline);
    RUN_TEST_CASE(shell, bg_job_done_with_fg_job);
    RUN_TEST_CASE(shell, builtin_in_bg);
    RUN_TEST_CASE(shell, pipeline_of_builtins_does_not_duplicate_prompt);
    RUN_TEST_CASE(shell, simple_andor_chain);
    RUN_TEST_CASE(shell, simple_bg_andor_chain);
    RUN_TEST_CASE(shell, and_if_logic);
    RUN_TEST_CASE(shell, or_if_logic);
    RUN_TEST_CASE(shell, or_if_logic_bg);
    RUN_TEST_CASE(shell, and_if_logic_bg);
    RUN_TEST_CASE(shell, and_or_chain_with_builtins);
    RUN_TEST_CASE(shell, prompt_after_launch_fg_builtin);
    RUN_TEST_CASE(shell, jobs_builtin);
    RUN_TEST_CASE(shell, builtin_and_or_logic);

    close_pty_test(&ptyt);
}
