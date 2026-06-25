#define _GNU_SOURCE

#include "unity.h"
#include "dyn_arr.h"

static da_int arr;

void validate_array(da_int *arr, size_t size, size_t cap) {
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL_size_t(size, arr->size);
    TEST_ASSERT_EQUAL_size_t(cap, arr->cap);
}

void setUp(void) {
    int rv = da_init(&arr);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_array(&arr, 0, 0);
}

void tearDown(void) {
    da_free(&arr);
}

void test_reserve_min_lte_cap_is_no_op(void) {
    int rv = da_reserve(&arr, 0);
    TEST_ASSERT_EQUAL_INT(0, rv);

    validate_array(&arr, 0, 0);
}

void test_reserve_empty_array(void) {
    int rv = da_reserve(&arr, 10);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_array(&arr, 0, 10);
}

void test_reserve_non_empty_array(void) {
    /* initial allocation ... realloc(NULL, size) */
    int rv = da_reserve(&arr, 10);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_array(&arr, 0, 10);
    /* actual realloc call ... realloc (data, size) */
    rv = da_reserve(&arr, 20);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_array(&arr, 0, 20);
}

void test_reserve_realloc_preserves_data(void) {
    int *p = da_push(&arr);
    TEST_ASSERT_NOT_NULL(p);
    validate_array(&arr, 1, 1);

    arr.data[0] = 100;

    p = da_push(&arr);
    TEST_ASSERT_NOT_NULL(p);
    validate_array(&arr, 2, 2);

    TEST_ASSERT_EQUAL_INT(100, arr.data[0]);
}

void test_push_returns_new_element(void) {
    int *p = da_push(&arr);
    TEST_ASSERT_EQUAL_PTR(&arr.data[0], p);
    validate_array(&arr, 1, 1);

    /* push should zero out new element */
    TEST_ASSERT_EQUAL_INT(0, *p);
}

void test_generic_init(void) {
    da_tok toks;
    TEST_ASSERT_EQUAL_PTR(da_tok_init, get_da_init(&toks));
}

void test_generic_free(void) {
    da_part parts;
    TEST_ASSERT_EQUAL_PTR(da_part_free, get_da_free(&parts));
}

void test_generic_reserve(void) {
    da_word words;
    TEST_ASSERT_EQUAL_PTR(da_word_reserve, get_da_reserve(&words));
}

void test_generic_push(void) {
    da_segment segments;
    TEST_ASSERT_EQUAL_PTR(da_segment_push, get_da_push(&segments));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_reserve_min_lte_cap_is_no_op);
    RUN_TEST(test_reserve_empty_array);
    RUN_TEST(test_reserve_non_empty_array);
    RUN_TEST(test_reserve_realloc_preserves_data);

    RUN_TEST(test_push_returns_new_element);

    RUN_TEST(test_generic_init);
    RUN_TEST(test_generic_free);
    RUN_TEST(test_generic_reserve);
    RUN_TEST(test_generic_push);

    return UNITY_END();
}
