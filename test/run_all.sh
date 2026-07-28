#!/bin/zsh

make >/dev/null || exit 1

PASS="$(printf '\033[32m✓\033[0m')"

VALGRIND_CMD=(
  valgrind
  --vgdb=no
  --quiet
  --leak-check=full
  --errors-for-leak-kinds=definite,indirect,possible
  --error-exitcode=1
)

run_test() {
    local TEST_BIN="$1"

    "${VALGRIND_CMD[@]}" \
        ./${TEST_BIN} >/tmp/unity_output 2>&1 && \
        echo "${TEST_BIN}  $PASS" || \
        cat /tmp/unity_output
}

run_test test_dyn_arr
run_test test_dyn_str

run_test test_parser
run_test test_lexer
run_test test_job_state

rm /tmp/unity_output
