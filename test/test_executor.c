#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "log.h"
#include "dyn_arr.h"
#include "executor.h"
#include "lexer.h"
#include "parser.h"
#include "expander.h"
#include "unity.h"
#include "utils.h"

#define BUF_SIZE 1024

#define TMP_DIR_PATH "/home/juta/Projects/Seashell/test/tmp/"
#define REDIR_TARGET TMP_DIR_PATH "output.txt"
#define REDIR_INPUT TMP_DIR_PATH "input.txt"

#define SET_FOO_BAR \
    const char *var_cmd = "set FOO bar"; \
    const char *var_output = ""; \
    validate_shell_output(var_cmd, var_output, strlen(var_output));

void setUp(void) {}
void tearDown(void) {}

void validate_shell_output(const char *shell_cmd, const char *exp_output, \
        size_t exp_msg_size) {
    da_tok tokens = {0};
    ps_job job = {0};

    TEST_ASSERT_EQUAL_INT(0, lx_tokenize(shell_cmd, &tokens));
    TEST_ASSERT_EQUAL_INT(0, ps_parse(&tokens, &job));
    TEST_ASSERT_EQUAL_INT(0, ex_expand(&job));

    int pfd[2];
    if (pipe(pfd) == -1) {
        perror("pipe");
        TEST_FAIL();
    }

    sh_run(&job, STDIN_FILENO, pfd[1]);

    TEST_ASSERT_EQUAL_INT(0, close(pfd[1]));

    char output[BUF_SIZE];

    int num_read = read(pfd[0], output, exp_msg_size);
    if (num_read == -1) {
        perror("read");
        TEST_FAIL();
    }

    output[num_read] = '\0';

    TEST_ASSERT_EQUAL_size_t(exp_msg_size, num_read);
    TEST_ASSERT_EQUAL_STRING(exp_output, output);

    TEST_ASSERT_EQUAL_INT(0, read(pfd[0], output, 1)); /* pipe should be empty */

    close(pfd[0]);
    lx_free(&tokens);
    ps_free(&job);
}

void test_three_pipeline_cmd(void) {
    const char *shell_cmd = "echo hello | wc -c | grep 6";
    const char *output = "6\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_andors(void) {
    const char *shell_cmd = \
        "echo hello | wc -c && true && echo 'coolio' || echo 'nope'";
    const char *output = "6\ncoolio\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_andors2(void) {
    const char *shell_cmd = \
        "false && true && true && true || echo -n hello, && echo world";
    const char *output = "hello,world\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_simple_variable_expansion(void) {
    SET_FOO_BAR

    const char *shell_cmd = "echo $FOO";
    const char *output = "bar\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_escaped_variable_should_not_expand(void) {
    SET_FOO_BAR

    const char *shell_cmd = "echo \\$FOO";
    const char *output = "$FOO\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_variable_expansion_should_be_greedy(void) {
    SET_FOO_BAR

    const char *shell_cmd = "echo $FOOzoo";
    const char *output = "\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_variable_expansion_then_word(void) {
    SET_FOO_BAR

    const char *shell_cmd = "echo $FOO zoo";
    const char *output = "bar zoo\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_word_directly_before_variable_expansion(void) {
    SET_FOO_BAR

    const char *shell_cmd = "echo zoo$FOO";
    const char *output = "zoobar\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_backslash_should_end_variable(void) {
    SET_FOO_BAR

    const char *shell_cmd = "echo $FOO\\zoo";
    const char *output = "barzoo\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_back_to_back_variables_should_both_expand(void) {
    SET_FOO_BAR

    const char *shell_cmd = "echo $FOO$FOO";
    const char *output = "barbar\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void test_variable_ended_with_backslash_then_variable(void) {
    SET_FOO_BAR

    const char *shell_cmd = "echo $FOO\\$FOO";
    const char *output = "bar$FOO\n";
    validate_shell_output(shell_cmd, output, strlen(output));
}

