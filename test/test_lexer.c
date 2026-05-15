#define _GNU_SOURCE

#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include "unity.h"
#include "lexer.h"

/* Utility functions */

void print_lx_tok_list(struct lx_tok *list, const char *msg) {
    printf("%s:", msg);
    for (struct lx_tok *tok = list; tok->kind != LX_TOK_END; ++tok) {
        switch (tok->kind) {
        case LX_TOK_WORD: printf(" LX_TOK_WORD [%s] ", tok->value); break;
        case LX_TOK_INBG: printf(" LX_TOK_INBG [&] "); break;
        case LX_TOK_REDL: printf(" LX_TOK_REDL [<] "); break;
        case LX_TOK_REDR: printf(" LX_TOK_REDR [>] "); break;
        case LX_TOK_PIPE: printf(" LX_TOK_PIPE [|] "); break;
        default: break;
        }
    }
    printf("\n");
}

void free_lx_tok_list(struct lx_tok *list, size_t len) {
    for (size_t i = 0; i < len; ++i )
        if (list[i].kind == LX_TOK_WORD)
            free(list[i].value);
    free(list);
}

void setUp(void) {}
void tearDown(void) {}

/* lx_push_tok() */

struct lx_tok *lx_push_tok(struct lx_tok **list, size_t *size, size_t *cap);
int lx_add_tok(struct lx_tok **list, size_t *size, size_t *cap,
        enum lx_code kind, const char *start, size_t len);

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

/* stress tests */

void stest_push_tok_size_cap_equal(void) {
    for (size_t i = 0; i < 10000; ++i) {
        size_t size = rand() % 100 + 1;
        size_t cap = size;
        helper_push_tok(size + 1, size, cap * 2, cap);
    }
}

void stest_push_tok_size_less_than_cap(void) {
    for (size_t i = 0; i < 10000; ++i) {
        size_t size = rand() % 100 + 1;
        size_t cap = rand() % 100 + size + 1;
        helper_push_tok(size + 1, size, cap, cap);
    }
}

void test_push_tok_resize_preserves_data(void) {
    size_t size = 1;
    size_t cap = 1;

    struct lx_tok *list = malloc(cap * sizeof(struct lx_tok));
    TEST_ASSERT_NOT_NULL(list);

    list[0].kind = LX_TOK_END;
    list[0].value = NULL;

    TEST_ASSERT_NOT_NULL(lx_push_tok(&list, &size, &cap));

    TEST_ASSERT_EQUAL_INT(LX_TOK_END, list[0].kind);
    TEST_ASSERT_EQUAL_PTR(NULL, list[0].value);

    free(list);
}

/* lx_add_tok */

void test_lx_add_tok_token_value_is_null_terminated(void) {
    size_t size = 0;
    size_t cap = 1;

    struct lx_tok *list = malloc(cap * sizeof(struct lx_tok));
    TEST_ASSERT_NOT_NULL(list);

    int token_len = 8;   /* 1 less then strlen(token_text) */
    const char *token_text = "Hi there!";

    TEST_ASSERT_NOT_EQUAL_INT(-1, lx_add_tok(&list, &size, &cap,
                LX_TOK_WORD, token_text, token_len));

    TEST_ASSERT_EQUAL_MEMORY("Hi there", list[0].value, token_len);
    TEST_ASSERT_EQUAL_CHAR('\0', list[0].value[token_len]);

    free_lx_tok_list(list, size);
}

/* lx_tokenize() */

void test_lx_tokenize_operators_only(void) {
    const char *cmd = "|<&>";

    struct lx_tok *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_EQUAL(LX_TOK_PIPE, list[0].kind);
    TEST_ASSERT_EQUAL(LX_TOK_REDL, list[1].kind);
    TEST_ASSERT_EQUAL(LX_TOK_INBG, list[2].kind);
    TEST_ASSERT_EQUAL(LX_TOK_REDR, list[3].kind);
    TEST_ASSERT_EQUAL_INT(LX_TOK_END, list[4].kind);

    for (size_t i = 0; i < 5; ++i)
        TEST_ASSERT_EQUAL_PTR(NULL, list[i].value);

    free_lx_tok_list(list, 1);
}

void test_lx_tokenize_word_only(void) {
    const char *cmd = "Hello,World!";

    struct lx_tok *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_EQUAL(LX_TOK_WORD, list[0].kind);
    TEST_ASSERT_EQUAL_STRING(cmd, list[0].value);

    TEST_ASSERT_EQUAL(LX_TOK_END, list[1].kind);
    TEST_ASSERT_EQUAL_PTR(NULL, list[1].value);

    free_lx_tok_list(list, 2);
}

void test_lx_tokenize_whitespace_only(void) {
    const char *cmd = "   \t   ";

    struct lx_tok *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_EQUAL(LX_TOK_END, list[0].kind);
    TEST_ASSERT_EQUAL_PTR(NULL, list[0].value);

    free_lx_tok_list(list, 1);
}

