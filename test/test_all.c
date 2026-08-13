#include "unity_fixture.h"
#include "log.h"

void run_all(void) {
    log_init("/home/juta/Projects/Seashell/logs/test");

    RUN_TEST_GROUP(array);
    RUN_TEST_GROUP(string);
    RUN_TEST_GROUP(lexer);
    RUN_TEST_GROUP(parser);
    RUN_TEST_GROUP(jobs);
    RUN_TEST_GROUP(shell);
}

int main(int argc, const char **argv) {
    return UnityMain(argc, argv, run_all);
}
