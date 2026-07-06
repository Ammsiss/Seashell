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

"${VALGRIND_CMD[@]}" \
    ./test_parser >/tmp/unity_output 2>&1 && \
    echo "test_parser  $PASS" || \
    cat /tmp/unity_output

"${VALGRIND_CMD[@]}" \
    ./test_lexer >/tmp/unity_output 2>&1 && \
    echo "test_lexer   $PASS" || \
    cat /tmp/unity_output

"${VALGRIND_CMD[@]}" \
    ./test_dyn_arr >/tmp/unity_output 2>&1 && \
    echo "test_dyn_arr $PASS" || \
    cat /tmp/unity_output

"${VALGRIND_CMD[@]}" \
    ./test_expander >/tmp/unity_output 2>&1 && \
    echo "test_expander $PASS" || \
    cat /tmp/unity_output

"${VALGRIND_CMD[@]}" \
    ./test_executor >/tmp/unity_output 2>&1 && \
    echo "test_executor $PASS" || \
    cat /tmp/unity_output

rm /tmp/unity_output
