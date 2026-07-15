#ifndef RUNNER_H
#define RUNNER_H

#include "dyn_arr.h"

typedef struct {
    da_pid pids;
    pid_t pgid;
} pline_info;

int exec_pline(const ps_pline *pline, bool bg, pline_info *info);

#endif
