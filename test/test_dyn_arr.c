#define _GNU_SOURCE

#include "unity.h"
#include "dyn_arr.h"

DEFINE_DYN_ARR(da_int, int)

void setUp(void) {}
void tearDown(void) {}

void validate_array(da_int *arr, int *data, size_t size, size_t cap) {
    TEST_ASSERT_EQUAL_PTR(data, arr->data);
    TEST_ASSERT_EQUAL_size_t(size, arr->size);
    TEST_ASSERT_EQUAL_size_t(cap, arr->cap);
}

void test_da_init_size_zero(void) {
    da_int arr;
    TEST_ASSERT_EQUAL_INT(0, da_int_init(&arr, 0));
    validate_array(&arr, NULL, 0, 0);

    da_int_free(&arr);
}

void test_da_init_non_zero_size(void) {
    da_int arr;
    TEST_ASSERT_EQUAL_INT(0, da_int_init(&arr, 1));
    validate_array(&arr, arr.data, 0, 1);

    da_int_free(&arr);
}

void test_da_push_return_last_element_no_realloc(void) {
    da_int arr;
    TEST_ASSERT_EQUAL_INT(0, da_int_init(&arr, 0));
    validate_array(&arr, NULL, 0, 0);

    TEST_ASSERT_NOT_NULL(da_int_push(&arr));

    validate_array(&arr, arr.data, 1, 1);

    da_int_free(&arr);
}

void test_da_push_return_last_element(void) {
    da_int arr;
    TEST_ASSERT_EQUAL_INT(0, da_int_init(&arr, 1));
    validate_array(&arr, arr.data, 0, 1);

    TEST_ASSERT_EQUAL_PTR(&arr.data[0], da_int_push(&arr));

    validate_array(&arr, arr.data, 1, 1);

    da_int_free(&arr);
}

void test_da_push_no_realloc(void) {
    da_int arr;
    TEST_ASSERT_EQUAL_INT(0, da_int_init(&arr, 1));
    validate_array(&arr, arr.data, 0, 1);

    TEST_ASSERT_NOT_NULL(da_int_push(&arr));
    validate_array(&arr, arr.data, 1, 1);

    da_int_free(&arr);
}

void test_da_push_realloc_preserves_data(void) {
    da_int arr;
    TEST_ASSERT_EQUAL_INT(0, da_int_init(&arr, 1));
    validate_array(&arr, arr.data, 0, 1);

    TEST_ASSERT_NOT_NULL(da_int_push(&arr));
    validate_array(&arr, arr.data, 1, 1);

    arr.data[0] = 69;

    TEST_ASSERT_NOT_NULL(da_int_push(&arr));
    validate_array(&arr, arr.data, 2, 2);

    TEST_ASSERT_EQUAL_INT(69, arr.data[0]);

    da_int_free(&arr);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_da_init_size_zero);
    RUN_TEST(test_da_init_non_zero_size);
    RUN_TEST(test_da_push_return_last_element_no_realloc);
    RUN_TEST(test_da_push_return_last_element);
    RUN_TEST(test_da_push_no_realloc);
    RUN_TEST(test_da_push_realloc_preserves_data);
    // overflow test

    return UNITY_END();
}
