#define _GNU_SOURCE

#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include "unity.h"
#include "lexer.h"

/*-Required by unity-*/

void setUp(void) {}
void tearDown(void) {}

/* lx_push_token() */

struct lx_token *lx_push_token(struct lx_token **list, int *size, int *cap);

void helper_push_token(int ex_size, int size, int ex_cap, int cap) {
    struct lx_token *list = malloc(cap * sizeof(struct lx_token));
    TEST_ASSERT_NOT_NULL_MESSAGE(list, "malloc returned null");

    struct lx_token *token = lx_push_token(&list, &size, &cap);

    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL_PTR(&list[size - 1], token);

    TEST_ASSERT_EQUAL_INT_MESSAGE(ex_size, size, "size != ex_size");
    TEST_ASSERT_EQUAL_INT_MESSAGE(ex_cap, cap, "cap != ex_cap");

    free(list);
}

void test_push_token_size_cap_equal(void) {
    helper_push_token(2, 1, 2, 1);
}

void test_push_token_size_less_than_cap(void) {
    helper_push_token(3, 2, 3, 3);
}

/* stress tests */

void stest_push_token_size_cap_equal(void) {
    for (int i = 0; i < 10000; ++i) {
        int size = rand() % 100 + 1;
        int cap = size;
        helper_push_token(size + 1, size, cap * 2, cap);
    }
}

void stest_push_token_size_less_than_cap(void) {
    for (int i = 0; i < 10000; ++i) {
        int size = rand() % 100 + 1;
        int cap = rand() % 100 + size + 1;
        helper_push_token(size + 1, size, cap, cap);
    }
}

void test_push_token_resize_preserves_data(void) {
    int size = 1;
    int cap = 1;

    struct lx_token *list = malloc(cap * sizeof(struct lx_token));
    TEST_ASSERT_NOT_NULL_MESSAGE(list, "malloc returned null");

    list[0].kind = LX_TOKEN_END;
    list[0].start = NULL;

    TEST_ASSERT_NOT_NULL_MESSAGE(lx_push_token(&list, &size, &cap), "push_token");

    TEST_ASSERT_EQUAL_INT_MESSAGE(LX_TOKEN_END, list[0].kind, "list[0].kind");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(NULL, list[0].start, "list[0].start");

    free(list);
}

/* lx_tokenize() */

void test_lx_tokenize_operators_only(void) {
    const char *cmd = "|<&>";

    struct lx_token *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL_MESSAGE(list, "list");

    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_PIPE, list[0].kind, "list[0].kind");
    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_REDL, list[1].kind, "list[1].kind");
    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_INBG, list[2].kind, "list[2].kind");
    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_REDR, list[3].kind, "list[3].kind");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LX_TOKEN_END, list[4].kind, "list[4].kind");

    free(list);
}

void test_lx_tokenize_word_only(void) {
    const char *cmd = "Hello,World!";

    struct lx_token *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL_MESSAGE(list, "list");

    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_WORD, list[0].kind, "list[0].kind");
    TEST_ASSERT_EQUAL_STRING_LEN(cmd, list[0].start, 12);
    TEST_ASSERT_EQUAL_INT(12, list[0].len);

    TEST_ASSERT_EQUAL(LX_TOKEN_END, list[1].kind);

    free(list);
}

void test_lx_tokenize_whitespace_only(void) {
    const char *cmd = "   \t   ";

    struct lx_token *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL_MESSAGE(list, "list");

    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_END, list[0].kind, "list[0].kind");

    free(list);
}

void test_lx_tokenize_words_operators_whitespace(void) {
    const char *cmd = "hello <world! ";

    struct lx_token *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL_MESSAGE(list, "list");

    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_WORD, list[0].kind, "list[0].kind");
    TEST_ASSERT_EQUAL_STRING_LEN("hello", list[0].start, 5);
    TEST_ASSERT_EQUAL_INT(5, list[0].len);

    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_REDL, list[1].kind, "list[1].kind");

    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_WORD, list[2].kind, "list[2].kind");
    TEST_ASSERT_EQUAL_STRING_LEN("world!", list[2].start, 6);
    TEST_ASSERT_EQUAL_INT(6, list[2].len);

    TEST_ASSERT_EQUAL(LX_TOKEN_END, list[3].kind);

    free(list);
}

