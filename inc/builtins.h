#ifndef BUILTINS_H
#define BUILTINS_H

#include "shell_state.h"

#define BUILTIN_COUNT 8

typedef int (*builtin_func)(char **, shell_env *);

typedef struct {
    const char *name;
    builtin_func func;
} sh_builtin;

bool try_run_builtin(char **argv, int *status);

#endif
