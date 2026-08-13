#include "unity_fixture.h"
#include "dyn_str.h"

TEST_GROUP(string);

/************ Shared utils ************/

static d_str s_str = {0};

static void validate_str(size_t exp_len, size_t exp_size, size_t exp_cap) {
    TEST_ASSERT_EQUAL_size_t(exp_len, s_str.len);
    TEST_ASSERT_EQUAL_size_t(exp_size, s_str.size);
    TEST_ASSERT_EQUAL_size_t(exp_cap, s_str.cap);
}

/************ Fixture ************/

TEST_SETUP(string) {
    int rv = d_str_init(&s_str);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_str(0, 1, 1);
}

TEST_TEAR_DOWN(string) {
    d_str_free(&s_str);
}

/************ Tests ************/

TEST(string, is_c_string_after_init) {
    TEST_ASSERT_EQUAL_CHAR('\0', s_str.c_str[0]);
}

TEST(string, is_c_string_after_push) {
    d_str_push(&s_str, 'a');
    TEST_ASSERT_EQUAL_STRING_LEN("a", s_str.c_str, 1);
    validate_str(1, 2, 2);

    d_str_push(&s_str, 'b');
    TEST_ASSERT_EQUAL_STRING_LEN("ab", s_str.c_str, 2);
    validate_str(2, 3, 3);

    d_str_push(&s_str, 'c');
    TEST_ASSERT_EQUAL_STRING_LEN("abc", s_str.c_str, 3);
    validate_str(3, 4, 4);
}

TEST(string, concat_right_after_init) {
    d_strcat(&s_str, "abc");
    TEST_ASSERT_EQUAL_STRING_LEN("abc", s_str.c_str, 3);
    validate_str(3, 4, 4);
}

TEST(string, concat_with_non_empty_string) {
    d_strcpy(&s_str, "abc");
    TEST_ASSERT_EQUAL_STRING_LEN("abc", s_str.c_str, 3);
    validate_str(3, 4, 4);

    d_strcat(&s_str, "def");
    TEST_ASSERT_EQUAL_STRING_LEN("abcdef", s_str.c_str, 6);
    validate_str(6, 7, 7);
}

TEST(string, vstrcat) {
    int x = 1;
    float y = 1.5;
    char *s = "Equation";

    d_vstrcat(&s_str, "%s: %d + %.1f = %.1f", s, x, y, x + y);
    validate_str(23, 24, 24);

    TEST_ASSERT_EQUAL_STRING("Equation: 1 + 1.5 = 2.5", s_str.c_str);
}

/************ Test runner ************/

TEST_GROUP_RUNNER(string) {
    RUN_TEST_CASE(string, is_c_string_after_init);
    RUN_TEST_CASE(string, is_c_string_after_push);
    RUN_TEST_CASE(string, concat_right_after_init);
    RUN_TEST_CASE(string, concat_with_non_empty_string);
    RUN_TEST_CASE(string, vstrcat);
}
