#define _GNU_SOURCE

#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include "unity.h"
#include "lexer.h"

/************************* Utility Funcs *************************/

void print_tok_list(const struct lx_tok *list, size_t size, const char *msg) {
    printf("%s: ", msg);
    for (size_t i = 0; i < size; ++i) {
        switch (list[i].kind) {
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
        case LX_TOK_WORD: printf("WORD(%s) ", list[i].value); break;
        default: break;
        }
    }
    printf("\n");
}

void setUp(void) {}
void tearDown(void) {}

/************************* lx_push_tok *************************/

void helper_push_tok(size_t ex_size, size_t size, size_t ex_cap, size_t cap) {
    struct lx_tok *list = malloc(cap * sizeof(struct lx_tok));
    TEST_ASSERT_NOT_NULL(list);

    struct lx_tok *token = lx_push_tok(&list, &size, &cap);

    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL_PTR(&list[size - 1], token);

    TEST_ASSERT_EQUAL_size_t(ex_size, size);
    TEST_ASSERT_EQUAL_size_t(ex_cap, cap);

    free(list);
}

void test_push_tok_size_cap_equal(void) {
    helper_push_tok(2, 1, 2, 1);
}

void test_push_tok_size_less_than_cap(void) {
    helper_push_tok(3, 2, 3, 3);
}

void test_push_tok_resize_preserves_data(void) {
    size_t size = 1;
    size_t cap = 1;

    struct lx_tok *list = malloc(cap * sizeof(struct lx_tok));
    TEST_ASSERT_NOT_NULL(list);

    const char *demo_value = "x";

    list[0].kind = LX_TOK_WORD;

    list[0].value = malloc(2);
    TEST_ASSERT_NOT_NULL(list[0].value);
    list[0].value[0] = 'x';
    list[0].value[1] = '\0';

    TEST_ASSERT_NOT_NULL(lx_push_tok(&list, &size, &cap));

    TEST_ASSERT_EQUAL_INT(LX_TOK_WORD, list[0].kind);
    TEST_ASSERT_EQUAL_STRING(demo_value, list[0].value);

    free(list[0].value);
    free(list);
}

void stress_push_tok_size_cap_equal(void) {
    for (size_t i = 0; i < 10000; ++i) {
        size_t size = rand() % 100 + 1;
        size_t cap = size;
        helper_push_tok(size + 1, size, cap * 2, cap);
    }
}

void stress_push_tok_size_less_than_cap(void) {
    for (size_t i = 0; i < 10000; ++i) {
        size_t size = rand() % 100 + 1;
        size_t cap = rand() % 100 + size + 1;
        helper_push_tok(size + 1, size, cap, cap);
    }
}

/************************* lx_add_tok *************************/

void test_lx_add_tok_token_value_is_null_terminated(void) {
    size_t size = 0;
    size_t cap = 1;

    struct lx_tok *list = malloc(cap * sizeof(struct lx_tok));
    TEST_ASSERT_NOT_NULL(list);

    const char *token_text = "Hi there!";

    int token_len = 8;
    const char *end = &token_text[7];

    TEST_ASSERT_NOT_EQUAL_INT(-1, lx_add_tok(&list, &size, &cap,
                LX_TOK_WORD, token_text, end + 1));

    TEST_ASSERT_EQUAL_MEMORY("Hi there", list[0].value, token_len);
    TEST_ASSERT_EQUAL_CHAR('\0', list[0].value[token_len]);

    lx_free(list, size);
}

/************************* lx_tokenize *************************/

