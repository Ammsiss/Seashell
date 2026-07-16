#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <bits/types/sigset_t.h>
#include <unistd.h>

#include "dyn_arr.h"

typedef struct {
    bool subshell;
    int tty_fd;
    sigset_t og_mask;
    da_vars vars;
} shell_env;

extern shell_env sh_env;

int env_init(void);
void env_free(void);

#endif