/*   ""hi"& |"&hello -> Token 1: hi"& |"   Token 2: &   Token 3: hello  */
void test_lx_tokenize_quoted_string_with_delimiters(void) {
    const char *cmd = "\"\"hi\"& |\"&hello";

    struct lx_token *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL_MESSAGE(list, "list");

    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_WORD, list[0].kind, "list[0].kind");
    TEST_ASSERT_EQUAL_STRING_LEN("hi\"& |\"", list[0].start, 7);
    TEST_ASSERT_EQUAL_INT(7, list[0].len);

    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_INBG, list[1].kind, "list[1].kind");

    TEST_ASSERT_EQUAL_MESSAGE(LX_TOKEN_WORD, list[2].kind, "list[2].kind");
    TEST_ASSERT_EQUAL_STRING_LEN("hello", list[2].start, 5);
    TEST_ASSERT_EQUAL_INT(5, list[2].len);

    TEST_ASSERT_EQUAL(LX_TOKEN_END, list[3].kind);

    free(list);
}

/* stress tests */

void stest_lx_tokenize_words_operators_whitespace(void) {
    #define LIST_SIZE 10000

    char cmd[LIST_SIZE + 1];
    enum lx_token_kind kinds[LIST_SIZE];

    int one_time_char = 0;
    for (int i = 0; i < LIST_SIZE; ++i) {
        int num = rand() % 6;
        if (num == 4) {
            if (one_time_char == 1) {
                --i;
                continue;
            } else
                one_time_char = 1;
        } else
            one_time_char = 0;

        switch (num) {
        case 0: cmd[i] = '|'; kinds[i] = LX_TOKEN_PIPE; break;
        case 1: cmd[i] = '<'; kinds[i] = LX_TOKEN_REDL; break;
        case 2: cmd[i] = '>'; kinds[i] = LX_TOKEN_REDR; break;
        case 3: cmd[i] = '&'; kinds[i] = LX_TOKEN_INBG; break;
        case 4: cmd[i] = 'W'; kinds[i] = LX_TOKEN_WORD; break;
        case 5: cmd[i] = ' '; kinds[i] = LX_TOKEN_WTSP; break;
        default: TEST_FAIL_MESSAGE("Illegal switch case");
        }
    }

    cmd[LIST_SIZE] = '\0';

    // printf("%s\n", cmd);

    struct lx_token *list = lx_tokenize(cmd);
    TEST_ASSERT_NOT_NULL_MESSAGE(list, "list");

    int ws_cnt = 0;
    for (int i = 0; i < LIST_SIZE; ++i) {
        if (i + ws_cnt == LIST_SIZE)
            break;

        if (kinds[i + ws_cnt] == LX_TOKEN_WTSP) {
            ++ws_cnt;
            --i;
            continue;
        }

        // printf("ex_kind: %d, kind: %d\n", kinds[i + ws_cnt], list[i].kind);

        TEST_ASSERT_EQUAL_MESSAGE(kinds[i + ws_cnt], list[i].kind, "list[i].kind");

        if (list[i].kind != LX_TOKEN_WORD)
            continue;

        // printf("ex_char: '%c', char: '%c'\n", cmd[i + ws_cnt], *list[i].start);

        TEST_ASSERT_EQUAL_CHAR_MESSAGE(cmd[i + ws_cnt], *list[i].start,
                "*list[i].start");
        TEST_ASSERT_EQUAL_INT(1, list[i].len);
    }

    free(list);
}

int main(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand(ts.tv_sec ^ ts.tv_nsec);

    UNITY_BEGIN();

    /* push_token() */
    RUN_TEST(test_push_token_size_cap_equal);
    RUN_TEST(test_push_token_size_less_than_cap);
    RUN_TEST(test_push_token_resize_preserves_data);
    /* stress tests */
    RUN_TEST(stest_push_token_size_less_than_cap);
    RUN_TEST(stest_push_token_size_cap_equal);

    /* lx_tokenize() */
    RUN_TEST(test_lx_tokenize_operators_only);
    RUN_TEST(test_lx_tokenize_word_only);
    RUN_TEST(test_lx_tokenize_whitespace_only);
    RUN_TEST(test_lx_tokenize_words_operators_whitespace);
    RUN_TEST(test_lx_tokenize_quoted_string_with_delimiters);
    /* stress tests */
    RUN_TEST(stest_lx_tokenize_words_operators_whitespace);

    return UNITY_END();
}
