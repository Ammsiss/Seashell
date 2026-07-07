#include "unity.h"
#include "expander.h"
#include "parser.h"
#include "lexer.h"

#define FOR_EACH_WORD \
    for (size_t i = 0; i < job.andors.size; ++i) { \
        ps_andor *andor = &job.andors.data[i]; \
        \
        for (size_t j = 0; j < andor->pline.cmds.size; ++j) { \
            ps_cmd *cmd = &andor->pline.cmds.data[j]; \
            \
            for (size_t k = 0; k < cmd->words.size; ++k) { \
                ps_word *word = &cmd->words.data[k];
#define END_FOR_EACH \
            } \
        } \
    } \

void setUp(void) {}
void tearDown(void) {}

void validate_args(const char *shell_cmd, char **exp) {
    da_tok tokens = {0};
    ps_job job = {0};

    TEST_ASSERT_EQUAL_INT(0, lx_tokenize(shell_cmd, &tokens));
    TEST_ASSERT_EQUAL_INT(0, ps_parse(&tokens, &job));
    TEST_ASSERT_EQUAL_INT(0, ex_expand(&job));

    FOR_EACH_WORD /* LOOP START - ps_word *word in scope */

    TEST_ASSERT_EQUAL_STRING(exp[k], word->arg);

    END_FOR_EACH /* LOOP END */

    lx_free(&tokens);
    ps_free(&job);
}

void test_multiple_of_same_segment(void) {
    char *exp_args[] = {
        "grep", "abc", "def", "ghi"
    };

    validate_args("grep \"a\"\"b\"\"c\" 'd''e''f' ghi", exp_args);
}

void test_multi_segment_words(void) {
    char *exp_args[] = {
        "echo", "abc"
    };

    validate_args("echo \"a\"'b'c", exp_args);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_multiple_of_same_segment);
    RUN_TEST(test_multi_segment_words);

    return UNITY_END();
}
