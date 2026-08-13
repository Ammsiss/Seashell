#include "unity_fixture.h"
#include "dyn_arr.h"

TEST_GROUP(array);

/************ Shared utils ************/

static da_int arr;

static void validate_array(da_int *arr, size_t size, size_t cap) {
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL_size_t(size, arr->size);
    TEST_ASSERT_EQUAL_size_t(cap, arr->cap);
}

/************ Fixture ************/

TEST_SETUP(array) {
    int rv = da_init(&arr);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_array(&arr, 0, 0);
}

TEST_TEAR_DOWN(array) {
    da_free(&arr);
}

/************ Tests ************/

TEST(array, reserve_min_lte_cap_is_no_op) {
    int rv = da_reserve(&arr, 0);
    TEST_ASSERT_EQUAL_INT(0, rv);

    validate_array(&arr, 0, 0);
}

TEST(array, reserve_empty_array) {
    int rv = da_reserve(&arr, 10);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_array(&arr, 0, 10);
}

TEST(array, reserve_non_empty_array) {
    /* initial allocation ... realloc(NULL, size) */
    int rv = da_reserve(&arr, 10);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_array(&arr, 0, 10);
    /* actual realloc call ... realloc (data, size) */
    rv = da_reserve(&arr, 20);
    TEST_ASSERT_EQUAL_INT(0, rv);
    validate_array(&arr, 0, 20);
}

TEST(array, reserve_realloc_preserves_data) {
    int *p = da_push(&arr);
    TEST_ASSERT_NOT_NULL(p);
    validate_array(&arr, 1, 1);

    arr.data[0] = 100;

    p = da_push(&arr);
    TEST_ASSERT_NOT_NULL(p);
    validate_array(&arr, 2, 2);

    TEST_ASSERT_EQUAL_INT(100, arr.data[0]);
}

TEST(array, push_returns_new_element) {
    int *p = da_push(&arr);
    TEST_ASSERT_EQUAL_PTR(&arr.data[0], p);
    validate_array(&arr, 1, 1);

    /* push should zero out new element */
    TEST_ASSERT_EQUAL_INT(0, *p);
}

TEST(array, delete_only_element) {
    da_push(&arr);
    validate_array(&arr, 1, 1);

    da_delete(&arr, 0);
    validate_array(&arr, 0, 1);
}

TEST(array, delete_first_element) {
    int *p1 = da_push(&arr);
    validate_array(&arr, 1, 1);
    *p1 = 1;

    int *p2 = da_push(&arr);
    validate_array(&arr, 2, 2);
    *p2 = 2;

    da_delete(&arr, 0);
    validate_array(&arr, 1, 2);

    TEST_ASSERT_EQUAL_INT(arr.data[0], 2);
}

TEST(array, delete_last_element) {
    int *p1 = da_push(&arr);
    validate_array(&arr, 1, 1);
    *p1 = 1;

    int *p2 = da_push(&arr);
    validate_array(&arr, 2, 2);
    *p2 = 2;

    da_delete(&arr, 1);
    validate_array(&arr, 1, 2);

    TEST_ASSERT_EQUAL_INT(arr.data[0], 1);
}

TEST(array, delete_middle_element) {
    int *p1 = da_push(&arr);
    validate_array(&arr, 1, 1);
    *p1 = 1;

    int *p2 = da_push(&arr);
    validate_array(&arr, 2, 2);
    *p2 = 2;

    int *p3 = da_push(&arr);
    validate_array(&arr, 3, 3);
    *p3 = 3;

    da_delete(&arr, 1);
    validate_array(&arr, 2, 3);

    TEST_ASSERT_EQUAL_INT(arr.data[0], 1);
    TEST_ASSERT_EQUAL_INT(arr.data[1], 3);
}

TEST(array, generic_init) {
    da_tok toks;
    TEST_ASSERT_EQUAL_PTR(da_tok_init, DA_GET(_init, &toks));
}

TEST(array, generic_free) {
    da_part parts;
    TEST_ASSERT_EQUAL_PTR(da_part_free, DA_GET(_free, &parts));
}

TEST(array, generic_reserve) {
    da_word words;
    TEST_ASSERT_EQUAL_PTR(da_word_reserve, DA_GET(_reserve, &words));
}

TEST(array, generic_push) {
    da_segment segments;
    TEST_ASSERT_EQUAL_PTR(da_segment_push, DA_GET(_push, &segments));
}

/************ Test runner ************/

TEST_GROUP_RUNNER(array) {
    RUN_TEST_CASE(array, reserve_min_lte_cap_is_no_op);
    RUN_TEST_CASE(array, reserve_empty_array);
    RUN_TEST_CASE(array, reserve_non_empty_array);
    RUN_TEST_CASE(array, reserve_realloc_preserves_data);
    RUN_TEST_CASE(array, push_returns_new_element);
    RUN_TEST_CASE(array, delete_only_element);
    RUN_TEST_CASE(array, delete_first_element);
    RUN_TEST_CASE(array, delete_last_element);
    RUN_TEST_CASE(array, delete_middle_element);
    RUN_TEST_CASE(array, generic_init);
    RUN_TEST_CASE(array, generic_free);
    RUN_TEST_CASE(array, generic_reserve);
    RUN_TEST_CASE(array, generic_push);
}
