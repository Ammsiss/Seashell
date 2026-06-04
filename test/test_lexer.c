#define _GNU_SOURCE

#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include "unity.h"
#include "lexer.h"

/************************* Utility Funcs *************************/

void print_tok_list(const da_lx_tok *list, const char *msg) {
    printf("%s: ", msg);
    for (size_t i = 0; i < list->size; ++i) {
        switch (list->data[i].kind) {
        /* Single ops */
        case LX_TOK_PIPE: printf("PIPE(|) "); break;
        case LX_TOK_BG: printf("INBG(&) "); break;
        case LX_TOK_RDR_IN: printf("RDR_IN(<) "); break;
        case LX_TOK_RDR_OUT: printf("RDR_OUT(>) "); break;
        case LX_TOK_LPAREN: printf("LPAREN(() "); break;
        case LX_TOK_RPAREN: printf("RPAREN()) "); break;
        case LX_TOK_SEMI: printf("SEMI(;) "); break;
        case LX_TOK_EOF: printf("EOF()) "); break;
        /* Double ops */
        case LX_TOK_HDOC: printf("HDOC(<<) "); break;
        case LX_TOK_APPEND: printf("APPEND(>>) "); break;
        case LX_TOK_AND_IF: printf("AND_IF(&&) "); break;
        case LX_TOK_OR_IF: printf("OR_IF(||) "); break;
        case LX_TOK_RDR_STDOUT: printf("RDR_STDOUT(1>) "); break;
        case LX_TOK_RDR_STDERR: printf("RDR_STDERR(2>) "); break;
        case LX_TOK_WORD:
            printf("WORD(%s) ", list->data[i].value);
            break;
        default:
            break;
        }
    }
    printf("\n");
}

void setUp(void) {}
void tearDown(void) {}

/************************* lx_add_tok *************************/

void test_lx_add_tok_token_value_is_null_terminated(void) {
    da_lx_tok list;
    TEST_ASSERT_EQUAL_INT(0, da_lx_tok_init(&list, 0));

    const char *token_text = "Hi there!";

    int token_len = 8;
    const char *end = &token_text[7];

    lx_scanner scanner = { LX_M_NORMAL, token_text, end + 1 };

    TEST_ASSERT_NOT_EQUAL_INT(-1, lx_add_tok(&list, LX_TOK_WORD, &scanner));

    TEST_ASSERT_EQUAL_MEMORY("Hi there",
            list.data[0].value, token_len);
    TEST_ASSERT_EQUAL_CHAR('\0',
            list.data[0].value[token_len]);

    lx_free(&list);
}

/************************* lx_tokenize *************************/

void helper_lx_tokenize_assert_tokens(
        const char *cmd,
        const lx_tok *expected,
        size_t expected_size
) {
    da_lx_tok list;
    TEST_ASSERT_EQUAL(0, lx_tokenize(cmd, &list));

    print_tok_list(&list, "List");

    TEST_ASSERT_EQUAL_size_t(expected_size, list.size);

    for (size_t i = 0; i < list.size; ++i) {
        lx_tok *tok = &list.data[i];
        TEST_ASSERT_EQUAL(expected[i].kind, tok->kind);
        if (!expected[i].value) {
            TEST_ASSERT_NULL(tok->value);
        } else
            TEST_ASSERT_EQUAL_STRING(expected[i].value, tok->value);
    }

    lx_free(&list);
}

void test_lx_tokenize_operators_only(void) {
    const lx_tok expected[12] = {
        { LX_TOK_RDR_IN, NULL },
        { LX_TOK_BG, NULL },
        { LX_TOK_SEMI, NULL },
        { LX_TOK_APPEND, NULL },
        { LX_TOK_RDR_OUT, NULL },
        { LX_TOK_RDR_STDOUT, NULL },
        { LX_TOK_RDR_STDERR, NULL },
        { LX_TOK_HDOC, NULL },
        { LX_TOK_RDR_IN, NULL },
        { LX_TOK_AND_IF, NULL },
        { LX_TOK_OR_IF, NULL },
        { LX_TOK_PIPE, NULL },
    };

    helper_lx_tokenize_assert_tokens("<&;>>>1>2><<<&&|||", expected, 12);
}

void test_lx_tokenize_word_only(void) {
    const lx_tok expected[1] = {
        { LX_TOK_WORD, "Hello,World!" },
    };

    helper_lx_tokenize_assert_tokens("Hello,World!", expected, 1);
}

void test_lx_tokenize_whitespace_only(void) {
    const char *cmd = "      ";

    da_lx_tok list;

    TEST_ASSERT_EQUAL(0, lx_tokenize(cmd, &list));
    TEST_ASSERT_EQUAL(0, list.size);

    lx_free(&list);
}

