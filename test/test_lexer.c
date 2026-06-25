#define _GNU_SOURCE

#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include "unity.h"
#include "lexer.h"

void setUp(void) {}
void tearDown(void) {}

void assert_tokens(
        const char *cmd,
        const lx_kind *exp_kind,
        const char **exp_raw,
        const lx_quote *exp_quote,
        size_t exp_size,
        int exp_rv
) {
    da_tok tokens = { 0 };
    TEST_ASSERT_EQUAL(exp_rv, lx_tokenize(cmd, &tokens));
    TEST_ASSERT_EQUAL_size_t(exp_size, tokens.size);

    if (exp_size == 0 || exp_rv == -1) {
        lx_free(&tokens);
        return;
    }

    size_t linear_i = 0;

    for (size_t i = 0; i < tokens.size; ++i) {
        lx_tok *tok = &tokens.data[i];
        TEST_ASSERT_EQUAL(exp_kind[i], tok->kind);

        if (exp_kind[i] == LX_TOK_WORD) {
            TEST_ASSERT_NOT_EQUAL_size_t(0, tok->parts.size);

            for (size_t y = 0; y < tok->parts.size; ++y) {
                lx_part *part = &tok->parts.data[y];
                TEST_ASSERT_EQUAL_STRING(exp_raw[linear_i], part->raw);
                TEST_ASSERT_EQUAL(exp_quote[linear_i], part->quote);
                ++linear_i;
            }
        } else {
            TEST_ASSERT_EQUAL_size_t(0, tok->parts.size);
            ++linear_i;
        }
    }

    lx_free(&tokens);
}

void test_all_operators(void) {
    const lx_kind kind[8] = {
        LX_TOK_PIPE,
        LX_TOK_BG,
        LX_TOK_RDR_IN,
        LX_TOK_APPEND,
        LX_TOK_RDR_OUT,
        LX_TOK_AND_IF,
        LX_TOK_OR_IF,
        LX_TOK_RDR_ERR,
    };

    assert_tokens("|&<>>>&&||2>", kind, NULL, NULL, 8, 0);
}

void test_shell_usage(void) {
    const lx_kind kind[9] = {
        LX_TOK_WORD, LX_TOK_WORD, LX_TOK_RDR_ERR, LX_TOK_WORD,
        LX_TOK_RDR_OUT, LX_TOK_WORD, LX_TOK_BG, LX_TOK_WORD,
        LX_TOK_WORD
    };
    const char *raw[10] = {
        "solaar", "show", NULL, "file.txt", NULL,
        "weird file", "3", NULL, "cat", "./file.txt"
    };
    const lx_quote quote[10] = {
        LX_Q_NONE, LX_Q_NONE, LX_Q_NONE, LX_Q_NONE, LX_Q_NONE,
        LX_Q_DOUBLE, LX_Q_NONE, LX_Q_NONE, LX_Q_NONE,
        LX_Q_NONE,
    };

    assert_tokens("solaar show 2>file.txt >  \"weird file\"3& cat ./file.txt",
            kind, raw, quote, 9, 0);
}

void test_quotes_galore(void) {
    const lx_kind kind[10] = {
        LX_TOK_WORD, LX_TOK_WORD, LX_TOK_WORD, LX_TOK_WORD,
        LX_TOK_WORD
    };
    const char *raw[9] = {
        "echo", "", "", "a", "", "b", "c", "", "d"
    };
    const lx_quote quote[9] = {
        LX_Q_NONE, LX_Q_SINGLE, LX_Q_DOUBLE, LX_Q_NONE,
        LX_Q_SINGLE, LX_Q_NONE, LX_Q_NONE, LX_Q_DOUBLE, LX_Q_NONE
    };

    assert_tokens("echo '' \"\" a''b c\"\"d",
            kind, raw, quote, 5, 0);
}

/* Whitespaces */

void test_whitespace_only(void) {
    assert_tokens(" ", NULL, NULL, NULL, 0, 0);
}

/* Operators */

void test_operator_single(void) {
    const lx_kind kind[1] = { LX_TOK_PIPE, };
    assert_tokens("|", kind, NULL, NULL, 1, 0);
}

