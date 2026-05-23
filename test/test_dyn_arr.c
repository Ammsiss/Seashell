#define _GNU_SOURCE

#include "unity.h"
#include "dyn_arr.h"

void setUp(void) {}
void tearDown(void) {}

void validate_array(struct dyn_arr *arr, void *data, size_t data_size,
        size_t size, size_t cap) {
    TEST_ASSERT_NOT_NULL(arr->data);
    if (data != NULL)
        TEST_ASSERT_EQUAL_PTR(data, arr->data);
    TEST_ASSERT_EQUAL_size_t(data_size, arr->data_size);
    TEST_ASSERT_EQUAL_size_t(size, arr->size);
    TEST_ASSERT_EQUAL_size_t(cap, arr->cap);
}

void test_da_init_size_zero(void) {
    struct dyn_arr arr;
    TEST_ASSERT_EQUAL_INT(0, da_init(&arr, 0, 1));
    validate_array(&arr, NULL, 1, 0, 1);

    da_free(&arr);
}

void test_da_init_non_zero_size(void) {
    struct dyn_arr arr;
    TEST_ASSERT_EQUAL_INT(0, da_init(&arr, 1, 1));
    validate_array(&arr, NULL, 1, 1, 1);

    da_free(&arr);
}

void test_da_push_return_last_element_no_realloc(void) {
    struct dyn_arr arr;
    TEST_ASSERT_EQUAL_INT(0, da_init(&arr, 0, 1));
    validate_array(&arr, NULL, 1, 0, 1);

    void *rv = da_push(&arr);
    TEST_ASSERT_NOT_NULL(rv);
    validate_array(&arr, NULL, 1, 1, 1);

    TEST_ASSERT_EQUAL_PTR(arr.data, rv);

    da_free(&arr);
}

void test_da_push_return_last_element_realloc(void) {
    struct dyn_arr arr;
    TEST_ASSERT_EQUAL_INT(0, da_init(&arr, 1, 1));
    validate_array(&arr, NULL, 1, 1, 1);

    void *rv = da_push(&arr);
    TEST_ASSERT_NOT_NULL(rv);
    validate_array(&arr, NULL, 1, 2, 2);

    TEST_ASSERT_EQUAL_PTR((char *) arr.data + 1, rv);

    da_free(&arr);
}

void test_da_push_no_realloc(void) {
    struct dyn_arr arr;
    TEST_ASSERT_EQUAL_INT(0, da_init(&arr, 0, 3));
    validate_array(&arr, NULL, 3, 0, 1);

    void *old_data = arr.data;
    TEST_ASSERT_NOT_NULL(da_push(&arr));
    validate_array(&arr, old_data, 3, 1, 1);

    da_free(&arr);
}

void test_da_push_realloc_preserves_data(void) {
    struct dyn_arr arr;
    TEST_ASSERT_EQUAL_INT(0, da_init(&arr, 1, 1));
    validate_array(&arr, NULL, 1, 1, 1);

    ((char *) arr.data)[0] = 'x';

    TEST_ASSERT_NOT_NULL(da_push(&arr));
    validate_array(&arr, NULL, 1, 2, 2);
    TEST_ASSERT_EQUAL_CHAR('x', ((char *) arr.data)[0]);

    da_free(&arr);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_da_init_size_zero);
    RUN_TEST(test_da_init_non_zero_size);
    RUN_TEST(test_da_push_return_last_element_no_realloc);
    RUN_TEST(test_da_push_return_last_element_realloc);
    RUN_TEST(test_da_push_no_realloc);
    RUN_TEST(test_da_push_realloc_preserves_data);

    return UNITY_END();
}
