#!/bin/bash

VALGRIND='valgrind
    --quiet
    --vgdb=no
    --leak-check=full
    --errors-for-leak-kinds=definite,indirect,possible'

bear --append -- make --quiet unit
$VALGRIND ./unit_test -s

bear --append -- make --quiet pty
$VALGRIND ./pty_test -s