void test_operator_multiple(void) {
    const lx_kind kind[2] = { LX_TOK_PIPE, LX_TOK_BG };
    assert_tokens("|&", kind, NULL, NULL, 2, 0);
}

void test_operator_before_word(void) {
    const lx_kind kind[2] = { LX_TOK_PIPE, LX_TOK_WORD };
    const char *raw[2] = { NULL, "a" };
    const lx_quote quote[2] = { LX_Q_NONE, LX_Q_NONE };
    assert_tokens("|a", kind, raw, quote, 2, 0);
}

void test_operator_before_space(void) {
    const lx_kind kind[1] = { LX_TOK_PIPE };
    assert_tokens("| ", kind, NULL, NULL, 1, 0);
}

void test_operator_after_space(void) {
    const lx_kind kind[1] = { LX_TOK_PIPE };
    assert_tokens(" |", kind, NULL, NULL, 1, 0);
}

void test_operator_two_char_op(void) {
    const lx_kind kind[1] = { LX_TOK_APPEND };
    assert_tokens(">>", kind, NULL, NULL, 1, 0);
}

void test_operator_two_char_op_before_word(void) {
    const lx_kind kind[2] = { LX_TOK_APPEND, LX_TOK_WORD };
    const char *raw[2] = { NULL, "a" };
    const lx_quote quote[2] = { LX_Q_NONE, LX_Q_NONE };
    assert_tokens(">>a", kind, raw, quote, 2, 0);
}

void test_operator_two_char_op_after_word(void) {
    const lx_kind kind[2] = { LX_TOK_WORD, LX_TOK_APPEND };
    const char *raw[2] = { "a",  NULL };
    const lx_quote quote[2] = { LX_Q_NONE, LX_Q_NONE };
    assert_tokens("a>>", kind, raw, quote, 2, 0);
}

void test_operator_two_char_op_before_op(void) {
    const lx_kind kind[2] = { LX_TOK_APPEND, LX_TOK_PIPE };
    assert_tokens(">>|", kind, NULL, NULL, 2, 0);
}

void test_operator_two_char_op_after_op(void) {
    const lx_kind kind[2] = { LX_TOK_PIPE, LX_TOK_APPEND };
    assert_tokens("|>>", kind, NULL, NULL, 2, 0);
}

void test_operator_two_char_op_before_whitespace(void) {
    const lx_kind kind[1] = { LX_TOK_APPEND };
    assert_tokens(">> ", kind, NULL, NULL, 1, 0);
}

void test_operator_two_char_op_after_whitespace(void) {
    const lx_kind kind[1] = { LX_TOK_APPEND };
    assert_tokens(" >>", kind, NULL, NULL, 1, 0);
}

/* Words */

void test_word_single_letter(void) {
    const lx_kind kind[1] = { LX_TOK_WORD, };
    const char *raw[1] = { "a" };
    const lx_quote quote[1] = { LX_Q_NONE };
    assert_tokens("a", kind, raw, quote, 1, 0);
}

void test_word_multiple_letter(void) {
    const lx_kind kind[1] = { LX_TOK_WORD, };
    const char *raw[1] = { "ab" };
    const lx_quote quote[1] = { LX_Q_NONE };
    assert_tokens("ab", kind, raw, quote, 1, 0);
}

void test_word_before_operator(void) {
    const lx_kind kind[2] = { LX_TOK_WORD, LX_TOK_PIPE };
    const char *raw[2] = { "a", NULL };
    const lx_quote quote[2] = { LX_Q_NONE, LX_Q_NONE };
    assert_tokens("a|", kind, raw, quote, 2, 0);
}

void test_word_before_space(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "a" };
    const lx_quote quote[1] = { LX_Q_NONE };
    assert_tokens("a ", kind, raw, quote, 1, 0);
}

void test_word_after_space(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "a" };
    const lx_quote quote[1] = { LX_Q_NONE };
    assert_tokens(" a", kind, raw, quote, 1, 0);
}

/* Backslashes */

