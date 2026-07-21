#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <bits/types/sigset_t.h>
#include <unistd.h>

#include "dyn_arr.h"
#include "runner.h"

typedef struct {
    bool subshell;
    int tty_fd;
    sigset_t og_mask;
    da_vars vars;
    jc_jst jctl;
    da_plan job_plans;
} shell_env;

extern shell_env sh_env;

int restore_signals(void);
int process_signals(void);

int env_init(void);
void env_free(void);

#endif
