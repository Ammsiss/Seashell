#!/bin/bash

VALGRIND='valgrind
    --quiet
    --vgdb=no
    --leak-check=full
    --errors-for-leak-kinds=definite,indirect
    --show-leak-kinds=definite,indirect'

bear --append -- make --quiet unit
$VALGRIND ./unit_test -s
