#include "unity.h"
#include "dyn_str.h"

static d_str s_str = {0};

void validate_str(size_t exp_len, size_t exp_size, size_t exp_cap) {
    TEST_ASSERT_EQUAL_size_t(exp_len, s_str.len);
    TEST_ASSERT_EQUAL_size_t(exp_size, s_str.size);
    TEST_ASSERT_EQUAL_size_t(exp_cap, s_str.cap);
}

void setUp(void) {
    int rv = d_str_init(&s_str);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_str(0, 1, 1);
}

void tearDown(void) {
    d_str_free(&s_str);
}

void test_is_c_string_after_init(void) {
    TEST_ASSERT_EQUAL_CHAR('\0', s_str.c_str[0]);
}

void test_is_c_string_after_push(void) {
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

void test_concat_right_after_init(void) {
    d_strcat(&s_str, "abc");
    TEST_ASSERT_EQUAL_STRING_LEN("abc", s_str.c_str, 3);
    validate_str(3, 4, 4);
}

void test_concat_with_non_empty_string(void) {
    d_strcpy(&s_str, "abc");
    TEST_ASSERT_EQUAL_STRING_LEN("abc", s_str.c_str, 3);
    validate_str(3, 4, 4);

    d_strcat(&s_str, "def");
    TEST_ASSERT_EQUAL_STRING_LEN("abcdef", s_str.c_str, 6);
    validate_str(6, 7, 7);
}

void test_vstrcat(void) {
    int x = 1;
    float y = 1.5;
    char *s = "Equation";

    d_vstrcat(&s_str, "%s: %d + %.1f = %.1f", s, x, y, x + y);
    validate_str(23, 24, 24);

    TEST_ASSERT_EQUAL_STRING("Equation: 1 + 1.5 = 2.5", s_str.c_str);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_c_string_after_init);
    RUN_TEST(test_is_c_string_after_push);
    RUN_TEST(test_concat_right_after_init);
    RUN_TEST(test_concat_with_non_empty_string);
    RUN_TEST(test_vstrcat);

    return UNITY_END();
}