void test_lx_tokenize_words_operators_whitespace(void) {
    const lx_tok expected[6] = {
        { LX_TOK_WORD, "hello" },
        { LX_TOK_RDR_IN, NULL },
        { LX_TOK_WORD, "world!" },
        { LX_TOK_WORD, "bye" },
        { LX_TOK_BG, NULL },
        { LX_TOK_WORD, "lol" },
    };

    helper_lx_tokenize_assert_tokens("hello <world! bye&lol", expected, 6);
}

void test_lx_tokenize_empty_quote(void) {
    const lx_tok expected[1] = {
        { LX_TOK_WORD, "\"\"" },
    };

    helper_lx_tokenize_assert_tokens("\"\"", expected, 1);
}

void test_lx_tokenize_quoted_string(void) {
    const lx_tok expected[1] = {
        { LX_TOK_WORD, "\"Hello,World!\"" },
    };

    helper_lx_tokenize_assert_tokens("\"Hello,World!\"", expected, 1);
}

void test_lx_tokenize_quoted_string_with_delimiters(void) {
    const lx_tok expected[3] = {
        { LX_TOK_WORD, "\"\"hi\"& |\"" },
        { LX_TOK_BG, NULL },
        { LX_TOK_WORD, "hello\" \"" },
    };

    helper_lx_tokenize_assert_tokens("\"\"hi\"& |\"&hello\" \"", expected, 3);
}

void test_lx_tokenize_unmatched_quotes_should_fail(void) {
    da_lx_tok list;
    TEST_ASSERT_EQUAL(-1, lx_tokenize("\"hello", &list));
}

void test_lx_tokenize_quoted_quotes(void) {
    const lx_tok expected[1] = {
        { LX_TOK_WORD, "\"\\\"nice\\\"\"" },
    };

    helper_lx_tokenize_assert_tokens(
            "\"\\\"nice\\\"\"", /*  "\"nice\""  */
            expected, 1
    );
}

void test_lx_tokenize_two_char_operators(void) {
    const lx_tok expected[9] = {
        { LX_TOK_WORD, "solaar" },
        { LX_TOK_WORD, "show" },
        { LX_TOK_RDR_STDERR, NULL },
        { LX_TOK_WORD, "\\dev\\null" },
        { LX_TOK_RDR_STDOUT, NULL },
        { LX_TOK_WORD, "file" },
        { LX_TOK_AND_IF, NULL },
        { LX_TOK_WORD, "echo" },
        { LX_TOK_WORD, "\"\\\"nice\\\"\"" },
    };

    helper_lx_tokenize_assert_tokens(
            "solaar show 2> \\dev\\null1> file && echo \"\\\"nice\\\"\"",
            expected, 9
    );
}

void test_lx_tokenize_escaped_quotes_are_skipped(void) {
    const lx_tok expected[1] = {
        { LX_TOK_WORD, "\"He said \\\"n&thing...\\\"\"" },
    };

    helper_lx_tokenize_assert_tokens(
            "\"He said \\\"n&thing...\\\"\"",
            expected, 1
    );
}

void test_lx_tokenize_escaped_backslashes_are_skipped(void) {
    const lx_tok expected[2] = {
        { LX_TOK_WORD, "echo" },
        { LX_TOK_WORD, "\"\\\\\"" },
    };

    helper_lx_tokenize_assert_tokens( "echo \"\\\\\"", expected, 2);
}

void test_lx_tokenize_unquoted_backslashes_are_words(void) {
    const lx_tok expected[1] = {
        { LX_TOK_WORD, "\\\\\\" },
    };

    helper_lx_tokenize_assert_tokens( "\\\\\\", expected, 1);
}

int main(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand(ts.tv_sec ^ ts.tv_nsec);

    UNITY_BEGIN();

    /* lx_add_tok */
    RUN_TEST(test_lx_add_tok_token_value_is_null_terminated);

    /* lx_tokenize */
    RUN_TEST(test_lx_tokenize_operators_only);
    RUN_TEST(test_lx_tokenize_word_only);
    RUN_TEST(test_lx_tokenize_whitespace_only);
    RUN_TEST(test_lx_tokenize_words_operators_whitespace);
    RUN_TEST(test_lx_tokenize_empty_quote);
    RUN_TEST(test_lx_tokenize_quoted_string);
    RUN_TEST(test_lx_tokenize_quoted_string_with_delimiters);
    RUN_TEST(test_lx_tokenize_unmatched_quotes_should_fail);
    RUN_TEST(test_lx_tokenize_quoted_quotes);
    RUN_TEST(test_lx_tokenize_two_char_operators);
    RUN_TEST(test_lx_tokenize_unquoted_backslashes_are_words);

    RUN_TEST(test_lx_tokenize_escaped_quotes_are_skipped);
    RUN_TEST(test_lx_tokenize_escaped_backslashes_are_skipped);

    return UNITY_END();
}