void validate_redir_target(const char *file_content, bool rmfile) {
    size_t exp_file_size = strlen(file_content);

    int fd = open(REDIR_TARGET, O_RDONLY);
    if (fd == -1) {
        perror("open");
        TEST_FAIL();
    }

    char buf[BUF_SIZE];
    int nbytes = (BUF_SIZE - 1 > exp_file_size) ? exp_file_size : BUF_SIZE - 1;

    int num_read = read(fd, buf, nbytes);
    if (num_read == -1) {
        perror("read");
        if (unlink(REDIR_TARGET) == -1)
            perror("unlink");
        TEST_FAIL();
    }

    buf[num_read] = '\0';

    TEST_ASSERT_EQUAL_size_t(exp_file_size, (size_t) num_read);
    TEST_ASSERT_EQUAL_STRING(file_content, buf);

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        TEST_FAIL();
    }

    TEST_ASSERT_EQUAL_INT(exp_file_size, sb.st_size);

    close (fd);

    if (rmfile)
        if (unlink(REDIR_TARGET) == -1)
            perror("unlink");
}

void test_redirect_stdout(void) {
    const char *shell_cmd = "echo -n 'x' >" REDIR_TARGET;
    const char *output = "";

    validate_shell_output(shell_cmd, output, strlen(output));
    validate_redir_target("x", true);
}

void test_redirect_stderr(void) {
    const char *shell_cmd = "perl -e \"print STDERR 'x'\" 2>" REDIR_TARGET;
    const char *output = "";

    validate_shell_output(shell_cmd, output, strlen(output));
    validate_redir_target("x", true);
}

void test_redirect_append(void) {
    const char *shell_cmd = "echo -n 'x' >>" REDIR_TARGET;
    const char *output = "";

    validate_shell_output(shell_cmd, output, strlen(output));
    validate_redir_target("x", false);

    validate_shell_output(shell_cmd, output, strlen(output));
    validate_redir_target("xx", true);
}

void test_redirect_stdin(void) {
    int fd = open(REDIR_INPUT, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd == -1) {
        perror("open");
        TEST_FAIL();
    }

    if (write(fd, "x", 1) == -1) {
        perror("write");
        TEST_FAIL();
    }

    const char *shell_cmd = "cat <" REDIR_INPUT;
    const char *output = "x";

    validate_shell_output(shell_cmd, output, strlen(output));

    close(fd);

    if (unlink(REDIR_INPUT) == -1)
        perror("unlink");
}


int main(void) {
    log_init();

    if (set_sig_action(SIGTTOU, SIG_IGN, 0, NULL) == -1)
        return EXIT_FAILURE;

    UNITY_BEGIN();

    RUN_TEST(test_three_pipeline_cmd);
    RUN_TEST(test_andors);
    RUN_TEST(test_andors2);

    /* Variable expansion */
    RUN_TEST(test_simple_variable_expansion);
    RUN_TEST(test_escaped_variable_should_not_expand);
    RUN_TEST(test_variable_expansion_should_be_greedy);
    RUN_TEST(test_variable_expansion_then_word);
    RUN_TEST(test_word_directly_before_variable_expansion);
    RUN_TEST(test_backslash_should_end_variable);
    RUN_TEST(test_back_to_back_variables_should_both_expand);
    RUN_TEST(test_variable_ended_with_backslash_then_variable);

    /* Redirections */
    if (mkdir(TMP_DIR_PATH, 0700) == -1) {
        perror("mkdir");
        return EXIT_FAILURE;
    }

    RUN_TEST(test_redirect_stdout);
    RUN_TEST(test_redirect_stderr);
    RUN_TEST(test_redirect_append);
    RUN_TEST(test_redirect_stdin);

    if (rmdir(TMP_DIR_PATH) == -1) {
        perror("rmdir");
        return EXIT_FAILURE;
    }

    return UNITY_END();
}
