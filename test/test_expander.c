#include "unity.h"
#include "expander.h"
#include "parser.h"
#include "lexer.h"

void setUp(void) {}
void tearDown(void) {}

void for_each_word(ps_job *job, void (* do_func)(ps_word *word)) {
    for (size_t i = 0; i < job->andors.size; ++i) {
        ps_andor *andor = &job->andors.data[i];

        for (size_t j = 0; j < andor->pipeline.cmds.size; ++j) {
            ps_cmd *cmd = &andor->pipeline.cmds.data[j];

            for (size_t k = 0; k < cmd->words.size; ++k) {
                ps_word *word = &cmd->words.data[k];
                do_func(word);
            }
        }
    }
}

void print_arg(ps_word *word) {
    printf("%s\n", word->arg);
}

void test_expansion(void) {
    const char *shell_cmd = "echo hello'world'\"x\"";

    da_tok tokens = {0};
    TEST_ASSERT_EQUAL_INT(0, lx_tokenize(shell_cmd, &tokens));

    ps_job job = {0};
    TEST_ASSERT_EQUAL_INT(0, ps_parse(&tokens, &job));

    TEST_ASSERT_EQUAL_INT(0, ex_expand(&job));

    for_each_word(&job, print_arg);

    lx_free(&tokens);
    ps_free(&job);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_expansion);

    return UNITY_END();
}