void test_backslash_only(void) {
    assert_tokens("\\", NULL, NULL, NULL, 0, -1);
}

void test_backslash_after_backslash(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "\\\\" };
    const lx_quote quote[1] = { LX_Q_NONE };
    assert_tokens("\\\\", kind, raw, quote, 1, 0);
}

void test_backslash_before_operator(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "\\|" };
    const lx_quote quote[1] = { LX_Q_NONE };
    assert_tokens("\\|", kind, raw, quote, 1, 0);
}

void test_backslash_after_operator(void) {
    assert_tokens("|\\", NULL, NULL, NULL, 0, -1);
}

void test_backslash_before_whitespace(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "\\ " };
    const lx_quote quote[1] = { LX_Q_NONE };
    assert_tokens("\\ ", kind, raw, quote, 1, 0);
}

void test_backslash_after_whitespace(void) {
    assert_tokens(" \\", NULL, NULL, NULL, 0, -1);
}

void test_backslash_before_word(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "\\a" };
    const lx_quote quote[1] = { LX_Q_NONE };
    assert_tokens("\\a", kind, raw, quote, 1, 0);
}

void test_backslash_after_word(void) {
    assert_tokens("a\\", NULL, NULL, NULL, 0, -1);
}

/* Double quotes */

void test_double_quote_empty(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "" };
    const lx_quote quote[1] = { LX_Q_DOUBLE };
    assert_tokens("\"\"", kind, raw, quote, 1, 0);
}

void test_double_quote_word(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "a" };
    const lx_quote quote[1] = { LX_Q_DOUBLE };
    assert_tokens("\"a\"", kind, raw, quote, 1, 0);
}

void test_double_quote_operator(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "|" };
    const lx_quote quote[1] = { LX_Q_DOUBLE };
    assert_tokens("\"|\"", kind, raw, quote, 1, 0);
}

void test_double_quote_whitespace(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { " " };
    const lx_quote quote[1] = { LX_Q_DOUBLE };
    assert_tokens("\" \"", kind, raw, quote, 1, 0);
}

void test_double_quote_escaped_quote(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "\\\"" };
    const lx_quote quote[1] = { LX_Q_DOUBLE };
    assert_tokens("\"\\\"\"", kind, raw, quote, 1, 0);
}

void test_double_quote_no_end_quote(void) {
    assert_tokens("\"", NULL, NULL, NULL, 0, -1);
}

/* Single quotes */

void test_single_quote_empty(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "" };
    const lx_quote quote[1] = { LX_Q_SINGLE };
    assert_tokens("''", kind, raw, quote, 1, 0);
}

void test_single_quote_word(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "a" };
    const lx_quote quote[1] = { LX_Q_SINGLE };
    assert_tokens("'a'", kind, raw, quote, 1, 0);
}

void test_single_quote_operator(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "|" };
    const lx_quote quote[1] = { LX_Q_SINGLE };
    assert_tokens("'|'", kind, raw, quote, 1, 0);
}

void test_single_quote_whitespace(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { " " };
    const lx_quote quote[1] = { LX_Q_SINGLE };
    assert_tokens("' '", kind, raw, quote, 1, 0);
}

void test_single_quote_backslash(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[1] = { "\\" };
    const lx_quote quote[1] = { LX_Q_SINGLE };
    assert_tokens("'\\'", kind, raw, quote, 1, 0);
}

void test_single_quote_no_end_quote(void) {
    assert_tokens("'", NULL, NULL, NULL, 0, -1);
}

/* Parts */

void test_part_none_then_double(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[2] = { "a", "b" };
    const lx_quote quote[2] = { LX_Q_NONE, LX_Q_DOUBLE };
    assert_tokens("a\"b\"", kind, raw, quote, 1, 0);
}

void test_part_double_then_none(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[2] = { "a", "b" };
    const lx_quote quote[2] = { LX_Q_DOUBLE, LX_Q_NONE };
    assert_tokens("\"a\"b", kind, raw, quote, 1, 0);
}

