#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>

#include "unity.h"

#define OUTPUT_BUF_SIZE 1024

void setUp(void) {}
void tearDown(void) {}

void validate_shell_output(char **argv, const char *exp_output, \
        size_t exp_msg_size) {

    int pfd[2];
    TEST_ASSERT_EQUAL_INT(0, pipe(pfd));

    pid_t child_pid;
    child_pid = fork();
    TEST_ASSERT_NOT_EQUAL_INT(-1, child_pid);

    if (child_pid == 0) {
        if (close(pfd[0]) == -1)
            _exit(EXIT_FAILURE);
        if (dup2(pfd[1], STDOUT_FILENO) == -1)
            _exit(EXIT_FAILURE);
        if (pfd[1] != STDOUT_FILENO)
            if (close(pfd[1]) == -1)
                _exit(EXIT_FAILURE);

        execvp(argv[0], argv);
        _exit(EXIT_FAILURE);
    }

    TEST_ASSERT_EQUAL_INT(0, close(pfd[1]));

    if (waitpid(child_pid, NULL, 0) == -1)
        _exit(EXIT_FAILURE);

    char c;
    char output[OUTPUT_BUF_SIZE];

    int num_read = read(pfd[0], output, exp_msg_size);
    output[num_read] = '\0';

    TEST_ASSERT_EQUAL_size_t(exp_msg_size, num_read);
    TEST_ASSERT_EQUAL_INT(0, read(pfd[0], &c, 1)); /* pipe should be empty */
    TEST_ASSERT_EQUAL_STRING(exp_output, output);

    close(pfd[0]);
}

void test_three_pipeline_cmd(void) {
    char *argv[] = {
        "../seashell", "-c", "echo hello | wc -c | grep 6", (char *) NULL
    };

    const char *output = "6\n";
    validate_shell_output(argv, output, strlen(output));
}

void test_andors(void) {
    char *argv[] = {
        "../seashell", "-c",
        "echo hello | wc -c && true && echo 'coolio' || echo 'nope'",
        (char *) NULL
    };

    const char *output = "6\ncoolio\n";
    validate_shell_output(argv, output, strlen(output));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_three_pipeline_cmd);
    RUN_TEST(test_andors);

    return UNITY_END();
}