void helper_lx_tokenize_assert_tokens(
        const char *cmd,
        const struct lx_tok *expected,
        size_t expected_size
) {
    struct lx_tok *list;
    size_t list_size;

    TEST_ASSERT_EQUAL(0, lx_tokenize(cmd, &list, &list_size));
    TEST_ASSERT_NOT_NULL(list);

    // print_tok_list(expected, expected_size, "EXPECT");
    // print_tok_list(list, list_size, "RESULT");

    TEST_ASSERT_EQUAL_size_t(expected_size, list_size);

    for (size_t i = 0; i < list_size; ++i) {
        TEST_ASSERT_EQUAL(expected[i].kind, list[i].kind);
        if (expected[i].value == NULL) {
            TEST_ASSERT_NULL(list[i].value);
        } else
            TEST_ASSERT_EQUAL_STRING(expected[i].value, list[i].value);
    }

    lx_free(list, list_size);
}

void test_lx_tokenize_operators_only(void) {
    const struct lx_tok expected[12] = {
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
    const struct lx_tok expected[1] = {
        { LX_TOK_WORD, "Hello,World!" },
    };

    helper_lx_tokenize_assert_tokens("Hello,World!", expected, 1);
}

void test_lx_tokenize_whitespace_only(void) {
    const char *cmd = "      ";

    struct lx_tok *list;
    size_t list_size;

    TEST_ASSERT_EQUAL(0, lx_tokenize(cmd, &list, &list_size));
    TEST_ASSERT_EQUAL(0, list_size);

    lx_free(list, list_size);
}

void test_lx_tokenize_words_operators_whitespace(void) {
    const struct lx_tok expected[6] = {
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
    const struct lx_tok expected[1] = {
        { LX_TOK_WORD, "\"\"" },
    };

    helper_lx_tokenize_assert_tokens("\"\"", expected, 1);
}

void test_lx_tokenize_quoted_string(void) {
    const struct lx_tok expected[1] = {
        { LX_TOK_WORD, "\"Hello,World!\"" },
    };

    helper_lx_tokenize_assert_tokens("\"Hello,World!\"", expected, 1);
}

void test_lx_tokenize_quoted_string_with_delimiters(void) {
    const struct lx_tok expected[3] = {
        { LX_TOK_WORD, "\"\"hi\"& |\"" },
        { LX_TOK_BG, NULL },
        { LX_TOK_WORD, "hello\" \"" },
    };

    helper_lx_tokenize_assert_tokens("\"\"hi\"& |\"&hello\" \"", expected, 3);
}

void test_lx_tokenize_unmatched_quotes_should_fail(void) {
    struct lx_tok *list;
    size_t list_size;

    TEST_ASSERT_EQUAL(-1, lx_tokenize("\"hello", &list, &list_size));
}

void test_lx_tokenize_escaped_quotes_are_skipped(void) {
    const struct lx_tok expected[1] = {
        { LX_TOK_WORD, "\"He said \\\"n&thing...\\\"\"" },
    };

    helper_lx_tokenize_assert_tokens(
            "\"He said \\\"n&thing...\\\"\"",
            expected, 1
    );
}

void test_lx_tokenize_escaped_backslashes_are_skipped(void) {
    const struct lx_tok expected[2] = {
        { LX_TOK_WORD, "echo" },
        { LX_TOK_WORD, "\"\\\\\"" },
    };

    helper_lx_tokenize_assert_tokens(
            "echo \"\\\\\"",
            expected, 2
    );
}

void test_lx_tokenize_two_char_operators(void) {
    const struct lx_tok expected[9] = {
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

int main(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand(ts.tv_sec ^ ts.tv_nsec);

    UNITY_BEGIN();

    /* lx_push_tok */
    RUN_TEST(test_push_tok_size_cap_equal);
    RUN_TEST(test_push_tok_size_less_than_cap);
    RUN_TEST(test_push_tok_resize_preserves_data);
    RUN_TEST(stress_push_tok_size_less_than_cap);
    RUN_TEST(stress_push_tok_size_cap_equal);

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
    RUN_TEST(test_lx_tokenize_escaped_quotes_are_skipped);
    RUN_TEST(test_lx_tokenize_escaped_backslashes_are_skipped);
    RUN_TEST(test_lx_tokenize_two_char_operators);

    return UNITY_END();
}
