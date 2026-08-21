#define _GNU_SOURCE

#include "unity_fixture.h"

TEST_GROUP(prompt);

/************ Shared utils ************/

/************ Fixture ************/

TEST_SETUP(prompt) {
}

TEST_TEAR_DOWN(prompt) {
}

/************ Tests ************/

TEST(prompt, init_test) {
    // TEST_FAIL();
}

/************ Test runner ************/

TEST_GROUP_RUNNER(prompt) {
    RUN_TEST_CASE(prompt, init_test);
}
