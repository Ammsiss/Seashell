#include "unity_internals.h"
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>

#include "log.h"
#include "dyn_arr.h"
#include "executor.h"
#include "lexer.h"
#include "parser.h"
#include "expander.h"
#include "unity.h"

#define OUTPUT_BUF_SIZE 1024

#define SET_FOO_BAR \
    const char *shell_cmd1 = "set FOO bar"; \
    const char *output1 = ""; \
    validate_shell_output(shell_cmd1, output1, strlen(output1));

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
    TEST_ASSERT_EQUAL_INT(0, pipe(pfd));

    sh_run(&job, STDIN_FILENO, pfd[1]);

    TEST_ASSERT_EQUAL_INT(0, close(pfd[1]));

    char c;
    char output[OUTPUT_BUF_SIZE];

    int num_read = read(pfd[0], output, exp_msg_size);
    output[num_read] = '\0';

    TEST_ASSERT_EQUAL_size_t(exp_msg_size, num_read);
    TEST_ASSERT_EQUAL_INT(0, read(pfd[0], &c, 1)); /* pipe should be empty */
    TEST_ASSERT_EQUAL_STRING(exp_output, output);

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
    const char *shell_cmd1 = "set FOO bar";
    const char *output1 = "";
    validate_shell_output(shell_cmd1, output1, strlen(output1));

    const char *shell_cmd2 = "echo $FOO";
    const char *output2 = "bar\n";
    validate_shell_output(shell_cmd2, output2, strlen(output2));
}

void test_escaped_variable_should_not_expand(void) {
    SET_FOO_BAR

    const char *shell_cmd2 = "echo \\$FOO";
    const char *output2 = "$FOO\n";
    validate_shell_output(shell_cmd2, output2, strlen(output2));
}

void test_variable_expansion_should_be_greedy(void) {
    SET_FOO_BAR

    const char *shell_cmd2 = "echo $FOOzoo";
    const char *output2 = "\n";
    validate_shell_output(shell_cmd2, output2, strlen(output2));
}

void test_variable_expansion_then_word(void) {
    SET_FOO_BAR

    const char *shell_cmd2 = "echo $FOO zoo";
    const char *output2 = "bar zoo\n";
    validate_shell_output(shell_cmd2, output2, strlen(output2));
}

void test_word_directly_before_variable_expansion(void) {
    SET_FOO_BAR

    const char *shell_cmd2 = "echo zoo$FOO";
    const char *output2 = "zoobar\n";
    validate_shell_output(shell_cmd2, output2, strlen(output2));
}

void test_backslash_should_end_variable(void) {
    SET_FOO_BAR

    const char *shell_cmd2 = "echo $FOO\\zoo";
    const char *output2 = "barzoo\n";
    validate_shell_output(shell_cmd2, output2, strlen(output2));
}

void test_back_to_back_variables_should_both_expand(void) {
    SET_FOO_BAR

    const char *shell_cmd2 = "echo $FOO$FOO";
    const char *output2 = "barbar\n";
    validate_shell_output(shell_cmd2, output2, strlen(output2));
}

void test_variable_ended_with_backslash_then_variable(void) {
    SET_FOO_BAR

    const char *shell_cmd2 = "echo $FOO\\$FOO";
    const char *output2 = "bar$FOO\n";
    validate_shell_output(shell_cmd2, output2, strlen(output2));
}

int main(void) {
    log_init();

    UNITY_BEGIN();

    RUN_TEST(test_three_pipeline_cmd);
    RUN_TEST(test_andors);
    RUN_TEST(test_andors2);
    RUN_TEST(test_simple_variable_expansion);
    RUN_TEST(test_escaped_variable_should_not_expand);
    RUN_TEST(test_variable_expansion_should_be_greedy);
    RUN_TEST(test_variable_expansion_then_word);
    RUN_TEST(test_word_directly_before_variable_expansion);
    RUN_TEST(test_backslash_should_end_variable);
    RUN_TEST(test_back_to_back_variables_should_both_expand);
    RUN_TEST(test_variable_ended_with_backslash_then_variable);

    return UNITY_END();
}