void test_part_double_then_double(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[2] = { "a", "b" };
    const lx_quote quote[2] = { LX_Q_DOUBLE, LX_Q_DOUBLE };
    assert_tokens("\"a\"\"b\"", kind, raw, quote, 1, 0);
}

void test_part_none_then_single(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[2] = { "a", "b" };
    const lx_quote quote[2] = { LX_Q_NONE, LX_Q_SINGLE };
    assert_tokens("a'b'", kind, raw, quote, 1, 0);
}

void test_part_single_then_none(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[2] = { "a", "b" };
    const lx_quote quote[2] = { LX_Q_SINGLE, LX_Q_NONE };
    assert_tokens("'a'b", kind, raw, quote, 1, 0);
}

void test_part_single_then_single(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[2] = { "a", "b" };
    const lx_quote quote[2] = { LX_Q_SINGLE, LX_Q_SINGLE };
    assert_tokens("'a''b'", kind, raw, quote, 1, 0);
}

void test_part_double_then_single(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[2] = { "a", "b" };
    const lx_quote quote[2] = { LX_Q_DOUBLE, LX_Q_SINGLE };
    assert_tokens("\"a\"'b'", kind, raw, quote, 1, 0);
}

void test_part_single_then_double(void) {
    const lx_kind kind[1] = { LX_TOK_WORD };
    const char *raw[2] = { "a", "b" };
    const lx_quote quote[2] = { LX_Q_SINGLE, LX_Q_DOUBLE };
    assert_tokens("'a'\"b\"", kind, raw, quote, 1, 0);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_all_operators);
    RUN_TEST(test_shell_usage);
    RUN_TEST(test_quotes_galore);

    RUN_TEST(test_whitespace_only);

    RUN_TEST(test_operator_single);
    RUN_TEST(test_operator_multiple);
    RUN_TEST(test_operator_before_word);
    RUN_TEST(test_operator_before_space);
    RUN_TEST(test_operator_after_space);
    RUN_TEST(test_operator_two_char_op);
    RUN_TEST(test_operator_two_char_op_before_word);
    RUN_TEST(test_operator_two_char_op_after_word);
    RUN_TEST(test_operator_two_char_op_before_op);
    RUN_TEST(test_operator_two_char_op_after_op);
    RUN_TEST(test_operator_two_char_op_before_whitespace);
    RUN_TEST(test_operator_two_char_op_after_whitespace);

    RUN_TEST(test_word_single_letter);
    RUN_TEST(test_word_multiple_letter);
    RUN_TEST(test_word_before_operator);
    RUN_TEST(test_word_before_space);
    RUN_TEST(test_word_after_space);

    RUN_TEST(test_backslash_only);
    RUN_TEST(test_backslash_after_backslash);
    RUN_TEST(test_backslash_before_operator);
    RUN_TEST(test_backslash_after_operator);
    RUN_TEST(test_backslash_before_whitespace);
    RUN_TEST(test_backslash_after_whitespace);
    RUN_TEST(test_backslash_before_word);
    RUN_TEST(test_backslash_after_word);

    RUN_TEST(test_double_quote_empty);
    RUN_TEST(test_double_quote_word);
    RUN_TEST(test_double_quote_operator);
    RUN_TEST(test_double_quote_whitespace);
    RUN_TEST(test_double_quote_escaped_quote);
    RUN_TEST(test_double_quote_no_end_quote);

    RUN_TEST(test_single_quote_empty);
    RUN_TEST(test_single_quote_word);
    RUN_TEST(test_single_quote_operator);
    RUN_TEST(test_single_quote_whitespace);
    RUN_TEST(test_single_quote_backslash);
    RUN_TEST(test_single_quote_no_end_quote);

    RUN_TEST(test_part_none_then_double);
    RUN_TEST(test_part_double_then_none);
    RUN_TEST(test_part_double_then_double);
    RUN_TEST(test_part_none_then_single);
    RUN_TEST(test_part_single_then_none);
    RUN_TEST(test_part_single_then_single);
    RUN_TEST(test_part_double_then_single);
    RUN_TEST(test_part_single_then_double);

    return UNITY_END();
}