/* Token 1: "hello"   Token 2: "<"   Token 3: "world!" */
void test_lx_tokenize_words_operators_whitespace(void) {
    const char *cmd = "hello <world! ";

    struct lx_tok *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_EQUAL(LX_TOK_WORD, list[0].kind);
    TEST_ASSERT_EQUAL_STRING("hello", list[0].value);

    TEST_ASSERT_EQUAL(LX_TOK_REDL, list[1].kind);
    TEST_ASSERT_EQUAL_PTR(NULL, list[1].value);

    TEST_ASSERT_EQUAL(LX_TOK_WORD, list[2].kind);
    TEST_ASSERT_EQUAL_STRING("world!", list[2].value);

    TEST_ASSERT_EQUAL(LX_TOK_END, list[3].kind);
    TEST_ASSERT_EQUAL_PTR(NULL, list[3].value);

    free_lx_tok_list(list, 4);
}

void test_lx_tokenize_quoted_string(void) {
    const char *cmd = "\"Hello,World!\"";

    struct lx_tok *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_EQUAL(LX_TOK_WORD, list[0].kind);
    TEST_ASSERT_EQUAL_STRING(cmd, list[0].value);

    TEST_ASSERT_EQUAL(LX_TOK_END, list[1].kind);
    TEST_ASSERT_EQUAL_PTR(NULL, list[1].value);

    free_lx_tok_list(list, 2);
}

/*   Token 1: ""hi"& |"   Token 2: &   Token 3: hello" "   */
void test_lx_tokenize_quoted_string_with_delimiters(void) {
    const char *cmd = "\"\"hi\"& |\"&hello\" \"";

    struct lx_tok *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL(list);

    TEST_ASSERT_EQUAL(LX_TOK_WORD, list[0].kind);
    TEST_ASSERT_EQUAL_STRING_LEN("\"\"hi\"& |\"", list[0].value, 7);

    TEST_ASSERT_EQUAL(LX_TOK_INBG, list[1].kind);
    TEST_ASSERT_EQUAL_PTR(NULL, list[1].value);

    TEST_ASSERT_EQUAL(LX_TOK_WORD, list[2].kind);
    TEST_ASSERT_EQUAL_STRING_LEN("hello\" \"", list[2].value, 5);

    TEST_ASSERT_EQUAL(LX_TOK_END, list[3].kind);
    TEST_ASSERT_EQUAL_PTR(NULL, list[3].value);

    // print_lx_tok_list(list, "RESULT");

    free_lx_tok_list(list, 4);
}

// /* stress tests */

void stest_lx_tokenize_words_operators_whitespace(void) {
    #define LIST_SIZE 10000

    char cmd[LIST_SIZE + 1];
    enum lx_code kinds[LIST_SIZE];

    int one_time_char = 0;
    for (size_t i = 0; i < LIST_SIZE; ++i) {
        size_t num = rand() % 6;
        if (num == 4) {
            if (one_time_char == 1) {
                --i;
                continue;
            } else
                one_time_char = 1;
        } else
            one_time_char = 0;

        switch (num) {
        case 0: cmd[i] = '|'; kinds[i] = LX_TOK_PIPE; break;
        case 1: cmd[i] = '<'; kinds[i] = LX_TOK_REDL; break;
        case 2: cmd[i] = '>'; kinds[i] = LX_TOK_REDR; break;
        case 3: cmd[i] = '&'; kinds[i] = LX_TOK_INBG; break;
        case 4: cmd[i] = 'W'; kinds[i] = LX_TOK_WORD; break;
        case 5: cmd[i] = ' '; kinds[i] = LX_WTSP; break;
        default: TEST_FAIL();
        }
    }

    cmd[LIST_SIZE] = '\0';

    struct lx_tok *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL(list);

    size_t ws_cnt = 0;
    for (size_t i = 0; i < LIST_SIZE; ++i) {
        if (i + ws_cnt == LIST_SIZE)
            break;

        if (kinds[i + ws_cnt] == LX_WTSP) {
            ++ws_cnt;
            --i;
            continue;
        }

        TEST_ASSERT_EQUAL(kinds[i + ws_cnt], list[i].kind);

        if (list[i].kind != LX_TOK_WORD) {
            TEST_ASSERT_EQUAL_PTR(NULL, list[i].value);
        } else
            TEST_ASSERT_EQUAL_CHAR(cmd[i + ws_cnt], *list[i].value);
    }

    free_lx_tok_list(list, LIST_SIZE - ws_cnt);
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
    /* stress tests */
    RUN_TEST(stest_push_tok_size_less_than_cap);
    RUN_TEST(stest_push_tok_size_cap_equal);

    /* lx_add_tok */
    RUN_TEST(test_lx_add_tok_token_value_is_null_terminated);

    /* lx_tokenize */
    RUN_TEST(test_lx_tokenize_operators_only);
    RUN_TEST(test_lx_tokenize_word_only);
    RUN_TEST(test_lx_tokenize_whitespace_only);
    RUN_TEST(test_lx_tokenize_words_operators_whitespace);
    RUN_TEST(test_lx_tokenize_quoted_string);
    RUN_TEST(test_lx_tokenize_quoted_string_with_delimiters);
    /* stress tests */
    RUN_TEST(stest_lx_tokenize_words_operators_whitespace);

    return UNITY_END();
}
