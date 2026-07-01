#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>

#include "unity.h"
#include "executor.h"
#include "expander.h"
#include "parser.h"
#include "lexer.h"

static da_tok tokens = {0};
static ps_job job = {0};

void setUp(void) {
    tokens = (da_tok){0};
    job = (ps_job){0};
}

void tearDown(void) {
    lx_free(&tokens);
    ps_free(&job);
}

sh_result get_result(const char *shell_cmd) {
    /* Should print 6 */

    if (lx_tokenize(shell_cmd, &tokens) == -1) {
        fprintf(stderr, "Lexer error\n");
        exit(EXIT_FAILURE);
    }

    if (ps_parse(&tokens, &job) == -1) {
        fprintf(stderr, "Parser error\n");
        exit(EXIT_FAILURE);
    }

    if (ex_expand(&job) == -1) {
        fprintf(stderr, "Expansion error\n");
        exit(EXIT_FAILURE);
    }

    sh_result result = sh_run(&job);
    return result;
}

void test_three_pipeline_cmd(void) {
    int pfd[2];
    TEST_ASSERT_EQUAL_INT(0, pipe2(pfd, O_CLOEXEC));
    TEST_ASSERT_EQUAL_INT(0, dup2(pfd[1], STDOUT_FILENO));
    if (pfd[1] != STDOUT_FILENO)
        TEST_ASSERT_EQUAL_INT(0, close(pfd[1]));

    sh_result result = get_result("echo hello | wc -c | grep 6");
    TEST_ASSERT_EQUAL_INT(SH_OK, result.exit_code);

    /* expected output '6' */
    char output[2];
    char c;

    TEST_ASSERT_EQUAL_INT(1, read(pfd[0], output, 1));
    /* shell execed program should close write end after exiting
     * so we should get 0 (EOF) if we read again */
    TEST_ASSERT_EQUAL_INT(0, read(pfd[0], &c, 1));

    output[1] = '\0';

    TEST_ASSERT_EQUAL_STRING("6", output);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_three_pipeline_cmd);

    return UNITY_END();
}
